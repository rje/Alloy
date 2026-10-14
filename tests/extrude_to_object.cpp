#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QFile>
#include <QJsonDocument>
#include <cmath>
#include <iostream>
#include <stdexcept>
namespace {
void check(bool ok,const QString& error){if(!ok)throw std::runtime_error(error.toStdString());}
void commit(CadDocument& d,const QJsonObject& next){const auto error=d.commit(next,"Test edit");check(error.isEmpty(),error);}
void near(double a,double b){check(std::abs(a-b)<1e-5,QString("Expected %1, got %2").arg(b).arg(a));}
QJsonObject ref(const EvaluatedSupportFace& f){return {{"producerFeatureId",f.producerFeatureId},{"bodyId",f.bodyId},{"faceId",f.faceId}};}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    CadDocument d;
    commit(d,CadDocument::componentCandidate(d.authored(),"Box"));
    const auto boxOwner=d.authored()["components"].toArray().first().toObject()["id"].toString();
    check(d.activateOwner(boxOwner).isEmpty(),"activate Box");
    auto box=QJsonObject{{"id",CadDocument::newId()},{"bodyId",CadDocument::newId()},{"ownerId",d.activeOwnerId()},
        {"type","box"},{"name","Box"},{"visible",true},{"parameters",QJsonObject{{"widthMm",50},{"depthMm",80},{"heightMm",40},{"xMm",0},{"yMm",0},{"zMm",0}}}};
    auto next=d.authored();next["features"]=QJsonArray{box};commit(d,next);
    QJsonObject top;for(const auto& face:d.bodies().front().supportFaces)if(face.role=="box.z1")top=ref(face);
    commit(d,CadDocument::shellCandidate(d.authored(),{{"ownerId",boxOwner},{"targetBodyId",box["bodyId"]},{"removedFace",top},{"thicknessMm",4}}));
    QJsonObject back;for(const auto& face:d.bodies().front().supportFaces)if(face.role=="shell-inner" && face.provenance["boxFace"]=="y1")back=ref(face);
    check(!back.isEmpty(),"Shell inner back reference");
    const auto shelled=d.authored();
    commit(d,CadDocument::componentCandidate(d.authored(),"Lid"));
    const auto lidOwner=d.authored()["components"].toArray().last().toObject()["id"].toString();
    check(d.activateOwner(lidOwner).isEmpty(),"activate Lid");
    auto sketch=SketchGeometry::newFeature(lidOwner,"XZ");
    sketch=SketchGeometry::addRectangle(sketch,{{"u1Mm",5},{"v1Mm",38},{"u2Mm",45},{"v2Mm",42}});
    commit(d,SketchGeometry::replaceFeature(d.authored(),sketch));
    const auto profile=d.sketches().back().profiles.front().profileId;
    commit(d,CadDocument::lengthParameterCandidate(d.authored(),{{"name","Clearance"},{"value",0.25},{"units","mm"}}));
    const auto clearance=d.authored()["userParameters"].toArray().first().toObject()["id"].toString();
    QJsonObject args{{"ownerId",lidOwner},{"sketchId",sketch["id"]},{"profileId",profile},{"extent","toObject"},{"endSupport",back},{"endOffsetMm",0.25},{"endOffsetParameterId",clearance}};
    commit(d,CadDocument::extrusionCandidate(d.authored(),args));
    const auto feature=d.authored()["features"].toArray().last().toObject();
    check(!feature["parameters"].toObject().contains("distanceMm"),"To Object persisted a driving distance");
    near(d.bodies().back().summary["boundsMm"].toObject()["max"].toArray()[1].toDouble(),75.75);
    near(d.bodies().back().summary["distanceMm"].toDouble(),-75.75);
    check(d.bodies().back().summary["valid"].toBool(),"valid new body");
    bool endCap=false;for(const auto& f:d.bodies().back().supportFaces)if(f.role=="extrude.endCap") {endCap=true;near(f.frame["origin"].toArray()[1].toDouble(),75.75);}
    check(endCap,"negative-direction retained end cap");
    next=d.authored();auto features=next["features"].toArray();auto editedBox=features[0].toObject();auto p=editedBox["parameters"].toObject();p["depthMm"]=90;editedBox["parameters"]=p;features[0]=editedBox;next["features"]=features;commit(d,next);
    near(d.bodies().back().summary["boundsMm"].toObject()["max"].toArray()[1].toDouble(),85.75);
    commit(d,CadDocument::lengthParameterCandidate(d.authored(),{{"parameterId",clearance},{"name","Gap"},{"value",0.5}}));
    near(d.bodies().back().summary["boundsMm"].toObject()["max"].toArray()[1].toDouble(),85.5);
    const auto stable=d.authored();const auto rev=d.revision();
    auto invalid=stable;features=invalid["features"].toArray();auto bad=features.last().toObject();p=bad["parameters"].toObject();auto missing=back;missing["faceId"]=CadDocument::newId();p["endSupport"]=missing;bad["parameters"]=p;features[features.size()-1]=bad;invalid["features"]=features;
    check(!d.commit(invalid,"Missing target").isEmpty() && d.authored()==stable && d.revision()==rev,"target failure not atomic");
    d.history().undo();near(d.bodies().back().summary["boundsMm"].toObject()["max"].toArray()[1].toDouble(),85.75);d.history().redo();check(d.authored()==stable,"redo target intent");
    QTemporaryDir tmp;check(d.save(tmp.path()+"/extent.alloy").isEmpty(),"save");CadDocument reopened;check(reopened.open(tmp.path()+"/extent.alloy").isEmpty() && reopened.authored()==stable,"reopen");
    near(reopened.bodies().back().summary["distanceMm"].toDouble(),-85.5);
    args["featureId"]=feature["id"];args["start"]="offset";args["startOffsetMm"]=-10;
    commit(reopened,CadDocument::extrusionCandidate(reopened.authored(),args));
    near(reopened.bodies().back().summary["boundsMm"].toObject()["min"].toArray()[1].toDouble(),10);
    near(reopened.bodies().back().summary["boundsMm"].toObject()["max"].toArray()[1].toDouble(),85.5);
    args.remove("endOffsetParameterId");args["endOffsetMm"]=-0.5;
    commit(reopened,CadDocument::extrusionCandidate(reopened.authored(),args));
    near(reopened.bodies().back().summary["boundsMm"].toObject()["max"].toArray()[1].toDouble(),86.5);
    args["endOffsetMm"]=100;
    const auto prior=reopened.authored();check(!reopened.commit(CadDocument::extrusionCandidate(prior,args),"Crossing offset").isEmpty() && reopened.authored()==prior,"crossed extent refused atomically");
    QFile sourceFile(tmp.path()+"/shell.alloy");check(sourceFile.open(QIODevice::WriteOnly),"write shell fixture");sourceFile.write(QJsonDocument(shelled).toJson());sourceFile.close();
    CadDocument cutter;check(cutter.open(sourceFile.fileName()).isEmpty(),"open shell fixture");
    auto cutSketch=SketchGeometry::newFeature(boxOwner,"XZ");cutSketch=SketchGeometry::addRectangle(cutSketch,{{"u1Mm",10},{"v1Mm",38},{"u2Mm",40},{"v2Mm",42}});
    commit(cutter,SketchGeometry::replaceFeature(cutter.authored(),cutSketch));
    const auto cutProfile=cutter.sketches().back().profiles.front().profileId;
    commit(cutter,CadDocument::extrusionCandidate(cutter.authored(),{{"ownerId",boxOwner},{"sketchId",cutSketch["id"]},{"profileId",cutProfile},{"extent","toObject"},{"endSupport",back},{"operation","cut"},{"targetBodyIds",QJsonArray{box["bodyId"]}}}));
    near(cutter.bodies().front().summary["volumeMm3"].toDouble(),51136-240);
    near(cutter.bodies().front().summary["distanceMm"].toDouble(),-76);
    std::cout<<"PASS retained sibling target, Cut, clearance, start, source edits and history\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
