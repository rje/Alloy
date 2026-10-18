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
    // Two Distance shares the retained source and exact profile evaluator.
    // Distance 1 belongs to pair[0], even if the API supplies the reversed pair.
    CadDocument twoDoc;auto twoSource=source;twoSource["ownerId"]=twoDoc.activeOwnerId();
    auto two=accept(SketchGeometry::chamferCorner(twoSource,{{"entityIds",pair},{"type","twoDistance"},{"distanceMm",10},{"distance2Mm",20}}));
    area(two,11250-100);const auto twoGroup=p(two).value("chamferGroups").toArray().first().toObject();
    auto displayed=p(SketchGeometry::chamferPresentation(two));QMap<QString,QPointF> twoCoords;
    for(const auto& row:displayed.value("points").toArray()){const auto pt=row.toObject();twoCoords[pt.value("id").toString()]={pt.value("uMm").toDouble(),pt.value("vMm").toDouble()};}
    check(QLineF(twoCoords[twoGroup.value("pointIds").toArray()[0].toString()],QPointF(75,27.5)).length()<1e-6,"First distance did not follow first source line");
    check(QLineF(twoCoords[twoGroup.value("pointIds").toArray()[1].toString()],QPointF(55,37.5)).length()<1e-6,"Second distance did not follow second source line");
    const auto reverseTwo=accept(SketchGeometry::chamferCorner(source,{{"entityIds",QJsonArray{pair[1],pair[0]}},{"type","twoDistance"},{"distanceMm",10},{"distance2Mm",20}}));
    check(p(reverseTwo).value("chamferGroups").toArray().first().toObject().value("sourceEntityIds")==QJsonArray{pair[1],pair[0]},"API source order was silently changed");area(reverseTwo,11150);
    check(twoDoc.commit(SketchGeometry::replaceFeature(twoDoc.authored(),two),"Two Distance").isEmpty(),"Commit Two Distance");
    const auto twoProfile=twoDoc.sketches().front().profiles.front().profileId;
    check(twoDoc.commit(CadDocument::extrusionCandidate(twoDoc.authored(),{{"sketchId",two.value("id")},{"profileId",twoProfile},{"distanceMm",5}}),"Extrude").isEmpty(),"Extrude Two Distance");
    const auto twoBody=twoDoc.bodies().front().bodyId;
    two=accept(SketchGeometry::chamferCorner(two,{{"chamferId",twoGroup.value("id")},{"distanceMm",12}}));
    check(p(two).value("chamferGroups").toArray().first().toObject().value("distance2Mm").toDouble()==20,"First edit changed second setback");area(two,11130);
    two=accept(SketchGeometry::chamferCorner(two,{{"chamferId",twoGroup.value("id")},{"distance2Mm",18}}));
    check(p(two).value("chamferGroups").toArray().first().toObject().value("distanceMm").toDouble()==12,"Second edit changed first setback");area(two,11142);
    two=accept(SketchGeometry::updateDimension(two,{{"dimensionId",dimensionIds[0].toObject().value("id")},{"valueMm",160}}));
    two=accept(SketchGeometry::updateDimension(two,{{"dimensionId",dimensionIds[1].toObject().value("id")},{"valueMm",80}}));area(two,12800-108);
    const auto finalGroup=p(two).value("chamferGroups").toArray().first().toObject();
    for(const auto* key:{"id","lineId","pointIds","sourceEntityIds"})check(finalGroup.value(key)==twoGroup.value(key),"Two Distance stable identity changed");
    for(const auto* key:{"distanceMm","distance2Mm"}){const auto bad=SketchGeometry::chamferCorner(two,{{"chamferId",twoGroup.value("id")},{key,200}});check(!bad.ok && bad.feature==two,"Invalid independent setback was not transactional");}
    check(twoDoc.commit(SketchGeometry::replaceFeature(twoDoc.authored(),two),"Edit Two Distance").isEmpty(),"Update Two Distance solid");
    check(twoDoc.bodies().front().bodyId==twoBody && std::abs(twoDoc.bodies().front().summary.value("volumeMm3").toDouble()-63460)<1e-5,"Two Distance solid mismatch");
    const auto twoFinal=twoDoc.authored();twoDoc.history().undo();twoDoc.history().redo();check(twoDoc.authored()==twoFinal,"Two Distance document history mismatch");
    const auto twoPath=temp.filePath("two-distance.alloy");check(twoDoc.save(twoPath).isEmpty(),"Save Two Distance");CadDocument twoReopened;check(twoReopened.open(twoPath).isEmpty() && twoReopened.authored()==twoFinal,"Reopen Two Distance");
    const auto equalConversion=accept(SketchGeometry::chamferCorner(two,{{"chamferId",twoGroup.value("id")},{"type","equalDistance"}}));area(equalConversion,12800-72);
    check(p(equalConversion).value("chamferGroups").toArray().first().toObject().value("id")==twoGroup.value("id"),"Mode change replaced retained identity");
    CadDocument angleDoc;auto angleSource=source;angleSource["ownerId"]=angleDoc.activeOwnerId();
    auto angled=accept(SketchGeometry::chamferCorner(angleSource,{{"entityIds",pair},{"type","distanceAngle"},{"distanceMm",10},{"angleDeg",60}}));
    const auto angleGroup=p(angled).value("chamferGroups").toArray().first().toObject();area(angled,11250-50*std::sqrt(3.0));
    const auto anglePoints=p(SketchGeometry::chamferPresentation(angled)).value("points").toArray();QMap<QString,QPointF> ap;
    for(const auto& row:anglePoints){const auto point=row.toObject();ap[point.value("id").toString()]={point.value("uMm").toDouble(),point.value("vMm").toDouble()};}
    const auto angleContacts=angleGroup.value("pointIds").toArray();check(QLineF(ap[angleContacts[0].toString()],QPointF(75,27.5)).length()<1e-6,"Angle first-side distance changed convention");
    check(QLineF(ap[angleContacts[1].toString()],QPointF(75-10*std::sqrt(3.0),37.5)).length()<1e-6,"60 degree second setback differs from published convention");
    const auto reverseAngle=accept(SketchGeometry::chamferCorner(source,{{"entityIds",QJsonArray{pair[1],pair[0]}},{"type","distanceAngle"},{"distanceMm",10},{"angleDeg",60}}));
    const auto reversedGroup=p(reverseAngle).value("chamferGroups").toArray().first().toObject();QMap<QString,QPointF> reversedCoords;
    for(const auto& row:p(SketchGeometry::chamferPresentation(reverseAngle)).value("points").toArray()){const auto point=row.toObject();reversedCoords[point.value("id").toString()]={point.value("uMm").toDouble(),point.value("vMm").toDouble()};}
    check(QLineF(reversedCoords[reversedGroup.value("pointIds").toArray()[0].toString()],QPointF(65,37.5)).length()<1e-6,"Reversed first support did not own distance");
    check(angleDoc.commit(SketchGeometry::replaceFeature(angleDoc.authored(),angled),"Distance And Angle").isEmpty(),"Commit angle Chamfer");
    const auto angleProfile=angleDoc.sketches().front().profiles.front().profileId;
    check(angleDoc.commit(CadDocument::extrusionCandidate(angleDoc.authored(),{{"sketchId",angled.value("id")},{"profileId",angleProfile},{"distanceMm",5}}),"Extrude").isEmpty(),"Extrude angle Chamfer");const auto angleBody=angleDoc.bodies().front().bodyId;
    angled=accept(SketchGeometry::chamferCorner(angled,{{"chamferId",angleGroup.value("id")},{"distanceMm",12}}));
    check(p(angled).value("chamferGroups").toArray().first().toObject().value("angleDeg").toDouble()==60,"Distance edit changed angle");
    angled=accept(SketchGeometry::chamferCorner(angled,{{"chamferId",angleGroup.value("id")},{"angleDeg",30}}));
    check(p(angled).value("chamferGroups").toArray().first().toObject().value("distanceMm").toDouble()==12,"Angle edit changed first distance");
    angled=accept(SketchGeometry::updateDimension(angled,{{"dimensionId",dimensionIds[0].toObject().value("id")},{"valueMm",160}}));
    angled=accept(SketchGeometry::updateDimension(angled,{{"dimensionId",dimensionIds[1].toObject().value("id")},{"valueMm",80}}));const double angleArea=12800-72/std::sqrt(3.0);area(angled,angleArea);
    const auto finalAngleGroup=p(angled).value("chamferGroups").toArray().first().toObject();for(const auto* key:{"id","lineId","pointIds","sourceEntityIds"})check(finalAngleGroup.value(key)==angleGroup.value(key),"Angle Chamfer identity changed");
    for(const auto theta:{-1,0,90,120,180}){const auto bad=SketchGeometry::chamferCorner(angled,{{"chamferId",angleGroup.value("id")},{"angleDeg",theta}});check(!bad.ok && bad.feature==angled,"Out-of-wedge angle was not transactional");}
    const auto angleSpan=SketchGeometry::chamferCorner(angled,{{"chamferId",angleGroup.value("id")},{"angleDeg",89}});check(!angleSpan.ok && angleSpan.feature==angled,"Angle producing an out-of-span setback was accepted");
    auto crossing=SketchGeometry::newFeature(document.activeOwnerId(),"XY");crossing=SketchGeometry::addLine(crossing,{{"u1Mm",-10},{"v1Mm",0},{"u2Mm",10},{"v2Mm",0}});crossing=SketchGeometry::addLine(crossing,{{"u1Mm",0},{"v1Mm",-10},{"u2Mm",0},{"v2Mm",10}});const auto crossingLines=p(crossing).value("entities").toArray();
    const auto quadrant=SketchGeometry::chamferCorner(crossing,{{"entityIds",QJsonArray{crossingLines[0].toObject().value("id"),crossingLines[1].toObject().value("id")}},{"type","distanceAngle"},{"distanceMm",2},{"angleDeg",30}});check(!quadrant.ok && quadrant.feature==crossing,"Unsupported crossing quadrant was not refused atomically");
    // A 60 degree finite wedge admits theta=90 (right corners do not).
    auto triangle=SketchGeometry::newFeature(document.activeOwnerId(),"XY");triangle=SketchGeometry::addLine(triangle,{{"u1Mm",0},{"v1Mm",0},{"u2Mm",100},{"v2Mm",0}});auto ts=p(triangle).value("entities").toArray();const auto origin=ts[0].toObject().value("startPointId"),east=ts[0].toObject().value("endPointId");
    triangle=SketchGeometry::addLine(triangle,{{"u1Mm",100},{"v1Mm",0},{"u2Mm",50},{"v2Mm",50*std::sqrt(3.0)},{"startPointId",east}});ts=p(triangle).value("entities").toArray();triangle=SketchGeometry::addLine(triangle,{{"u1Mm",50},{"v1Mm",50*std::sqrt(3.0)},{"u2Mm",0},{"v2Mm",0},{"startPointId",ts[1].toObject().value("endPointId")},{"endPointId",origin}});ts=p(triangle).value("entities").toArray();
    const QJsonArray trianglePair{ts[0].toObject().value("id"),ts[2].toObject().value("id")};const auto oblique=accept(SketchGeometry::chamferCorner(triangle,{{"entityIds",trianglePair},{"type","distanceAngle"},{"distanceMm",10},{"angleDeg",90}}));area(oblique,2500*std::sqrt(3.0)-50*std::sqrt(3.0));
    const auto beyondWedge=SketchGeometry::chamferCorner(triangle,{{"entityIds",trianglePair},{"type","distanceAngle"},{"distanceMm",10},{"angleDeg",120}});check(!beyondWedge.ok && beyondWedge.feature==triangle,"Oblique wedge limit was ignored");
    check(angleDoc.commit(SketchGeometry::replaceFeature(angleDoc.authored(),angled),"Edit angle Chamfer").isEmpty(),"Update angle Chamfer solid");check(angleDoc.bodies().front().bodyId==angleBody && std::abs(angleDoc.bodies().front().summary.value("volumeMm3").toDouble()-5*angleArea)<1e-5,"Angle Chamfer solid mismatch");
    const auto angleFinal=angleDoc.authored();angleDoc.history().undo();angleDoc.history().redo();check(angleDoc.authored()==angleFinal,"Angle Chamfer history mismatch");const auto anglePath=temp.filePath("distance-angle.alloy");check(angleDoc.save(anglePath).isEmpty(),"Save angle Chamfer");CadDocument angleReopened;check(angleReopened.open(anglePath).isEmpty() && angleReopened.authored()==angleFinal,"Reopen angle Chamfer");
    std::cout<<"PASS: retained Equal/Two Distance/Distance And Angle Chamfers, source ordering and edits, stable IDs, atomic refusal, extrusion, Undo/Redo and reopen\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
