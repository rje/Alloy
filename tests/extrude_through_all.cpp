#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
#include <numbers>
#include <stdexcept>

namespace {
void check(bool ok,const QString& message){if(!ok)throw std::runtime_error(message.toStdString());}
void near(double actual,double expected){check(std::abs(actual-expected)<1e-5,QString("Expected %1, got %2").arg(expected).arg(actual));}
void commit(CadDocument& document,const QJsonObject& candidate){const auto error=document.commit(candidate,"Through All test");check(error.isEmpty(),error);}
QJsonObject primitive(const CadDocument& document,const QString& type,const QJsonObject& p){return {{"id",CadDocument::newId()},{"bodyId",CadDocument::newId()},{"ownerId",document.activeOwnerId()},{"type",type},{"name",type},{"visible",true},{"parameters",p}};}
QJsonObject add(CadDocument& document,const QJsonObject& feature){auto candidate=document.authored();auto features=candidate["features"].toArray();features.append(feature);candidate["features"]=features;return candidate;}
QJsonObject circleSketch(CadDocument& document){auto sketch=SketchGeometry::newFeature(document.activeOwnerId(),"XY");sketch=SketchGeometry::addCircle(sketch,{{"centerUMm",10},{"centerVMm",10},{"diameterMm",4}});commit(document,SketchGeometry::replaceFeature(document.authored(),sketch));return sketch;}
QJsonObject cutArgs(const QJsonObject& target,const QJsonObject& sketch,const CadDocument& document,bool flip=false){return {{"sketchId",sketch["id"]},{"profileId",document.sketches().back().profiles.front().profileId},{"extent","all"},{"flip",flip},{"operation","cut"},{"targetBodyIds",QJsonArray{target["bodyId"]}}};}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try {
    CadDocument document;auto box=primitive(document,"box",{{"widthMm",20},{"depthMm",20},{"heightMm",10},{"xMm",0},{"yMm",0},{"zMm",0}});commit(document,add(document,box));
    auto sketch=circleSketch(document);auto args=cutArgs(box,sketch,document);
    const auto candidate=CadDocument::extrusionCandidate(document.authored(),args);
    const auto preview=CadDocument::evaluateCandidate(candidate);check(preview.ok,preview.error);
    check(document.authored()["features"].toArray().size()==2,"Preview changed authored history");
    commit(document,candidate);const auto cut=document.authored()["features"].toArray().last().toObject();
    check(cut["parameters"].toObject()["extent"]=="all" && !cut["parameters"].toObject().contains("distanceMm"),"All persisted a numeric distance");
    near(document.bodies().front().summary["volumeMm3"].toDouble(),4000-40*std::numbers::pi);
    check(document.bodies().front().summary["distanceMm"].toDouble()>10,"All failed to pass target back face");
    auto changed=document.authored();auto features=changed["features"].toArray();auto taller=features[0].toObject();auto p=taller["parameters"].toObject();p["heightMm"]=20;taller["parameters"]=p;features[0]=taller;changed["features"]=features;commit(document,changed);
    near(document.bodies().front().summary["volumeMm3"].toDouble(),8000-80*std::numbers::pi);
    check(document.bodies().front().summary["distanceMm"].toDouble()>20,"All did not follow target thickness");
    const auto stable=document.authored();const auto revision=document.revision();args["featureId"]=cut["id"];args["flip"]=true;
    const auto bad=CadDocument::extrusionCandidate(stable,args);check(!document.commit(bad,"Invalid flip").isEmpty(),"Wrong-side cut accepted");
    check(document.authored()==stable && document.revision()==revision,"Failed recompute changed document");
    document.history().undo();near(document.bodies().front().summary["volumeMm3"].toDouble(),4000-40*std::numbers::pi);
    document.history().redo();check(document.authored()==stable,"Redo lost All intent");
    QTemporaryDir folder;check(document.save(folder.path()+"/through.alloy").isEmpty(),"Save failed");CadDocument reopened;
    check(reopened.open(folder.path()+"/through.alloy").isEmpty() && reopened.authored()==stable,"Reopen changed All intent");
    near(reopened.bodies().front().summary["volumeMm3"].toDouble(),8000-80*std::numbers::pi);
    CadDocument curved;auto cylinder=primitive(curved,"cylinder",{{"radiusMm",10},{"heightMm",10},{"xMm",10},{"yMm",10},{"zMm",0}});commit(curved,add(curved,cylinder));auto roundSketch=circleSketch(curved);
    commit(curved,CadDocument::extrusionCandidate(curved.authored(),cutArgs(cylinder,roundSketch,curved)));
    near(curved.bodies().front().summary["volumeMm3"].toDouble(),960*std::numbers::pi);
    check(curved.bodies().front().summary["distanceMm"].toDouble()>10,"Curved target bound did not reach through");
    CadDocument reverse;auto below=primitive(reverse,"box",{{"widthMm",20},{"depthMm",20},{"heightMm",10},{"xMm",0},{"yMm",0},{"zMm",-10}});commit(reverse,add(reverse,below));auto flippedSketch=circleSketch(reverse);
    commit(reverse,CadDocument::extrusionCandidate(reverse.authored(),cutArgs(below,flippedSketch,reverse,true)));
    near(reverse.bodies().front().summary["volumeMm3"].toDouble(),4000-40*std::numbers::pi);
    check(reverse.bodies().front().summary["distanceMm"].toDouble()<-10,"Flip did not cut through the reverse target");
    std::cout<<"PASS retained All, target resize, curved bounds, Flip, atomic direction, undo/reopen\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
