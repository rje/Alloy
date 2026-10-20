// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QJsonArray>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
#include <stdexcept>
void check(bool ok,const QString& why){if(!ok)throw std::runtime_error(why.toStdString());}
QJsonObject lookup(const QJsonObject& s,const char* field,const QString& id){for(const auto& v:s["parameters"].toObject()[field].toArray())if(v.toObject()["id"]==id)return v.toObject();return {};}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
 CadDocument doc;auto s=SketchGeometry::newFeature(doc.authored()["documentId"].toString(),"XY");QStringList axes;
 for(const auto& end:QJsonArray{QJsonArray{10,0},QJsonArray{0,10}}){const auto xy=end.toArray();s=SketchGeometry::addLine(s,{{"u1Mm",0},{"v1Mm",0},{"u2Mm",xy[0]},{"v2Mm",xy[1]}});const auto id=s["parameters"].toObject()["entities"].toArray().last().toObject()["id"].toString();axes.append(id);const auto construction=SketchGeometry::setConstruction(s,{{"entityIds",QJsonArray{id}},{"construction",true}});check(construction.ok,construction.error);s=construction.feature;}
 s=SketchGeometry::addCircle(s,{{"centerUMm",8},{"centerVMm",7},{"diameterMm",3}});const auto seed=s["parameters"].toObject()["entities"].toArray().last().toObject();
 auto radius=SketchGeometry::addDimension(s,{{"kind","radius"},{"entityId",seed["id"]},{"valueMm",1.5}});check(radius.ok,radius.error);s=radius.feature;const auto radiusId=s["parameters"].toObject()["dimensions"].toArray().last().toObject().value("id");
 const auto before=SketchGeometry::solveDraft(s);check(before.ok,before.error);
 QJsonObject args{{"sourceEntityId",seed["id"]},{"direction1EntityId",axes[0]},{"direction2EntityId",axes[1]},{"quantity1",2},{"quantity2",3},{"spacing1Mm",14},{"spacing2Mm",11}};
 auto pattern=SketchGeometry::rectangularPattern(s,args);check(pattern.ok,pattern.error);check(pattern.diagnostics["dof"]==before.diagnostics["dof"],"Derived circles add no solver freedom");s=pattern.feature;const auto group=s["parameters"].toObject()["patternGroups"].toArray().first().toObject();
 check(group["instances"].toArray().size()==5,"2 by 3 includes unchanged source");
 for(const auto& v:group["instances"].toArray()){const auto instance=v.toObject(),circle=lookup(s,"entities",instance["entityId"].toString()),point=lookup(s,"points",instance["pointId"].toString());check(std::abs(point["uMm"].toDouble()-8-instance["i"].toInt()*14)<1e-6 && std::abs(point["vMm"].toDouble()-7-instance["j"].toInt()*11)<1e-6,"Exact indexed center");check(circle["radiusMm"].toDouble()==1.5,"Source radius");check(pattern.diagnostics["entityFreedom"].toObject()[circle["id"].toString()].toObject()["derived"].toBool(),"Derived diagnostic");}
 check(SketchGeometry::evaluateFeature(s).profiles.size()==6,"Six independent profiles");
 const auto result=group["instances"].toArray().first().toObject();
 for(const auto& bad:QJsonArray{QJsonObject{{"quantity1",2.5}},QJsonObject{{"spacing1Mm",0}},QJsonObject{{"spacing1Mm",2}},QJsonObject{{"direction2EntityId",axes[0]}}}){auto a=args;const auto b=bad.toObject();for(auto it=b.begin();it!=b.end();++it)a[it.key()]=it.value();a["groupId"]=group["id"];const auto refusal=SketchGeometry::rectangularPattern(s,a);check(!refusal.ok && refusal.feature==s,"Invalid pattern is atomic");}
 const auto drag=SketchGeometry::movePoint(s,{{"pointId",result["pointId"]},{"uMm",33},{"vMm",22}});check(!drag.ok && drag.feature==s,"Derived drag refused");
 const auto constraint=SketchGeometry::toggleFixed(s,{{"entityIds",QJsonArray{result["entityId"]}}});check(!constraint.ok && constraint.feature==s,"Derived Fix refused");
 const auto dim=SketchGeometry::addDimension(s,{{"kind","radius"},{"entityId",result["entityId"]},{"valueMm",2}});check(!dim.ok && dim.feature==s,"Derived dimension refused");
 for(const auto& id:QJsonArray{seed["id"],axes[0],result["entityId"]}){const auto d=SketchGeometry::deleteEntities(s,QJsonArray{id},{});check(!d.ok && d.feature==s,"Owner Delete refused atomically");}
 auto edited=SketchGeometry::updateDimension(s,{{"dimensionId",radiusId},{"valueMm",2}});check(edited.ok,edited.error);s=edited.feature;
 edited=SketchGeometry::movePoint(s,{{"pointId",seed["centerPointId"]},{"uMm",9},{"vMm",8}});check(edited.ok,edited.error);s=edited.feature;
 check(lookup(s,"entities",result["entityId"].toString())["radiusMm"].toDouble()==2,"Derived radius follows solved source");
 args["groupId"]=group["id"];args["quantity1"]=3;args["quantity2"]=4;args["spacing1Mm"]=16;args["spacing2Mm"]=12;edited=SketchGeometry::rectangularPattern(s,args);check(edited.ok,edited.error);s=edited.feature;
 const auto larger=s["parameters"].toObject()["patternGroups"].toArray().first().toObject();check(larger["instances"].toArray()[0]==group["instances"].toArray()[0],"Surviving indexed identity stable");
 args["quantity2"]=3;edited=SketchGeometry::rectangularPattern(s,args);check(edited.ok,edited.error);args["quantity2"]=4;edited=SketchGeometry::rectangularPattern(edited.feature,args);check(edited.ok && edited.feature==s,"Regrowing index restores deterministic cache");
 const auto error=doc.commit(SketchGeometry::replaceFeature(doc.authored(),s),"Rectangular Pattern");check(error.isEmpty(),error);const auto authored=doc.authored();doc.history().undo();doc.history().redo();check(doc.authored()==authored,"Exact retained history");QTemporaryDir dir;const auto path=dir.filePath("pattern.alloy");check(doc.save(path).isEmpty(),"Save");CadDocument opened;check(opened.open(path).isEmpty() && opened.authored()==authored,"Exact authored reopen");std::cout<<"RECTANGULAR_PATTERN_CORE_OK\n";return 0;
 }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
