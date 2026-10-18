#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
#include <stdexcept>
namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void near(double a,double b,const char* message){check(std::abs(a-b)<1e-5,message);}
QJsonObject good(SketchGeometry::SolveResult r){if(!r.ok)throw std::runtime_error(r.error.toStdString());return r.feature;}
QJsonObject group(const QJsonObject& f){return f["parameters"].toObject()["polygonGroups"].toArray().last().toObject();}
QJsonObject point(const QJsonObject& f,const QJsonValue& id){for(const auto& row:f["parameters"].toObject()["points"].toArray())if(row.toObject()["id"]==id)return row.toObject();throw std::runtime_error("missing point");}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
 CadDocument doc;const auto empty=SketchGeometry::newFeature(doc.activeOwnerId(),"XY");
 for(const auto* mode:{"inscribed","circumscribed","edge"}){
  auto f=good(SketchGeometry::addStandalonePoint(empty,{{"uMm",10},{"vMm",0}}));const auto source=f["parameters"].toObject()["entities"].toArray().last().toObject().value("pointId");
  const QJsonObject target{{"kind","point"},{"pointId",source}};
  f=good(SketchGeometry::polygon(f,{{"mode",mode},{"uMm",0},{"vMm",0},{"sizeMm",10},{"angleDeg",0},{"count",6},{"anchorSnapTarget",QJsonObject{{"kind","origin"}}},{"driverSnapTarget",target}}));const auto before=group(f);
  check(before["driverSource"]==target,"second source retained");
  f=good(SketchGeometry::movePoint(f,{{"pointId",source},{"uMm",0},{"vMm",20}}));near(group(f)["sizeMm"].toDouble(),20,"second point owns scale");near(group(f)["angleDeg"].toDouble(),90,"second point owns angle");
  // A fully constrained source remains authoritative when an external follower is attached.
  f=good(SketchGeometry::addConstraint(f,{{"type","fixed"},{"pointIds",QJsonArray{source}}}));
  const auto witness=before["pointIds"].toArray().at(2);const auto pos=point(f,witness);
  f=good(SketchGeometry::addStandalonePoint(f,{{"uMm",pos["uMm"]},{"vMm",pos["vMm"]}}));const auto follower=f["parameters"].toObject()["entities"].toArray().last().toObject().value("pointId");
  f=good(SketchGeometry::addConstraint(f,{{"type","coincident"},{"pointIds",QJsonArray{follower,witness}}}));
  f=good(SketchGeometry::polygon(f,{{"polygonId",before["id"]},{"count",8}},true));const auto after=group(f);
  check(after["driverPointId"]==before["driverPointId"] && after["driverConstraintId"]==before["driverConstraintId"] && after["anchorConstraintId"]==before["anchorConstraintId"],"count preserves both drivers");
  check(after["pointIds"].toArray().at(2)==witness && after["entityIds"].toArray().at(0)==before["entityIds"].toArray().at(0),"count preserves surviving boundary IDs");
  near(point(f,follower)["uMm"].toDouble(),point(f,witness)["uMm"].toDouble(),"external follower participates in count edit");
  check(SketchGeometry::evaluateFeature(f).profiles.size()==1,"two-driver profile closes");
  for(const auto& edit:{QJsonObject{{"sizeMm",21}},QJsonObject{{"angleDeg",91}}}){auto a=edit;a["polygonId"]=before["id"];const auto refused=SketchGeometry::polygon(f,a,true);check(!refused.ok && refused.feature==f,"conflicting numeric edit is atomic");}
  check(good(SketchGeometry::polygon(f,{{"polygonId",before["id"]},{"sizeMm",20},{"angleDeg",450}},true))["parameters"].toObject()["polygonGroups"].toArray().size()==1,"equivalent numeric input allowed");
  const auto degenerate=SketchGeometry::polygon(empty,{{"mode",mode},{"uMm",0},{"vMm",0},{"sizeMm",1},{"driverSnapTarget",QJsonObject{{"kind","origin"}}}});check(!degenerate.ok && degenerate.feature==empty,"coincident drivers refuse atomically");
  check(doc.commit(SketchGeometry::replaceFeature(doc.authored(),f),"Two drivers").isEmpty(),"document commit");const auto saved=doc.authored();doc.history().undo();doc.history().redo();check(doc.authored()==saved,"two-driver undo redo");QTemporaryDir dir;check(doc.save(dir.path()+"/drivers.alloy").isEmpty(),"save drivers");CadDocument reopened;check(reopened.open(dir.path()+"/drivers.alloy").isEmpty() && reopened.authored()==saved,"two-driver reopen");
 }
 auto f=SketchGeometry::addLine(empty,{{"u1Mm",5},{"v1Mm",10},{"u2Mm",15},{"v2Mm",10}});const auto line=f["parameters"].toObject()["entities"].toArray().last().toObject();f=good(SketchGeometry::setConstruction(f,{{"entityIds",QJsonArray{line["id"]}},{"construction",true}}));
 const QJsonObject midpoint{{"kind","midpoint"},{"entityId",line["id"]}};
 const QJsonObject args{{"mode","circumscribed"},{"uMm",0},{"vMm",0},{"sizeMm",std::sqrt(200.0)},{"angleDeg",45},{"driverSnapTarget",midpoint}};
 const auto base=f;f=good(SketchGeometry::polygon(f,args));f=good(SketchGeometry::movePoint(f,{{"pointId",line["endPointId"]},{"uMm",35},{"vMm",10}}));near(group(f)["sizeMm"].toDouble(),std::sqrt(500.0),"midpoint owns radius");near(group(f)["angleDeg"].toDouble(),std::atan2(10,20)*180/std::acos(-1),"midpoint owns rotation");
 for(const auto* mode:{"inscribed","edge"}){auto a=args;a["mode"]=mode;const auto refused=SketchGeometry::polygon(base,a);check(!refused.ok && refused.feature==base,"unsupported midpoint refuses");}
 auto conflict=args;conflict["sizeMm"]=11;check(!SketchGeometry::polygon(base,conflict).ok,"creation numeric conflict refuses");
 std::cout<<"Polygon two-driver core contract passed\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
