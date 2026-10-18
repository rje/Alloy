#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
#include <numbers>
#include <stdexcept>
namespace {
void check(bool good,const char* message){if(!good)throw std::runtime_error(message);}
void near(double a,double b,const char* message){check(std::abs(a-b)<1e-6,message);}
QJsonObject group(const QJsonObject& sketch){return sketch["parameters"].toObject()["polygonGroups"].toArray().last().toObject();}
QJsonObject point(const QJsonObject& sketch,const QJsonValue& id){for(const auto& p:sketch["parameters"].toObject()["points"].toArray())if(p.toObject()["id"]==id)return p.toObject();throw std::runtime_error("missing point");}
QJsonObject require(SketchGeometry::SolveResult result){if(!result.ok)throw std::runtime_error(result.error.toStdString());return result.feature;}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
 CadDocument document;const auto empty=SketchGeometry::newFeature(document.activeOwnerId(),"XY");
 for(const auto& mode:{"inscribed","circumscribed","edge"}){
  auto sketch=require(SketchGeometry::polygon(empty,{{"mode",mode},{"uMm",10},{"vMm",20},{"sizeMm",12},{"angleDeg",0},{"count",6}}));
  auto g=group(sketch);check(g["count"]==6,"retained count");const auto ids=g["pointIds"].toArray();
  const auto a=point(sketch,ids[0]),b=point(sketch,ids[1]);
  if(QString(mode)=="edge"){near(a["uMm"].toDouble(),10,"edge first vertex x");near(a["vMm"].toDouble(),20,"edge first vertex y");near(b["uMm"].toDouble(),22,"edge second vertex x");near(b["vMm"].toDouble(),20,"edge second vertex y");}
  if(QString(mode)=="inscribed"){near(a["uMm"].toDouble(),22,"inscribed vertex radius");near(a["vMm"].toDouble(),20,"inscribed radial orientation");}
  if(QString(mode)=="circumscribed"){near((a["uMm"].toDouble()+b["uMm"].toDouble())/2,22,"circumscribed midpoint radius");near((a["vMm"].toDouble()+b["vMm"].toDouble())/2,20,"circumscribed midpoint orientation");}
  auto evaluated=SketchGeometry::evaluateFeature(sketch);check(evaluated.profiles.size()==1,"one closed real profile");
  const double area=QString(mode)=="inscribed" ? 3*144*std::sin(std::numbers::pi/3) : QString(mode)=="circumscribed" ? 6*144*std::tan(std::numbers::pi/6) : 6*144/(4*std::tan(std::numbers::pi/6));
  near(evaluated.profiles[0].summary["areaMm2"].toDouble(),area,"independent regular polygon area");
  auto updated=require(SketchGeometry::polygon(sketch,{{"polygonId",g["id"]},{"sizeMm",18},{"count",8}},true));
  const auto edited=group(updated);check(edited["id"]==g["id"],"retained object identity");
  for(int i=0;i<6;++i)check(edited["pointIds"].toArray()[i]==ids[i] && edited["entityIds"].toArray()[i]==g["entityIds"].toArray()[i],"surviving boundary identities");
  check(SketchGeometry::evaluateFeature(updated).profiles.size()==1,"count edit remains closed");
  auto refused=SketchGeometry::polygon(updated,{{"polygonId",g["id"]},{"count",2}},true);check(!refused.ok && refused.feature==updated,"invalid count is transactional");
  refused=SketchGeometry::polygon(updated,{{"polygonId",g["id"]},{"sizeMm",0}},true);check(!refused.ok && refused.feature==updated,"zero size is transactional");
  const auto withMeasurement=require(SketchGeometry::addDimension(updated,{{"kind","linear"},{"entityId",edited["entityIds"].toArray()[7]},{"driving",false}}));
  refused=SketchGeometry::polygon(withMeasurement,{{"polygonId",g["id"]},{"count",4}},true);check(!refused.ok && refused.feature==withMeasurement,"count edit cannot strand external dimension");
  check(document.commit(SketchGeometry::replaceFeature(document.authored(),updated),"Polygon").isEmpty(),"commit polygon");
 }
 const auto authored=document.authored();document.history().undo();document.history().redo();check(document.authored()==authored,"exact history restoration");
 QTemporaryDir dir;const auto path=dir.path()+"/polygon.alloy";check(document.save(path).isEmpty(),"save retained polygon");CadDocument reopened;check(reopened.open(path).isEmpty() && reopened.authored()==authored,"exact save and reopen");
 auto right=require(SketchGeometry::polygon(empty,{{"mode","edge"},{"uMm",0},{"vMm",0},{"sizeMm",10},{"count",4},{"side",-1}}));
 auto ps=group(right)["pointIds"].toArray();near(point(right,ps[1])["uMm"].toDouble(),10,"right polygon retains base edge");near(point(right,ps[2])["vMm"].toDouble(),-10,"right side polygon orientation");
 std::cout<<"Retained Polygon core contract passed\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
