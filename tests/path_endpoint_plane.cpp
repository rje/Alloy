// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include "SketchSpline.h"
#include <BRepAdaptor_Curve.hxx>
#include <QCoreApplication>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool okay,const char* message){if(!okay)throw std::runtime_error(message);}
void append(CadDocument& document,const QJsonObject& feature){auto candidate=document.authored();
    auto features=candidate["features"].toArray();features.append(feature);candidate["features"]=features;
    check(document.commit(candidate,"Append feature").isEmpty(),"Feature commit failed");}
double dot(const QJsonArray& a,const QJsonArray& b){double result=0;for(int i=0;i<3;++i)result+=a[i].toDouble()*b[i].toDouble();return result;}
double distance(const QJsonArray& a,const QJsonArray& b){double squared=0;for(int i=0;i<3;++i)squared+=std::pow(a[i].toDouble()-b[i].toDouble(),2);return std::sqrt(squared);}
QJsonObject pathPlane(const QString& owner,const QString& sketchId,const QString& entityId,int endpoint){
    return {{"id",CadDocument::newId()},{"ownerId",owner},{"type","constructionPlane"},
        {"name",endpoint?"End Plane":"Start Plane"},{"visible",true},
        {"parameters",QJsonObject{{"source",QJsonObject{{"path",QJsonObject{{"sketchId",sketchId},
            {"entityId",entityId},{"endpoint",endpoint}}}}}}}};}
}

int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    CadDocument document;const auto owner=document.authored()["documentId"].toString();
    auto path=SketchGeometry::newFeature(owner,"YZ");
    const auto spline=SketchSpline::add(path,{{"points",QJsonArray{QJsonObject{{"uMm",0},{"vMm",0}},
        QJsonObject{{"uMm",50},{"vMm",16}},QJsonObject{{"uMm",130},{"vMm",0}}}}});
    check(spline.ok,"Source fit spline failed");path=spline.feature;
    const auto entity=path["parameters"].toObject()["entities"].toArray()[0].toObject();
    const auto pointIds=entity["pointIds"].toArray();
    auto fixed=SketchGeometry::addConstraint(path,{{"type","fixed"},{"pointIds",QJsonArray{pointIds[0]}}});
    check(fixed.ok,"Fixing the path start failed");path=fixed.feature;
    auto dimension=SketchGeometry::addDimension(path,{{"kind","horizontal"},
        {"pointIds",QJsonArray{pointIds[0],pointIds[1]}},{"valueMm",50}});
    check(dimension.ok,"Source path span dimension failed");path=dimension.feature;
    const auto sketchId=path["id"].toString(),entityId=entity["id"].toString();append(document,path);
    auto first=pathPlane(owner,sketchId,entityId,0),last=pathPlane(owner,sketchId,entityId,1);
    append(document,first);append(document,last);
    const auto frames=document.evaluation().constructionPlanes;
    check(frames.size()==2,"Two endpoint planes missing");
    const QJsonArray beginning{0,0,0},ending{0,130,0};
    check(distance(frames[0].frame["origin"].toArray(),beginning)<1e-7 &&
        distance(frames[1].frame["origin"].toArray(),ending)<1e-7,"Planes do not sit at authored path ends");
    for(const auto& plane:frames){const auto frame=plane.frame;
        const auto u=frame["uAxis"].toArray(),v=frame["vAxis"].toArray(),n=frame["normal"].toArray();
        check(std::abs(dot(u,n))<1e-9 && std::abs(dot(v,n))<1e-9 &&
            std::abs(dot(u,v))<1e-9 && std::abs(dot(n,n)-1)<1e-9,"Endpoint plane frame is not orthonormal");
        check(u[0].toDouble()>.999999,"Source sketch normal did not set stable in-plane axis");}
    auto section=SketchGeometry::newConstructionPlaneFeature(owner,last["id"].toString(),"End section");
    section=SketchGeometry::addRectangle(section,{{"u1Mm",-15},{"v1Mm",-5},{"u2Mm",15},{"v2Mm",5}});
    append(document,section);
    check(distance(document.sketches()[1].resolvedFrame["origin"].toArray(),ending)<1e-7,
        "Dependent end sketch did not use path plane");
    const auto beforeNormal=frames[1].frame["normal"].toArray();
    auto candidate=document.authored();auto features=candidate["features"].toArray();auto changedPath=features[0].toObject();
    auto changedParameters=changedPath["parameters"].toObject();auto dimensions=changedParameters["dimensions"].toArray();
    auto changedDimension=dimensions[0].toObject();changedDimension["valueMm"]=65;dimensions[0]=changedDimension;
    changedParameters["dimensions"]=dimensions;changedPath["parameters"]=changedParameters;features[0]=changedPath;
    const auto originalPoints=path["parameters"].toObject()["points"].toArray();
    check(changedParameters["points"].toArray()==originalPoints,"Dimension edit unexpectedly rewrote authored fit points");
    candidate["features"]=features;check(document.commit(candidate,"Edit fit spline").isEmpty(),"Source edit failed");
    const auto solved=document.sketches()[0].summary["worldPoints"].toArray();
    bool middleMoved=false;for(const auto& row:solved)if(row.toObject()["id"]==pointIds[1])
        middleMoved=std::abs(row.toObject()["positionMm"].toArray()[1].toDouble()-65)<1e-6;
    check(middleMoved,"Dimension did not move the evaluated fit point beyond authored coordinates");
    const auto changed=document.evaluation().constructionPlanes[1].frame;
    check(distance(changed["normal"].toArray(),beforeNormal)>1e-3,"End plane tangent did not follow source edit");
    const BRepAdaptor_Curve evaluatedCurve(document.sketches()[0].entityEdges.value(entityId));
    gp_Pnt evaluatedEnd;gp_Vec evaluatedTangent;
    evaluatedCurve.D1(evaluatedCurve.LastParameter(),evaluatedEnd,evaluatedTangent);
    evaluatedTangent.Normalize();
    const auto normal=changed["normal"].toArray(),origin=changed["origin"].toArray();
    check(std::abs(normal[0].toDouble()-evaluatedTangent.X())<1e-8 &&
        std::abs(normal[1].toDouble()-evaluatedTangent.Y())<1e-8 &&
        std::abs(normal[2].toDouble()-evaluatedTangent.Z())<1e-8 &&
        evaluatedEnd.Distance(gp_Pnt(origin[0].toDouble(),origin[1].toDouble(),origin[2].toDouble()))<1e-8,
        "Plane does not use the dimension-solved evaluated edge endpoint and tangent");
    check(distance(document.sketches()[1].resolvedFrame["normal"].toArray(),changed["normal"].toArray())<1e-9,
        "Dependent section did not follow edited endpoint plane");
    const auto stable=document.authored();document.history().undo();document.history().redo();
    check(document.authored()==stable,"History did not restore authored path plane");
    QTemporaryDir directory;const auto filename=directory.filePath("path-plane.alloy");
    check(document.save(filename).isEmpty(),"Save failed");CadDocument reopened;
    check(reopened.open(filename).isEmpty() && reopened.authored()==stable,"Reopen changed authored path plane");
    candidate=stable;features=candidate["features"].toArray();auto bad=features[1].toObject();
    auto parameters=bad["parameters"].toObject();auto source=parameters["source"].toObject();
    auto reference=source["path"].toObject();reference["entityId"]=CadDocument::newId();
    source["path"]=reference;parameters["source"]=source;bad["parameters"]=parameters;features[1]=bad;
    candidate["features"]=features;
    check(!reopened.commit(candidate,"Invalid path").isEmpty() && reopened.authored()==stable,
        "Unavailable source edit was not atomic");
    std::cout<<"retained fit-spline endpoint path planes pass\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
