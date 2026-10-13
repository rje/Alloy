// SPDX-License-Identifier: MIT
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QJsonDocument>
#include <QUuid>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
QJsonObject accept(const SketchGeometry::SolveResult& result){if(!result.ok)throw std::runtime_error(result.error.toStdString());return result.feature;}
QJsonObject entity(const QJsonObject& sketch,int index){return sketch["parameters"].toObject()["entities"].toArray()[index].toObject();}
QJsonObject point(const QJsonObject& sketch,const QJsonValue& id){for(const auto& value:sketch["parameters"].toObject()["points"].toArray())if(value.toObject()["id"]==id)return value.toObject();throw std::runtime_error("Missing point");}
void near(double a,double b){check(std::abs(a-b)<1e-6,"Unexpected solved coordinate");}

void sourceArcFixture(){
    const auto owner=QUuid::createUuid().toString(QUuid::WithoutBraces);
    auto sketch=SketchGeometry::addRectangle(SketchGeometry::newFeature(owner,"XY"),{{"u1Mm",-16},{"v1Mm",40},{"u2Mm",15},{"v2Mm",13}});
    sketch=accept(SketchGeometry::setConstruction(sketch,{{"entityIds",QJsonArray{entity(sketch,1)["id"],entity(sketch,3)["id"]}},{"construction",true}}));
    sketch=SketchGeometry::addCircle(sketch,{{"centerUMm",0},{"centerVMm",0},{"diameterMm",10}});
    const auto circle=entity(sketch,4);
    // The vertical rectangle sides are retained construction supports for
    // two outward arcs. The arc endpoints reuse the exact corner point IDs.
    sketch=SketchGeometry::addArc(sketch,{{"startUMm",-16},{"startVMm",40},{"throughUMm",-25},{"throughVMm",26.5},{"endUMm",-16},{"endVMm",13}});
    sketch=SketchGeometry::addArc(sketch,{{"startUMm",15},{"startVMm",13},{"throughUMm",24},{"throughVMm",26.5},{"endUMm",15},{"endVMm",40}});
    sketch=accept(SketchGeometry::solveDraft(sketch));
    const auto left=entity(sketch,5),right=entity(sketch,6);
    check(left["startPointId"]==entity(sketch,3)["endPointId"] && right["endPointId"]==entity(sketch,1)["startPointId"],"Arc did not reuse rectangle corners");
    const auto firstCenter=point(sketch,left["centerPointId"]);
    const auto applied=SketchGeometry::addConstraint(sketch,{{"type","concentric"},{"entityIds",QJsonArray{left["id"],right["id"]}}});
    if(!applied.ok)std::cerr << QJsonDocument(applied.diagnostics).toJson().constData() << applied.error.toStdString() << '\n';
    sketch=accept(applied);
    near(point(sketch,left["centerPointId"])["uMm"].toDouble(),firstCenter["uMm"].toDouble());
    near(point(sketch,left["centerPointId"])["vMm"].toDouble(),firstCenter["vMm"].toDouble());
    near(point(sketch,left["centerPointId"])["uMm"].toDouble(),point(sketch,right["centerPointId"])["uMm"].toDouble());
    near(point(sketch,left["centerPointId"])["vMm"].toDouble(),point(sketch,right["centerPointId"])["vMm"].toDouble());
    check(sketch["parameters"].toObject()["constraints"].toArray().last().toObject()["type"]=="concentric","Concentric relation lost");
    sketch=accept(SketchGeometry::addConstraint(sketch,{{"type","horizontalVertical"},{"pointIds",QJsonArray{circle["centerPointId"],left["centerPointId"]}}}));
    check(sketch["parameters"].toObject()["constraints"].toArray().last().toObject()["type"]=="vertical","Center alignment chose the wrong axis");
    near(point(sketch,circle["centerPointId"])["uMm"].toDouble(),0);
    near(point(sketch,left["centerPointId"])["uMm"].toDouble(),point(sketch,circle["centerPointId"])["uMm"].toDouble());
    sketch=accept(SketchGeometry::addConstraint(sketch,{{"type","equal"},{"entityIds",QJsonArray{left["id"],right["id"]}}}));
    near(entity(sketch,5)["radiusMm"].toDouble(),entity(sketch,6)["radiusMm"].toDouble());
    const auto repeated=SketchGeometry::addConstraint(sketch,{{"type","equal"},{"entityIds",QJsonArray{left["id"],right["id"]}}});
    check(!repeated.ok && repeated.feature==sketch,"Unrelated redundancy was accepted in Concentric sketch");
    const auto repeatedConcentric=SketchGeometry::addConstraint(sketch,{{"type","concentric"},{"entityIds",QJsonArray{right["id"],left["id"]}}});
    check(!repeatedConcentric.ok && repeatedConcentric.feature==sketch,"Duplicate Concentric relation was accepted");
    check(accept(SketchGeometry::solveDraft(QJsonDocument::fromJson(QJsonDocument(sketch).toJson()).object()))==sketch,"Concentric roundtrip changed authored sketch");
}

void pointAlignmentAndMidpoint(){
    const auto owner=QUuid::createUuid().toString(QUuid::WithoutBraces);
    auto sketch=SketchGeometry::addLine(SketchGeometry::newFeature(owner,"XY"),{{"u1Mm",0},{"v1Mm",0},{"u2Mm",20},{"v2Mm",0}});
    const auto source=entity(sketch,0);
    sketch=accept(SketchGeometry::addPointerEntity(sketch,"line",{{"u1Mm",9.7},{"v1Mm",0.2},{"u2Mm",9.8},{"v2Mm",15},
        {"startSnapTarget",QJsonObject{{"kind","midpoint"},{"entityId",source["id"]}}}}));
    const auto line=entity(sketch,1),first=point(sketch,line["startPointId"]);
    near(first["uMm"].toDouble(),10);near(first["vMm"].toDouble(),0);
    const auto constraints=sketch["parameters"].toObject()["constraints"].toArray();
    check(constraints.last().toObject()["type"]=="midpoint","Line start midpoint relation missing");
    auto other=SketchGeometry::addLine(sketch,{{"u1Mm",-4},{"v1Mm",8},{"u2Mm",4},{"v2Mm",8}});
    const auto free=entity(other,2);
    other=accept(SketchGeometry::addConstraint(other,{{"type","horizontal"},{"pointIds",QJsonArray{line["endPointId"],free["startPointId"]}}}));
    near(point(other,free["startPointId"])["vMm"].toDouble(),point(other,line["endPointId"])["vMm"].toDouble());
    check(other["parameters"].toObject()["constraints"].toArray().last().toObject()["type"]=="horizontal","Point H relation missing");
    check(accept(SketchGeometry::solveDraft(other))==other,"Point alignment was not stable on reopen");

    auto both=SketchGeometry::addLine(SketchGeometry::newFeature(QUuid::createUuid().toString(QUuid::WithoutBraces),"XY"),
        {{"u1Mm",0},{"v1Mm",0},{"u2Mm",20},{"v2Mm",0}});
    both=SketchGeometry::addLine(both,{{"u1Mm",30},{"v1Mm",0},{"u2Mm",50},{"v2Mm",0}});
    const auto a=entity(both,0),b=entity(both,1);
    both=accept(SketchGeometry::addConstraint(both,{{"type","fixed"},{"pointIds",QJsonArray{a["startPointId"]}}}));
    both=accept(SketchGeometry::addConstraint(both,{{"type","horizontal"},{"entityIds",QJsonArray{a["id"]}}}));
    both=accept(SketchGeometry::addDimension(both,{{"kind","horizontal"},{"entityId",a["id"]},{"valueMm",20}}));
    const auto widthId=both["parameters"].toObject()["dimensions"].toArray().last().toObject()["id"];
    both=accept(SketchGeometry::addPointerEntity(both,"line",{{"u1Mm",9.7},{"v1Mm",0.2},{"u2Mm",40.2},{"v2Mm",-0.1},
        {"startSnapTarget",QJsonObject{{"kind","midpoint"},{"entityId",a["id"]}}},
        {"endSnapTarget",QJsonObject{{"kind","midpoint"},{"entityId",b["id"]}}}}));
    const auto bridge=entity(both,2);const auto entries=both["parameters"].toObject()["constraints"].toArray();
    check(entries.size()>=2 && entries[entries.size()-2].toObject()["type"]=="midpoint" && entries.last().toObject()["type"]=="midpoint","Both endpoint relations were not retained");
    near(point(both,bridge["startPointId"])["uMm"].toDouble(),10);
    near(point(both,bridge["endPointId"])["uMm"].toDouble(),40);
    both=accept(SketchGeometry::updateDimension(both,{{"dimensionId",widthId},{"valueMm",30}}));
    near(point(both,bridge["startPointId"])["uMm"].toDouble(),15);
    check(accept(SketchGeometry::solveDraft(both))==both,"Two-endpoint midpoint line drifted on reopen");
}

void conflictingCenters(){
    const auto owner=QUuid::createUuid().toString(QUuid::WithoutBraces);
    auto sketch=SketchGeometry::addCircle(SketchGeometry::newFeature(owner,"XY"),{{"centerUMm",0},{"centerVMm",0},{"diameterMm",10}});
    sketch=SketchGeometry::addCircle(sketch,{{"centerUMm",10},{"centerVMm",0},{"diameterMm",12}});
    const auto a=entity(sketch,0),b=entity(sketch,1);
    sketch=accept(SketchGeometry::addConstraint(sketch,{{"type","fixed"},{"pointIds",QJsonArray{a["centerPointId"]}}}));
    sketch=accept(SketchGeometry::addConstraint(sketch,{{"type","fixed"},{"pointIds",QJsonArray{b["centerPointId"]}}}));
    const auto conflict=SketchGeometry::addConstraint(sketch,{{"type","concentric"},{"entityIds",QJsonArray{a["id"],b["id"]}}});
    check(!conflict.ok && conflict.feature==sketch,"Incompatible fixed centers were accepted");
}
void pairRoutesAndReverseOrder(){
    const auto owner=QUuid::createUuid().toString(QUuid::WithoutBraces);
    auto sketch=SketchGeometry::addCircle(SketchGeometry::newFeature(owner,"XY"),{{"centerUMm",0},{"centerVMm",0},{"diameterMm",8}});
    sketch=SketchGeometry::addArc(sketch,{{"startUMm",11},{"startVMm",4},{"throughUMm",13},{"throughVMm",2},{"endUMm",11},{"endVMm",0}});
    const auto circle=entity(sketch,0),arc=entity(sketch,1);
    sketch=accept(SketchGeometry::addConstraint(sketch,{{"type","concentric"},{"entityIds",QJsonArray{circle["id"],arc["id"]}}}));
    near(point(sketch,arc["centerPointId"])["uMm"].toDouble(),0);
    near(point(sketch,arc["centerPointId"])["vMm"].toDouble(),0);
    sketch=SketchGeometry::addCircle(sketch,{{"centerUMm",16},{"centerVMm",-3},{"diameterMm",5}});
    const auto second=entity(sketch,2);
    sketch=accept(SketchGeometry::addConstraint(sketch,{{"type","concentric"},{"entityIds",QJsonArray{circle["id"],second["id"]}}}));
    near(point(sketch,second["centerPointId"])["uMm"].toDouble(),0);

    auto lines=SketchGeometry::addLine(SketchGeometry::newFeature(QUuid::createUuid().toString(QUuid::WithoutBraces),"XY"),
        {{"u1Mm",3},{"v1Mm",2},{"u2Mm",8},{"v2Mm",4}});
    lines=SketchGeometry::addLine(lines,{{"u1Mm",-5},{"v1Mm",10},{"u2Mm",-2},{"v2Mm",13}});
    const auto first=entity(lines,0),last=entity(lines,1);
    lines=accept(SketchGeometry::addConstraint(lines,{{"type","vertical"},{"pointIds",QJsonArray{last["startPointId"],first["startPointId"]}}}));
    near(point(lines,last["startPointId"])["uMm"].toDouble(),-5);
    near(point(lines,first["startPointId"])["uMm"].toDouble(),-5);
    for(const auto& value:lines["parameters"].toObject()["constraints"].toArray())check(value.toObject()["type"]!="fixed","Point order created hidden Fixed intent");
}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{sourceArcFixture();pointAlignmentAndMidpoint();conflictingCenters();pairRoutesAndReverseOrder();std::cout << "outlet constraints passed\n";return 0;}catch(const std::exception& error){std::cerr << error.what() << '\n';return 1;}}
