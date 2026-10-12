#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QJsonArray>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool value, const char* message) {if (!value) throw std::runtime_error(message);}
void near(double value, double expected, const char* message) {check(std::abs(value-expected) < 1e-6,message);}
QJsonObject solved(const SketchGeometry::SolveResult& result) {
    check(result.ok,result.error.toUtf8().constData());return result.feature;
}
double volume(const EvaluatedDocument& document) {return document.bodies.at(0).summary["volumeMm3"].toDouble();}
void pluralExtrusion() {
    CadDocument document;const auto owner=document.authored()["documentId"].toString();
    auto sketch=SketchGeometry::newFeature(owner,"XY","Connected three regions");
    sketch=SketchGeometry::addLine(sketch,{{"u1Mm",0},{"v1Mm",0},{"u2Mm",500},{"v2Mm",0}});
    sketch=SketchGeometry::addLine(sketch,{{"u1Mm",500},{"v1Mm",100},{"u2Mm",0},{"v2Mm",100}});
    sketch=SketchGeometry::addCircle(sketch,{{"centerUMm",0},{"centerVMm",50},{"diameterMm",100}});
    sketch=SketchGeometry::addCircle(sketch,{{"centerUMm",500},{"centerVMm",50},{"diameterMm",100}});
    sketch=solved(SketchGeometry::solveDraft(sketch));
    check(document.commit(SketchGeometry::replaceFeature(document.authored(),sketch),"Three regions").isEmpty(),"Commit plural region source");
    const auto profiles=document.sketches()[0].profiles;check(profiles.size()==3,"Plural extrusion fixture needs three real regions");
    QJsonArray ids,disks;double area=0;
    for(const auto& profile:profiles){ids.append(profile.profileId);area+=profile.summary["areaMm2"].toDouble();
        if(profile.summary["sourceEntityIds"].toArray().size()==1)disks.append(profile.profileId);}
    check(disks.size()==2,"Disconnected fixture needs two end disks");
    const auto before=document.authored();const auto sketchId=sketch["id"];
    const auto candidate=CadDocument::extrusionCandidate(before,{{"sketchId",sketchId},{"profileIds",ids},{"distanceMm",50}});
    const auto preview=CadDocument::evaluateCandidate(candidate);check(preview.ok,preview.error.toUtf8().constData());
    check(preview.evaluation->bodies.size()==1,"Plural extrusion produced multiple bodies");
    const auto summary=preview.evaluation->bodies[0].summary;
    check(summary["topology"].toObject()["solids"]==1 && summary["closed"].toBool() && summary["valid"].toBool(),"Plural extrusion is not one valid closed solid");
    near(summary["volumeMm3"].toDouble(),area*50,"Plural extrusion volume is not region area times distance");
    near(summary["sourceAreaMm2"].toDouble(),area,"Plural extrusion lost source union area");
    check(summary["sourceProfileIds"].toArray().size()==3,"Plural extrusion lost retained source profile IDs");
    check(document.authored()==before && document.bodies().empty(),"Plural preview mutated document");
    check(document.commit(candidate,"Extrude three regions").isEmpty(),"Commit plural extrusion");
    const auto committed=document.authored();const auto revision=document.revision();const auto history=document.history().count();
    const auto disjoint=CadDocument::extrusionCandidate(before,{{"sketchId",sketchId},{"profileIds",disks},{"distanceMm",50}});
    check(!document.commit(disjoint,"Disconnected regions").isEmpty(),"Disjoint regions silently became one feature body");
    check(document.authored()==committed && document.revision()==revision && document.history().count()==history,"Disconnected failure changed usable work");
    bool duplicateRejected=false;try{CadDocument::extrusionCandidate(before,{{"sketchId",sketchId},{"profileIds",QJsonArray{ids[0],ids[0]}},{"distanceMm",50}});}catch(const std::exception&){duplicateRejected=true;}
    check(duplicateRejected,"Plural selection accepted duplicate IDs");
    const auto missing=CadDocument::evaluateCandidate(CadDocument::extrusionCandidate(before,{{"sketchId",sketchId},{"profileIds",QJsonArray{ids[0],CadDocument::newId()}},{"distanceMm",50}}));
    check(!missing.ok && missing.diagnostics["code"]=="profile_unavailable","Missing plural reference had no useful error");
    document.history().undo();check(document.authored()==before,"Plural undo lost source document");document.history().redo();check(document.authored()==committed,"Plural redo changed retained input IDs");
    QTemporaryDir directory;check(directory.isValid(),"Plural storage fixture directory");
    const auto path=directory.path()+"/three-region-extrusion.alloy";check(document.save(path).isEmpty(),"Save plural extrusion");
    CadDocument reopened;check(reopened.open(path).isEmpty() && reopened.authored()==committed,"Plural storage lost schema/reference IDs");
    near(volume(GeometryKernel::evaluateDocument(reopened.authored())),area*50,"Reopened plural extrusion volume");
}
QJsonObject replaceExtrusionParameters(const QJsonObject& document, const QJsonObject& edits) {
    auto candidate = document;auto features = candidate["features"].toArray();
    auto feature = features[1].toObject();auto parameters = feature["parameters"].toObject();
    for (auto it = edits.begin(); it != edits.end(); ++it) parameters[it.key()] = it.value();
    feature["parameters"] = parameters;features[1] = feature;candidate["features"] = features;return candidate;
}
}

int main(int argc, char** argv) {
    QCoreApplication app(argc,argv);
    try {
        CadDocument document;
        const auto root = document.authored()["documentId"].toString();
        auto sketch = SketchGeometry::addRectangle(SketchGeometry::newFeature(root,"XY"),
            {{"u1Mm",0},{"v1Mm",0},{"u2Mm",40},{"v2Mm",25}});
        const auto points = sketch["parameters"].toObject()["points"].toArray();
        const auto entities = sketch["parameters"].toObject()["entities"].toArray();
        sketch = solved(SketchGeometry::addConstraint(sketch,{{"type","fixed"},{"pointIds",QJsonArray{points[0].toObject()["id"]}}}));
        sketch = solved(SketchGeometry::addDimension(sketch,{{"kind","horizontal"},{"entityId",entities[0].toObject()["id"]},{"valueMm",40}}));
        const auto widthId = sketch["parameters"].toObject()["dimensions"].toArray().last().toObject()["id"];
        sketch = solved(SketchGeometry::addDimension(sketch,{{"kind","vertical"},{"entityId",entities[1].toObject()["id"]},{"valueMm",25}}));
        check(document.commit(SketchGeometry::replaceFeature(document.authored(),sketch),"Create sketch").isEmpty(),"authored profile source commits");
        const auto baseline = document.authored();
        const auto sketchId = sketch["id"].toString(), profileId = document.sketches()[0].profiles.at(0).profileId;
        const auto featureId = CadDocument::newId(), bodyId = CadDocument::newId();
        QJsonObject arguments{{"sketchId",sketchId},{"profileId",profileId},{"distanceMm",5},
            {"newFeatureId",featureId},{"newBodyId",bodyId},{"name","Rectangle extrusion"}};
        auto candidate = CadDocument::extrusionCandidate(document.authored(),arguments);
        const auto preview = CadDocument::evaluateCandidate(candidate);
        check(preview.ok,preview.error.toUtf8().constData());
        check(preview.evaluation->bodies.size() == 1 && preview.evaluation->sketches.size() == 1
            && preview.evaluation->featureOrder == std::vector<QString>{sketchId,featureId},"candidate evaluates prior sketch/profile before the extrusion");
        near(volume(*preview.evaluation),5000,"40 by 25 by 5 extrusion volume");
        check(preview.evaluation->bodies[0].summary["valid"].toBool()
            && preview.evaluation->bodies[0].summary["closed"].toBool(),"extrusion is a valid closed OCCT solid");
        check(document.authored() == baseline && document.bodies().empty() && document.history().count() == 1,"preview and cancel do not install geometry or journal entries");
        arguments["distanceMm"] = 8;
        const auto changedPreview = CadDocument::evaluateCandidate(CadDocument::extrusionCandidate(document.authored(),arguments));
        check(changedPreview.ok && changedPreview.evaluation->bodies[0].featureId == featureId
            && changedPreview.evaluation->bodies[0].bodyId == bodyId,"preview updates retain reserved feature/body identities");
        near(volume(*changedPreview.evaluation),8000,"distance preview evaluates the current candidate");

        auto features = candidate["features"].toArray();auto hiddenSketch = features[0].toObject();
        hiddenSketch["visible"] = false;features[0] = hiddenSketch;candidate["features"] = features;
        check(document.commit(candidate,"Extrude profile").isEmpty() && document.history().count() == 2,"body and explicit source visibility commit together once");
        check(!document.sketches()[0].visible && document.bodies()[0].bodyId == bodyId,"hidden source remains evaluated and stable");
        document.history().undo();
        check(document.authored() == baseline && document.bodies().empty() && document.sketches()[0].visible,"undo restores the source and removes its dependent solid");
        document.history().redo();near(document.bodies()[0].summary["volumeMm3"].toDouble(),5000,"redo restores dependent geometry");
        auto editedSketch = SketchGeometry::findFeature(document.authored(),sketchId);
        editedSketch = solved(SketchGeometry::updateDimension(editedSketch,{{"dimensionId",widthId},{"valueMm",50}}));
        check(document.commit(SketchGeometry::replaceFeature(document.authored(),editedSketch),"Widen sketch").isEmpty(),"upstream dimension edit recomputes the full chain");
        near(document.bodies()[0].summary["volumeMm3"].toDouble(),6250,"50 by 25 by 5 dependent volume");
        check(document.bodies()[0].featureId == featureId && document.bodies()[0].bodyId == bodyId
            && document.sketches()[0].profiles[0].profileId == profileId,"dimension recompute preserves sketch/profile/extrusion/body identities");
        document.history().undo();near(document.bodies()[0].summary["volumeMm3"].toDouble(),5000,"undo restores earlier profile and solid");
        document.history().redo();near(document.bodies()[0].summary["volumeMm3"].toDouble(),6250,"redo restores resized profile and solid");

        const auto stable = document.authored();const auto revision = document.revision();
        const auto journalCount = document.history().count(), journalIndex = document.history().index();
        const auto staleProfile = replaceExtrusionParameters(stable,{{"profileId",CadDocument::newId()}});
        check(!document.commit(staleProfile,"Stale profile").isEmpty()
            && document.lastEvaluationError()["code"] == "profile_unavailable"
            && document.lastEvaluationError()["featureId"] == featureId,"missing profile yields a structured feature error with no fallback");
        check(document.authored() == stable && document.revision() == revision
            && document.history().count() == journalCount && document.history().index() == journalIndex,"failed evaluation preserves the paired document and journal");
        near(document.bodies()[0].summary["volumeMm3"].toDouble(),6250,"failed candidate cannot install partial or stale geometry");
        auto reordered = stable;features = stable["features"].toArray();reordered["features"] = QJsonArray{features[1],features[0]};
        check(CadDocument::evaluateCandidate(reordered).diagnostics["code"] == "forward_dependency","forward sources are refused");
        auto missing = stable;missing["features"] = QJsonArray{features[1]};
        check(CadDocument::evaluateCandidate(missing).diagnostics["code"] == "missing_dependency","deleted sources are refused");
        check(CadDocument::evaluateCandidate(replaceExtrusionParameters(stable,{{"sketchId",featureId}})).diagnostics["code"] == "cyclic_dependency","self dependencies are refused");
        check(!CadDocument::evaluateCandidate(replaceExtrusionParameters(stable,{{"distanceMm",0}})).ok,"zero distance cannot create a sheet or stale body");

        auto openSketch = SketchGeometry::findFeature(stable,sketchId);auto parameters = openSketch["parameters"].toObject();
        auto lines = parameters["entities"].toArray();lines.removeLast();parameters["entities"] = lines;
        auto constraints = parameters["constraints"].toArray();constraints.removeAt(3);parameters["constraints"] = constraints;openSketch["parameters"] = parameters;
        const auto openCandidate = CadDocument::evaluateCandidate(SketchGeometry::replaceFeature(stable,openSketch));
        check(!openCandidate.ok && !openCandidate.evaluation && openCandidate.diagnostics["code"] == "profile_unavailable"
            && openCandidate.diagnostics["profileDiagnostics"].toObject()["code"] == "open_profile","a valid open source cannot feed the prior closed region to its dependent solid");

        const auto reversed = CadDocument::evaluateCandidate(CadDocument::extrusionCandidate(stable,
            {{"sketchId",sketchId},{"profileId",profileId},{"distanceMm",-5},{"featureId",featureId}}));
        check(reversed.ok,reversed.error.toUtf8().constData());
        near(volume(*reversed.evaluation),6250,"reversed direction preserves positive solid volume");
        const auto bounds = reversed.evaluation->bodies[0].summary["boundsMm"].toObject();
        near(bounds["min"].toArray()[2].toDouble(),-5,"negative distance follows reverse source normal");
        near(bounds["max"].toArray()[2].toDouble(),0,"one-side reverse extrusion retains its source plane");

        auto xz = stable;auto xzFeatures = xz["features"].toArray();
        auto xzSketch = xzFeatures[0].toObject();auto xzParameters = xzSketch["parameters"].toObject();
        xzParameters["plane"] = "XZ";xzSketch["parameters"] = xzParameters;xzFeatures[0] = xzSketch;xz["features"] = xzFeatures;
        const auto perpendicular = CadDocument::evaluateCandidate(xz);
        check(perpendicular.ok,perpendicular.error.toUtf8().constData());
        near(volume(*perpendicular.evaluation),6250,"XZ extrusion has the same area-times-distance volume");
        const auto xzBounds = perpendicular.evaluation->bodies[0].summary["boundsMm"].toObject();
        near(xzBounds["min"].toArray()[1].toDouble(),-5,"XZ extrusion follows its negative-Y normal");
        near(xzBounds["size"].toArray()[2].toDouble(),25,"XZ profile height remains on world Z");

        QTemporaryDir directory;check(directory.isValid(),"native storage fixture directory");
        const auto path = directory.path()+"/rectangle-extrusion.alloy";
        check(document.save(path).isEmpty(),"save authored sketch and profile reference");
        CadDocument reopened;check(reopened.open(path).isEmpty() && reopened.authored() == stable,"reopen retains authored dependency references and IDs");
        check(reopened.history().count() == 0 && reopened.bodies()[0].bodyId == bodyId
            && reopened.sketches()[0].profiles[0].profileId == profileId,"reopen rebuilds current geometry with a fresh editing journal");
        near(reopened.bodies()[0].summary["volumeMm3"].toDouble(),6250,"reopened dependent volume");
        check(reopened.commit(CadDocument::extrusionCandidate(reopened.authored(),
            {{"sketchId",sketchId},{"profileId",profileId},{"distanceMm",3},{"featureId",featureId}}),"Edit extrusion").isEmpty(),"retained extrusion remains editable");
        near(reopened.bodies()[0].summary["volumeMm3"].toDouble(),3750,"retained extrusion distance recomputes");
        pluralExtrusion();
        std::cout << "PASS: ordered profile extrusion, signed distance, stable identity, atomic failure and undo/reopen\n";
        return 0;
    } catch (const std::exception& error) {std::cerr << error.what() << '\n';return 1;}
}
