// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QJsonArray>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
#include <stdexcept>
namespace {
void check(bool good,const char* message){if(!good)throw std::runtime_error(message);}
void near(double a,double b,const char* message){check(std::abs(a-b)<1e-6,message);}
QJsonObject require(const SketchGeometry::SolveResult& r){if(!r.ok)throw std::runtime_error(r.error.toStdString());return r.feature;}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
 CadDocument document;const auto empty=SketchGeometry::newFeature(document.activeOwnerId(),"XY");
 const QJsonObject args{{"u1Mm",0},{"v1Mm",0},{"u2Mm",24},{"v2Mm",18},{"u3Mm",15},{"v3Mm",30},{"firstDistanceMm",30},{"secondDistanceMm",15},{"startSnapTarget",QJsonObject{{"kind","origin"}}}};
 auto sketch=require(SketchGeometry::addThreePointRectangle(empty,args));const auto parameters=sketch["parameters"].toObject();const auto entities=parameters["entities"].toArray(),dimensions=parameters["dimensions"].toArray();
 check(entities.size()==4 && dimensions.size()==2,"four retained edges and two typed dimensions");
 int parallel=0,perpendicular=0;for(const auto& row:parameters["constraints"].toArray()){const auto c=row.toObject();parallel+=c["type"]=="parallel";perpendicular+=c["type"]=="perpendicular";check(c["type"]!="horizontal" && c["type"]!="vertical","rotated rectangle has no axis relations");}
 check(parallel==2 && perpendicular==1,"automatic opposite parallel and adjacent perpendicular relations");
 auto evaluated=SketchGeometry::evaluateFeature(sketch);check(evaluated.profiles.size()==1,"one closed profile");near(evaluated.profiles[0].summary["areaMm2"].toDouble(),450,"independent rotated area");
 for(const auto& invalid:{QJsonObject{{"u3Mm",48},{"v3Mm",36}},QJsonObject{{"u2Mm",0},{"v2Mm",0}},QJsonObject{{"firstDistanceMm",12}}}){auto bad=args;for(auto it=invalid.begin();it!=invalid.end();++it)bad[it.key()]=it.value();const auto refusal=SketchGeometry::addThreePointRectangle(empty,bad);check(!refusal.ok && refusal.feature==empty,"degenerate or conflicting placement refuses atomically");}
 check(document.commit(SketchGeometry::replaceFeature(document.authored(),sketch),"Rectangle").isEmpty(),"commit rectangle");
 const auto profile=document.sketches()[0].profiles[0].profileId;
 check(document.commit(CadDocument::extrusionCandidate(document.authored(),{{"sketchId",sketch["id"]},{"profileId",profile},{"distanceMm",5}}),"Extrude").isEmpty(),"extrude profile");near(document.bodies()[0].summary["volumeMm3"].toDouble(),2250,"prism volume");
 sketch=require(SketchGeometry::updateDimension(sketch,{{"dimensionId",dimensions[0].toObject()["id"]},{"valueMm",36}}));check(sketch["parameters"].toObject()["entities"].toArray()==entities,"dimension edit retains four edge IDs");
 check(document.commit(SketchGeometry::replaceFeature(document.authored(),sketch),"Resize rectangle").isEmpty(),"upstream dimension reevaluates solid");near(document.bodies()[0].summary["volumeMm3"].toDouble(),2700,"edited prism volume");
 const auto final=document.authored();document.history().undo();near(document.bodies()[0].summary["volumeMm3"].toDouble(),2250,"undo upstream edit");document.history().redo();check(document.authored()==final,"exact redo");
 QTemporaryDir folder;const auto path=folder.path()+"/three-point.alloy";check(document.save(path).isEmpty(),"save");CadDocument reopened;check(reopened.open(path).isEmpty() && reopened.authored()==final,"exact retained reopen");
 auto opposite=args;opposite["u3Mm"]=33;opposite["v3Mm"]=6;auto mirrored=require(SketchGeometry::addThreePointRectangle(empty,opposite));near(SketchGeometry::evaluateFeature(mirrored).profiles[0].summary["areaMm2"].toDouble(),450,"opposite side closed profile");
 auto rough=args;rough.remove("firstDistanceMm");rough.remove("secondDistanceMm");auto free=SketchGeometry::addThreePointRectangle(empty,rough);check(free.ok && free.feature["parameters"].toObject()["dimensions"].toArray().isEmpty() && free.diagnostics["dof"].toInt()==3,"rough origin rectangle preserves size and rotational freedom");
 std::cout<<"Three-point Rectangle retained geometry passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
