// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
#include <numbers>
#include <stdexcept>
namespace {void check(bool v,const char* m){if(!v)throw std::runtime_error(m);}void near(double a,double b){check(std::abs(a-b)<1e-5,"thin exact geometry");}}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
 CadDocument d;QTemporaryDir temp;auto sketch=SketchGeometry::newFeature(d.activeOwnerId(),"XY");
 sketch=SketchGeometry::addLine(sketch,{{"u1Mm",0},{"v1Mm",0},{"u2Mm",40},{"v2Mm",0}});
 check(d.commit(SketchGeometry::replaceFeature(d.authored(),sketch),"Source").isEmpty(),"source");
 const auto sid=sketch.value("id"),eid=sketch.value("parameters").toObject().value("entities").toArray().first().toObject().value("id");QString fid;
 auto args=[&](double thickness,const QString& side){QJsonObject a{{"sketchId",sid},{"sourceEntityId",eid},{"distanceMm",20},{"wallThicknessMm",thickness},{"wallLocation",side}};if(!fid.isEmpty())a["featureId"]=fid;return a;};
 auto apply=[&](double thickness,const QString& side){check(d.commit(CadDocument::extrusionCandidate(d.authored(),args(thickness,side)),"Thin").isEmpty(),"commit thin");if(fid.isEmpty())fid=d.authored()["features"].toArray().last().toObject()["id"].toString();};
 for(const auto& side:QStringList{"center","side1","side2"}){apply(4,side);const auto b=d.bodies().front().summary;near(b["volumeMm3"].toDouble(),3200);const auto bounds=b["boundsMm"].toObject();near(bounds["min"].toArray()[0].toDouble(),0);near(bounds["max"].toArray()[0].toDouble(),40);near(bounds["min"].toArray()[1].toDouble(),side=="center" ? -2 : side=="side1" ? 0 : -4);near(bounds["max"].toArray()[1].toDouble(),side=="center" ? 2 : side=="side1" ? 4 : 0);near(bounds["max"].toArray()[2].toDouble(),20);}
 apply(4,"center");const auto cap=d.bodies().front().supportFaces.front();check(d.bodies().front().supportFaces.size()==1 && d.bodies().front().solidEdges.empty(),"far cap only");
 auto attached=SketchGeometry::newSupportedFeature(d.activeOwnerId(),{{"producerFeatureId",cap.producerFeatureId},{"bodyId",cap.bodyId},{"faceId",cap.faceId}},"Far cap",{});
 attached=SketchGeometry::addCircle(attached,{{"centerUMm",20},{"centerVMm",0},{"diameterMm",2}});check(d.commit(SketchGeometry::replaceFeature(d.authored(),attached),"Cap circle").isEmpty(),"cap circle");
 check(d.commit(CadDocument::extrusionCandidate(d.authored(),{{"sketchId",attached.value("id")},{"profileId",d.sketches().back().profiles.front().profileId},{"distanceMm",-0.5},{"operation","cut"},{"targetBodyIds",QJsonArray{cap.bodyId}}}),"Cut").isEmpty(),"downstream cut");
 near(d.bodies().front().summary["volumeMm3"].toDouble(),3200-std::numbers::pi/2);apply(6,"center");near(d.bodies().front().summary["volumeMm3"].toDouble(),4800-std::numbers::pi/2);check(d.sketches().back().resolvedFrame["origin"].toArray()[2].toDouble()==20,"stable cap plane");check(d.authored()["features"].toArray()[1].toObject()["parameters"].toObject()["sourceEntityId"]==eid,"retained source");
 const auto stable=d.authored();const auto rev=d.revision();for(const auto& extra:std::vector<QJsonObject>{{{"wallThicknessMm",0}},{{"wallThicknessMm",-1}},{{"wallThicknessMm",QJsonValue()}},{{"wallLocation","bad"}},{{"direction","symmetric"},{"measurement","halfLength"}},{{"taperAngleDeg",1}},{{"distanceMm",-20}}}){auto a=args(4,"center");for(auto i=extra.begin();i!=extra.end();++i)a[i.key()]=i.value();bool refused=false;try{refused=!d.commit(CadDocument::extrusionCandidate(stable,a),"Bad").isEmpty();}catch(const std::exception&){refused=true;}check(refused && d.authored()==stable && d.revision()==rev,"atomic thin refusal");}
 d.history().undo();d.history().redo();check(d.authored()==stable,"undo redo");check(d.save(temp.path()+"/thin.alloy").isEmpty(),"save");CadDocument reopened;check(reopened.open(temp.path()+"/thin.alloy").isEmpty() && reopened.authored()==stable,"reopen");near(reopened.bodies().front().summary["volumeMm3"].toDouble(),4800-std::numbers::pi/2);
 std::cout<<"PASS retained open Line, 3 sides, exact geometry, downstream cut, atomic refusal, undo/reopen\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
