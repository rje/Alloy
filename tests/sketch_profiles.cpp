#include "SketchProfiles.h"
#include "SketchGeometry.h"
#include <BRepCheck_Analyzer.hxx>
#include <QCoreApplication>
#include <QJsonArray>
#include <QUuid>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool value, const char* message) {if (!value) throw std::runtime_error(message);}
void near(double value, double expected, const char* message) {check(std::abs(value-expected) < 1e-6,message);}
QJsonObject solved(const SketchGeometry::SolveResult& result) {
    check(result.ok,result.error.toUtf8().constData()); return result.feature;
}
}

int main(int argc, char** argv) {
    QCoreApplication app(argc,argv);
    try {
        const auto root = QUuid::createUuid().toString(QUuid::WithoutBraces);
        auto rectangle = SketchGeometry::addRectangle(SketchGeometry::newFeature(root,"XY"),
            {{"u1Mm",0},{"v1Mm",0},{"u2Mm",40},{"v2Mm",25}});
        const auto points = rectangle["parameters"].toObject()["points"].toArray();
        const auto entities = rectangle["parameters"].toObject()["entities"].toArray();
        rectangle = solved(SketchGeometry::addConstraint(rectangle,{{"type","fixed"},{"pointIds",QJsonArray{points[0].toObject()["id"]}}}));
        rectangle = solved(SketchGeometry::addDimension(rectangle,{{"kind","horizontal"},{"entityId",entities[0].toObject()["id"]},{"valueMm",40}}));
        const auto widthId = rectangle["parameters"].toObject()["dimensions"].toArray().last().toObject()["id"];
        rectangle = solved(SketchGeometry::addDimension(rectangle,{{"kind","vertical"},{"entityId",entities[1].toObject()["id"]},{"valueMm",25}}));
        const auto authored = rectangle;
        const auto profile = SketchProfiles::derive(rectangle);
        check(profile.ok,profile.error.toUtf8().constData());
        check(!profile.profile.face.IsNull() && BRepCheck_Analyzer(profile.profile.face).IsValid(),"profile is a real valid planar OCCT face");
        near(profile.profile.summary["areaMm2"].toDouble(),1000,"40 by 25 rectangle face area");
        near(profile.profile.summary["analyticAreaMm2"].toDouble(),1000,"independent line-boundary area");
        check(rectangle == authored,"derived closure does not mutate authored geometry");
        check(profile.profile.sketchId == rectangle["id"].toString()
            && profile.profile.summary["sourcePointIds"].toArray().size() == 4
            && profile.profile.summary["sourceEntityIds"].toArray().size() == 4
            && profile.profile.summary["edgeProvenance"].toArray().size() == 4,"profile retains sketch and boundary provenance");
        rectangle = solved(SketchGeometry::updateDimension(rectangle,{{"dimensionId",widthId},{"valueMm",50}}));
        const auto resized = SketchProfiles::derive(rectangle);
        check(resized.ok,resized.error.toUtf8().constData());
        near(resized.profile.summary["areaMm2"].toDouble(),1250,"driving width rebuilds the face area");
        check(resized.profile.profileId == profile.profile.profileId
            && resized.profile.summary["sourceEntityIds"] == profile.profile.summary["sourceEntityIds"],"dimension edits retain the derived profile identity and edge provenance");

        auto open = SketchGeometry::addLine(SketchGeometry::newFeature(root,"XY"),
            {{"u1Mm",0},{"v1Mm",0},{"u2Mm",40},{"v2Mm",0}});
        open = solved(SketchGeometry::solveDraft(open));
        const auto openResult = SketchProfiles::derive(open);
        check(!openResult.ok && openResult.profile.face.IsNull() && openResult.diagnostics["code"] == "open_profile"
            && !openResult.error.isEmpty(),"an open line remains a valid sketch with actionable profile refusal");

        auto crossed = SketchGeometry::newFeature(root,"XY");
        for (const auto& args : {QJsonObject{{"u1Mm",0},{"v1Mm",0},{"u2Mm",40},{"v2Mm",25}},
            QJsonObject{{"u1Mm",40},{"v1Mm",25},{"u2Mm",0},{"v2Mm",25}},
            QJsonObject{{"u1Mm",0},{"v1Mm",25},{"u2Mm",40},{"v2Mm",0}},
            QJsonObject{{"u1Mm",40},{"v1Mm",0},{"u2Mm",0},{"v2Mm",0}}}) crossed = SketchGeometry::addLine(crossed,args);
        crossed = solved(SketchGeometry::solveDraft(crossed));
        const auto crossedResult = SketchProfiles::derive(crossed);
        check(!crossedResult.ok && crossedResult.profile.face.IsNull()
            && crossedResult.diagnostics["code"] == "self_intersecting_profile"
            && crossedResult.diagnostics["entityIds"].toArray().size() == 2,"self-crossing boundary identifies the lines to repair");
        std::cout << "PASS: real closed line profile, analytic area, stable dimension-edit identity and open/crossed refusals\n";
        return 0;
    } catch (const std::exception& error) {std::cerr << error.what() << '\n';return 1;}
}
