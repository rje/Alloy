// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool yes,const char* message){if(!yes)throw std::runtime_error(message);}
bool close(double a,double b){return std::abs(a-b)<1e-7;}
QJsonObject support(const EvaluatedSupportFace& face){return {{"producerFeatureId",face.producerFeatureId},{"bodyId",face.bodyId},{"faceId",face.faceId}};}
QJsonArray origin(const EvaluatedSketch& sketch){return sketch.resolvedFrame["origin"].toArray();}
QJsonObject replaceBox(const QJsonObject& authored,const QString& id,double x,double y){
    auto result=authored;auto features=result["features"].toArray();
    for(int i=0;i<features.size();++i)if(features[i].toObject()["id"]==id){auto box=features[i].toObject(),p=box["parameters"].toObject();p["xMm"]=x;p["yMm"]=y;box["parameters"]=p;features[i]=box;break;}
    result["features"]=features;return result;
}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    CadDocument d;const auto root=d.authored()["documentId"].toString();
    const auto boxId=CadDocument::newId(),bodyId=CadDocument::newId();
    auto authored=d.authored();auto features=authored["features"].toArray();
    features.append(QJsonObject{{"id",boxId},{"bodyId",bodyId},{"ownerId",root},{"type","box"},{"name","Offset case"},{"visible",true},
        {"parameters",QJsonObject{{"widthMm",64},{"depthMm",93},{"heightMm",30},{"xMm",-32},{"yMm",-46.5},{"zMm",0}}}});
    authored["features"]=features;check(d.commit(authored,"Offset case").isEmpty(),"Create offset support");
    const auto& body=d.bodies().front();
    const EvaluatedSupportFace* face=nullptr;
    for(const auto& f:body.supportFaces)if(f.role=="box.y0")face=&f;
    check(face,"Find actual offset planar face");
    const auto ref=support(*face);const auto oldFaceOrigin=face->frame["origin"].toArray();
    check(close(oldFaceOrigin[0].toDouble(),-32) && close(oldFaceOrigin[1].toDouble(),-46.5),"Fixture needs displaced face origin");

    auto fresh=SketchGeometry::newSupportedFeature(root,ref,"Projected owner origin",GeometryKernel::supportEdgeDescriptors(*face));
    auto p=fresh["parameters"].toObject();p["originConvention"]="projectedOwnerV1";fresh["parameters"]=p;
    const auto frame=GeometryKernel::sketchFrame(d.evaluation(),fresh);const auto o=frame["origin"].toArray();
    check(close(o[0].toDouble(),0) && close(o[1].toDouble(),-46.5) && close(o[2].toDouble(),0),"Projected origin must lie on actual support plane");
    const auto projected=GeometryKernel::projectionContext(d.evaluation(),fresh);
    check(!projected["lines"].toArray().isEmpty(),"Support Project must resolve in new sketch frame");
    const auto circle=SketchGeometry::addPointerCircle(fresh,{{"centerUMm",32},{"centerVMm",0},{"diameterMm",4},
        {"centerSnapTarget",QJsonObject{{"kind","origin"}}}},projected);
    check(circle.ok,circle.error.toUtf8().constData());fresh=circle.feature;
    check(d.commit(SketchGeometry::replaceFeature(d.authored(),fresh),"Fresh face sketch").isEmpty(),"Commit V1 face sketch");
    check(origin(d.sketches().front())==o,"History used a different frame than creation");
    bool snapped=false;for(const auto& value:d.sketches().front().summary["worldPoints"].toArray()){
        const auto point=value.toObject()["positionMm"].toArray();
        if(close(point[0].toDouble(),0)&&close(point[1].toDouble(),-46.5)&&close(point[2].toDouble(),0))snapped=true;
    }
    check(snapped,"Origin snap did not place a real sketch point on projected component origin");
    const auto freshId=fresh["id"].toString();
    check(d.commit(replaceBox(d.authored(),boxId,-37,-50),"Move support face").isEmpty(),"Upstream face edit");
    const auto moved=origin(d.sketches().front());
    check(close(moved[0].toDouble(),0)&&close(moved[1].toDouble(),-50),"Projected origin failed to follow support-plane displacement");
    check(SketchGeometry::findFeature(d.authored(),freshId)==fresh,"Upstream edit rewrote authored sketch coordinates");

    auto legacy=SketchGeometry::newSupportedFeature(root,ref,"Legacy face coordinates");
    legacy=SketchGeometry::addCircle(legacy,{{"centerUMm",37},{"centerVMm",0},{"diameterMm",4}});
    check(d.commit(SketchGeometry::replaceFeature(d.authored(),legacy),"Legacy sketch").isEmpty(),"Commit legacy face sketch");
    check(!SketchGeometry::findFeature(d.authored(),legacy["id"].toString())["parameters"].toObject().contains("originConvention"),"Legacy sketch acquired V1 marker");
    check(close(origin(d.sketches().back())[0].toDouble(),-37),"Legacy face origin silently rebased");
    QTemporaryDir dir;check(dir.isValid(),"Temporary save directory");const auto path=dir.path()+"/face-origin.alloy";
    const auto snapshot=d.authored();check(d.save(path).isEmpty(),"Save face sketches");CadDocument reopened;
    check(reopened.open(path).isEmpty()&&reopened.authored()==snapshot,"Reopen changed versioned or legacy sketch intent");
    check(origin(reopened.sketches().front())==moved&&close(origin(reopened.sketches().back())[0].toDouble(),-37),"Reopen changed sketch frames");
    CadDocument caseDocument;
    check(caseDocument.commit(CadDocument::componentCandidate(caseDocument.authored(),"Case"),"Create Case").isEmpty(),"Create Case owner");
    const auto caseOwner=caseDocument.authored()["components"].toArray().last().toObject()["id"].toString();
    auto caseAuthored=caseDocument.authored();auto caseFeatures=caseAuthored["features"].toArray();
    auto caseBox=features.first().toObject();caseBox["id"]=CadDocument::newId();caseBox["bodyId"]=CadDocument::newId();caseBox["ownerId"]=caseOwner;
    caseFeatures.append(caseBox);caseAuthored["features"]=caseFeatures;
    check(caseDocument.commit(caseAuthored,"Case stock").isEmpty(),"Create native Case-owned support");
    const auto caseFace=std::find_if(caseDocument.bodies().front().supportFaces.begin(),caseDocument.bodies().front().supportFaces.end(),
        [](const auto& value){return value.role=="box.y0";});
    check(caseFace!=caseDocument.bodies().front().supportFaces.end(),"Find Case-owned face");
    auto caseSketch=SketchGeometry::newSupportedFeature(caseOwner,support(*caseFace),"Case-owned origin");
    auto caseParameters=caseSketch["parameters"].toObject();caseParameters["originConvention"]="projectedOwnerV1";caseSketch["parameters"]=caseParameters;
    caseSketch=SketchGeometry::addCircle(caseSketch,{{"centerUMm",0},{"centerVMm",0},{"diameterMm",3}});
    check(caseDocument.commit(SketchGeometry::replaceFeature(caseDocument.authored(),caseSketch),"Case-owned face sketch").isEmpty(),"Identity Case owner should project its origin onto its own face");
    check(close(origin(caseDocument.sketches().back())[0].toDouble(),0)&&close(origin(caseDocument.sketches().back())[1].toDouble(),-46.5),
        "Case-owned origin projection differs from root-owned identity context");
    const auto converted=CadDocument::componentsFromBodiesCandidate(d.authored(),d.evaluation(),QJsonArray{bodyId});
    check(d.commit(converted,"Organize Case").isEmpty(),"Convert source body to component");
    const auto original=converted["occurrences"].toArray().last().toObject();
    const QJsonObject placement{{"axis","Z"},{"angleDeg",0},{"translationMm",QJsonArray{100,0,0}}};
    const auto copied=CadDocument::copyOccurrenceCandidate(d.authored(),original["id"].toString(),placement)["authored"].toObject();
    check(d.commit(copied,"Copy Case").isEmpty(),"Later copy must not invalidate existing V1 sketch");
    check(origin(d.sketches().front())==moved,"Later copy rebased existing V1 sketch");
    auto ambiguous=SketchGeometry::newSupportedFeature(original["definitionId"].toString(),ref,"Ambiguous copied face");
    auto ambiguousParameters=ambiguous["parameters"].toObject();ambiguousParameters["originConvention"]="projectedOwnerV1";ambiguous["parameters"]=ambiguousParameters;
    const auto beforeRefusal=d.authored();
    check(!d.commit(SketchGeometry::replaceFeature(beforeRefusal,ambiguous),"Unsupported copied support").isEmpty()
        && d.lastEvaluationError()["code"]=="unsupported_sketch_origin_context" && d.authored()==beforeRefusal,
        "New sketch on ambiguous placed support did not refuse atomically");
    std::cout<<"face_sketch_origin: projected owner, origin snap, Project, upstream edit, legacy, reopen passed\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
