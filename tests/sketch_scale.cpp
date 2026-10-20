// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include "SketchScale.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <iostream>
#include <cmath>
#include <stdexcept>
void check(bool ok,const QString& message){if(!ok)throw std::runtime_error(message.toStdString());}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    CadDocument doc;auto source=SketchGeometry::newFeature(doc.authored()["documentId"].toString(),"XY");
    const double xy[4][2]={{10,10},{30,10},{27,20},{12,24}};
    for(int n=0;n<4;++n)source=SketchGeometry::addLine(source,{{"u1Mm",xy[n][0]},{"v1Mm",xy[n][1]},{"u2Mm",xy[(n+1)%4][0]},{"v2Mm",xy[(n+1)%4][1]}});
    QJsonArray ids;for(const auto& e:source["parameters"].toObject()["entities"].toArray())ids.append(e.toObject()["id"]);
    auto horizontal=SketchGeometry::addConstraint(source,{{"type","horizontal"},{"entityIds",QJsonArray{ids[0]}}});check(horizontal.ok,horizontal.error);source=horizontal.feature;
    const auto added=SketchGeometry::addStandalonePoint(source,{{"uMm",4},{"vMm",6}});check(added.ok,added.error);source=added.feature;
    const auto p=source["parameters"].toObject();const auto pivot=p["entities"].toArray().last().toObject()["pointId"].toString();const auto points=p["points"].toArray();
    const auto profile=SketchGeometry::evaluateFeature(source).profiles[0].profileId;
    auto scale=[&](const QJsonObject& f,double value,const QString& group=QString()){return SketchGeometry::retainedScale(f,{{"entityIds",ids},{"pivotPointId",pivot},{"factor",value},{"groupId",group}});};
    const auto scaled=scale(source,1.5);check(scaled.ok,scaled.error);const auto group=scaled.feature["parameters"].toObject()["scaleGroups"].toArray().first().toObject();
    auto verify=[&](const QJsonObject& f,double value,double c=27){const auto pp=f["parameters"].toObject();const auto out=pp["points"].toArray();for(int n=0;n<4;++n){const double u=n==2?c:xy[n][0];check(std::abs(out[n].toObject()["uMm"].toDouble()-(4+value*(u-4)))<1e-6 && std::abs(out[n].toObject()["vMm"].toDouble()-(6+value*(xy[n][1]-6)))<1e-6,"Exact uniform output");check(out[n].toObject()["id"]==points[n].toObject()["id"],"Point identity");}check(pp["entities"]==p["entities"] && pp["constraints"]==p["constraints"],"Entities/relations unchanged");const auto e=SketchGeometry::evaluateFeature(f);check(e.profiles.size()==1 && e.profiles[0].profileId==profile,"Profile identity");};verify(scaled.feature,1.5);
    for(int n=0;n<3;++n){const auto r=SketchGeometry::solveDraft(scaled.feature);check(r.ok && r.feature==scaled.feature,"Idempotent source solve: "+r.error);}
    const auto edited=scale(scaled.feature,2,group["id"].toString());check(edited.ok,edited.error);verify(edited.feature,2);check(edited.feature["parameters"].toObject()["scaleGroups"].toArray().first().toObject()["sourcePoints"]==group["sourcePoints"],"Factor replaces same source");
    const auto restored=scale(edited.feature,1.5,group["id"].toString());check(restored.ok && restored.feature==scaled.feature,"Factor edit replacement exact");
    const auto moved=SketchGeometry::movePoint(restored.feature,{{"pointId",points[2].toObject()["id"]},{"uMm",35.5},{"vMm",27}});check(moved.ok,moved.error);verify(moved.feature,1.5,25);const auto movedGroup=moved.feature["parameters"].toObject()["scaleGroups"].toArray().first().toObject();check(movedGroup["id"]==group["id"],"Source edit owner identity");
    const auto badSource=SketchGeometry::movePoint(moved.feature,{{"pointId",points[0].toObject()["id"]},{"uMm",13},{"vMm",14}});check(!badSource.ok && badSource.feature==moved.feature,"Horizontal source conflict atomic");
    const auto pivotEdit=SketchGeometry::movePoint(moved.feature,{{"pointId",pivot},{"uMm",5},{"vMm",7}});check(pivotEdit.ok,pivotEdit.error);check(pivotEdit.feature["parameters"].toObject()["scaleGroups"]==moved.feature["parameters"].toObject()["scaleGroups"],"Pivot edit preserves owner/source");
    check(!SketchGeometry::retainedMove(moved.feature,{{"entityIds",ids},{"deltaUMm",1},{"deltaVMm",1}}).ok,"Scale output cannot gain Move ownership");
    check(!SketchGeometry::deleteEntities(moved.feature,ids,{}).ok,"Owned Delete refuses");check(!SketchGeometry::deleteEntities(moved.feature,{},QJsonArray{pivot}).ok,"Pivot Delete refuses");check(!SketchGeometry::addConstraint(moved.feature,{{"type","vertical"},{"entityIds",QJsonArray{ids[1]}}}).ok,"New output relation refuses");
    for(double value:{0.,-1.,1e-7,100001.}){const auto r=scale(source,value);check(!r.ok && r.feature==source,"Factor refusal atomic");}
    for(const auto& args:QJsonArray{QJsonObject{{"groupId",""},{"factor",1.5},{"pivotPointId",pivot}},QJsonObject{{"entityIds",ids},{"factor",1.5},{"pivotPointId",points[0].toObject()["id"]}},QJsonObject{{"entityIds",QJsonArray{ids[0],ids[1],ids[2]}},{"factor",1.5},{"pivotPointId",pivot}}}){const auto r=SketchGeometry::retainedScale(source,args.toObject());check(!r.ok && r.feature==source,"Selection guard atomic");}
    auto fixed=SketchGeometry::addConstraint(source,{{"type","fixed"},{"pointIds",QJsonArray{points[0].toObject()["id"]}}});check(fixed.ok,fixed.error);check(!scale(fixed.feature,1.5).ok,"Fixed refusal");
    auto external=SketchGeometry::addStandalonePoint(source,{{"uMm",10},{"vMm",10}});check(external.ok,external.error);const auto anchor=external.feature["parameters"].toObject()["entities"].toArray().last().toObject()["pointId"].toString();external=SketchGeometry::addConstraint(external.feature,{{"type","coincident"},{"pointIds",QJsonArray{points[0].toObject()["id"],anchor}}});check(external.ok,external.error);check(!scale(external.feature,1.5).ok,"External anchor refusal");
    for(bool driving:{true,false}){QJsonObject args{{"kind","linear"},{"pointIds",QJsonArray{points[0].toObject()["id"],points[1].toObject()["id"]}},{"driving",driving}};if(driving)args["valueMm"]=20;const auto dim=SketchGeometry::addDimension(source,args);check(dim.ok,dim.error);check(!scale(dim.feature,1.5).ok,"Dimension refusal");}
    auto corrupt=scaled.feature,cp=corrupt["parameters"].toObject();auto cache=cp["points"].toArray();auto q=cache[0].toObject();q["uMm"]=14;cache[0]=q;cp["points"]=cache;corrupt["parameters"]=cp;check(!SketchGeometry::solveDraft(corrupt).ok,"Tampered cache refuses");
    check(doc.commit(SketchGeometry::replaceFeature(doc.authored(),moved.feature),"Scale").isEmpty(),"Document commit");const auto authored=doc.authored();doc.history().undo();doc.history().redo();check(doc.authored()==authored,"Exact history");QTemporaryDir dir;check(doc.save(dir.filePath("scale.alloy")).isEmpty(),"Save");CadDocument reopened;check(reopened.open(dir.filePath("scale.alloy")).isEmpty() && reopened.authored()==authored,"Exact reopen");std::cout<<"SKETCH_SCALE_CORE_OK\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
