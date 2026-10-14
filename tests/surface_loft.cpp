// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include "SketchSpline.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QJsonDocument>
#include <BRepTools.hxx>
#include <cmath>
#include <iostream>
#include <stdexcept>
namespace {
void check(bool v,const char* text){if(!v)throw std::runtime_error(text);}
void append(CadDocument& doc,const QJsonObject& f){auto a=doc.authored();auto rows=a["features"].toArray();rows.append(f);a["features"]=rows;const auto e=doc.commit(a,"Add source");if(!e.isEmpty())throw std::runtime_error(e.toStdString());}
QJsonObject plane(const QString& owner,const QString& sketch,const QString& entity,int end){return {{"id",CadDocument::newId()},{"ownerId",owner},{"type","constructionPlane"},{"name",end?"End Plane":"Start Plane"},{"visible",true},{"parameters",QJsonObject{{"source",QJsonObject{{"path",QJsonObject{{"sketchId",sketch},{"entityId",entity},{"endpoint",end}}}}}}}};}
QJsonObject source(CadDocument& doc){const auto owner=doc.authored()["documentId"].toString();
 auto path=SketchGeometry::newFeature(owner,"YZ","Path");auto spline=SketchSpline::add(path,{{"points",QJsonArray{QJsonObject{{"uMm",0},{"vMm",0}},QJsonObject{{"uMm",50},{"vMm",16}},QJsonObject{{"uMm",130},{"vMm",0}}}}});check(spline.ok,"Spline source");path=spline.feature;
 const auto pe=path["parameters"].toObject()["entities"].toArray()[0].toObject();auto r=SketchGeometry::addConstraint(path,{{"type","fixed"},{"pointIds",QJsonArray{pe["pointIds"].toArray()[0]}}});check(r.ok,"Fixed path");path=r.feature;append(doc,path);
 const auto front=plane(owner,path["id"].toString(),pe["id"].toString(),0),rear=plane(owner,path["id"].toString(),pe["id"].toString(),1);append(doc,front);append(doc,rear);
 auto first=SketchGeometry::newConstructionPlaneFeature(owner,front["id"].toString(),"Arc");first=SketchGeometry::addArc(first,{{"startUMm",-25},{"startVMm",10},{"throughUMm",0},{"throughVMm",0},{"endUMm",25},{"endVMm",10}});const auto ae=first["parameters"].toObject()["entities"].toArray()[0].toObject();r=SketchGeometry::addConstraint(first,{{"type","midpoint"},{"origin",true},{"entityIds",QJsonArray{ae["id"]}}});check(r.ok,"Arc midpoint");first=r.feature;append(doc,first);
 auto last=SketchGeometry::newConstructionPlaneFeature(owner,rear["id"].toString(),"Line");last=SketchGeometry::addLine(last,{{"u1Mm",-15},{"v1Mm",0},{"u2Mm",15},{"v2Mm",0}});const auto le=last["parameters"].toObject()["entities"].toArray()[0].toObject();r=SketchGeometry::addConstraint(last,{{"type","midpoint"},{"origin",true},{"entityIds",QJsonArray{le["id"]}}});check(r.ok,"Line midpoint");last=r.feature;append(doc,last);
 return {{"operation","newComponent"},{"componentName","Shoehorn"},{"name","Guided surface"},{"sections",QJsonArray{QJsonObject{{"sketchId",first["id"]},{"entityId",ae["id"]}},QJsonObject{{"sketchId",last["id"]},{"entityId",le["id"]}}}},{"centerline",QJsonObject{{"sketchId",path["id"]},{"entityId",pe["id"]}}}};
}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    CadDocument doc;auto args=source(doc);
    if(argc>1)check(doc.save(QString::fromLocal8Bit(argv[1])).isEmpty(),"Save source fixture");
    auto candidate=CadDocument::loftCandidate(doc.authored(),args);
    auto preview=CadDocument::evaluateCandidate(candidate);
    if(!preview.ok)throw std::runtime_error((preview.error+" "+QString::fromUtf8(QJsonDocument(preview.diagnostics).toJson(QJsonDocument::Compact))).toStdString());
    check(doc.commit(candidate,"Loft").isEmpty(),"Loft commit");const auto body=doc.bodies().back();
    check(body.summary["type"]=="surface" && body.summary["valid"]==true && body.summary["topology"].toObject()["faces"]==1
        && body.summary["topology"].toObject()["edges"]==4 && body.summary["topology"].toObject()["solids"]==0,"Open face summary");
    check(body.supportFaces.size()==1 && body.supportFaces[0].boundaryEdges.size()==4 && body.summary["supportFaces"].toArray()[0].toObject()["planar"]==false,"Curved face refs");
    if(argc>2)BRepTools::Write(body.shape,argv[2]);
    check(doc.authored()["components"].toArray().size()==1 && body.ownerId==doc.authored()["components"].toArray()[0].toObject()["id"],"New Component owner");
    const QJsonObject faceRef{{"bodyId",body.bodyId},{"faceId",body.supportFaces[0].faceId},{"producerFeatureId",body.featureId}};
    bool refused=false;try{GeometryKernel::resolveSketchSupport(doc.evaluation(),faceRef);}catch(const GeometryKernel::EvaluationError&){refused=true;}
    check(refused,"Curved sheet incorrectly accepted as planar sketch support");
    const auto initial=doc.authored();const auto faceId=body.supportFaces[0].faceId;const auto area=body.summary["areaMm2"].toDouble();
    auto changed=initial;auto features=changed["features"].toArray();auto path=features[0].toObject();auto parameters=path["parameters"].toObject();auto points=parameters["points"].toArray();
    const auto middle=parameters["entities"].toArray()[0].toObject()["pointIds"].toArray()[1].toString();
    bool moved=false;for(int i=0;i<points.size();++i)if(points[i].toObject()["id"]==middle){auto fit=points[i].toObject();fit["vMm"]=20.0;points[i]=fit;moved=true;}
    check(moved,"Middle fit point unavailable");parameters["points"]=points;path["parameters"]=parameters;features[0]=path;changed["features"]=features;
    check(doc.commit(changed,"Edit spline bend").isEmpty(),"Spline edit failed");
    check(doc.bodies().back().supportFaces[0].faceId==faceId && std::abs(doc.bodies().back().summary["areaMm2"].toDouble()-area)>1,
        "Surface did not follow edited retained centerline with stable identity");
    const auto edited=doc.authored();doc.history().undo();check(doc.authored()==initial && doc.bodies().back().supportFaces[0].faceId==faceId,"Undo source edit");
    doc.history().redo();check(doc.authored()==edited && doc.bodies().back().supportFaces[0].faceId==faceId,"Redo source edit");
    QTemporaryDir directory;const auto filename=directory.filePath("surface-loft.alloy");check(doc.save(filename).isEmpty(),"Save sheet");CadDocument opened;
    check(opened.open(filename).isEmpty() && opened.authored()==edited && opened.bodies().back().supportFaces[0].faceId==faceId,"Reopen sheet");
    auto invalid=args;invalid["centerline"]=QJsonObject{{"sketchId",args["centerline"].toObject()["sketchId"]},{"entityId",CadDocument::newId()}};
    const auto bad=CadDocument::loftCandidate(opened.authored(),{{"featureId",body.featureId},{"operation","newComponent"},
        {"sections",args["sections"]},{"centerline",invalid["centerline"]}});
    check(!opened.commit(bad,"Invalid guide").isEmpty() && opened.authored()==edited,"Missing centerline edit was not atomic");
    std::cout<<"SURFACE_LOFT_OK area="<<area<<" face="<<faceId.toStdString()<<"\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
