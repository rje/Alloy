// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include "SketchSpline.h"
#include "SurfaceTrim.h"
#include "SurfaceRailLoft.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <BRepTools.hxx>
#include <BRep_Tool.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Wire.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <gp_Ax1.hxx>
#include <iostream>
#include <stdexcept>
#include <cmath>

namespace {
void check(bool value,const QString& message){if(!value)throw std::runtime_error(message.toStdString());}
void append(CadDocument& document,const QJsonObject& feature){
    auto candidate=document.authored();auto rows=candidate["features"].toArray();rows.append(feature);candidate["features"]=rows;
    const auto error=document.commit(candidate,"Add source");check(error.isEmpty(),error);
}
QJsonObject addSpline(QJsonObject sketch,const QJsonArray& points){const auto result=SketchSpline::add(sketch,{{"points",points}});check(result.ok,result.error);return result.feature;}
QJsonObject editPoint(QJsonObject authored,const QString& sketchId,const QString& pointId,const char* axis,double value){
    auto rows=authored["features"].toArray();for(int i=0;i<rows.size();++i)if(rows[i].toObject()["id"]==sketchId){
        auto sketch=rows[i].toObject(),p=sketch["parameters"].toObject();auto points=p["points"].toArray();
        for(int j=0;j<points.size();++j)if(points[j].toObject()["id"]==pointId){auto point=points[j].toObject();point[axis]=value;points[j]=point;}
        p["points"]=points;sketch["parameters"]=p;rows[i]=sketch;break;
    }authored["features"]=rows;return authored;
}
QSet<QString> edgeIds(const EvaluatedBody& body){QSet<QString> result;for(const auto& edge:body.solidEdges){check(!BRep_Tool::Degenerated(edge.edge),"Collapsed tip published as selectable edge");result.insert(edge.edgeId);}return result;}
QSet<QString> faceIds(const EvaluatedBody& body){QSet<QString> result;for(const auto& face:body.supportFaces)result.insert(face.faceId);return result;}
void holes(const EvaluatedBody& body){
    int holeCount=0;
    for(const auto& support:body.supportFaces){const auto outer=BRepTools::OuterWire(support.face);int perFace=0;
        for(TopExp_Explorer it(support.face,TopAbs_WIRE);it.More();it.Next())if(!it.Current().IsSame(outer)){
            const auto wire=TopoDS::Wire(it.Current());TopTools_IndexedMapOfShape edges;TopExp::MapShapes(wire,TopAbs_EDGE,edges);
            check(wire.Closed() && BRepCheck_Analyzer(wire).IsValid() && edges.Extent()==2,"Expected closed valid two-edge side hole");
            ++perFace;++holeCount;
        }check(perFace==1,"Each exterior side needs exactly one handle hole");
    }check(holeCount==2,"Expected two side holes");
}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    CadDocument document;const auto root=document.authored()["documentId"].toString();
    QJsonObject plane{{"id",CadDocument::newId()},{"ownerId",root},{"type","constructionPlane"},{"name","Heel plane"},{"visible",true},
        {"parameters",QJsonObject{{"source",QJsonObject{{"plane","YZ"}}},{"offsetMm",120.0}}}};append(document,plane);
    auto back=SketchGeometry::newConstructionPlaneFeature(root,plane["id"].toString(),"Back arc");
    back=SketchGeometry::addArc(back,{{"startUMm",-35},{"startVMm",0},{"throughUMm",0},{"throughVMm",45},{"endUMm",35},{"endVMm",0}});append(document,back);
    const auto arc=back["parameters"].toObject()["entities"].toArray().first().toObject();
    auto lower=SketchGeometry::newFeature(root,"XY","Lower Mirror pair");
    lower=SketchGeometry::addLine(lower,{{"u1Mm",0},{"v1Mm",0},{"u2Mm",120},{"v2Mm",0}});
    const auto axis=lower["parameters"].toObject()["entities"].toArray().first().toObject();
    auto changed=SketchGeometry::setConstruction(lower,{{"entityIds",QJsonArray{axis["id"]}},{"construction",true}});check(changed.ok,changed.error);lower=changed.feature;
    lower=addSpline(lower,QJsonArray{QJsonObject{{"uMm",120},{"vMm",35}},QJsonObject{{"uMm",85},{"vMm",38}},QJsonObject{{"uMm",45},{"vMm",25}},QJsonObject{{"uMm",0},{"vMm",0}}});
    const auto right=lower["parameters"].toObject()["entities"].toArray().last().toObject();
    changed=SketchSpline::edit(lower,"tangent",{{"entityId",right["id"]},{"pointId",right["pointIds"].toArray().last()},{"duMm",0},{"dvMm",-120}});check(changed.ok,changed.error);lower=changed.feature;
    changed=SketchGeometry::mirrorEntities(lower,{{"sourceEntityIds",QJsonArray{right["id"]}},{"axisEntityId",axis["id"]}});check(changed.ok,changed.error);lower=changed.feature;append(document,lower);
    const auto pair=lower["parameters"].toObject()["mirrorGroups"].toArray().first().toObject()["entityPairs"].toArray().first().toObject();
    const auto leftId=pair["resultEntityId"].toString();
    auto rail=SketchGeometry::newFeature(root,"XZ","Crown rail");
    rail=addSpline(rail,QJsonArray{QJsonObject{{"uMm",120},{"vMm",45}},QJsonObject{{"uMm",85},{"vMm",50}},QJsonObject{{"uMm",40},{"vMm",32}},QJsonObject{{"uMm",0},{"vMm",0}}});append(document,rail);
    const auto railEntity=rail["parameters"].toObject()["entities"].toArray().last().toObject();
    const QJsonArray sections{QJsonObject{{"sketchId",back["id"]},{"entityIds",QJsonArray{arc["id"]}}},QJsonObject{{"sketchId",lower["id"]},{"entityIds",QJsonArray{leftId,right["id"]}}}};
    const QJsonObject args{{"operation","newComponent"},{"guideType","rail"},{"componentName","Iron exterior"},{"name","Bilateral exterior"},
        {"sections",sections},{"rail",QJsonObject{{"sketchId",rail["id"]},{"entityId",railEntity["id"]}}}};
    if(argc>1)check(document.save(QString::fromLocal8Bit(argv[1])).isEmpty(),"Save prepared sketches");
    const auto prior=document.evaluation();const auto candidate=CadDocument::loftCandidate(document.authored(),args);
    const auto error=document.commit(candidate,"Bilateral Rail Loft");check(error.isEmpty(),error+" / "+document.lastEvaluationError()["code"].toString());
    const auto source=document.bodies().back();const auto topology=source.summary["topology"].toObject();
    check(source.summary["valid"].toBool() && topology["faces"]==2 && topology["edges"]==7 && topology["solids"]==0,"Valid bilateral sheet topology");
    check(source.supportFaces.size()==2 && source.solidEdges.size()==5,"Two faces and five ordinary edges");
    for(const auto& face:source.supportFaces)check(face.boundaryEdges.size()==3,"Three ordinary edges per side");
    const auto originalFaces=faceIds(source),originalEdges=edgeIds(source);
    check(BRep_Tool::Surface(source.supportFaces[0].face)==BRep_Tool::Surface(source.supportFaces[1].face),"Both sides must share exact support");
    const auto authored=document.authored();const auto loft=authored["features"].toArray().last().toObject();
    // The evaluator is invariant under a rigid transform of all solved inputs.
    auto moved=prior;gp_Trsf rotation;rotation.SetRotation(gp_Ax1(gp_Pnt(0,0,0),gp_Dir(1,2,3)),.47);rotation.SetTranslationPart(gp_Vec(19,-27,11));
    for(auto& sketch:moved.sketches){for(auto it=sketch.entityEdges.begin();it!=sketch.entityEdges.end();++it)it.value()=TopoDS::Edge(BRepBuilderAPI_Transform(it.value(),rotation,true).Shape());
        auto points=sketch.summary["worldPoints"].toArray();for(int i=0;i<points.size();++i){auto row=points[i].toObject();const auto xyz=row["positionMm"].toArray();const auto p=gp_Pnt(xyz[0].toDouble(),xyz[1].toDouble(),xyz[2].toDouble()).Transformed(rotation);row["positionMm"]=QJsonArray{p.X(),p.Y(),p.Z()};points[i]=row;}sketch.summary["worldPoints"]=points;}
    const auto transformed=SurfaceRailLoft::evaluate(loft,moved);check(faceIds(transformed)==originalFaces && edgeIds(transformed)==originalEdges
        && std::abs(transformed.summary["areaMm2"].toDouble()-source.summary["areaMm2"].toDouble())<1e-6,"Source-local construction must survive arbitrary rigid frame");
    auto tool=SketchGeometry::newFeature(root,"XZ","Handle lens");
    tool=addSpline(tool,QJsonArray{QJsonObject{{"uMm",30},{"vMm",15}},QJsonObject{{"uMm",60},{"vMm",30}},QJsonObject{{"uMm",90},{"vMm",15}}});
    tool=addSpline(tool,QJsonArray{QJsonObject{{"uMm",30},{"vMm",15}},QJsonObject{{"uMm",60},{"vMm",10}},QJsonObject{{"uMm",90},{"vMm",15}}});append(document,tool);
    const auto tools=tool["parameters"].toObject()["entities"].toArray();const QJsonArray toolIds{tools[0].toObject()["id"],tools[1].toObject()["id"]};
    if(argc>2)check(document.save(QString::fromLocal8Bit(argv[2])).isEmpty(),"Save pre-Trim source");
    const auto trimId=CadDocument::newId();const QJsonObject trimFeature{{"id",trimId},{"type","surfaceTrim"},{"parameters",QJsonObject{
        {"targetBodyId",source.bodyId},{"toolSketchId",tool["id"]},{"toolEntityIds",toolIds},{"removedRegionIds",QJsonArray{}},
        {"throughMarginMm",10.0},{"projectionMode","sketchNormalThroughTarget"},{"sourcePlane","XZ"},{"sourceNormal",QJsonArray{0,-1,0}}}}};
    const auto split=SurfaceTrim::split(trimFeature,source,document.evaluation());QJsonArray removed;
    for(const auto& region:split.regions)if(region.inside)removed.append(region.regionId);
    check(split.regions.size()==4 && removed.size()==2,"Two independent bilateral inside regions");
    const auto trim=CadDocument::surfaceTrimCandidate(document.authored(),{{"newFeatureId",trimId},{"ownerId",source.ownerId},{"targetBodyId",source.bodyId},
        {"toolSketchId",tool["id"]},{"toolEntityIds",toolIds},{"removedRegionIds",removed}});
    const auto trimError=document.commit(trim,"Trim bilateral handle");check(trimError.isEmpty(),trimError+" / "+document.lastEvaluationError()["code"].toString());
    const auto initial=document.authored();const auto trimmed=document.bodies().back();holes(trimmed);const auto trimFaces=faceIds(trimmed),trimEdges=edgeIds(trimmed);
    check(trimmed.summary["valid"].toBool() && trimmed.supportFaces.size()==2,"Trim leaves valid bilateral sides");
    const auto edited=editPoint(initial,rail["id"].toString(),railEntity["pointIds"].toArray()[1].toString(),"vMm",55);
    const auto editError=document.commit(edited,"Raise crown");check(editError.isEmpty(),editError);
    const auto editedBody=document.bodies().back();holes(editedBody);
    check(faceIds(editedBody)==trimFaces && edgeIds(editedBody)==trimEdges && std::abs(editedBody.summary["areaMm2"].toDouble()-trimmed.summary["areaMm2"].toDouble())>100,"Rail edit retains Trim identities and changes shape");
    document.history().undo();check(document.authored()==initial,"Undo source edit");document.history().redo();check(document.authored()==edited,"Redo source edit");
    QTemporaryDir temporary;const auto path=argc>3?QString::fromLocal8Bit(argv[3]):temporary.filePath("bilateral.alloy");check(document.save(path).isEmpty(),"Save bilateral Trim");
    CadDocument reopened;const auto openError=reopened.open(path);check(openError.isEmpty() && reopened.authored()==edited,openError+" / reopen authored equality");holes(reopened.bodies().back());check(faceIds(reopened.bodies().back())==trimFaces && edgeIds(reopened.bodies().back())==trimEdges,"Reopen identities");
    const auto broken=editPoint(edited,rail["id"].toString(),railEntity["pointIds"].toArray().first().toString(),"vMm",44);
    check(!document.commit(broken,"Break back contact").isEmpty() && document.authored()==edited,"Broken contact refuses atomically");
    auto noMirror=prior;for(auto& sketch:noMirror.sketches)if(sketch.featureId==lower["id"])sketch.summary.remove("mirrorGroups");
    bool refused=false;try{SurfaceRailLoft::evaluate(loft,noMirror);}catch(const GeometryKernel::EvaluationError&){refused=true;}check(refused,"Missing retained Mirror ancestry must refuse");
    std::cout<<"SURFACE_BILATERAL_RAIL_LOFT_OK faces=2 ordinaryEdges=5 holes=2 sourceEdit history reopen rigidFrame atomicRefusal\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
