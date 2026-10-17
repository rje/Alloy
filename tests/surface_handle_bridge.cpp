// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include "SketchSpline.h"
#include "SurfaceTrim.h"
#include "SurfaceClosedLoft.h"
#include <GeomAPI_ProjectPointOnSurf.hxx>
#include <Geom_Surface.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QFileInfo>
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
#include <array>

namespace {
void check(bool value,const QString& message){if(!value)throw std::runtime_error(message.toStdString());}
void append(CadDocument& document,const QJsonObject& feature){
    auto candidate=document.authored();auto rows=candidate["features"].toArray();auto source=feature;
    // This independent fixture presents only the actual body and middle ellipse.
    if((source["type"]=="sketch" || source["type"]=="constructionPlane") && source["name"]!="Handle middle ellipse")source["visible"]=false;
    rows.append(source);candidate["features"]=rows;const auto error=document.commit(candidate,"Add source");check(error.isEmpty(),error);
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
void supportAgreement(const EvaluatedBody& bridge,const std::vector<TopoDS_Edge>& sources){
    double maximum=0;
    for(const auto& edge:sources){BRepAdaptor_Curve curve(edge);
        for(int i=0;i<=40;++i){const auto p=curve.Value(curve.FirstParameter()+(curve.LastParameter()-curve.FirstParameter())*i/40.);double distance=1e100;
            for(const auto& face:bridge.supportFaces){double u0,u1,v0,v1;BRepTools::UVBounds(face.face,u0,u1,v0,v1);
                GeomAPI_ProjectPointOnSurf projected(p,BRep_Tool::Surface(face.face),u0,u1,v0,v1,1e-8);
                if(projected.NbPoints())distance=std::min(distance,projected.LowerDistance());}
            maximum=std::max(maximum,distance);
        }
    }check(maximum<1e-4,"Retained section missed actual bridge support (shared topology is insufficient)");
}
void asymmetricRegression(const QString& path){
    CadDocument document;check(document.open(QFileInfo(path).absoluteFilePath()).isEmpty(),"Open asymmetric retained Trim/ellipse regression");
    const auto before=document.authored();const auto trimmed=document.bodies().front();
    QString cutterId,middleId,ellipseId;QJsonObject panel,middle;
    for(const auto& row:before["features"].toArray()){const auto f=row.toObject();
        if(f["name"]=="Handle side panel"){panel=f;cutterId=f["parameters"].toObject()["entities"].toArray()[0].toObject()["id"].toString();}
        if(f["name"]=="Handle middle ellipse"){middle=f;middleId=f["id"].toString();
            for(const auto& e:f["parameters"].toObject()["entities"].toArray())if(e.toObject()["type"]=="ellipse")ellipseId=e.toObject()["id"].toString();}
    }
    check(!cutterId.isEmpty()&&!ellipseId.isEmpty(),"Regression retained sources unavailable");
    QJsonArray sections;std::vector<TopoDS_Edge> sourceEdges;
    for(const auto& face:trimmed.supportFaces){QString selected;
        for(const auto& e:trimmed.solidEdges)if(e.provenance["role"]=="trimBoundary"
            && e.provenance["sourceFaceId"]==face.provenance["sourceFaceId"] && e.provenance["sourceEntityId"]==cutterId)selected=e.edgeId;
        check(!selected.isEmpty(),"Regression actual Trim loop unavailable");
        auto reference=SurfaceClosedLoft::boundaryReference(document.evaluation(),trimmed.bodyId,selected);
        // Pick the other cutter on the opposite side: correspondence must use
        // shared ancestry rather than whichever edge the user selected first.
        if(!sections.isEmpty())reference=SurfaceClosedLoft::boundaryReference(document.evaluation(),trimmed.bodyId,reference["edgeIds"].toArray()[1].toString());
        sections.append(reference);
        for(const auto& id:reference["edgeIds"].toArray())for(const auto& e:trimmed.solidEdges)if(e.edgeId==id)sourceEdges.push_back(e.edge);
    }
    sections.insert(1,QJsonObject{{"sketchId",middleId},{"entityIds",QJsonArray{ellipseId}}});
    for(const auto& sketch:document.sketches())if(sketch.featureId==middleId)sourceEdges.push_back(sketch.entityEdges.value(ellipseId));
    const QJsonObject args{{"operation","newBody"},{"guideType","none"},{"sections",sections},{"name","Asymmetric bridge"},{"ownerId",trimmed.ownerId}};
    auto candidate=CadDocument::loftCandidate(before,args);const auto error=document.commit(candidate,"Asymmetric retained bridge");
    check(error.isEmpty(),error+" / "+document.lastEvaluationError()["code"].toString());
    const auto body=document.bodies().back();const auto authored=document.authored();
    check(body.summary["valid"].toBool()&&body.summary["topology"].toObject()["faces"]==2
        && body.solidEdges.size()==6 && !body.shape.Closed(),"Corrected bridge must be a valid two-face open shell");
    const auto mode=body.summary["interpolation"].toObject();
    check(!mode["compatibility"].toBool()&&mode["sectionNormalization"]=="retainedCutterRadialEllipseTwoEdges","Explicit correspondence mode missing");
    supportAgreement(body,sourceEdges);const auto faces=faceIds(body),edges=edgeIds(body);
    // Rigidly rotate the entire evaluated fixture, including retained point and
    // frame inspection. This exercises ellipse-local axes, not global X/Z.
    auto rotated=document.evaluation();gp_Trsf pose;pose.SetRotation(gp_Ax1(gp_Pnt(0,0,0),gp_Dir(1,2,3)),.43);pose.SetTranslationPart(gp_Vec(13,-7,19));
    const TopLoc_Location location(pose);
    auto pointArray=[&](QJsonArray xyz,bool direction){gp_XYZ value(xyz[0].toDouble(),xyz[1].toDouble(),xyz[2].toDouble());
        if(direction){gp_Vec v(value);v.Transform(pose);value=v.XYZ();}else{gp_Pnt p(value);p.Transform(pose);value=p.XYZ();}
        return QJsonArray{value.X(),value.Y(),value.Z()};};
    for(auto& body:rotated.bodies){body.shape=body.shape.Moved(location);
        for(auto& edge:body.solidEdges)edge.edge=TopoDS::Edge(edge.edge.Moved(location));
        for(auto& face:body.supportFaces){face.face=TopoDS::Face(face.face.Moved(location));
            for(auto& edge:face.boundaryEdges)edge.edge=TopoDS::Edge(edge.edge.Moved(location));}}
    for(auto& sketch:rotated.sketches){sketch.shape=sketch.shape.Moved(location);
        for(auto it=sketch.entityEdges.begin();it!=sketch.entityEdges.end();++it)it.value()=TopoDS::Edge(it.value().Moved(location));
        auto points=sketch.summary["worldPoints"].toArray();for(int i=0;i<points.size();++i){auto p=points[i].toObject();p["positionMm"]=pointArray(p["positionMm"].toArray(),false);points[i]=p;}sketch.summary["worldPoints"]=points;
        for(const auto& key:{"normal","uAxis","vAxis"})sketch.resolvedFrame[key]=pointArray(sketch.resolvedFrame[key].toArray(),true);
        sketch.resolvedFrame["origin"]=pointArray(sketch.resolvedFrame["origin"].toArray(),false);
    }
    const auto feature=candidate["features"].toArray().last().toObject();
    const auto rigid=SurfaceClosedLoft::evaluate(feature,rotated);check(faceIds(rigid)==faces&&edgeIds(rigid)==edges,"Rigid frame changed retained identities");
    std::vector<TopoDS_Edge> moved;for(const auto& edge:sourceEdges)moved.push_back(TopoDS::Edge(edge.Moved(location)));supportAgreement(rigid,moved);
    // Selecting the other first cutter reverses winding and swaps the arc
    // roles. Geometry/contact must agree without assuming identical face IDs.
    auto otherSections=sections;const auto firstReference=sections[0].toObject();
    otherSections[0]=SurfaceClosedLoft::boundaryReference(document.evaluation(),trimmed.bodyId,firstReference["edgeIds"].toArray()[1].toString());
    auto otherFeature=feature;auto otherParameters=otherFeature["parameters"].toObject();otherParameters["sections"]=otherSections;otherFeature["parameters"]=otherParameters;
    const auto other=SurfaceClosedLoft::evaluate(otherFeature,document.evaluation());supportAgreement(other,sourceEdges);
    check(std::abs(other.summary["areaMm2"].toDouble()-body.summary["areaMm2"].toDouble())<1e-5,"First cutter pick changed bridge geometry");
    // Small panel and ellipse-source edits retain topology IDs; history and
    // authored reopen remain separate from mere non-crashing construction.
    auto changed=editPoint(authored,panel["id"].toString(),panel["parameters"].toObject()["entities"].toArray()[0].toObject()["pointIds"].toArray()[2].toString(),"vMm",74.1);
    check(document.commit(changed,"Small panel fit-point edit").isEmpty(),"Asymmetric panel edit");
    check(faceIds(document.bodies().back())==faces&&edgeIds(document.bodies().back())==edges,"Panel edit changed stable identities");
    document.history().undo();check(document.authored()==authored,"Exact panel undo");document.history().redo();check(document.authored()==changed,"Exact panel redo");
    document.history().undo();
    auto transformEllipse=[&](const QJsonObject& base,double dx,double dz,double radians){
        auto result=base;auto features=result["features"].toArray();
        for(int i=0;i<features.size();++i)if(features[i].toObject()["id"]==middleId){
            auto f=features[i].toObject(),p=f["parameters"].toObject();auto points=p["points"].toArray();
            for(int k=0;k<points.size();++k){auto point=points[k].toObject();
                const double x=point["uMm"].toDouble()-114,z=point["vMm"].toDouble()-63;
                point["uMm"]=114+dx+x*std::cos(radians)-z*std::sin(radians);
                point["vMm"]=63+dz+x*std::sin(radians)+z*std::cos(radians);points[k]=point;
            }p["points"]=points;f["parameters"]=p;features[i]=f;break;
        }result["features"]=features;return result;
    };
    const auto ellipseEdit=transformEllipse(authored,.5,.25,.01);
    check(document.commit(ellipseEdit,"Small ellipse center and tilt edit").isEmpty(),"Asymmetric ellipse center/tilt edit");
    check(faceIds(document.bodies().back())==faces&&edgeIds(document.bodies().back())==edges,"Ellipse edit changed stable identities");
    document.history().undo();check(document.authored()==authored,"Exact ellipse undo");
    document.history().redo();check(document.authored()==ellipseEdit,"Exact ellipse redo");document.history().undo();
    // Put the valid ellipse center on the exterior extension of cutter A/B:
    // both endpoint rays coincide, so explicit splitting must refuse atomically.
    const auto bad=transformEllipse(authored,-225,-107.4,0);
    check(!document.commit(bad,"Coincident radial ellipse seams").isEmpty()&&document.authored()==authored
        &&document.lastEvaluationError()["code"]=="loft_section_unsupported","Ambiguous radial seams must refuse atomically");
    QTemporaryDir out;const auto saved=out.filePath("asymmetric-bridge.alloy");check(document.save(saved).isEmpty(),"Save asymmetric bridge");
    CadDocument reopened;check(reopened.open(saved).isEmpty()&&reopened.authored()==authored,"Reopen asymmetric bridge");
    check(faceIds(reopened.bodies().back())==faces&&edgeIds(reopened.bodies().back())==edges,"Reopen asymmetric identities");
    std::cout<<"ASYMMETRIC_BRIDGE_OK actualTrim smallTiltedEllipse oppositePick supportContact rigidFrame panelEdit history reopen atomicRefusal\n";
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    if(argc>1 && QString::fromLocal8Bit(argv[1])=="--asymmetric-fixture"){
        check(argc==3,"Provide one native asymmetric fixture path");asymmetricRegression(QString::fromLocal8Bit(argv[2]));return 0;
    }
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
    const auto candidate=CadDocument::loftCandidate(document.authored(),args);
    const auto error=document.commit(candidate,"Bilateral Rail Loft");check(error.isEmpty(),error+" / "+document.lastEvaluationError()["code"].toString());
    const auto source=document.bodies().back();
    auto tool=SketchGeometry::newFeature(root,"XZ","Handle lens");
    tool=addSpline(tool,QJsonArray{QJsonObject{{"uMm",30},{"vMm",15}},QJsonObject{{"uMm",60},{"vMm",30}},QJsonObject{{"uMm",90},{"vMm",15}}});
    tool=addSpline(tool,QJsonArray{QJsonObject{{"uMm",30},{"vMm",15}},QJsonObject{{"uMm",60},{"vMm",10}},QJsonObject{{"uMm",90},{"vMm",15}}});append(document,tool);
    const auto tools=tool["parameters"].toObject()["entities"].toArray();const QJsonArray toolIds{tools[0].toObject()["id"],tools[1].toObject()["id"]};
    const auto trimId=CadDocument::newId();const QJsonObject trimFeature{{"id",trimId},{"type","surfaceTrim"},{"parameters",QJsonObject{
        {"targetBodyId",source.bodyId},{"toolSketchId",tool["id"]},{"toolEntityIds",toolIds},{"removedRegionIds",QJsonArray{}},
        {"throughMarginMm",10.0},{"projectionMode","sketchNormalThroughTarget"},{"sourcePlane","XZ"},{"sourceNormal",QJsonArray{0,-1,0}}}}};
    const auto split=SurfaceTrim::split(trimFeature,source,document.evaluation());QJsonArray removed;
    for(const auto& region:split.regions)if(region.inside)removed.append(region.regionId);
    check(split.regions.size()==4 && removed.size()==2,"Two independent bilateral inside regions");
    const auto trim=CadDocument::surfaceTrimCandidate(document.authored(),{{"newFeatureId",trimId},{"ownerId",source.ownerId},{"targetBodyId",source.bodyId},
        {"toolSketchId",tool["id"]},{"toolEntityIds",toolIds},{"removedRegionIds",removed}});
    const auto trimError=document.commit(trim,"Trim bilateral handle");check(trimError.isEmpty(),trimError+" / "+document.lastEvaluationError()["code"].toString());

    const auto trimmed=document.bodies().back();holes(trimmed);
    auto middle=SketchGeometry::newFeature(root,"XZ","Handle middle ellipse");
    const auto ellipseResult=SketchGeometry::addCenterPointEllipse(middle,{{"centerUMm",60},{"centerVMm",15},
        {"firstUMm",30},{"firstVMm",15},{"contourUMm",60},{"contourVMm",27}});
    check(ellipseResult.ok,ellipseResult.error);middle=ellipseResult.feature;append(document,middle);
    const auto middleEntity=middle["parameters"].toObject()["entities"].toArray().last().toObject();
    std::array<QJsonObject,2> outer;
    for(int side=0;side<2;++side){QString selected;
        for(const auto& edge:trimmed.solidEdges)if(edge.provenance["role"]=="trimBoundary"
            && edge.provenance["sourceFaceId"]==trimmed.supportFaces[side].provenance["sourceFaceId"]
            && edge.provenance["sourceEntityId"]==tools[0].toObject()["id"])selected=edge.edgeId;
        check(!selected.isEmpty(),"Retained upper cutter boundary unavailable");
        outer[side]=SurfaceClosedLoft::boundaryReference(document.evaluation(),trimmed.bodyId,selected);
    }
    const QJsonArray bridgeSections{outer[0],QJsonObject{{"sketchId",middle["id"]},{"entityIds",QJsonArray{middleEntity["id"]}}},outer[1]};
    const QJsonObject bridgeArgs{{"operation","newBody"},{"guideType","none"},{"sections",bridgeSections},{"ownerId",root},{"name","Handle bridge"}};
    const auto beforeBridge=document.authored();
    if(argc>1)check(document.save(QString::fromLocal8Bit(argv[1])).isEmpty(),"Save actual boundary/ellipse source");
    const auto bridgeCandidate=CadDocument::loftCandidate(beforeBridge,bridgeArgs);
    const auto bridgeError=document.commit(bridgeCandidate,"Loft retained handle bridge");check(bridgeError.isEmpty(),bridgeError+" / "+document.lastEvaluationError()["code"].toString());
    const auto bridge=document.bodies().back();const auto authored=document.authored();
    check(bridge.summary["valid"].toBool() && bridge.summary["topology"].toObject()["faces"]==2
        && bridge.summary["topology"].toObject()["solids"]==0 && bridge.supportFaces.size()==2 && bridge.solidEdges.size()==6,"Expected valid uncapped retained two-face bridge");
    check(bridge.ownerId==root && authored["components"]==beforeBridge["components"] && document.bodies().size()==2,"New Body must respect active owner and preserve source component");
    const auto bridgeFaces=faceIds(bridge),bridgeEdges=edgeIds(bridge);const auto sourceEdges=edgeIds(trimmed);
    check(document.bodies().front().bodyId==trimmed.bodyId && document.bodies().front().ownerId==trimmed.ownerId
        && edgeIds(document.bodies().front())==sourceEdges
        && std::abs(document.bodies().front().summary["areaMm2"].toDouble()-trimmed.summary["areaMm2"].toDouble())<1e-8,"Bridge must preserve actual Trim result");
    auto checkSources=[&](const CadDocument& current){std::vector<TopoDS_Edge> edges;
        for(const auto& ref:outer)for(const auto& id:ref["edgeIds"].toArray())for(const auto& edge:current.bodies().front().solidEdges)if(edge.edgeId==id)edges.push_back(edge.edge);
        for(const auto& sketch:current.sketches())if(sketch.featureId==middle["id"])edges.push_back(sketch.entityEdges.value(middleEntity["id"].toString()));
        check(edges.size()==5,"Exact retained boundary/ellipse source set");supportAgreement(current.bodies().back(),edges);
    };checkSources(document);
    auto sourceEdit=editPoint(authored,rail["id"].toString(),railEntity["pointIds"].toArray()[1].toString(),"vMm",55);
    check(document.commit(sourceEdit,"Raise exterior crown").isEmpty(),"Exterior edit through actual Trim/bridge failed");
    check(faceIds(document.bodies().back())==bridgeFaces && edgeIds(document.bodies().back())==bridgeEdges,"Source edit changed bridge identities");checkSources(document);
    auto ellipseEdit=document.authored();QString secondAxis;
    for(const auto& entry:middle["parameters"].toObject()["entities"].toArray())if(entry.toObject()["id"]==middleEntity["secondAxisId"]){
        const auto axis=entry.toObject();
        ellipseEdit=editPoint(ellipseEdit,middle["id"].toString(),axis["startPointId"].toString(),"vMm",1);
        ellipseEdit=editPoint(ellipseEdit,middle["id"].toString(),axis["endPointId"].toString(),"vMm",29);break;
    }
    check(document.commit(ellipseEdit,"Edit middle ellipse minor radius").isEmpty(),"Middle ellipse edit failed");
    check(faceIds(document.bodies().back())==bridgeFaces && edgeIds(document.bodies().back())==bridgeEdges
        && std::abs(document.bodies().back().summary["areaMm2"].toDouble()-bridge.summary["areaMm2"].toDouble())>1,"Ellipse edit needs changed geometry with stable identity");checkSources(document);
    const auto edited=document.authored();document.history().undo();check(document.authored()==sourceEdit,"Undo ellipse edit");document.history().redo();check(document.authored()==edited,"Redo ellipse edit");
    QTemporaryDir directory;const auto path=argc>2?QString::fromLocal8Bit(argv[2]):directory.filePath("handle-bridge.alloy");
    check(document.save(path).isEmpty(),"Save bridge");CadDocument reopened;check(reopened.open(path).isEmpty() && reopened.authored()==edited,"Reopen retained bridge");
    check(faceIds(reopened.bodies().back())==bridgeFaces && edgeIds(reopened.bodies().back())==bridgeEdges,"Reopen identity");checkSources(reopened);
    auto bad=bridgeArgs;auto badSections=bridgeSections;auto broken=outer[0];auto brokenEdges=broken["edgeIds"].toArray();brokenEdges[0]=CadDocument::newId();broken["edgeIds"]=brokenEdges;badSections[0]=broken;
    bad["sections"]=badSections;bad["featureId"]=bridge.featureId;
    check(!document.commit(CadDocument::loftCandidate(edited,bad),"Unavailable retained chain").isEmpty() && document.authored()==edited,"Unavailable chain must refuse atomically");
    auto placed=edited;auto occurrences=placed["occurrences"].toArray();
    for(int i=0;i<occurrences.size();++i)if(occurrences[i].toObject()["definitionId"]==trimmed.ownerId){auto row=occurrences[i].toObject();
        row["placement"]=QJsonObject{{"axis","Z"},{"angleDeg",15},{"translationMm",QJsonArray{10,0,0}}};occurrences[i]=row;break;}
    placed["occurrences"]=occurrences;
    check(!document.commit(placed,"Unsupported placed boundary instance").isEmpty() && document.authored()==edited,"Placed instance input must refuse atomically");
    auto ownerArgs=bridgeArgs;ownerArgs["ownerId"]=trimmed.ownerId;const auto ownerCandidate=CadDocument::loftCandidate(beforeBridge,ownerArgs);
    check(ownerCandidate["features"].toArray().last().toObject()["ownerId"]==trimmed.ownerId,"Explicit active component owner must survive authoring");
    std::cout<<"SURFACE_HANDLE_BRIDGE_OK actualTrimLoops ellipse supportsContact sourceAndEllipseEdits identity history reopen atomicRefusal newBodyOwner\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
