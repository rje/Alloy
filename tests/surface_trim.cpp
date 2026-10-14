// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include "SketchSpline.h"
#include "SurfaceTrim.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <BRepTools.hxx>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void append(CadDocument& document,const QJsonObject& feature){
    auto candidate=document.authored();auto rows=candidate["features"].toArray();rows.append(feature);candidate["features"]=rows;
    const auto error=document.commit(candidate,"Add source");if(!error.isEmpty())throw std::runtime_error(error.toStdString());
}
QJsonObject editPoint(const QJsonObject& authored,const QString& sketchId,const QString& pointId,double value){
    auto candidate=authored;auto rows=candidate["features"].toArray();
    for(int i=0;i<rows.size();++i)if(rows[i].toObject()["id"]==sketchId){
        auto sketch=rows[i].toObject();auto p=sketch["parameters"].toObject();auto points=p["points"].toArray();
        for(int j=0;j<points.size();++j)if(points[j].toObject()["id"]==pointId){auto point=points[j].toObject();point["vMm"]=value;points[j]=point;}
        p["points"]=points;sketch["parameters"]=p;rows[i]=sketch;break;
    }
    candidate["features"]=rows;return candidate;
}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    CadDocument document;const auto root=document.authored()["documentId"].toString();
    auto back=SketchGeometry::newFeature(root,"XZ","Back open arc");
    back=SketchGeometry::addArc(back,{{"startUMm",-25},{"startVMm",0},{"throughUMm",0},{"throughVMm",10},
        {"endUMm",25},{"endVMm",0}});append(document,back);
    const auto arc=back["parameters"].toObject()["entities"].toArray()[0].toObject();
    const QJsonObject plane{{"id",CadDocument::newId()},{"ownerId",root},{"type","constructionPlane"},
        {"name","Lower section plane"},{"visible",true},
        {"parameters",QJsonObject{{"source",QJsonObject{{"plane","XZ"}}},{"offsetMm",-80.0}}}};
    append(document,plane);
    auto lower=SketchGeometry::newConstructionPlaneFeature(root,plane["id"].toString(),"Mirrored lower chain");
    lower=SketchGeometry::addLine(lower,{{"u1Mm",0},{"v1Mm",-5},{"u2Mm",-25},{"v2Mm",0}});
    lower=SketchGeometry::addLine(lower,{{"u1Mm",0},{"v1Mm",-5},{"u2Mm",25},{"v2Mm",0}});
    append(document,lower);
    const auto lowerEntities=lower["parameters"].toObject()["entities"].toArray();
    auto rail=SketchGeometry::newFeature(root,"XY","Exterior guide rail");
    const auto railResult=SketchSpline::add(rail,{{"points",QJsonArray{
        QJsonObject{{"uMm",-25},{"vMm",0}},QJsonObject{{"uMm",-31},{"vMm",40}},
        QJsonObject{{"uMm",-25},{"vMm",80}}}}});
    check(railResult.ok,"Guide rail sketch failed");rail=railResult.feature;append(document,rail);
    const auto railEntity=rail["parameters"].toObject()["entities"].toArray()[0].toObject();
    QJsonArray sections{QJsonObject{{"sketchId",back["id"]},{"entityIds",QJsonArray{arc["id"]}}},
        QJsonObject{{"sketchId",lower["id"]},{"entityIds",QJsonArray{lowerEntities[0].toObject()["id"],lowerEntities[1].toObject()["id"]}}}};
    const QJsonObject railRef{{"sketchId",rail["id"]},{"entityId",railEntity["id"]}};
    auto loft=CadDocument::loftCandidate(document.authored(),{{"operation","newComponent"},{"guideType","rail"},
        {"componentName","Iron upper"},{"sections",sections},{"rail",railRef}});
    check(document.commit(loft,"Outer sheet").isEmpty(),"Outer rail sheet failed");
    const auto source=document.bodies().back();const auto sourceArea=source.summary["areaMm2"].toDouble();
    auto tool=SketchGeometry::newFeature(root,"XZ","Paired handle splines");
    const auto upper=SketchSpline::add(tool,{{"points",QJsonArray{
        QJsonObject{{"uMm",-20},{"vMm",3}},QJsonObject{{"uMm",0},{"vMm",10}},
        QJsonObject{{"uMm",20},{"vMm",3}}}}});check(upper.ok,"Upper trim spline failed");tool=upper.feature;
    const auto lowerTool=SketchSpline::add(tool,{{"points",QJsonArray{
        QJsonObject{{"uMm",-20},{"vMm",3}},QJsonObject{{"uMm",0},{"vMm",0}},
        QJsonObject{{"uMm",20},{"vMm",3}}}}});check(lowerTool.ok,"Lower trim spline failed");tool=lowerTool.feature;
    append(document,tool);
    const auto entities=tool["parameters"].toObject()["entities"].toArray();
    const QJsonArray toolIds{entities[0].toObject()["id"],entities[1].toObject()["id"]};
    const auto featureId=CadDocument::newId();
    const QJsonObject feature{{"id",featureId},{"type","surfaceTrim"},{"ownerId",source.ownerId},
        {"name","Handle opening"},{"visible",true},{"parameters",QJsonObject{{"targetBodyId",source.bodyId},
            {"toolSketchId",tool["id"]},{"toolEntityIds",toolIds},{"removedRegionIds",QJsonArray{}},
            {"throughMarginMm",10.0},{"projectionMode","sketchNormalThroughTarget"},
            {"sourcePlane","XZ"},{"sourceNormal",QJsonArray{0,-1,0}}}}};
    const auto split=SurfaceTrim::split(feature,source,document.evaluation());
    check(split.regions.size()==4,"Two separate inner region picks unavailable");
    QJsonArray removed;double removedArea=0;
    for(const auto& region:split.regions)if(region.inside){removed.append(region.regionId);removedArea+=region.areaMm2;}
    check(removed.size()==2 && removedArea>1000,"Expected two visible inner regions");
    auto args=QJsonObject{{"newFeatureId",featureId},{"ownerId",source.ownerId},{"targetBodyId",source.bodyId},
        {"toolSketchId",tool["id"]},{"toolEntityIds",toolIds},{"removedRegionIds",removed}};
    if(argc>1)check(document.save(QString::fromLocal8Bit(argv[1])).isEmpty(),"Save pre-Trim fixture");
    auto candidate=CadDocument::surfaceTrimCandidate(document.authored(),args);
    if(const auto error=document.commit(candidate,"Trim two inner regions");!error.isEmpty())
        throw std::runtime_error((error+" / "+document.lastEvaluationError()["code"].toString()).toStdString());
    const auto trimmed=document.bodies().back();const auto accepted=document.authored();
    check(trimmed.bodyId==source.bodyId && trimmed.summary["topology"].toObject()["faces"]==2
        && trimmed.summary["topology"].toObject()["solids"]==0 && trimmed.summary["valid"].toBool(),
        "Trim must leave the same valid open sheet body");
    check(std::abs((sourceArea-removedArea)-trimmed.summary["areaMm2"].toDouble())<.05,
        "Retained sheet area disagrees with selected inner regions");
    check(trimmed.supportFaces.size()==2 && trimmed.solidEdges.size()>=4,"Trim boundary lineage unavailable");
    if(argc>2)BRepTools::Write(trimmed.shape,argv[2]);
    const auto stableFace=trimmed.supportFaces[0].faceId;const auto stableEdges=trimmed.solidEdges.size();
    auto changed=editPoint(accepted,tool["id"].toString(),entities[0].toObject()["pointIds"].toArray()[1].toString(),11);
    check(document.commit(changed,"Edit upper handle spline").isEmpty(),"Tool sketch edit failed");
    check(document.bodies().back().supportFaces[0].faceId==stableFace
        && document.bodies().back().solidEdges.size()==stableEdges,"Trim lineage changed under source edit");
    const auto edited=document.authored();document.history().undo();check(document.authored()==accepted,"Undo tool edit");
    document.history().redo();check(document.authored()==edited,"Redo tool edit");
    QTemporaryDir temporary;const auto path=temporary.filePath("trim.alloy");
    check(document.save(path).isEmpty(),"Save Trim");CadDocument reopened;
    check(reopened.open(path).isEmpty() && reopened.authored()==edited
        && reopened.bodies().back().supportFaces[0].faceId==stableFace,"Reopen retained Trim");
    const auto broken=editPoint(edited,tool["id"].toString(),entities[0].toObject()["pointIds"].toArray()[0].toString(),4);
    check(!document.commit(broken,"Break tool loop").isEmpty() && document.authored()==edited,
        "Broken shared endpoint must refuse atomically");
    std::cout<<"SURFACE_TRIM_OK removed="<<removedArea<<" kept="<<trimmed.summary["areaMm2"].toDouble()<<'\n';
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
