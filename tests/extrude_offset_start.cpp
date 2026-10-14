#include "CadDocument.h"
#include "SketchGeometry.h"
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
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    QTemporaryDir temporary;check(temporary.isValid(),"temporary directory");
    CadDocument offset;
    check(offset.commit(CadDocument::lengthParameterCandidate(offset.authored(),{{"name","StartGap"},{"value",7},{"units","mm"}}),"Start length").isEmpty(),"start length created");
    auto source=SketchGeometry::newFeature(offset.authored()["documentId"].toString(),"XY");
    source=SketchGeometry::addRectangle(source,{{"u1Mm",0},{"v1Mm",0},{"u2Mm",20},{"v2Mm",10}});
    const auto sourceId=source["id"].toString();
    check(offset.commit(SketchGeometry::replaceFeature(offset.authored(),source),"Source sketch").isEmpty(),"offset source sketch commits");
    const auto sourceProfile=offset.sketches().front().profiles.front().profileId;
    const QJsonValue startId=row(offset.authored(),"StartGap").value("id");
    auto shifted=CadDocument::extrusionCandidate(offset.authored(),{{"sketchId",sourceId},{"profileId",sourceProfile},{"distanceMm",10},
        {"start","offset"},{"startOffsetMm",7},{"startOffsetParameterId",startId}});
    check(offset.commit(shifted,"Offset extrusion").isEmpty(),"bound offset extrusion commits");
    const auto extrusionId=offset.authored()["features"].toArray().last().toObject()["id"].toString();
    auto zBounds=[&]{const auto bounds=offset.bodies().front().summary["boundsMm"].toObject();return bounds["min"].toArray()[2].toDouble();};
    near(zBounds(),7,"shifted extrusion starts at bound offset");
    near(offset.bodies().front().summary["boundsMm"].toObject()["max"].toArray()[2].toDouble(),17,"shifted end cap reaches offset plus distance");
    near(offset.sketches().front().summary["boundsMm"].toObject()["min"].toArray()[2].toDouble(),0,"authored sketch remains on its plane");
    bool shiftedCap=false;for(const auto& face:offset.bodies().front().supportFaces)if(face.role=="extrude.endCap"){
        near(face.frame["origin"].toArray()[2].toDouble(),17,"support frame follows shifted cap");shiftedCap=true;
    }check(shiftedCap,"shifted end cap has support lineage");
    check(offset.commit(CadDocument::lengthParameterCandidate(offset.authored(),{{"parameterId",startId},{"name","RenamedGap"},{"value",9}}),"Change start length").isEmpty(),"bound start recomputes");
    near(zBounds(),9,"renamed start offset recomputes geometry");
    check(offset.authored()["features"].toArray().last().toObject()["parameters"].toObject()["startOffsetParameterId"]==startId,"start binding retains stable ID");
    const auto offsetStable=offset.authored();const auto offsetRevision=offset.revision();
    auto missing=offsetStable;auto missingFeatures=missing["features"].toArray();auto missingFeature=missingFeatures.last().toObject();auto missingParameters=missingFeature["parameters"].toObject();
    missingParameters["startOffsetParameterId"]=CadDocument::newId();missingFeature["parameters"]=missingParameters;missingFeatures[missingFeatures.size()-1]=missingFeature;missing["features"]=missingFeatures;
    check(!offset.commit(missing,"Missing start binding").isEmpty() && offset.authored()==offsetStable && offset.revision()==offsetRevision,"missing start binding is atomic");
    offset.history().undo();offset.history().redo();check(offset.authored()==offsetStable,"offset binding survives undo and redo");
    const auto offsetPath=temporary.path()+"/offset.alloy";check(offset.save(offsetPath).isEmpty(),"save offset");
    CadDocument reopenedOffset;check(reopenedOffset.open(offsetPath).isEmpty() && reopenedOffset.authored()==offset.authored(),"reopen offset and stable binding");
    shifted=CadDocument::extrusionCandidate(reopenedOffset.authored(),{{"featureId",extrusionId},{"sketchId",sourceId},{"profileId",sourceProfile},
        {"distanceMm",5},{"start","offset"},{"startOffsetMm",-10}});
    check(reopenedOffset.commit(shifted,"Literal negative start").isEmpty(),"literal negative start replaces binding");
    near(reopenedOffset.bodies().front().summary["boundsMm"].toObject()["min"].toArray()[2].toDouble(),-10,"negative start stays independent of positive extent");
    near(reopenedOffset.bodies().front().summary["boundsMm"].toObject()["max"].toArray()[2].toDouble(),-5,"positive extent follows shifted start");
    check(!reopenedOffset.authored()["features"].toArray().last().toObject()["parameters"].toObject().contains("startOffsetParameterId"),"literal edit clears start binding");
    shifted=CadDocument::extrusionCandidate(reopenedOffset.authored(),{{"featureId",extrusionId},{"sketchId",sourceId},{"profileId",sourceProfile},
        {"distanceMm",-5},{"start","offset"},{"startOffsetMm",-10}});
    check(reopenedOffset.commit(shifted,"Reverse extent").isEmpty(),"negative extent commits");
    near(reopenedOffset.bodies().front().summary["boundsMm"].toObject()["min"].toArray()[2].toDouble(),-15,"distance sign does not flip start offset");
    near(reopenedOffset.bodies().front().summary["boundsMm"].toObject()["max"].toArray()[2].toDouble(),-10,"reverse extent ends at same shifted start");
    CadDocument front;
    auto frontSource=SketchGeometry::newFeature(front.authored()["documentId"].toString(),"XZ");
    frontSource=SketchGeometry::addRectangle(frontSource,{{"u1Mm",0},{"v1Mm",0},{"u2Mm",20},{"v2Mm",10}});
    const auto frontId=frontSource["id"].toString();
    check(front.commit(SketchGeometry::replaceFeature(front.authored(),frontSource),"Front source").isEmpty(),"front source commits");
    const auto frontProfile=front.sketches().front().profiles.front().profileId;
    near(front.sketches().front().profiles.front().summary["frame"].toObject()["normal"].toArray()[1].toDouble(),-1,"front sketch normal points negative Y");
    check(front.commit(CadDocument::extrusionCandidate(front.authored(),{{"sketchId",frontId},{"profileId",frontProfile},
        {"distanceMm",5},{"start","offset"},{"startOffsetMm",-10}}),"Front shifted extrusion").isEmpty(),"front offset extrusion commits");
    const auto frontBounds=front.bodies().front().summary["boundsMm"].toObject();
    near(frontBounds["min"].toArray()[1].toDouble(),5,"front offset tool near face is world Y5");
    near(frontBounds["max"].toArray()[1].toDouble(),10,"front offset tool far face is world Y10");
    near(front.sketches().front().summary["boundsMm"].toObject()["min"].toArray()[1].toDouble(),0,"front source remains on world Y0");
    EvaluatedSupportFace frontCap;for(const auto& face:front.bodies().front().supportFaces)if(face.role=="extrude.endCap")frontCap=face;
    check(!frontCap.faceId.isEmpty(),"front shifted end cap is retained");
    near(frontCap.frame["origin"].toArray()[1].toDouble(),5,"front cap frame is on actual shifted end");
    auto supported=SketchGeometry::newSupportedFeature(front.activeOwnerId(),{{"producerFeatureId",frontCap.producerFeatureId},
        {"bodyId",frontCap.bodyId},{"faceId",frontCap.faceId}},"Downstream cap sketch",GeometryKernel::supportEdgeDescriptors(frontCap));
    check(front.commit(SketchGeometry::replaceFeature(front.authored(),supported),"Cap sketch").isEmpty(),"downstream sketch attaches to shifted cap");
    near(front.sketches().back().resolvedFrame["origin"].toArray()[1].toDouble(),5,"downstream sketch resolves shifted support frame");
    std::cout<<"PASS: offset-start extrusion evaluates shifted geometry and stable bindings\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
