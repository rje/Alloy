// SPDX-License-Identifier: MIT
#include "SketchGeometry.h"
#include "SketchSpline.h"
#include "CadDocument.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QUuid>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
QJsonObject accept(const SketchGeometry::SolveResult& result){if(!result.ok)throw std::runtime_error(result.error.toStdString());return result.feature;}
QJsonObject point(const QJsonObject& sketch,const QString& id){for(const auto& value:sketch["parameters"].toObject()["points"].toArray())
    if(value.toObject()["id"]==id)return value.toObject();throw std::runtime_error("Missing point");}
QJsonObject arc(const QJsonObject& sketch){for(const auto& value:sketch["parameters"].toObject()["entities"].toArray())
    if(value.toObject()["type"]=="arc")return value.toObject();throw std::runtime_error("Missing arc");}
void near(double a,double b){check(std::abs(a-b)<1e-5,"Unexpected solved coordinate");}
void checkHalfSweep(const QJsonObject& sketch,const QString& pointId){
    const auto e=arc(sketch),center=point(sketch,e["centerPointId"].toString()),target=point(sketch,pointId);
    const double angle=(e["startAngleDeg"].toDouble()+e["sweepAngleDeg"].toDouble()/2)*std::acos(-1.0)/180;
    near(target["uMm"].toDouble(),center["uMm"].toDouble()+e["radiusMm"].toDouble()*std::cos(angle));
    near(target["vMm"].toDouble(),center["vMm"].toDouble()+e["radiusMm"].toDouble()*std::sin(angle));
}
void append(CadDocument& document,const QJsonObject& feature){auto candidate=document.authored();
    auto features=candidate["features"].toArray();features.append(feature);candidate["features"]=features;
    check(document.commit(candidate,"Append section").isEmpty(),"Feature commit failed");}
QJsonObject pathPlane(const QString& owner,const QString& sketchId,const QString& entityId,int endpoint){
    return {{"id",CadDocument::newId()},{"ownerId",owner},{"type","constructionPlane"},
        {"name",endpoint?"End Plane":"Start Plane"},{"visible",true},
        {"parameters",QJsonObject{{"source",QJsonObject{{"path",QJsonObject{{"sketchId",sketchId},
            {"entityId",entityId},{"endpoint",endpoint}}}}}}}};}
void sourceArc(){
    const auto owner=QUuid::createUuid().toString(QUuid::WithoutBraces);
    auto sketch=SketchGeometry::newFeature(owner,"XY","Front profile");
    sketch=SketchGeometry::addArc(sketch,{{"startUMm",-25},{"startVMm",10},{"throughUMm",0},{"throughVMm",0},
        {"endUMm",25},{"endVMm",10}});
    const auto e=arc(sketch);
    const auto start=e["startPointId"],end=e["endPointId"];
    sketch=accept(SketchGeometry::addConstraint(sketch,{{"type","midpoint"},{"origin",true},
        {"entityIds",QJsonArray{e["id"]}}}));
    const auto origin=sketch["parameters"].toObject()["originPointId"].toString();
    check(!origin.isEmpty(),"Midpoint did not retain the sketch origin");
    checkHalfSweep(sketch,origin);
    sketch=accept(SketchGeometry::addDimension(sketch,{{"kind","horizontal"},{"pointIds",QJsonArray{start,end}},
        {"valueMm",50}}));
    const auto widthId=sketch["parameters"].toObject()["dimensions"].toArray().last().toObject().value("id");
    sketch=accept(SketchGeometry::addDimension(sketch,{{"kind","vertical"},{"pointIds",QJsonArray{origin,start}},
        {"valueMm",10}}));
    sketch=accept(SketchGeometry::addDimension(sketch,{{"kind","vertical"},{"pointIds",QJsonArray{origin,end}},
        {"valueMm",10}}));
    checkHalfSweep(sketch,origin);
    sketch=accept(SketchGeometry::updateDimension(sketch,{{"dimensionId",widthId},{"valueMm",60}}));
    checkHalfSweep(sketch,origin);
    near(point(sketch,start.toString())["uMm"].toDouble(),-30);
    near(point(sketch,end.toString())["uMm"].toDouble(),30);
    check(accept(SketchGeometry::solveDraft(sketch))==sketch,"Arc midpoint changed on reopen");

    auto opposite=SketchGeometry::addArc(SketchGeometry::newFeature(owner,"XY","Opposite sweep"),
        {{"startUMm",-25},{"startVMm",10},{"throughUMm",0},{"throughVMm",72.5},
            {"endUMm",25},{"endVMm",10}});
    const auto wrong=arc(opposite);
    const auto refused=SketchGeometry::addConstraint(opposite,{{"type","midpoint"},{"origin",true},
        {"entityIds",QJsonArray{wrong["id"]}}});
    check(!refused.ok && refused.feature==opposite && refused.error.contains("residual"),
        "Opposite arc-sweep midpoint committed a false branch");
}
void attachedSections(){
    CadDocument document;const auto owner=document.authored()["documentId"].toString();
    auto path=SketchGeometry::newFeature(owner,"YZ","Shoehorn spine");
    const auto spline=SketchSpline::add(path,{{"points",QJsonArray{QJsonObject{{"uMm",0},{"vMm",0}},
        QJsonObject{{"uMm",50},{"vMm",16}},QJsonObject{{"uMm",130},{"vMm",0}}}}});
    check(spline.ok,"Source spline failed");path=spline.feature;
    const auto source=path["parameters"].toObject()["entities"].toArray()[0].toObject();
    const auto fitPoints=source["pointIds"].toArray();
    path=accept(SketchGeometry::addConstraint(path,{{"type","fixed"},{"pointIds",QJsonArray{fitPoints[0]}}}));
    path=accept(SketchGeometry::addDimension(path,{{"kind","horizontal"},
        {"pointIds",QJsonArray{fitPoints[0],fitPoints[1]}},{"valueMm",50}}));
    append(document,path);
    const auto front=pathPlane(owner,path["id"].toString(),source["id"].toString(),0);
    const auto rear=pathPlane(owner,path["id"].toString(),source["id"].toString(),1);
    append(document,front);append(document,rear);
    auto first=SketchGeometry::newConstructionPlaneFeature(owner,front["id"].toString(),"Front arc");
    first=SketchGeometry::addArc(first,{{"startUMm",-25},{"startVMm",10},{"throughUMm",0},{"throughVMm",0},
        {"endUMm",25},{"endVMm",10}});
    const auto frontArc=arc(first);
    const auto start=frontArc["startPointId"],end=frontArc["endPointId"];
    first=accept(SketchGeometry::addConstraint(first,{{"type","midpoint"},{"origin",true},
        {"entityIds",QJsonArray{frontArc["id"]}}}));
    const auto frontOrigin=first["parameters"].toObject()["originPointId"].toString();
    first=accept(SketchGeometry::addDimension(first,{{"kind","horizontal"},{"pointIds",QJsonArray{start,end}},
        {"valueMm",50}}));
    first=accept(SketchGeometry::addDimension(first,{{"kind","vertical"},{"pointIds",QJsonArray{frontOrigin,start}},
        {"valueMm",10}}));
    first=accept(SketchGeometry::addDimension(first,{{"kind","vertical"},{"pointIds",QJsonArray{frontOrigin,end}},
        {"valueMm",10}}));
    append(document,first);
    auto last=SketchGeometry::newConstructionPlaneFeature(owner,rear["id"].toString(),"Rear line");
    last=SketchGeometry::addLine(last,{{"u1Mm",-15},{"v1Mm",0},{"u2Mm",15},{"v2Mm",0}});
    const auto line=last["parameters"].toObject()["entities"].toArray()[0].toObject();
    last=accept(SketchGeometry::addConstraint(last,{{"type","midpoint"},{"origin",true},
        {"entityIds",QJsonArray{line["id"]}}}));
    last=accept(SketchGeometry::addConstraint(last,{{"type","horizontal"},{"entityIds",QJsonArray{line["id"]}}}));
    last=accept(SketchGeometry::addDimension(last,{{"kind","linear"},
        {"pointIds",QJsonArray{line["startPointId"],line["endPointId"]}},{"valueMm",30}}));
    append(document,last);
    check(document.evaluation().constructionPlanes.size()==2 && document.sketches().size()==3,
        "Endpoint section topology is incomplete");
    const auto before=document.sketches()[1].resolvedFrame["normal"].toArray();
    auto candidate=document.authored();auto features=candidate["features"].toArray();
    auto changed=features[0].toObject(),parameters=changed["parameters"].toObject();
    auto dimensions=parameters["dimensions"].toArray();auto dimension=dimensions[0].toObject();
    dimension["valueMm"]=65;dimensions[0]=dimension;parameters["dimensions"]=dimensions;
    changed["parameters"]=parameters;features[0]=changed;candidate["features"]=features;
    check(document.commit(candidate,"Edit source spine").isEmpty(),"Source edit failed");
    const auto& sections=document.sketches();
    check(sections[1].resolvedFrame["normal"].toArray()!=before,"Front frame ignored source edit");
    check(sections[1].resolvedFrame==document.evaluation().constructionPlanes[0].frame &&
        sections[2].resolvedFrame==document.evaluation().constructionPlanes[1].frame,
        "Attached section frame detached from path plane");
    checkHalfSweep(features[3].toObject(),frontOrigin);
    const auto stable=document.authored();document.history().undo();document.history().redo();
    check(document.authored()==stable,"Section history changed authored constraints");
    QTemporaryDir directory;const auto filename=directory.filePath("arc-midpoint.alloy");
    check(document.save(filename).isEmpty(),"Save failed");CadDocument reopened;
    check(reopened.open(filename).isEmpty() && reopened.authored()==stable,"Reopen changed authored midpoint");
}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{sourceArc();attachedSections();std::cout<<"arc midpoint passed\n";return 0;}
    catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
