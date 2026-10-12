// SPDX-License-Identifier: MIT
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
void check(bool ok,const QString& message){if(!ok)throw std::runtime_error(message.toStdString());}
QJsonObject accepted(const SketchGeometry::SolveResult& result){check(result.ok,result.error);return result.feature;}
void near(double actual,double expected){check(std::abs(actual-expected)<std::max(1e-5,std::abs(expected)*1e-9),"Cut volume mismatch");}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try {
    CadDocument document;const auto owner=document.authored()["documentId"].toString();
    auto base=SketchGeometry::newFeature(owner,"XY","Cut stock");
    base=accepted(SketchGeometry::solveDraft(SketchGeometry::addRectangle(base,{{"u1Mm",0},{"v1Mm",0},{"u2Mm",1000},{"v2Mm",300}})));
    check(document.commit(SketchGeometry::replaceFeature(document.authored(),base),"Stock sketch").isEmpty(),"Commit stock sketch");
    const auto baseProfile=document.sketches()[0].profiles[0].profileId;
    check(document.commit(CadDocument::extrusionCandidate(document.authored(),{{"sketchId",base["id"]},{"profileId",baseProfile},{"distanceMm",50}}),"Stock extrusion").isEmpty(),"Extrude stock");
    const auto producer=document.authored()["features"].toArray().last().toObject();
    const auto cap=document.bodies()[0].supportFaces[0];
    const QJsonObject support{{"producerFeatureId",producer["id"]},{"bodyId",producer["bodyId"]},{"faceId",cap.faceId}};
    auto holes=SketchGeometry::newSupportedFeature(owner,support,"Five equal holes",GeometryKernel::supportEdgeDescriptors(cap));
    const auto context=GeometryKernel::projectionContext(document.evaluation(),holes);QJsonArray circleIds;
    for(int i=0;i<5;++i){holes=SketchGeometry::addCircle(holes,{{"centerUMm",100+200*i},{"centerVMm",150},{"diameterMm",50}});
        const auto circle=holes["parameters"].toObject()["entities"].toArray().last().toObject();circleIds.append(circle["id"]);
        holes=accepted(SketchGeometry::addConstraint(holes,{{"type","fixed"},{"pointIds",QJsonArray{circle["centerPointId"]}}},context));
        if(i)holes=accepted(SketchGeometry::addConstraint(holes,{{"type","equal"},{"entityIds",QJsonArray{circleIds[i-1],circleIds[i]}}},context));}
    holes=accepted(SketchGeometry::addDimension(holes,{{"kind","diameter"},{"entityId",circleIds[0]},{"valueMm",50}},context));
    const auto diameterId=holes["parameters"].toObject()["dimensions"].toArray()[0].toObject()["id"];
    check(document.commit(SketchGeometry::replaceFeature(document.authored(),holes),"Hole sketch").isEmpty(),"Commit five hole profiles");
    QJsonArray profiles;for(const auto& profile:document.sketches()[1].profiles)profiles.append(profile.profileId);
    check(profiles.size()==5,"Expected five disconnected hole regions");const auto beforeCut=document.authored();const int beforeCount=document.history().count();
    auto cutArgs=QJsonObject{{"sketchId",holes["id"]},{"profileIds",profiles},{"distanceMm",-50},{"operation","cut"},{"targetBodyIds",QJsonArray{producer["bodyId"]}}};
    check(document.commit(CadDocument::extrusionCandidate(beforeCut,cutArgs),"Five through holes").isEmpty(),"Commit five-profile Cut");
    const auto cutFeature=document.authored()["features"].toArray().last().toObject();const auto cutId=cutFeature["id"];
    check(!cutFeature.contains("bodyId") && document.authored()["features"].toArray().size()==4 && document.bodies().size()==1,"Cut authored cutter features or a new body ID");
    check(document.history().count()==beforeCount+1,"Cut was not one history transaction");
    auto inspect=[&](double diameter,double depth,double cutDepth){const auto& body=document.bodies()[0];
        check(body.bodyId==producer["bodyId"].toString() && body.featureId==producer["id"].toString(),"Cut changed producer/body identity");
        check(body.summary["lastModifierFeatureId"]==cutId && body.supportFaces.empty(),"Cut result retained unsupported old face naming");
        check(body.summary["valid"].toBool() && body.summary["closed"].toBool() && body.summary["topology"].toObject()["solids"].toInt()==1,"Cut did not produce one valid closed solid");
        near(body.summary["volumeMm3"].toDouble(),1000*300*depth-5*std::numbers::pi*diameter*diameter/4*cutDepth);
        near(body.summary["removedVolumeMm3"].toDouble(),5*std::numbers::pi*diameter*diameter/4*cutDepth);
    };
    inspect(50,50,50);
    const auto earlier=document.evaluationBefore(holes["id"].toString());check(earlier.bodies.size()==1 && earlier.bodies[0].supportFaces.size()==1,"Earlier sketch edit lost its pre-Cut support");
    holes=accepted(SketchGeometry::updateDimension(holes,{{"dimensionId",diameterId},{"valueMm",60}},GeometryKernel::projectionContext(earlier,holes)));
    check(document.commit(SketchGeometry::replaceFeature(document.authored(),holes),"Resize holes").isEmpty(),"Upstream diameter edit failed after Cut");inspect(60,50,50);
    check(document.commit(CadDocument::extrusionCandidate(document.authored(),{{"featureId",producer["id"]},{"sketchId",base["id"]},{"profileId",baseProfile},{"distanceMm",60}}),"Thicken stock").isEmpty(),"Parent depth edit failed");inspect(60,60,50);
    cutArgs["featureId"]=cutId;cutArgs["distanceMm"]=-60;
    check(document.commit(CadDocument::extrusionCandidate(document.authored(),cutArgs),"Extend through cut").isEmpty(),"Edit retained Cut failed");inspect(60,60,60);
    const auto stable=document.authored();const auto stableRevision=document.revision();const auto stableCount=document.history().count();
    auto missing=cutArgs;missing["targetBodyIds"]=QJsonArray{CadDocument::newId()};check(!document.commit(CadDocument::extrusionCandidate(stable,missing),"Missing target").isEmpty(),"Missing Cut target accepted");
    auto away=cutArgs;away["distanceMm"]=60;check(!document.commit(CadDocument::extrusionCandidate(stable,away),"No overlap").isEmpty() && document.lastEvaluationError()["code"]=="cut_no_intersection","Nonintersecting Cut was accepted");
    check(!document.commit(CadDocument::extrusionCandidate(stable,{{"sketchId",holes["id"]},{"profileIds",profiles},{"distanceMm",10}}),"Disconnected New Body").isEmpty(),"Cut relaxed New Body connectivity validation");
    bool refused=false;try {auto switchArgs=cutArgs;switchArgs["operation"]="newBody";switchArgs.remove("targetBodyIds");CadDocument::extrusionCandidate(stable,switchArgs);}catch(const std::invalid_argument&){refused=true;}check(refused,"Cut/New Body edit silently changed ownership");
    check(document.authored()==stable && document.revision()==stableRevision && document.history().count()==stableCount,"Rejected Cut changed accepted geometry/history");
    document.history().undo();inspect(60,60,50);document.history().redo();check(document.authored()==stable,"Cut edit redo lost retained references");
    for(int i=0;i<4;++i)document.history().undo();check(document.authored()==beforeCut,"Undo Cut chain did not restore uncut stock");for(int i=0;i<4;++i)document.history().redo();check(document.authored()==stable,"Redo Cut chain changed authored IDs");
    QTemporaryDir directory;const auto path=directory.path()+"/five-hole-cut.alloy";check(document.save(path).isEmpty(),"Save Cut");CadDocument reopened;check(reopened.open(path).isEmpty() && reopened.authored()==stable && reopened.bodies().size()==1,"Reopen lost Cut intent");near(reopened.bodies()[0].summary["volumeMm3"].toDouble(),document.bodies()[0].summary["volumeMm3"].toDouble());
    std::cout<<"PASS: five disconnected profiles, one retained Cut target, diameter/depth edit, prefix support, atomic rejection, history and storage\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
