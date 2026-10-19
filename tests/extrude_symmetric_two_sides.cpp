// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include "SketchProfiles.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <iostream>
#include <cmath>
#include <stdexcept>
namespace {
void check(bool v,const char* m){if(!v)throw std::runtime_error(m);}
void near(double a,double b){check(std::abs(a-b)<1e-5,"exact extrusion geometry");}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
CadDocument d;QTemporaryDir temp;
auto sketch=SketchGeometry::newFeature(d.activeOwnerId(),"XY");sketch=SketchGeometry::addRectangle(sketch,{{"u1Mm",-20},{"v1Mm",-15},{"u2Mm",20},{"v2Mm",15}});
check(d.commit(SketchGeometry::replaceFeature(d.authored(),sketch),"Source").isEmpty(),"source");
const auto sid=sketch.value("id"),pid=QJsonValue(d.sketches().front().profiles.front().profileId);QString id;
auto apply=[&](QJsonObject args){args["sketchId"]=sid;args["profileId"]=pid;if(!id.isEmpty())args["featureId"]=id;
check(d.commit(CadDocument::extrusionCandidate(d.authored(),args),"Extrude").isEmpty(),"commit extent");
if(id.isEmpty())id=d.authored()["features"].toArray().last().toObject()["id"].toString();};
auto geometry=[&](double low,double high){const auto summary=d.bodies().front().summary;const auto b=summary["boundsMm"].toObject();near(b["min"].toArray()[2].toDouble(),low);near(b["max"].toArray()[2].toDouble(),high);near(summary["volumeMm3"].toDouble(),1200*(high-low));};
apply({{"direction","symmetric"},{"measurement","halfLength"},{"distanceMm",10}});geometry(-10,10);
const auto initial=d.authored();d.history().undo();check(d.bodies().empty(),"undo removes body");d.history().redo();check(d.authored()==initial,"redo restores inputs");
apply({{"direction","symmetric"},{"measurement","halfLength"},{"distanceMm",12}});geometry(-12,12);
apply({{"direction","symmetric"},{"measurement","wholeLength"},{"distanceMm",20}});geometry(-10,10);
apply({{"direction","symmetric"},{"measurement","wholeLength"},{"distanceMm",24}});geometry(-12,12);
apply({{"direction","twoSides"},{"negativeDistanceMm",8},{"distanceMm",14}});geometry(-8,14);
EvaluatedSupportFace cap;for(const auto& face:d.bodies().front().supportFaces)if(face.role=="extrude.endCap")cap=face;check(!cap.faceId.isEmpty(),"positive cap identity");
auto attached=SketchGeometry::newSupportedFeature(d.activeOwnerId(),{{"producerFeatureId",cap.producerFeatureId},{"bodyId",cap.bodyId},{"faceId",cap.faceId}},"Positive cap",GeometryKernel::supportEdgeDescriptors(cap));
check(d.commit(SketchGeometry::replaceFeature(d.authored(),attached),"Attached sketch").isEmpty(),"cap sketch");
apply({{"direction","twoSides"},{"negativeDistanceMm",9},{"distanceMm",14}});geometry(-9,14);near(d.sketches().back().resolvedFrame["origin"].toArray()[2].toDouble(),14);
apply({{"direction","twoSides"},{"negativeDistanceMm",9},{"distanceMm",16}});geometry(-9,16);near(d.sketches().back().resolvedFrame["origin"].toArray()[2].toDouble(),16);
const auto stable=d.authored();const auto revision=d.revision();
for(const auto args:std::vector<QJsonObject>{{{"direction","twoSides"},{"negativeDistanceMm",0},{"distanceMm",14}},{{"direction","symmetric"},{"measurement","halfLength"},{"distanceMm",0}},{{"direction","symmetric"},{"measurement","halfLength"},{"distanceMm",-10}},{{"direction","twoSides"},{"negativeDistanceMm",8},{"distanceMm",-14}}}){
 auto a=args;a["sketchId"]=sid;a["profileId"]=pid;a["featureId"]=id;bool refused=false;try{const auto c=CadDocument::extrusionCandidate(d.authored(),a);refused=!d.commit(c,"Invalid").isEmpty();}catch(const std::exception&){refused=true;}check(refused && d.authored()==stable && d.revision()==revision,"invalid atomicity");}
auto crossed=SketchGeometry::newFeature(d.activeOwnerId(),"XY");
for(const auto& line:std::vector<QJsonObject>{{{"u1Mm",-20},{"v1Mm",-15},{"u2Mm",20},{"v2Mm",15}},{{"u1Mm",20},{"v1Mm",15},{"u2Mm",-20},{"v2Mm",15}},{{"u1Mm",-20},{"v1Mm",15},{"u2Mm",20},{"v2Mm",-15}},{{"u1Mm",20},{"v1Mm",-15},{"u2Mm",-20},{"v2Mm",-15}}})crossed=SketchGeometry::addLine(crossed,line);
const auto crossing=SketchProfiles::derive(crossed);check(!crossing.ok && crossing.diagnostics["code"]=="self_intersecting_profile","inherited self-intersection guard");
crossed["id"]=sid;const auto invalidSource=SketchGeometry::replaceFeature(stable,crossed);
check(!d.commit(invalidSource,"Crossed source").isEmpty() && d.authored()==stable && d.revision()==revision,"self-intersecting source replacement refuses atomically");geometry(-9,16);
check(d.save(temp.path()+"/extent.alloy").isEmpty(),"save");CadDocument reopened;check(reopened.open(temp.path()+"/extent.alloy").isEmpty() && reopened.authored()==stable,"reopen retained inputs");near(reopened.sketches().back().resolvedFrame["origin"].toArray()[2].toDouble(),16);
QJsonObject retained;for(const auto& value:stable["features"].toArray())if(value.toObject()["id"]==id)retained=value.toObject();check(retained["parameters"].toObject()["profileId"]==pid,"profile identity");
std::cout<<"PASS: symmetric Half/Whole and independent sides, cap support, edit identity, atomic refusal, history/reopen\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
