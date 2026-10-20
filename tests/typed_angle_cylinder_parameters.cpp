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
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
void near(double value,double expected){check(std::abs(value-expected)<1e-5,"wrong evaluated geometry");}
QJsonObject user(const CadDocument& d,const QString& name){for(const auto& v:d.authored()["userParameters"].toArray())if(v.toObject()["name"]==name)return v.toObject();return {};}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    CadDocument d;
    auto commit=[&](const QJsonObject& candidate){const auto error=d.commit(candidate,"Typed parameters");if(!error.isEmpty())throw std::runtime_error(error.toStdString());};
    commit(CadDocument::lengthParameterCandidate(d.authored(),{{"name","Opening"},{"value",30},{"units","deg"}}));
    commit(CadDocument::lengthParameterCandidate(d.authored(),{{"name","Height"},{"value",10},{"units","mm"}}));
    const QJsonValue aid=user(d,"Opening").value("id"),hid=user(d,"Height").value("id");
    check(!user(d,"Opening").contains("valueMm"),"angle must not masquerade as length");
    check(CadDocument::evaluatedLengthParameters(d.authored()).size()==1,"length-only helper leaked angle");
    auto candidate=d.authored();const auto cylinderId=CadDocument::newId(),bodyId=CadDocument::newId();
    candidate["features"]=QJsonArray{QJsonObject{{"id",cylinderId},{"bodyId",bodyId},{"ownerId",candidate["documentId"]},{"type","cylinder"},{"name","Stock"},{"visible",true},
        {"parameters",QJsonObject{{"radiusMm",10},{"heightMm",10},{"xMm",0},{"yMm",0},{"zMm",0},{"heightParameterId",hid}}}}};commit(candidate);
    auto sketch=SketchGeometry::newFeature(d.authored()["documentId"].toString(),"XY");
    sketch=SketchGeometry::addLine(sketch,{{"u1Mm",0},{"v1Mm",0},{"u2Mm",30},{"v2Mm",0}});
    const auto base=sketch["parameters"].toObject()["entities"].toArray()[0].toObject();
    sketch=SketchGeometry::addLine(sketch,{{"u1Mm",0},{"v1Mm",0},{"u2Mm",30*std::cos(std::numbers::pi/6)},{"v2Mm",15},{"startPointId",base["startPointId"]}});
    const auto side=sketch["parameters"].toObject()["entities"].toArray()[1].toObject();
    sketch=SketchGeometry::addLine(sketch,{{"u1Mm",30},{"v1Mm",0},{"u2Mm",30*std::cos(std::numbers::pi/6)},{"v2Mm",15},{"startPointId",base["endPointId"]},{"endPointId",side["endPointId"]}});
    auto solve=[&](const SketchGeometry::SolveResult& r){if(!r.ok)throw std::runtime_error(r.error.toStdString());sketch=r.feature;};
    solve(SketchGeometry::addConstraint(sketch,{{"type","fixed"},{"pointIds",QJsonArray{base["startPointId"]}}}));
    solve(SketchGeometry::addConstraint(sketch,{{"type","horizontal"},{"entityIds",QJsonArray{base["id"]}}}));
    for(const auto& e:{base,side})solve(SketchGeometry::addDimension(sketch,{{"kind","linear"},{"entityId",e["id"]},{"valueMm",30}}));
    solve(SketchGeometry::addDimension(sketch,{{"kind","angle"},{"entityIds",QJsonArray{base["id"],side["id"]}},{"raySigns",QJsonArray{1,1}},{"valueDeg",30},{"parameterId",aid}}));
    const auto sid=sketch["id"].toString();const QJsonValue did=sketch["parameters"].toObject()["dimensions"].toArray().last().toObject().value("id");
    commit(SketchGeometry::replaceFeature(d.authored(),sketch));
    commit(CadDocument::extrusionCandidate(d.authored(),{{"sketchId",sid},{"profileId",d.sketches()[0].profiles[0].profileId},{"distanceMm",30},{"operation","cut"},{"targetBodyIds",QJsonArray{bodyId}}}));
    auto volume=[&](double angle,double height){near(d.bodies()[0].summary["volumeMm3"].toDouble(),(std::numbers::pi-angle*std::numbers::pi/360)*100*height);};volume(30,10);
    commit(CadDocument::lengthParameterCandidate(d.authored(),{{"parameterId",aid},{"value",60},{"name","CutAngle"}}));volume(60,10);
    commit(CadDocument::lengthParameterCandidate(d.authored(),{{"parameterId",hid},{"value",15},{"name","StockHeight"}}));volume(60,15);
    const auto before=d.authored();const auto revision=d.revision();
    const QJsonObject angleTarget{{"featureId",sid},{"dimensionId",did}},heightTarget{{"featureId",cylinderId},{"field","heightMm"}};
    for(const auto& target:{angleTarget,heightTarget}){try{CadDocument::modelLengthCandidate(d.authored(),{{"target",target},{"expression",target==angleTarget ? "StockHeight" : "CutAngle"}});check(false,"type mismatch accepted");}catch(const std::invalid_argument&){} }
    check(d.authored()==before && d.revision()==revision,"mismatch published state");
    for(const auto& formula:{QString("CutAngle*2"),QString("StockHeight+CutAngle")})try{CadDocument::lengthParameterCandidate(d.authored(),{{"name","Bad"},{"expression",formula},{"units","mm"}});check(false,"length formula accepted angle");}catch(const std::invalid_argument&){}
    try{CadDocument::bindExtrudeDistanceExpression(d.authored(),"StockHeight+CutAngle","mm");check(false,"extrude formula accepted angle");}catch(const std::invalid_argument&){}
    candidate=d.authored();auto fs=candidate["features"].toArray();auto f=fs[0].toObject(),p=f["parameters"].toObject();p["heightParameterId"]=aid;f["parameters"]=p;fs[0]=f;candidate["features"]=fs;
    check(!d.commit(candidate,"Wrong type").isEmpty() && d.authored()==before && d.revision()==revision,"raw mismatched ID not atomic");
    d.history().undo();volume(60,10);d.history().redo();check(d.authored()==before,"history loses bindings");
    QTemporaryDir dir;const auto path=dir.path()+"/wedge.alloy";check(d.save(path).isEmpty(),"save failed");CadDocument reopened;check(reopened.open(path).isEmpty() && reopened.authored()==d.authored(),"reopen changed bindings");
    commit(CadDocument::modelLengthCandidate(d.authored(),{{"target",angleTarget},{"expression","60 deg"}}));
    commit(CadDocument::modelLengthCandidate(d.authored(),{{"target",heightTarget},{"expression","15 mm"}}));
    commit(CadDocument::lengthParameterCandidate(d.authored(),{{"parameterId",aid},{"value",45}}));
    commit(CadDocument::lengthParameterCandidate(d.authored(),{{"parameterId",hid},{"value",20}}));volume(60,15);
    std::cout<<"Typed angle/Cylinder Height core passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
