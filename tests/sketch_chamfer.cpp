// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QJsonArray>
#include <QLineF>
#include <QMap>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
#include <stdexcept>
namespace {
void check(bool v,const QString& m){if(!v)throw std::runtime_error(m.toStdString());}
QJsonObject accept(const SketchGeometry::SolveResult& r){check(r.ok,r.error);return r.feature;}
QJsonObject p(const QJsonObject& f){return f.value("parameters").toObject();}
void area(const QJsonObject& f,double expected){const auto e=SketchGeometry::evaluateFeature(f);check(e.profiles.size()==1,"Chamfer did not yield one closed profile");check(std::abs(e.profiles[0].summary.value("areaMm2").toDouble()-expected)<1e-6,"Chamfer analytic area differs");}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    CadDocument document;
    auto source=accept(SketchGeometry::addCenterRectangle(SketchGeometry::newFeature(document.activeOwnerId(),"XZ"),{{"centerUMm",0},{"centerVMm",0},{"widthMm",150},{"heightMm",75}}));
    const auto entities=p(source).value("entities").toArray();const QJsonArray pair{entities[1].toObject().value("id"),entities[2].toObject().value("id")};
    const auto dimensionIds=p(source).value("dimensions").toArray();const auto sourceConstraints=p(source).value("constraints");
    const auto refused=SketchGeometry::chamferCorner(source,{{"entityIds",pair},{"distanceMm",80}});check(!refused.ok && refused.feature==source,"Initial oversized Chamfer was not transactional");
    auto sketch=accept(SketchGeometry::chamferCorner(source,{{"entityIds",pair},{"distanceMm",10}}));
    const auto group=p(sketch).value("chamferGroups").toArray().first().toObject();const auto id=group.value("id"),lineId=group.value("lineId"),contacts=group.value("pointIds");
    check(p(sketch).value("entities")==p(source).value("entities") && p(sketch).value("points")==p(source).value("points"),"Source geometry was rewritten");
    check(p(sketch).value("constraints")==sourceConstraints && p(sketch).value("dimensions")==p(source).value("dimensions"),"Source design intent was changed");area(sketch,150*75-50);
    // Adjacent upper corners own opposite endpoints of the shared top line.
    const QJsonArray secondPair{entities[2].toObject().value("id"),entities[3].toObject().value("id")};
    sketch=accept(SketchGeometry::chamferCorner(sketch,{{"entityIds",secondPair},{"distanceMm",8}}));
    const auto second=p(sketch).value("chamferGroups").toArray()[1].toObject();
    area(sketch,150*75-50-32);
    const auto duplicate=SketchGeometry::chamferCorner(sketch,{{"entityIds",QJsonArray{pair[1],pair[0]}},{"distanceMm",2}});
    check(!duplicate.ok && duplicate.feature==sketch,"Duplicate corner was not transactional");
    const auto overlap=SketchGeometry::chamferCorner(sketch,{{"chamferId",second.value("id")},{"distanceMm",140}});
    check(!overlap.ok && overlap.feature==sketch,"Adjacent cuts touching on shared line were accepted");
    check(document.commit(SketchGeometry::replaceFeature(document.authored(),sketch),"Equal Distance Chamfer").isEmpty(),"Cannot commit retained Chamfer");
    const auto profileId=document.sketches().front().profiles.front().profileId;
    check(document.commit(CadDocument::extrusionCandidate(document.authored(),{{"sketchId",sketch.value("id")},{"profileId",profileId},{"distanceMm",5}}),"Extrude").isEmpty(),"Chamfer profile cannot extrude");
    const auto bodyId=document.bodies().front().bodyId;
    sketch=accept(SketchGeometry::chamferCorner(sketch,{{"chamferId",id},{"distanceMm",12}}));
    sketch=accept(SketchGeometry::chamferCorner(sketch,{{"chamferId",second.value("id")},{"distanceMm",9}}));
    const auto secondEdited=p(sketch).value("chamferGroups").toArray()[1].toObject();
    auto expectedSecond=second;expectedSecond["distanceMm"]=9;
    check(secondEdited==expectedSecond,"Independent Chamfer edit changed stable source/generated IDs");
    sketch=accept(SketchGeometry::updateDimension(sketch,{{"dimensionId",dimensionIds[0].toObject().value("id")},{"valueMm",160}}));
    sketch=accept(SketchGeometry::updateDimension(sketch,{{"dimensionId",dimensionIds[1].toObject().value("id")},{"valueMm",80}}));area(sketch,160*80-72-40.5);
    const auto edited=p(sketch).value("chamferGroups").toArray().first().toObject();check(edited.value("id")==id && edited.value("lineId")==lineId && edited.value("pointIds")==contacts,"Chamfer generated identities changed through edits");
    const auto evaluated=SketchGeometry::evaluateFeature(sketch);check(evaluated.profiles.front().profileId==profileId,"Chamfer source edits changed profile identity");
    const auto display=SketchGeometry::chamferPresentation(sketch);const auto dp=p(display);QMap<QString,QPointF> coords;for(const auto& row:dp.value("points").toArray()){const auto v=row.toObject();coords[v.value("id").toString()]={v.value("uMm").toDouble(),v.value("vMm").toDouble()};}
    const auto a=coords[contacts.toArray()[0].toString()],b=coords[contacts.toArray()[1].toString()];check(std::abs(QLineF(a,b).length()-std::sqrt(2.0)*12)<1e-6,"Chamfer is not two equal setbacks");
    auto reversed=sketch;auto reversedParameters=p(sketch);const auto gs=reversedParameters.value("chamferGroups").toArray();reversedParameters["chamferGroups"]=QJsonArray{gs[1],gs[0]};reversed["parameters"]=reversedParameters;
    area(reversed,12800-72-40.5);
    const auto narrow=accept(SketchGeometry::updateDimension(sketch,{{"dimensionId",dimensionIds[0].toObject().value("id")},{"valueMm",25}}));
    const auto sharedOverlap=SketchGeometry::chamferCorner(narrow,{{"chamferId",second.value("id")},{"distanceMm",14}});
    check(!sharedOverlap.ok && sharedOverlap.feature==narrow,"Individually valid setbacks overlapping on shared source were accepted");
    const auto sourceOverlap=SketchGeometry::updateDimension(sketch,{{"dimensionId",dimensionIds[0].toObject().value("id")},{"valueMm",20}});
    check(!sourceOverlap.ok && sourceOverlap.feature==sketch,"Source edit overlapping adjacent trims was not transactional");
    const auto tooLarge=SketchGeometry::chamferCorner(sketch,{{"chamferId",id},{"distanceMm",80}});check(!tooLarge.ok && tooLarge.feature==sketch,"Oversized edited Chamfer was not transactional");
    const auto collapse=SketchGeometry::updateDimension(sketch,{{"dimensionId",dimensionIds[1].toObject().value("id")},{"valueMm",10}});check(!collapse.ok && collapse.feature==sketch,"Source size collapse was not transactional");
    auto modified=sketch;auto mp=p(modified);auto lines=mp.value("entities").toArray();lines.removeAt(1);mp["entities"]=lines;modified["parameters"]=mp;check(!SketchGeometry::solveDraft(modified).ok,"Deleted Chamfer source was silently detached");
    check(document.commit(SketchGeometry::replaceFeature(document.authored(),sketch),"Resize Chamfer source").isEmpty(),"Cannot update downstream Chamfer solid");check(document.bodies().front().bodyId==bodyId && std::abs(document.bodies().front().summary.value("volumeMm3").toDouble()-5*(12800-72-40.5))<1e-5,"Downstream volume/body changed");
    const auto final=document.authored();document.history().undo();document.history().redo();check(document.authored()==final,"Chamfer Undo/Redo mismatch");QTemporaryDir temp;const auto path=temp.filePath("chamfer.alloy");check(document.save(path).isEmpty(),"Save Chamfer");CadDocument reopened;check(reopened.open(path).isEmpty() && reopened.authored()==final,"Chamfer reopen mismatch");area(reopened.sketches().front().summary.isEmpty()?sketch:SketchGeometry::findFeature(reopened.authored(),sketch.value("id").toString()),12800-72-40.5);
    std::cout<<"PASS: multiple adjacent retained equal setbacks, source dimensions, stable profile/line IDs, atomic refusal, extrusion, Undo/Redo and reopen\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
