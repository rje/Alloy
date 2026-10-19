// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
#include <numbers>
#include <stdexcept>
namespace {
void check(bool v,const char* m){if(!v)throw std::runtime_error(m);}
void near(double a,double b){check(std::abs(a-b)<1e-5,"exact taper geometry");}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
 CadDocument d;QTemporaryDir temp;auto sketch=SketchGeometry::newFeature(d.activeOwnerId(),"XY");
 sketch=SketchGeometry::addRectangle(sketch,{{"u1Mm",-20},{"v1Mm",-10},{"u2Mm",20},{"v2Mm",10}});
 check(d.commit(SketchGeometry::replaceFeature(d.authored(),sketch),"Source").isEmpty(),"source");
 const auto sid=sketch.value("id"),pid=QJsonValue(d.sketches().front().profiles.front().profileId);QString fid;
 auto apply=[&](double angle){QJsonObject args{{"sketchId",sid},{"profileId",pid},{"distanceMm",10},{"taperAngleDeg",angle}};if(!fid.isEmpty())args["featureId"]=fid;
 check(d.commit(CadDocument::extrusionCandidate(d.authored(),args),"Taper").isEmpty(),"commit taper");if(fid.isEmpty())fid=d.authored()["features"].toArray().last().toObject()["id"].toString();};
 for(double angle:{-5.,0.,5.}){apply(angle);double t=std::tan(angle*std::numbers::pi/180);near(d.bodies().front().summary["volumeMm3"].toDouble(),10*(800+600*t+400./3*t*t));}
 apply(-5);const auto cap=d.bodies().front().supportFaces.front();check(cap.role=="extrude.endCap","cap identity");check(d.bodies().front().supportFaces.size()==1 && d.bodies().front().solidEdges.empty(),"no unqualified side/edge claims");
 auto attached=SketchGeometry::newSupportedFeature(d.activeOwnerId(),{{"producerFeatureId",cap.producerFeatureId},{"bodyId",cap.bodyId},{"faceId",cap.faceId}},"Far cap",GeometryKernel::supportEdgeDescriptors(cap));
 attached=SketchGeometry::addCircle(attached,{{"centerUMm",0},{"centerVMm",0},{"diameterMm",4}});
 check(d.commit(SketchGeometry::replaceFeature(d.authored(),attached),"Cap circle").isEmpty(),"attached circle");
 apply(-3);near(d.sketches().back().resolvedFrame["origin"].toArray()[2].toDouble(),10);const auto stable=d.authored();const auto rev=d.revision();
 for(auto extra:std::vector<QJsonObject>{{{"taperAngleDeg",-80}},{{"taperAngleDeg",-90}},{{"taperAngleDeg",90}},{{"taperAngleDeg","bad"}},{{"direction","symmetric"},{"measurement","halfLength"}},{{"start","offset"},{"startOffsetMm",2}},{{"distanceMm",-10}}}){
 QJsonObject args{{"sketchId",sid},{"profileId",pid},{"featureId",fid},{"distanceMm",10}};for(auto i=extra.begin();i!=extra.end();++i)args[i.key()]=i.value();bool refused=false;try{refused=!d.commit(CadDocument::extrusionCandidate(stable,args),"Invalid").isEmpty();}catch(const std::exception&){refused=true;}
 check(refused && d.authored()==stable && d.revision()==rev,"atomic invalid taper");}
 QJsonObject curved{{"sketchId",attached.value("id")},{"profileId",d.sketches().back().profiles.front().profileId},{"distanceMm",2},{"taperAngleDeg",-3}};
 check(!d.commit(CadDocument::extrusionCandidate(stable,curved),"Unsupported curved taper").isEmpty() && d.authored()==stable && d.revision()==rev,"curved taper refuses atomically");
 d.history().undo();d.history().redo();check(d.authored()==stable,"undo redo");check(d.save(temp.path()+"/taper.alloy").isEmpty(),"save");CadDocument reopened;check(reopened.open(temp.path()+"/taper.alloy").isEmpty() && reopened.authored()==stable,"reopen");
 std::cout<<"PASS retained taper, signed geometry, positive-cap circle recompute, atomic refusals, undo/reopen\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
