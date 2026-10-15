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
    CadDocument doc;auto args=source(doc);check(doc.commit(CadDocument::loftCandidate(doc.authored(),args),"Loft").isEmpty(),"Loft source");
    const auto body=doc.bodies().back();const auto face=body.supportFaces[0];const auto edge=body.solidEdges[1];
    const QJsonObject tangent{{"face",QJsonObject{{"bodyId",body.bodyId},{"faceId",face.faceId},{"producerFeatureId",face.producerFeatureId}}},
        {"point",QJsonObject{{"bodyId",body.bodyId},{"edgeId",edge.edgeId},{"endpoint",0}}}};
    const auto id=CadDocument::newId();const QJsonObject plane{{"id",id},{"ownerId",body.ownerId},{"type","constructionPlane"},{"name","Retained endpoint tangent"},{"visible",true},
        {"parameters",QJsonObject{{"source",QJsonObject{{"tangent",tangent}}}}}};
    append(doc,plane);const auto frame=doc.evaluation().constructionPlanes.back().frame;
    const auto origin=frame["origin"].toArray(),normal=frame["normal"].toArray();
    const auto expected=edge.provenance["startMm"].toArray();
    for(int i=0;i<3;++i)check(std::abs(origin[i].toDouble()-expected[i].toDouble())<1e-7,"Plane missed retained endpoint");
    auto dot=[](const QJsonArray& a,const QJsonArray& b){double d=0;for(int i=0;i<3;++i)d+=a[i].toDouble()*b[i].toDouble();return d;};
    check(std::abs(dot(normal,normal)-1)<1e-8 && std::abs(dot(normal,frame["uAxis"].toArray()))<1e-8 && std::abs(dot(normal,frame["vAxis"].toArray()))<1e-8,"Nonorthonormal tangent frame");
    auto sketch=SketchGeometry::newConstructionPlaneFeature(body.ownerId,id,"Hole sketch");
    sketch=SketchGeometry::withOriginReference(sketch);
    const auto originId=sketch["parameters"].toObject()["originPointId"].toString();
    check(!originId.isEmpty() && sketch["parameters"].toObject()["entities"].toArray().isEmpty(),"Origin witness authored geometry");
    sketch=SketchGeometry::addCircle(sketch,{{"centerUMm",0},{"centerVMm",10},{"diameterMm",5}});
    const auto circle=sketch["parameters"].toObject()["entities"].toArray().last().toObject();
    auto located=SketchGeometry::addConstraint(sketch,{{"type","horizontalVertical"},
        {"pointIds",QJsonArray{originId,circle["centerPointId"]}}});check(located.ok,"Y-axis alignment");sketch=located.feature;
    located=SketchGeometry::addDimension(sketch,{{"kind","vertical"},{"pointIds",QJsonArray{originId,circle["centerPointId"]}},
        {"driving",true},{"valueMm",10}});check(located.ok,"Driving endpoint distance");sketch=located.feature;
    check(sketch["parameters"].toObject()["dimensions"].toArray().last().toObject()["valueMm"].toDouble()==10,"10 mm dimension unavailable");
    append(doc,sketch);
    const auto stable=doc.authored();doc.history().undo();doc.history().redo();check(doc.authored()==stable,"Undo/redo changed tangent support");
    QTemporaryDir dir;const auto filename=dir.filePath("tangent.alloy");check(doc.save(filename).isEmpty(),"Save tangent");CadDocument opened;check(opened.open(filename).isEmpty() && opened.authored()==stable,"Reopen tangent");
    auto candidate=stable;auto rows=candidate["features"].toArray();auto path=rows[0].toObject();auto p=path["parameters"].toObject();auto points=p["points"].toArray();
    const auto middle=p["entities"].toArray()[0].toObject()["pointIds"].toArray()[1];
    for(int i=0;i<points.size();++i)if(points[i].toObject()["id"]==middle){auto point=points[i].toObject();point["vMm"]=20;points[i]=point;}
    p["points"]=points;path["parameters"]=p;rows[0]=path;candidate["features"]=rows;check(opened.commit(candidate,"Edit bend").isEmpty(),"Tangent parent edit");
    const auto changed=opened.evaluation().constructionPlanes.back().frame;double difference=0;for(int i=0;i<3;++i)difference+=std::abs(normal[i].toDouble()-changed["normal"].toArray()[i].toDouble());
    check(difference>1e-3,"Tangent normal ignored source edit");
    check(opened.sketches().back().resolvedFrame==changed,"Sketch did not follow tangent support");
    const auto edited=opened.authored();candidate=edited;rows=candidate["features"].toArray();
    for(int i=0;i<rows.size();++i)if(rows[i].toObject()["id"]==id){auto f=rows[i].toObject();auto t=tangent;auto point=t["point"].toObject();point["edgeId"]=CadDocument::newId();t["point"]=point;f["parameters"]=QJsonObject{{"source",QJsonObject{{"tangent",t}}}};rows[i]=f;}
    candidate["features"]=rows;check(!opened.commit(candidate,"Missing endpoint").isEmpty() && opened.authored()==edited,"Missing endpoint was not atomic");
    if(argc>1)check(opened.save(QString::fromLocal8Bit(argv[1])).isEmpty(),"Save source fixture");
    std::cout<<"TANGENT_POINT_PLANE_OK\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
