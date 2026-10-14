// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include "SketchSpline.h"
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
    const auto error=document.commit(candidate,"Add rail source");if(!error.isEmpty())throw std::runtime_error(error.toStdString());
}
QJsonObject editPoint(const QJsonObject& authored,const QString& sketchId,const QString& pointId,const QString& axis,double value){
    auto candidate=authored;auto rows=candidate["features"].toArray();
    for(int i=0;i<rows.size();++i)if(rows[i].toObject()["id"]==sketchId){
        auto sketch=rows[i].toObject();auto p=sketch["parameters"].toObject();auto points=p["points"].toArray();
        for(int j=0;j<points.size();++j)if(points[j].toObject()["id"]==pointId){auto point=points[j].toObject();point[axis]=value;points[j]=point;}
        p["points"]=points;sketch["parameters"]=p;rows[i]=sketch;break;
    }
    candidate["features"]=rows;return candidate;
}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    CadDocument document;const auto root=document.authored()["documentId"].toString();
    auto back=SketchGeometry::newFeature(root,"XZ","Back open arc");
    back=SketchGeometry::addArc(back,{{"startUMm",-25},{"startVMm",0},{"throughUMm",0},{"throughVMm",10},
        {"endUMm",25},{"endVMm",0}});
    append(document,back);
    const auto arc=back["parameters"].toObject()["entities"].toArray()[0].toObject();
    const QJsonObject plane{{"id",CadDocument::newId()},{"ownerId",root},{"type","constructionPlane"},
        {"name","Lower section plane"},{"visible",true},
        {"parameters",QJsonObject{{"source",QJsonObject{{"plane","XZ"}}},{"offsetMm",-80.0}}}};
    append(document,plane);
    auto lower=SketchGeometry::newConstructionPlaneFeature(root,plane["id"].toString(),"Mirrored lower chain");
    lower=SketchGeometry::addLine(lower,{{"u1Mm",0},{"v1Mm",-5},{"u2Mm",-25},{"v2Mm",0}});
    lower=SketchGeometry::addLine(lower,{{"u1Mm",0},{"v1Mm",-5},{"u2Mm",25},{"v2Mm",0}});
    append(document,lower);
    const auto entities=lower["parameters"].toObject()["entities"].toArray();
    const auto left=entities[0].toObject(),right=entities[1].toObject();
    auto rail=SketchGeometry::newFeature(root,"XY","Exterior guide rail");
    const auto spline=SketchSpline::add(rail,{{"points",QJsonArray{
        QJsonObject{{"uMm",-25},{"vMm",0}},QJsonObject{{"uMm",-31},{"vMm",40}},
        QJsonObject{{"uMm",-25},{"vMm",80}}}}});
    check(spline.ok,"Could not create guide rail");rail=spline.feature;append(document,rail);
    const auto railEntity=rail["parameters"].toObject()["entities"].toArray()[0].toObject();
    const QJsonArray sections{QJsonObject{{"sketchId",back["id"]},{"entityIds",QJsonArray{arc["id"]}}},
        QJsonObject{{"sketchId",lower["id"]},{"entityIds",QJsonArray{left["id"],right["id"]}}}};
    const QJsonObject railRef{{"sketchId",rail["id"]},{"entityId",railEntity["id"]}};
    auto args=QJsonObject{{"operation","newComponent"},{"guideType","rail"},{"componentName","Iron upper"},
        {"name","Exterior rail sheet"},{"sections",sections},{"rail",railRef}};
    if(argc>1)check(document.save(QString::fromLocal8Bit(argv[1])).isEmpty(),"Could not save prepared source");
    auto candidate=CadDocument::loftCandidate(document.authored(),args);
    if(const auto error=document.commit(candidate,"Rail Loft");!error.isEmpty())
        throw std::runtime_error((error+" / "+document.lastEvaluationError()["code"].toString()).toStdString());
    const auto body=document.bodies().back();const auto initial=document.authored();
    check(body.summary["type"]=="surface" && body.summary["valid"]==true
        && body.summary["topology"].toObject()["faces"]==2
        && body.summary["topology"].toObject()["edges"]==7
        && body.summary["topology"].toObject()["solids"]==0,"Expected a valid two-face zero-solid sheet");
    check(body.supportFaces.size()==2 && body.solidEdges.size()==7
        && body.supportFaces[0].boundaryEdges.size()==4 && body.supportFaces[1].boundaryEdges.size()==4,
        "Unique section, rail and shared-middle lineage unavailable");
    if(argc>2)BRepTools::Write(body.shape,argv[2]);
    const auto area=body.summary["areaMm2"].toDouble();
    const auto face0=body.supportFaces[0].faceId,face1=body.supportFaces[1].faceId;
    const auto railMiddle=railEntity["pointIds"].toArray()[1].toString();
    auto changed=editPoint(initial,rail["id"].toString(),railMiddle,"uMm",-36);
    check(document.commit(changed,"Edit rail bend").isEmpty(),"Rail bend edit failed");
    check(document.bodies().back().supportFaces[0].faceId==face0 && document.bodies().back().supportFaces[1].faceId==face1
        && std::abs(document.bodies().back().summary["areaMm2"].toDouble()-area)>1,
        "Sheet did not follow the current rail with stable face IDs");
    const auto edited=document.authored();document.history().undo();
    check(document.authored()==initial && document.bodies().back().supportFaces[0].faceId==face0,"Undo rail edit");
    document.history().redo();check(document.authored()==edited,"Redo rail edit");
    QTemporaryDir temporary;const auto file=temporary.filePath("rail-loft.alloy");
    check(document.save(file).isEmpty(),"Save rail sheet");CadDocument reopened;
    check(reopened.open(file).isEmpty() && reopened.authored()==edited
        && reopened.bodies().back().supportFaces[0].faceId==face0,"Reopen rail sheet");
    const auto sourcePoint=railEntity["pointIds"].toArray()[0].toString();
    const auto broken=editPoint(document.authored(),rail["id"].toString(),sourcePoint,"uMm",-24);
    check(!document.commit(broken,"Break rail contact").isEmpty() && document.authored()==edited,
        "Broken rail-section contact must refuse atomically");
    auto disconnected=args;auto badSections=sections;auto second=badSections[1].toObject();
    second["entityIds"]=QJsonArray{left["id"],CadDocument::newId()};badSections[1]=second;
    disconnected["sections"]=badSections;disconnected["featureId"]=body.featureId;
    const auto invalid=CadDocument::loftCandidate(document.authored(),disconnected);
    check(!document.commit(invalid,"Missing section").isEmpty() && document.authored()==edited,
        "Missing lower section must refuse atomically");
    std::cout<<"SURFACE_RAIL_LOFT_OK area="<<area<<" faces="<<face0.toStdString()<<","<<face1.toStdString()<<'\n';
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
