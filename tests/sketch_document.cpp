#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QJsonArray>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
void near(double actual, double expected, const char* message) { check(std::abs(actual - expected) < 1e-6, message); }
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    try {
        CadDocument document;
        const auto root = document.authored()["documentId"].toString();
        auto candidate = document.authored();
        const QJsonObject box{{"id", CadDocument::newId()}, {"bodyId", CadDocument::newId()},
            {"ownerId", root}, {"type", "box"}, {"name", "Box"}, {"visible", true},
            {"parameters", QJsonObject{{"widthMm", 20}, {"depthMm", 30}, {"heightMm", 10}, {"xMm", 0}, {"yMm", 0}, {"zMm", 0}}}};
        candidate["features"] = QJsonArray{box};
        check(document.commit(candidate, "Create box").isEmpty(), "existing primitive intent remains accepted");
        const auto baseline = document.authored();
        const auto baselineRevision = document.revision();
        auto xy = SketchGeometry::newFeature(root, "XY");
        check(SketchGeometry::evaluateFeature(xy).shape.IsNull(), "empty sketch has no invented geometry");
        xy = SketchGeometry::addRectangle(xy, {{"u1Mm", 0}, {"v1Mm", 0}, {"u2Mm", 40}, {"v2Mm", 25}});
        const auto xyParameters = xy["parameters"].toObject();
        check(xyParameters["points"].toArray().size() == 4 && xyParameters["entities"].toArray().size() == 4,
            "rectangle retains four points and four authored lines");
        check(xyParameters["constraints"].toArray().size() == 4, "rectangle authors four automatic horizontal/vertical constraints");
        check(document.authored() == baseline && document.revision() == baselineRevision && document.history().count() == 1,
            "creating or cancelling a disposable sketch draft does not edit the document");
        check(document.commit(SketchGeometry::replaceFeature(document.authored(), xy), "Finish sketch").isEmpty(), "finish sketch evaluates and commits");
        check(document.history().count() == 2 && document.bodies().size() == 1 && document.sketches().size() == 1,
            "finish uses one journal entry and keeps sketches separate from solids");
        auto bounds = document.sketches()[0].summary["boundsMm"].toObject()["size"].toArray();
        near(bounds[0].toDouble(), 40, "XY rectangle x bounds"); near(bounds[1].toDouble(), 25, "XY rectangle y bounds"); near(bounds[2].toDouble(), 0, "XY plane z bounds");
        const auto finishedXY = document.authored();
        document.history().undo();
        check(document.authored() == baseline && document.sketches().empty() && document.bodies().size() == 1, "undo removes authored and evaluated sketch together");
        document.history().redo();
        check(document.authored() == finishedXY && document.sketches()[0].featureId == xy["id"].toString(), "redo retains all sketch IDs");

        for (const auto& plane : {"XZ", "YZ"}) {
            auto sketch = SketchGeometry::newFeature(root, plane);
            sketch = SketchGeometry::addLine(sketch, {{"u1Mm", 2}, {"v1Mm", 3}, {"u2Mm", 12}, {"v2Mm", 8}});
            check(document.commit(SketchGeometry::replaceFeature(document.authored(), sketch), "Finish sketch").isEmpty(), "finish other origin plane");
            const auto& evaluated = document.sketches().back();
            const auto point = evaluated.summary["worldPoints"].toArray()[1].toObject()["positionMm"].toArray();
            near(point[0].toDouble(), QString(plane) == "XZ" ? 12 : 0, "origin-plane world x");
            near(point[1].toDouble(), QString(plane) == "YZ" ? 12 : 0, "origin-plane world y");
            near(point[2].toDouble(), 8, "origin-plane world z");
            check(!evaluated.shape.IsNull() && evaluated.summary["solverIntegrated"].toBool()
                && evaluated.summary["solver"].toObject()["dof"].toInt() > 0, "unconstrained edges report actual solver freedom");
        }
        check(document.state()["sketches"].toInt() == 3 && document.state()["bodies"].toInt() == 1,
            "inspection distinguishes sketch and body counts");
        const auto threePlanes = document.authored();
        const auto priorPoints = xyParameters["points"].toArray();
        auto edit = SketchGeometry::findFeature(document.authored(), xy["id"].toString());
        edit = SketchGeometry::addLine(edit, {{"u1Mm", 40}, {"v1Mm", 25}, {"u2Mm", 50}, {"v2Mm", 35}});
        check(edit["parameters"].toObject()["points"].toArray().size() == 5, "connected line reuses endpoint identity");
        check(document.commit(SketchGeometry::replaceFeature(document.authored(), edit), "Edit sketch").isEmpty(), "reentered sketch commits to same feature");
        check(document.sketches().size() == 3 && document.sketches()[0].featureId == xy["id"].toString(), "edit does not duplicate sketch");
        document.history().undo();
        check(document.authored() == threePlanes && SketchGeometry::findFeature(document.authored(), xy["id"].toString())["parameters"].toObject()["points"].toArray() == priorPoints,
            "edit undo restores previous point IDs and evaluated geometry");

        bool refused = false;
        try { SketchGeometry::addLine(xy, {{"u1Mm", 0}, {"v1Mm", 0}, {"u2Mm", 0}, {"v2Mm", 0}}); }
        catch (const std::invalid_argument&) { refused = true; }
        check(refused && document.authored() == threePlanes, "zero-length input leaves authored document unchanged");
        auto invalid = xy;
        auto parameters = invalid["parameters"].toObject();
        auto entities = parameters["entities"].toArray();
        auto line = entities[0].toObject(); line["startPointId"] = CadDocument::newId(); entities[0] = line;
        parameters["entities"] = entities; invalid["parameters"] = parameters;
        auto invalidCandidate = document.authored();
        auto features = invalidCandidate["features"].toArray(); features[1] = invalid; invalidCandidate["features"] = features;
        check(!document.commit(invalidCandidate, "Invalid endpoint").isEmpty() && document.authored() == threePlanes,
            "invalid authored endpoints cannot replace current document");

        QTemporaryDir directory;
        check(directory.isValid(), "temporary native storage directory");
        const auto path = directory.path() + "/sketches.alloy";
        check(document.save(path).isEmpty() && !document.state()["dirty"].toBool(), "save authored sketches atomically");
        CadDocument reopened;
        check(reopened.open(path).isEmpty() && reopened.authored() == document.authored(), "native reopen retains sketch IDs and intent");
        check(reopened.sketches().size() == 3 && reopened.bodies().size() == 1 && reopened.history().count() == 0,
            "reopen rebuilds evaluated sketches with a fresh session journal");
        near(reopened.bodies()[0].summary["volumeMm3"].toDouble(), 6000, "primitive volume survives mixed document reopen");
        auto constrained = SketchGeometry::findFeature(reopened.authored(), xy["id"].toString());
        const auto points = constrained["parameters"].toObject()["points"].toArray();
        const auto lines = constrained["parameters"].toObject()["entities"].toArray();
        auto accept = [&](const SketchGeometry::SolveResult& result) {
            check(result.ok, result.error.toUtf8().constData()); constrained = result.feature;
        };
        accept(SketchGeometry::addConstraint(constrained, {{"type", "fixed"}, {"pointIds", QJsonArray{points[0].toObject()["id"]}}}));
        check(SketchGeometry::solveDraft(constrained).diagnostics["dof"].toInt() == 2,
            "fixed rectangle retains only width and height freedom");
        accept(SketchGeometry::addDimension(constrained, {{"kind", "horizontal"}, {"entityId", lines[0].toObject()["id"]}, {"valueMm", 40}}));
        const auto widthId = constrained["parameters"].toObject()["dimensions"].toArray().last().toObject()["id"];
        accept(SketchGeometry::addDimension(constrained, {{"kind", "vertical"}, {"entityId", lines[1].toObject()["id"]}, {"valueMm", 25}}));
        const auto previous = reopened.authored();
        check(reopened.commit(SketchGeometry::replaceFeature(previous, constrained), "Constrain rectangle").isEmpty(), "constrained finish is authoritative");
        check(reopened.sketches()[0].summary["solver"].toObject()["dof"].toInt() == 0, "fixed dimensioned rectangle is fully constrained");
        accept(SketchGeometry::updateDimension(constrained, {{"dimensionId", widthId}, {"valueMm", 50}}));
        check(reopened.commit(SketchGeometry::replaceFeature(reopened.authored(), constrained), "Resize sketch").isEmpty(), "dimension changes solved authored geometry");
        const auto savedSolved = reopened.authored();
        near(reopened.sketches()[0].summary["boundsMm"].toObject()["size"].toArray()[0].toDouble(), 50, "solver width matches OCCT edges");
        const auto conflict = SketchGeometry::addDimension(constrained, {{"kind", "horizontal"}, {"entityId", lines[0].toObject()["id"]}, {"valueMm", 41}});
        check(!conflict.ok && conflict.feature == constrained && reopened.authored() == savedSolved,
            "conflicting dimension preserves last usable draft and live document");
        reopened.history().undo(); reopened.history().redo();
        check(reopened.authored() == savedSolved, "constraint dimension IDs and solved coordinates survive undo/redo");
        check(reopened.save(path).isEmpty() && document.open(path).isEmpty() && document.authored() == savedSolved,
            "native reopen rebuilds constraints without changing solved authored identity");
        std::cout << "PASS: origin-plane sketches, authored line identity, atomic history and mixed native reopen\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
