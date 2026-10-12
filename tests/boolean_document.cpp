#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QJsonArray>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
#include <numbers>
#include <stdexcept>

namespace {
void check(bool value, const char* message) {if (!value) throw std::runtime_error(message);}
void near(double value, double expected, const char* message) {check(std::abs(value-expected) < 1e-6,message);}
QJsonObject primitive(const QJsonObject& document, const QString& type, QJsonObject parameters, bool visible = true) {
    return {{"id",CadDocument::newId()},{"bodyId",CadDocument::newId()},{"ownerId",document["documentId"]},
        {"type",type},{"name",type},{"visible",visible},{"parameters",parameters}};
}
QJsonObject boxParameters(double width = 20, double x = 0) {
    return {{"widthMm",width},{"depthMm",30},{"heightMm",5},{"xMm",x},{"yMm",0},{"zMm",0}};
}
QJsonObject append(const QJsonObject& document, const QJsonObject& feature) {
    auto candidate = document;auto features = candidate["features"].toArray();features.append(feature);candidate["features"] = features;return candidate;
}
QJsonObject combine(const QJsonObject& target, const QJsonObject& tool, const QString& operation, bool keepTools = false) {
    return {{"operation",operation},{"targetBodyId",target["bodyId"]},
        {"toolBodyIds",QJsonArray{tool["bodyId"]}},{"keepTools",keepTools}};
}
const EvaluatedBody& body(const CadDocument& document, const QJsonObject& producer) {
    for (const auto& item : document.bodies()) if (item.bodyId == producer["bodyId"].toString()) return item;
    throw std::runtime_error("Expected body missing");
}
QJsonObject editParameters(const QJsonObject& document, int index, const QJsonObject& changes) {
    auto candidate = document;auto features = candidate["features"].toArray();auto feature = features[index].toObject();
    auto parameters = feature["parameters"].toObject();
    for (auto it = changes.begin(); it != changes.end(); ++it) parameters[it.key()] = it.value();
    feature["parameters"] = parameters;features[index] = feature;candidate["features"] = features;return candidate;
}
void commit(CadDocument& document, const QJsonObject& candidate, const QString& label) {
    const auto error = document.commit(candidate,label);check(error.isEmpty(),(label+": "+error).toUtf8().constData());
}
}

int main(int argc, char** argv) {
    QCoreApplication app(argc,argv);
    try {
        CadDocument document;
        const auto plate = primitive(document.authored(),"box",boxParameters());
        const auto cutter = primitive(document.authored(),"cylinder",
            {{"radiusMm",3},{"heightMm",10},{"xMm",10},{"yMm",15},{"zMm",-1}},false);
        commit(document,append(append(document.authored(),plate),cutter),"Create plate and tool");
        const auto before = document.authored();const auto beforeShape = body(document,plate).shape;
        auto args = combine(plate,cutter,"cut");const auto booleanId = CadDocument::newId();args["newFeatureId"] = booleanId;
        const auto candidate = CadDocument::booleanCandidate(before,args);
        const auto preview = CadDocument::evaluateCandidate(candidate);
        check(preview.ok,preview.error.toUtf8().constData());
        check(document.authored() == before && document.bodies().size() == 2 && document.history().count() == 1,
            "Boolean preview is disposable and hidden tool inputs are explicit");
        near(GeometryKernel::summarize(beforeShape)["volumeMm3"].toDouble(),3000,"candidate leaves shared live input topology unchanged");
        near(preview.evaluation->bodies[0].summary["volumeMm3"].toDouble(),3000-45*std::numbers::pi,"radius-3 through-hole analytic volume");
        commit(document,candidate,"Cut through-hole");
        check(document.bodies().size() == 1 && body(document,plate).featureId == plate["id"].toString()
            && body(document,plate).summary["lastModifierFeatureId"] == booleanId && body(document,plate).visible,
            "tool consumption preserves target producer identity, visibility and modifier identity");
        check(body(document,plate).summary["valid"].toBool() && body(document,plate).summary["closed"].toBool()
            && body(document,plate).summary["topology"].toObject()["solids"] == 1,"cut produces one real valid closed solid");
        document.history().undo();check(document.authored() == before && document.bodies().size() == 2,"undo restores target and consumed tool");
        document.history().redo();near(body(document,plate).summary["volumeMm3"].toDouble(),3000-45*std::numbers::pi,"redo restores cut");
        commit(document,editParameters(document.authored(),0,{{"widthMm",25}}),"Widen plate");
        near(body(document,plate).summary["volumeMm3"].toDouble(),3750-45*std::numbers::pi,"upstream primitive edit recomputes the Boolean");
        document.history().undo();document.history().redo();

        auto edit = combine(plate,cutter,"join",true);edit["featureId"] = booleanId;
        commit(document,CadDocument::booleanCandidate(document.authored(),edit),"Edit Combine operation");
        near(body(document,plate).summary["volumeMm3"].toDouble(),3750+45*std::numbers::pi,"join includes only boss volume outside plate");
        check(document.bodies().size() == 2 && !body(document,cutter).visible,"Keep Tools retains tool identity and hidden presentation");
        const auto saved = document.authored();
        QTemporaryDir directory;check(directory.isValid(),"native storage fixture directory");
        const auto path = directory.path()+"/plate-combine.alloy";
        check(document.save(path).isEmpty(),"Boolean authored save");
        CadDocument reopened;check(reopened.open(path).isEmpty() && reopened.authored() == saved
            && reopened.history().count() == 0,"native reopen preserves intent/IDs and rebuilds with fresh undo history");
        near(body(reopened,plate).summary["volumeMm3"].toDouble(),3750+45*std::numbers::pi,"reopened Boolean geometry");
        edit["operation"] = "cut";edit["keepTools"] = false;
        commit(reopened,CadDocument::booleanCandidate(reopened.authored(),edit),"Edit reopened Combine");
        near(body(reopened,plate).summary["volumeMm3"].toDouble(),3750-45*std::numbers::pi,"retained Combine remains editable");

        const auto stable = reopened.authored();const auto revision = reopened.revision();
        const auto historyCount = reopened.history().count(), historyIndex = reopened.history().index();
        auto failed = CadDocument::booleanCandidate(stable,combine(plate,cutter,"join"));
        check(!reopened.commit(failed,"Use consumed tool").isEmpty()
            && reopened.lastEvaluationError()["code"] == "consumed_dependency","consumed tools cannot feed stale geometry to later features");
        auto distant = editParameters(stable,1,{{"xMm",100}});
        distant = editParameters(distant,2,{{"operation","intersect"}});
        check(!reopened.commit(distant,"Empty intersection").isEmpty()
            && reopened.lastEvaluationError()["code"] == "boolean_empty_result"
            && reopened.lastEvaluationError()["featureId"] == booleanId,"empty results carry feature/input diagnostics");
        distant = editParameters(distant,2,{{"operation","join"}});
        check(!reopened.commit(distant,"Disconnected join").isEmpty()
            && reopened.lastEvaluationError()["code"] == "boolean_multiple_solids","disconnected join is explicitly unsupported");
        check(reopened.authored() == stable && reopened.revision() == revision
            && reopened.history().count() == historyCount && reopened.history().index() == historyIndex,
            "failed candidate leaves paired state, revision and journal unchanged");
        near(body(reopened,plate).summary["volumeMm3"].toDouble(),3750-45*std::numbers::pi,"failed evaluation preserves live solid");
        auto features = stable["features"].toArray();auto reorder = stable;
        reorder["features"] = QJsonArray{features[0],features[2],features[1]};
        check(CadDocument::evaluateCandidate(reorder).diagnostics["code"] == "forward_dependency","forward body inputs are refused");
        check(CadDocument::evaluateCandidate(editParameters(stable,2,{{"targetBodyId",CadDocument::newId()}})).diagnostics["code"] == "missing_dependency",
            "deleted body inputs are refused");
        check(CadDocument::evaluateCandidate(editParameters(stable,2,{{"targetBodyId",booleanId}})).diagnostics["code"] == "cyclic_dependency",
            "self dependencies are refused");
        check(!CadDocument::evaluateCandidate(editParameters(stable,2,{{"toolBodyIds",QJsonArray{plate["bodyId"]}}})).ok,
            "target cannot also be its own tool");

        auto secondHoleParameters = cutter["parameters"].toObject();secondHoleParameters["xMm"] = 20;
        const auto secondHole = primitive(reopened.authored(),"cylinder",secondHoleParameters);
        commit(reopened,append(reopened.authored(),secondHole),"Create second tool");
        const auto secondCombineId = CadDocument::newId();auto secondCut = combine(plate,secondHole,"cut");
        secondCut["newFeatureId"] = secondCombineId;
        commit(reopened,CadDocument::booleanCandidate(reopened.authored(),secondCut),"Cut modified target again");
        near(body(reopened,plate).summary["volumeMm3"].toDouble(),3750-90*std::numbers::pi,"later Boolean uses current prior target result");
        edit["operation"] = "join";
        commit(reopened,CadDocument::booleanCandidate(reopened.authored(),edit),"Edit earlier Boolean");
        near(body(reopened,plate).summary["volumeMm3"].toDouble(),3750,"earlier Boolean operation edit recomputes the downstream Boolean");
        check(reopened.bodies().size() == 1 && body(reopened,plate).summary["lastModifierFeatureId"] == secondCombineId,
            "ordered Boolean chain retains original target ID and latest modifier");
        reopened.history().undo();
        near(body(reopened,plate).summary["volumeMm3"].toDouble(),3750-90*std::numbers::pi,"undo restores both evaluated Boolean stages");

        CadDocument intersection;
        auto first = primitive(intersection.authored(),"box",{{"widthMm",10},{"depthMm",10},{"heightMm",10},{"xMm",0},{"yMm",0},{"zMm",0}},false);
        const auto second = primitive(intersection.authored(),"box",{{"widthMm",10},{"depthMm",10},{"heightMm",10},{"xMm",5},{"yMm",0},{"zMm",0}});
        commit(intersection,append(append(intersection.authored(),first),second),"Create intersecting boxes");
        commit(intersection,CadDocument::booleanCandidate(intersection.authored(),combine(first,second,"intersect")),"Intersect boxes");
        near(body(intersection,first).summary["volumeMm3"].toDouble(),500,"intersecting box analytic volume");
        check(intersection.bodies().size() == 1 && !body(intersection,first).visible,"explicit hidden target keeps its visibility after intersection");
        intersection.history().undo();check(intersection.bodies().size() == 2,"intersection undo restores both inputs");

        CadDocument grouped;
        const auto target = primitive(grouped.authored(),"box",boxParameters());
        const auto left = primitive(grouped.authored(),"box",boxParameters(12));
        const auto right = primitive(grouped.authored(),"box",boxParameters(12,8));
        commit(grouped,append(append(append(grouped.authored(),target),left),right),"Create grouped inputs");
        auto groupedArgs = combine(target,left,"intersect");groupedArgs["toolBodyIds"] = QJsonArray{left["bodyId"],right["bodyId"]};
        commit(grouped,CadDocument::booleanCandidate(grouped.authored(),groupedArgs),"Intersect tool group");
        near(body(grouped,target).summary["volumeMm3"].toDouble(),3000,"multiple tools intersect as their union rather than sequential overlap");
        check(grouped.bodies().size() == 1,"all explicit tools are consumed once");

        // A finished profile extrusion is the same authoritative solid input as
        // a primitive; Boolean support does not replace New Body extrusion.
        CadDocument sketchDocument;
        const auto sketch = SketchGeometry::addRectangle(SketchGeometry::newFeature(sketchDocument.authored()["documentId"].toString(),"XY"),
            {{"u1Mm",0},{"v1Mm",0},{"u2Mm",20},{"v2Mm",30}});
        commit(sketchDocument,SketchGeometry::replaceFeature(sketchDocument.authored(),sketch),"Create profile");
        commit(sketchDocument,CadDocument::extrusionCandidate(sketchDocument.authored(),{{"sketchId",sketch["id"]},
            {"profileId",sketchDocument.sketches()[0].profiles[0].profileId},{"distanceMm",5}}),"Extrude new body");
        const auto extrusion = sketchDocument.authored()["features"].toArray()[1].toObject();
        const auto hole = primitive(sketchDocument.authored(),"cylinder",cutter["parameters"].toObject());
        commit(sketchDocument,append(sketchDocument.authored(),hole),"Create hole tool");
        commit(sketchDocument,CadDocument::booleanCandidate(sketchDocument.authored(),combine(extrusion,hole,"cut")),"Cut extrusion");
        near(body(sketchDocument,extrusion).summary["volumeMm3"].toDouble(),3000-45*std::numbers::pi,"profile extrusion feeds the same Boolean evaluation");
        std::cout << "PASS: Boolean analytic geometry, explicit identity/tools, ordered recompute, atomic failure and undo/reopen\n";
        return 0;
    } catch (const std::exception& error) {std::cerr << error.what() << '\n';return 1;}
}
