// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <BRepTools.hxx>
#include <BRepAlgoAPI_Check.hxx>
#include <sstream>
#include <iostream>
#include <numbers>
#include <cmath>
namespace {
void check(bool yes,const QString& message){if(!yes)throw std::runtime_error(message.toStdString());}
QJsonObject append(QJsonObject doc,const QJsonObject& feature){auto rows=doc["features"].toArray();rows.append(feature);doc["features"]=rows;return doc;}
// Only the OCCT container ownership flag is normalized in this test baseline.
std::string bytes(TopoDS_Shape shape){shape.Free(false);std::ostringstream out;BRepTools::Write(shape,out);return out.str();}
void unchanged(const EvaluatedBody& before,const EvaluatedBody& after){check(before.shape.IsSame(after.shape)&&before.bodyId==after.bodyId&&before.featureId==after.featureId&&before.supportFaces.size()==after.supportFaces.size(),"exact accepted body preserved");for(size_t i=0;i<before.supportFaces.size();++i)check(before.supportFaces[i].faceId==after.supportFaces[i].faceId&&before.supportFaces[i].face.IsSame(after.supportFaces[i].face),"exact support references preserved");}
}
int main(int argc,char**argv){QCoreApplication app(argc,argv);try{
 CadDocument d;const auto owner=d.activeOwnerId();QJsonObject box{{"id",CadDocument::newId()},{"bodyId",CadDocument::newId()},{"ownerId",owner},{"type","box"},{"name","Box"},{"visible",true},{"parameters",QJsonObject{{"widthMm",10},{"depthMm",10},{"heightMm",10},{"xMm",0},{"yMm",0},{"zMm",0}}}};
 check(d.commit(append(d.authored(),box),"Box").isEmpty(),"box");const auto original=d.bodies()[0];
 QJsonObject plane{{"id",CadDocument::newId()},{"ownerId",owner},{"type","constructionPlane"},{"name","Cut plane"},{"visible",true},{"parameters",QJsonObject{{"source",QJsonObject{{"plane","XY"}}},{"offsetMm",-1}}}};
 auto candidate=append(d.authored(),plane);auto ready=d.prepareCandidate(candidate);check(ready.ok&&ready.diagnostics["recompute"]=="exact_prefix_append","plane append selected");unchanged(original,ready.evaluation->bodies[0]);check(d.commit(candidate,"Plane").isEmpty(),"plane commit");
 auto sketch=SketchGeometry::newConstructionPlaneFeature(owner,plane["id"].toString(),"Bore");sketch=SketchGeometry::addCircle(sketch,{{"centerUMm",5},{"centerVMm",5},{"diameterMm",2}});candidate=SketchGeometry::replaceFeature(d.authored(),sketch);ready=d.prepareCandidate(candidate);check(ready.ok&&ready.diagnostics["recompute"]=="exact_prefix_append","sketch append selected");unchanged(original,ready.evaluation->bodies[0]);check(d.commit(candidate,"Bore sketch").isEmpty(),"sketch commit");
 const auto before=d.authored();const auto snapshot=d.evaluation();const auto baseline=bytes(snapshot.bodies[0].shape);const auto revision=d.revision();const auto history=d.history().count();QJsonObject cut{{"sketchId",sketch["id"]},{"profileId",d.sketches().back().profiles[0].profileId},{"distanceMm",.5},{"operation","cut"},{"targetBodyIds",QJsonArray{original.bodyId}}};
 auto hide=[&](QJsonObject doc){auto rows=doc["features"].toArray();auto row=rows[2].toObject();row["visible"]=false;rows[2]=row;doc["features"]=rows;return doc;};
 candidate=hide(CadDocument::extrusionCandidate(before,cut));ready=d.prepareCandidate(candidate);check(!ready.ok&&ready.diagnostics["code"]=="cut_no_intersection","failed append Cut refuses");check(!d.commit(candidate,"Failed Cut").isEmpty()&&d.authored()==before&&d.revision()==revision&&d.history().count()==history&&d.sketches().back().visible,"failed suffix atomic");check(bytes(snapshot.bodies[0].shape)==baseline,"failed Cut preserves prior snapshot BRep");
 cut["distanceMm"]=12;candidate=hide(CadDocument::extrusionCandidate(before,cut));ready=d.prepareCandidate(candidate);check(ready.ok&&ready.diagnostics["recompute"]=="exact_prefix_append","Cut and automatic source hide append selected");check(d.authored()==before&&d.sketches().back().visible,"preview leaves authoritative state unchanged");check(!ready.evaluation->sketches.back().visible&&snapshot.sketches.back().visible,"source visibility only copied");check(bytes(snapshot.bodies[0].shape)==baseline,"successful Cut preserves prior snapshot BRep");check(BRepAlgoAPI_Check(ready.evaluation->bodies[0].shape,true,true).IsValid(),"actual Cut Boolean valid");const double volume=1000-10*std::numbers::pi;check(std::abs(ready.evaluation->bodies[0].summary["volumeMm3"].toDouble()-volume)<1e-6,"actual through bore volume");
 const auto full=CadDocument::evaluateCandidate(candidate);check(full.ok&&full.authored==ready.authored&&std::abs(full.evaluation->bodies[0].summary["volumeMm3"].toDouble()-volume)<1e-6,"append agrees with full authority");check(d.commit(candidate,"Bore Cut").isEmpty()&&d.history().count()==history+1,"single authored hide/Cut entry");const auto accepted=d.authored();check(d.state()["evaluation"].toObject()["recompute"]=="exact_prefix_append","inspection reports append authority");d.history().undo();check(d.authored()==before&&d.sketches().back().visible,"undo exact source hide/Cut");unchanged(snapshot.bodies[0],d.bodies()[0]);d.history().redo();check(d.authored()==accepted&&!d.sketches().back().visible,"redo exact source hide/Cut");
 // A feature visibility-only edit must copy the immutable evaluated snapshot,
 // update producer visibility (not the body's last modifier), and skip OCCT.
 const auto acceptedEvaluation=d.evaluation();const auto acceptedBody=acceptedEvaluation.bodies[0];
 const auto acceptedBodyBytes=bytes(acceptedBody.shape);auto visibilityCandidate=accepted;
 auto visibilityRows=visibilityCandidate["features"].toArray();
 auto hiddenBox=visibilityRows[0].toObject();hiddenBox["visible"]=false;visibilityRows[0]=hiddenBox;
 auto hiddenPlane=visibilityRows[1].toObject();hiddenPlane["visible"]=false;visibilityRows[1]=hiddenPlane;
 auto shownSketch=visibilityRows[2].toObject();shownSketch["visible"]=true;visibilityRows[2]=shownSketch;
 // The final Cut stays visible while its source body producer is hidden.
 visibilityCandidate["features"]=visibilityRows;
 auto visibilityReady=d.prepareCandidate(visibilityCandidate);
 check(visibilityReady.ok&&visibilityReady.diagnostics["recompute"]=="visibility_only","visibility-only candidate skips geometry evaluation");
 check(visibilityReady.evaluation->bodies[0].visible==false,"body visibility follows its producer feature, not the Cut modifier");
 check(visibilityReady.evaluation->sketches.back().visible&& !visibilityReady.evaluation->constructionPlanes.back().visible,"sketch and plane visibility copied by producer identity");
 unchanged(acceptedBody,visibilityReady.evaluation->bodies[0]);
 check(visibilityReady.evaluation->bodies[0].summary==acceptedBody.summary
       &&bytes(visibilityReady.evaluation->bodies[0].shape)==acceptedBodyBytes,"visibility copy preserves geometry identity and summary");
 check(acceptedEvaluation.bodies[0].visible&&acceptedEvaluation.sketches.back().visible==false
       &&acceptedEvaluation.constructionPlanes.back().visible,"old shared evaluation snapshot remains unchanged");
 const auto visibilityHistory=d.history().count();
 check(d.commit(visibilityCandidate,"Hide source features").isEmpty()&&d.history().count()==visibilityHistory+1,"visibility edit commits as one history action");
 check(d.state()["evaluation"].toObject()["recompute"]=="visibility_only","committed inspection reports visibility-only recompute");
 d.history().undo();check(d.authored()==accepted&&d.bodies()[0].visible,"visibility undo restores authored and evaluated state");
 d.history().redo();check(d.authored()==visibilityCandidate&&!d.bodies()[0].visible,"visibility redo reapplies producer visibility");
 const auto noOpHistory=d.history().count();check(d.commit(visibilityCandidate,"No-op visibility").isEmpty()&&d.history().count()==noOpHistory,"no-op visibility creates no history action");
 auto invalidVisibility=visibilityCandidate;visibilityRows=invalidVisibility["features"].toArray();hiddenBox=visibilityRows[0].toObject();hiddenBox["visible"]="false";visibilityRows[0]=hiddenBox;invalidVisibility["features"]=visibilityRows;
 check(!d.commit(invalidVisibility,"Invalid visibility").isEmpty()&&d.authored()==visibilityCandidate
       &&d.history().count()==noOpHistory&&!d.bodies()[0].visible,"invalid visibility refuses atomically");
 // A hidden sketch remains a valid source for an ordinary geometry edit; the
 // mixed visibility/geometry candidate must take the full ordered path.
 auto hiddenSourceEdit=visibilityCandidate;visibilityRows=hiddenSourceEdit["features"].toArray();
 shownSketch=visibilityRows[2].toObject();shownSketch["visible"]=false;visibilityRows[2]=shownSketch;
 auto changedCut=visibilityRows[3].toObject(),changedCutParameters=changedCut["parameters"].toObject();
 changedCutParameters["distanceMm"]=13;changedCut["parameters"]=changedCutParameters;visibilityRows[3]=changedCut;hiddenSourceEdit["features"]=visibilityRows;
 const auto hiddenSourceReady=d.prepareCandidate(hiddenSourceEdit);
 check(hiddenSourceReady.ok&&hiddenSourceReady.diagnostics["recompute"]=="full_ordered"
       &&!hiddenSourceReady.evaluation->sketches.back().visible
       &&hiddenSourceReady.evaluation->bodies[0].visible==false,"hidden sketch still drives a mixed geometry edit through full evaluation");
 QTemporaryDir visibilityTemp;check(d.save(visibilityTemp.filePath("visibility.alloy")).isEmpty(),"save visibility-only state");
 CadDocument visibilityReopened;check(visibilityReopened.open(visibilityTemp.filePath("visibility.alloy" )).isEmpty()
       &&visibilityReopened.authored()==visibilityCandidate
       &&visibilityReopened.evaluation().recompute=="full_ordered"
       &&!visibilityReopened.bodies()[0].visible
       &&visibilityReopened.sketches().back().visible
       &&!visibilityReopened.evaluation().constructionPlanes[0].visible,"cold reopen reevaluates and preserves authored visibility");
 d.history().undo();check(d.authored()==accepted&&d.bodies()[0].visible,"return to pre-visibility state for existing source-edit checks");
 auto edited=accepted;auto rows=edited["features"].toArray();auto change=rows[0].toObject(),p=change["parameters"].toObject();p["widthMm"]=12;change["parameters"]=p;rows[0]=change;edited["features"]=rows;const auto fallback=d.prepareCandidate(append(edited,{{"id",CadDocument::newId()},{"ownerId",owner},{"type","constructionPlane"},{"name","After edit"},{"visible",true},{"parameters",plane["parameters"]}}));check(fallback.ok&&fallback.diagnostics["recompute"]=="full_ordered"&&std::abs(fallback.evaluation->bodies[0].summary["volumeMm3"].toDouble()-(1200-10*std::numbers::pi))<1e-6,"upstream edit rebuilds full prefix");
 QTemporaryDir temp;check(d.save(temp.filePath("append.alloy")).isEmpty(),"save");CadDocument reopened;check(reopened.open(temp.filePath("append.alloy")).isEmpty()&&reopened.authored()==accepted&&reopened.evaluation().recompute=="full_ordered","open full rebuild and exact authored state");
 std::cout<<"PASS exact plane/sketch/Cut append, source hide, snapshot refs/BRep, failure/history, upstream/full/open\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
