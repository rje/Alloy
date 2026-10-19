// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QPointF>
#include <iostream>
#include <numbers>
#include <cmath>
namespace {
void check(bool ok,const QString& reason){if(!ok)throw std::runtime_error(reason.toStdString());}
QJsonObject accept(const SketchGeometry::SolveResult& r){check(r.ok,r.error);return r.feature;}
void commit(CadDocument& d,const QJsonObject& a){check(d.commit(a,"Fixture").isEmpty(),"commit");}
QPointF center(const EvaluatedSketch& sketch,const QString& id){for(const auto& p:sketch.summary["worldPoints"].toArray())if(p.toObject()["id"]==id){const auto v=p.toObject()["positionMm"].toArray();return {v[0].toDouble(),v[1].toDouble()};}throw std::runtime_error("center absent");}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try {
    CadDocument d;const auto owner=d.authored()["documentId"].toString();auto base=SketchGeometry::newFeature(owner,"XY","Plate");base=SketchGeometry::addRectangle(base,{{"u1Mm",0},{"v1Mm",0},{"u2Mm",40},{"v2Mm",25}});auto entities=base["parameters"].toObject()["entities"].toArray();
    base=accept(SketchGeometry::addConstraint(base,{{"type","fixed"},{"pointIds",QJsonArray{entities[0].toObject()["startPointId"]}}}));
    base=accept(SketchGeometry::addDimension(base,{{"kind","horizontal"},{"entityId",entities[0].toObject()["id"]},{"valueMm",40}}));const auto widthId=base.value("parameters").toObject().value("dimensions").toArray().last().toObject().value("id");
    base=accept(SketchGeometry::addDimension(base,{{"kind","vertical"},{"entityId",entities[1].toObject()["id"]},{"valueMm",25}}));commit(d,SketchGeometry::replaceFeature(d.authored(),base));commit(d,CadDocument::extrusionCandidate(d.authored(),{{"sketchId",base["id"]},{"profileId",d.sketches()[0].profiles[0].profileId},{"distanceMm",5}}));const auto body=d.bodies()[0].bodyId;
    EvaluatedSupportFace cap;for(const auto& face:d.bodies()[0].supportFaces)if(face.role=="extrude.endCap")cap=face;QJsonObject support{{"bodyId",body},{"producerFeatureId",cap.producerFeatureId},{"faceId",cap.faceId}};
    auto hole=SketchGeometry::newSupportedFeature(owner,support,"Retained hole",GeometryKernel::supportEdgeDescriptors(cap));auto context=GeometryKernel::projectionContext(d.evaluation(),hole);QString right,bottom;for(const auto& value:context["lines"].toArray()){const auto line=value.toObject();if(std::abs(line["u1Mm"].toDouble()-40)<1e-7 && std::abs(line["u2Mm"].toDouble()-40)<1e-7)right=line["id"].toString();if(std::abs(line["v1Mm"].toDouble())<1e-7 && std::abs(line["v2Mm"].toDouble())<1e-7)bottom=line["id"].toString();}check(!right.isEmpty() && !bottom.isEmpty(),"right/bottom projections");
    hole=SketchGeometry::addCircle(hole,{{"centerUMm",30},{"centerVMm",12},{"diameterMm",6}});const auto circle=hole["parameters"].toObject()["entities"].toArray()[0].toObject();const auto point=circle["centerPointId"].toString();auto solve=SketchGeometry::solveDraft(hole,context);check(solve.ok && solve.diagnostics["dof"]==3,"free center and radius");
    solve=SketchGeometry::addDimension(hole,{{"kind","pointLineDistance"},{"pointIds",QJsonArray{point}},{"entityId",right},{"valueMm",5}},context);check(solve.ok && solve.diagnostics["dof"]==2,"one distance removes one DOF only");hole=solve.feature;const auto first=hole["parameters"].toObject()["dimensions"].toArray()[0].toObject();
    solve=SketchGeometry::addDimension(hole,{{"kind","pointLineDistance"},{"pointIds",QJsonArray{point}},{"entityId",bottom},{"valueMm",10}},context);check(solve.ok && solve.diagnostics["dof"]==1,"perpendicular distance leaves radius free");hole=solve.feature;
    check(std::abs(hole["parameters"].toObject()["entities"].toArray()[0].toObject()["radiusMm"].toDouble()-3)<1e-9,"distance preserves radius");
    hole=accept(SketchGeometry::addDimension(hole,{{"kind","radius"},{"entityId",circle["id"]},{"valueMm",3}},context));const auto stableHole=hole;
    // A duplicate matching point/retained-edge driver offers a measurement only.
    const QJsonObject matching{{"kind","pointLineDistance"},{"pointIds",QJsonArray{point}},{"entityId",right},{"valueMm",5}};
    const auto redundant=SketchGeometry::addDimension(hole,matching,context);
    check(!redundant.ok && redundant.feature==hole,"redundant point-edge driver atomic");
    const auto offer=SketchGeometry::drivenAlternative(hole,matching,redundant,context);
    check(offer && offer->ok,"point-edge matching redundancy offers Driven");
    const auto measured=offer->feature.value("parameters").toObject().value("dimensions").toArray().last().toObject();
    check(!measured.value("driving").toBool() && !measured.contains("valueMm") && measured.value("pointIds")==QJsonArray{point} && measured.value("entityId")==right,"Driven retains exact point-edge references");
    check(offer->feature.value("parameters").toObject().value("points")==hole.value("parameters").toObject().value("points") && offer->feature.value("parameters").toObject().value("entities")==hole.value("parameters").toObject().value("entities"),"Driven acceptance preserves solved geometry");
    const auto moved=SketchGeometry::updateDimension(offer->feature,{{"dimensionId",first.value("id")},{"valueMm",6}},context);
    check(moved.ok,moved.error);bool measuresSix=false;
    for(const auto& value:moved.diagnostics.value("dimensionValues").toArray())if(value.toObject().value("id")==measured.value("id"))measuresSix=std::abs(value.toObject().value("valueMm").toDouble()-6)<1e-7;
    check(measuresSix && moved.feature.value("parameters").toObject().value("dimensions").toArray().last().toObject()==measured,"Driven follows source driver without authoring a value");
    const QJsonObject conflicting{{"kind","pointLineDistance"},{"pointIds",QJsonArray{point}},{"entityId",right},{"valueMm",6}};
    check(!SketchGeometry::drivenAlternative(hole,conflicting,SketchGeometry::addDimension(hole,conflicting,context),context),"point-edge conflicting requested value offers no measurement");
    const auto conflict=SketchGeometry::addDimension(hole,{{"kind","pointLineDistance"},{"pointIds",QJsonArray{point}},{"entityId",right},{"valueMm",6}},context);check(!conflict.ok && conflict.feature==hole,"conflict draft atomic");
    hole=accept(SketchGeometry::updateDimension(hole,{{"dimensionId",first["id"]},{"valueMm",6}},context));check(hole["parameters"].toObject()["dimensions"].toArray()[0].toObject()["side"]==first["side"],"edit preserves branch");hole=stableHole;
    commit(d,SketchGeometry::replaceFeature(d.authored(),hole));check((center(d.sketches()[1],point)-QPointF(35,10)).manhattanLength()<1e-6,"solver center35,10");const auto holeId=hole["id"].toString();commit(d,CadDocument::extrusionCandidate(d.authored(),{{"sketchId",holeId},{"profileId",d.sketches()[1].profiles[0].profileId},{"operation","cut"},{"targetBodyIds",QJsonArray{body}},{"distanceMm",-5}}));const auto before=d.authored();check(std::abs(d.bodies()[0].summary["volumeMm3"].toDouble()-(5000-45*std::numbers::pi))<1e-5,"cut plate volume");
    base=accept(SketchGeometry::updateDimension(base,{{"dimensionId",widthId},{"valueMm",50}}));commit(d,SketchGeometry::replaceFeature(d.authored(),base));check((center(d.sketches()[1],point)-QPointF(45,10)).manhattanLength()<1e-6,"upstream center follows retained edge");check(std::abs(d.bodies()[0].summary["volumeMm3"].toDouble()-(6250-45*std::numbers::pi))<1e-5,"resized cut volume");const auto after=d.authored();const auto retained=SketchGeometry::findFeature(after,holeId)["parameters"].toObject();check(retained["dimensions"]==stableHole["parameters"].toObject()["dimensions"] && retained["projections"]==stableHole["parameters"].toObject()["projections"] && retained["constraints"].toArray()==stableHole["parameters"].toObject()["constraints"].toArray(),"upstream preserves equations/references without adding fixed coordinates");d.history().undo();check(d.authored()==before,"exact undo");d.history().redo();check(d.authored()==after,"exact redo");
    QTemporaryDir tmp;auto path=tmp.filePath("retained-hole.alloy");check(d.save(path).isEmpty(),"save");CadDocument reopened;check(reopened.open(path).isEmpty() && reopened.authored()==after,"exact reopen");check((center(reopened.sketches()[1],point)-QPointF(45,10)).manhattanLength()<1e-6,"reopened retained center");std::cout<<"Point-edge DOF/side/retained-size/Cut/history/storage checks passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
