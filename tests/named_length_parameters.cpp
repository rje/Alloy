#include "CadDocument.h"
#include "SketchGeometry.h"
#include "SolidFillet.h"
#include <QCoreApplication>
#include <QJsonArray>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void near(double actual,double expected,const char* message){check(std::abs(actual-expected)<1e-5,message);}
QJsonObject row(const QJsonObject& document,const QString& name){for(const auto& value:document["userParameters"].toArray())if(value.toObject()["name"]==name)return value.toObject();return {};}
QJsonObject dimension(const QJsonObject& document,const QString& featureId,const QString& kind){
    const auto feature=SketchGeometry::findFeature(document,featureId);
    for(const auto& value:feature["parameters"].toObject()["dimensions"].toArray())if(value.toObject()["kind"]==kind)return value.toObject();
    return {};
}
}

int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    CadDocument document;
    for(const auto& entry:QVector<QPair<QString,double>>{{"Length",75},{"Width",50},{"Thickness",10}})
        check(document.commit(CadDocument::lengthParameterCandidate(document.authored(),{{"name",entry.first},{"value",entry.second},{"units","mm"}}),"Add length").isEmpty(),"create named length");
    const QJsonValue lengthId=row(document.authored(),"Length").value("id"),widthId=row(document.authored(),"Width").value("id"),thicknessId=row(document.authored(),"Thickness").value("id");
    check(document.commit(CadDocument::lengthParameterCandidate(document.authored(),{{"parameterId",widthId},{"units","in"},{"value",2}}),"Edit length units").isEmpty(),"inches accepted as explicit unit");
    near(row(document.authored(),"Width")["valueMm"].toDouble(),50.8,"inch value stored in canonical millimeters");
    check(document.commit(CadDocument::lengthParameterCandidate(document.authored(),{{"parameterId",widthId},{"units","mm"},{"value",50}}),"Restore length units").isEmpty(),"unit edit preserves stable ID");
    auto sketch=SketchGeometry::newFeature(document.authored()["documentId"].toString(),"XY");
    sketch=SketchGeometry::addRectangle(sketch,{{"u1Mm",-30},{"v1Mm",-20},{"u2Mm",30},{"v2Mm",20}});
    const auto entities=sketch["parameters"].toObject()["entities"].toArray();
    auto accept=[&](const SketchGeometry::SolveResult& result){check(result.ok,result.error.toUtf8().constData());sketch=result.feature;};
    const auto points=sketch["parameters"].toObject()["points"].toArray();
    accept(SketchGeometry::addConstraint(sketch,{{"type","fixed"},{"pointIds",QJsonArray{points[0].toObject()["id"]}}}));
    accept(SketchGeometry::addDimension(sketch,{{"kind","horizontal"},{"entityId",entities[0].toObject()["id"]},{"valueMm",75},{"parameterId",lengthId}}));
    accept(SketchGeometry::addDimension(sketch,{{"kind","vertical"},{"entityId",entities[1].toObject()["id"]},{"valueMm",50},{"parameterId",widthId}}));
    const auto sketchId=sketch["id"].toString();
    check(document.commit(SketchGeometry::replaceFeature(document.authored(),sketch),"Finish sketch").isEmpty(),"finish bound sketch");
    const auto profiles=document.sketches().front().profiles;
    check(profiles.size()==1,"rectangle has one region");
    auto extrusion=CadDocument::extrusionCandidate(document.authored(),{{"sketchId",sketchId},{"profileId",profiles.front().profileId},{"distanceMm",10},{"distanceParameterId",thicknessId}});
    check(document.commit(extrusion,"Extrude").isEmpty(),"finish bound extrude");
    near(document.bodies().front().summary["volumeMm3"].toDouble(),37500,"initial parameter volume");
    const auto baseline=document.authored();
    check(document.commit(CadDocument::lengthParameterCandidate(document.authored(),{{"parameterId",lengthId},{"value",85}}),"Edit length").isEmpty(),"update bound length");
    near(dimension(document.authored(),sketchId,"horizontal")["valueMm"].toDouble(),85,"dimension numeric cache recomputed");
    near(document.sketches().front().summary["boundsMm"].toObject()["size"].toArray()[0].toDouble(),85,"sketch rebuilt");
    near(document.bodies().front().summary["volumeMm3"].toDouble(),42500,"solid rebuilt");
    check(document.commit(CadDocument::lengthParameterCandidate(document.authored(),{{"parameterId",thicknessId},{"name","Depth"},{"value",12}}),"Rename and edit length").isEmpty(),"rename and edit bound distance");
    near(document.bodies().front().summary["volumeMm3"].toDouble(),51000,"distance rebuilt after rename");
    check(document.authored()["features"].toArray().last().toObject()["parameters"].toObject()["distanceParameterId"]==thicknessId,"extrude retains stable binding ID");
    const auto beforeFailure=document.authored();const auto revision=document.revision();
    try{CadDocument::lengthParameterCandidate(document.authored(),{{"parameterId",lengthId},{"name","Width"}});check(false,"duplicate name accepted");}catch(const std::invalid_argument&){}
    try{CadDocument::lengthParameterCandidate(document.authored(),{{"parameterId",lengthId},{"value",-1}});check(false,"invalid value accepted");}catch(const std::invalid_argument&){}
    auto broken=document.authored();auto features=broken["features"].toArray();auto feature=features.last().toObject(),parameters=feature["parameters"].toObject();parameters["distanceParameterId"]=CadDocument::newId();feature["parameters"]=parameters;features[features.size()-1]=feature;broken["features"]=features;
    check(!document.commit(broken,"Missing binding").isEmpty() && document.authored()==beforeFailure && document.revision()==revision,"missing binding is atomic");
    document.history().undo();check(document.authored()["features"].toArray().last().toObject()["parameters"].toObject()["distanceMm"].toDouble()==10,"undo restores old evaluated distance");
    document.history().redo();check(document.authored()==beforeFailure,"redo restores stable bindings");
    QTemporaryDir temporary;check(temporary.isValid(),"temporary directory");const auto path=temporary.path()+"/lengths.alloy";
    check(document.save(path).isEmpty(),"save bound document");CadDocument reopened;check(reopened.open(path).isEmpty() && reopened.authored()==document.authored(),"reopen retained table and IDs");
    near(reopened.bodies().front().summary["volumeMm3"].toDouble(),51000,"reopen regenerated bound solid");
    QJsonArray cornerIds;
    for(const auto& value:SolidFillet::descriptors(reopened.bodies().front().solidEdges)){
        const auto edge=value.toObject();if(edge["available"].toBool() && edge["provenance"].toObject()["role"]=="sideCorner"){
            cornerIds.append(edge["edgeId"]);break;
        }
    }
    check(cornerIds.size()==1,"retained corner available for dependent fillet");
    check(reopened.commit(CadDocument::filletCandidate(reopened.authored(),{{"targetBodyId",reopened.bodies().front().bodyId},{"edgeIds",cornerIds},{"radiusMm",4}}),"Corner fillet").isEmpty(),"dependent fillet commits");
    const auto beforeDownstreamFailure=reopened.authored();const auto downstreamRevision=reopened.revision();
    check(!reopened.commit(CadDocument::lengthParameterCandidate(reopened.authored(),{{"parameterId",widthId},{"value",2}}),"Impossible dependent fillet").isEmpty(),"downstream fillet must reject narrowed width");
    check(reopened.authored()==beforeDownstreamFailure && reopened.revision()==downstreamRevision,"downstream evaluation refusal is atomic");
    CadDocument box;
    for(const auto& entry:QVector<QPair<QString,double>>{{"BoxWidth",50},{"BoxLength",80},{"BoxHeight",40}})
        check(box.commit(CadDocument::lengthParameterCandidate(box.authored(),{{"name",entry.first},{"value",entry.second},{"units","mm"}}),"Box parameter").isEmpty(),"box parameter created");
    QJsonObject boxParameters{{"widthMm",50},{"depthMm",80},{"heightMm",40},{"xMm",0},{"yMm",0},{"zMm",0},
        {"widthParameterId",row(box.authored(),"BoxWidth")["id"]},{"depthParameterId",row(box.authored(),"BoxLength")["id"]},{"heightParameterId",row(box.authored(),"BoxHeight")["id"]}};
    auto authoredBox=box.authored();auto boxFeatures=authoredBox["features"].toArray();
    boxFeatures.append(QJsonObject{{"id",CadDocument::newId()},{"bodyId",CadDocument::newId()},{"ownerId",authoredBox["documentId"]},
        {"type","box"},{"name","Box"},{"visible",true},{"parameters",boxParameters}});
    authoredBox["features"]=boxFeatures;
    check(box.commit(authoredBox,"Bound Box").isEmpty(),"bound Box commits");
    near(box.bodies().front().summary["volumeMm3"].toDouble(),160000,"Box initial volume");
    const QJsonValue boxWidthId=row(box.authored(),"BoxWidth").value("id");
    check(box.commit(CadDocument::lengthParameterCandidate(box.authored(),{{"parameterId",boxWidthId},{"name","NewWidth"},{"value",60}}),"Resize Box").isEmpty(),"Box recomputes on rename and update");
    near(box.bodies().front().summary["volumeMm3"].toDouble(),192000,"Box updated volume");
    check(box.authored()["features"].toArray().last().toObject()["parameters"].toObject()["widthParameterId"]==boxWidthId,"Box retains stable ID");
    const auto boxStable=box.authored();const auto boxRevision=box.revision();
    auto brokenBox=box.authored();auto brokenFeatures=brokenBox["features"].toArray();auto brokenFeature=brokenFeatures.last().toObject();auto brokenParameters=brokenFeature["parameters"].toObject();
    brokenParameters["widthParameterId"]=CadDocument::newId();brokenFeature["parameters"]=brokenParameters;brokenFeatures[brokenFeatures.size()-1]=brokenFeature;brokenBox["features"]=brokenFeatures;
    check(!box.commit(brokenBox,"Missing Box binding").isEmpty() && box.authored()!=brokenBox,"missing Box binding rejected");
    box.history().undo();box.history().redo();
    check(box.authored()==boxStable && box.revision()>boxRevision,"Box undo/redo restores binding");
    const auto boxPath=temporary.path()+"/box-lengths.alloy";check(box.save(boxPath).isEmpty(),"save bound Box");
    CadDocument reopenedBox;check(reopenedBox.open(boxPath).isEmpty() && reopenedBox.authored()==box.authored(),"reopen bound Box");
    const QString boxLengthId=row(box.authored(),"BoxLength").value("id").toString();
    const QString spanId=row(box.authored(),"NewWidth").value("id").toString();
    check(box.commit(CadDocument::lengthParameterCandidate(box.authored(),{{"parameterId",boxLengthId},{"expression","NewWidth*2"}}),"Length expression").isEmpty(),"formula commits");
    check(!row(box.authored(),"BoxLength").contains("valueMm"),"formula result is not a second authored value");
    near(row(box.state(),"BoxLength")["valueMm"].toDouble(),120,"formula evaluates in shared state");
    near(box.bodies().front().summary["volumeMm3"].toDouble(),288000,"formula drives bound Box");
    const auto originalExpression=box.authored();const auto originalRevision=box.revision();
    for(const auto& expression:QStringList{"NewWidth+5","NewWidth*NewWidth","NewWidth/0","Missing*2","NewWidth/2 mm"}){
        try{CadDocument::lengthParameterCandidate(box.authored(),{{"parameterId",boxLengthId},{"expression",expression}});
            check(false,"invalid length expression accepted");}catch(const std::invalid_argument&){}
    }
    try{CadDocument::lengthParameterCandidate(box.authored(),{{"parameterId",boxLengthId},
        {"expression",QString(70,'(')+"NewWidth"+QString(70,')')}});
        check(false,"excessive expression depth accepted");}catch(const std::invalid_argument&){}
    check(box.authored()==originalExpression && box.revision()==originalRevision,"invalid formulas leave authored state unchanged");
    try{CadDocument::lengthParameterCandidate(box.authored(),{{"parameterId",spanId},{"expression","BoxLength/2"}});
        check(false,"cycle accepted");}catch(const std::invalid_argument&){}
    check(box.commit(CadDocument::lengthParameterCandidate(box.authored(),{{"parameterId",spanId},{"name","Span"},{"value",65}}),"Rename formula source").isEmpty(),"rename and source edit commit");
    check(row(box.authored(),"BoxLength")["expression"]=="Span*2"
        && row(box.authored(),"BoxLength")["referenceIds"].toArray().first()==spanId,"rename rewrites tokens but retains source UUID");
    near(row(box.state(),"BoxLength")["valueMm"].toDouble(),130,"rename retains evaluated ID dependency");
    near(box.bodies().front().summary["volumeMm3"].toDouble(),338000,"renamed source recomputes geometry");
    check(box.commit(CadDocument::lengthParameterCandidate(box.authored(),{{"parameterId",boxLengthId},{"expression","Span+Span"}}),"Repeated reference").isEmpty(),"repeated identifiers commit");
    check(row(box.authored(),"BoxLength")["referenceIds"].toArray()==QJsonArray{spanId,spanId},"each repeated occurrence retains an ID");
    near(row(box.state(),"BoxLength")["valueMm"].toDouble(),130,"repeated references evaluate");
    check(box.commit(CadDocument::lengthParameterCandidate(box.authored(),{{"parameterId",boxLengthId},{"expression","Span+5 mm"},{"units","in"}}),"Explicit mm expression").isEmpty(),"explicit length ignores row unit");
    near(row(box.state(),"BoxLength")["valueMm"].toDouble(),70,"explicit mm in inch row");
    box.history().undo();near(row(box.state(),"BoxLength")["valueMm"].toDouble(),130,"undo restores renamed formula");box.history().redo();
    const auto formulaPath=temporary.path()+"/formulas.alloy";check(box.save(formulaPath).isEmpty(),"save formula");
    CadDocument formulaReopened;check(formulaReopened.open(formulaPath).isEmpty(),"reopen formula");
    near(row(formulaReopened.state(),"BoxLength")["valueMm"].toDouble(),70,"reopened formula evaluated");
    std::cout<<"PASS: named lengths bind sketch and extrusion, rebuild atomically, undo and reopen\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
