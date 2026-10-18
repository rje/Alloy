// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <iostream>
#include <stdexcept>
#include <cmath>
static void check(bool b,const char* m){if(!b)throw std::runtime_error(m);}
static void commit(CadDocument& d,const QJsonObject& a){auto e=d.commit(a,"Thread fixture");if(!e.isEmpty())throw std::runtime_error(e.toStdString());}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    CadDocument doc;auto authored=doc.authored();
    QJsonObject cylinder{{"id",CadDocument::newId()},{"bodyId",CadDocument::newId()},{"ownerId",authored["documentId"]},{"type","cylinder"},{"name","Boss"},{"visible",true},
        {"parameters",QJsonObject{{"xMm",5},{"yMm",-3},{"zMm",4},{"radiusMm",4},{"heightMm",8}}}};
    authored["features"]=QJsonArray{cylinder};commit(doc,authored);
    QJsonObject support;for(const auto& f:doc.bodies()[0].supportFaces)if(f.role=="cylinder.lateral")support={{"producerFeatureId",f.producerFeatureId},{"bodyId",f.bodyId},{"faceId",f.faceId}};
    check(!support.isEmpty(),"Retained cylinder face required");
    QJsonObject args{{"support",support},{"nominalDiameterMm",8},{"lengthMm",4},{"startOffsetMm",1}};
    const auto before=doc.authored();const auto baseRevision=doc.revision();
    const auto candidate=CadDocument::threadCandidate(before,args);const auto prepared=CadDocument::evaluateCandidate(candidate);
    check(prepared.ok,"Prepared Thread evaluation");
    check(doc.commitThreadEvaluation(prepared,candidate,before,baseRevision).isEmpty(),"Adopt Thread evaluation");
    const auto retained=doc.authored();const auto threadId=retained["features"].toArray().last().toObject().value("id");
    check(doc.bodies().size()==1 && doc.bodies()[0].summary["valid"].toBool(),"One valid threaded body");
    const double originalVolume=doc.bodies()[0].summary["volumeMm3"].toDouble();
    check(originalVolume<3.141592653589793*16*8,"Groove removed material");
    doc.history().undo();check(doc.authored()==before,"Exact undo");doc.history().redo();check(doc.authored()==retained,"Exact redo");
    auto changed=retained;auto fs=changed["features"].toArray();auto boss=fs[0].toObject();auto bp=boss["parameters"].toObject();bp["heightMm"]=10;boss["parameters"]=bp;fs[0]=boss;changed["features"]=fs;commit(doc,changed);
    check(std::abs(doc.bodies()[0].summary["volumeMm3"].toDouble()-originalVolume-32*3.141592653589793)<1e-4,"Upstream height edit replays thread");
    const auto stable=doc.authored();const auto revision=doc.revision();
    auto stale=support;stale["faceId"]=CadDocument::newId();
    check(!doc.commit(CadDocument::threadCandidate(stable,{{"featureId",threadId},{"support",stale}}),"Stale face").isEmpty(),"Stale face refused");
    check(doc.authored()==stable && doc.revision()==revision,"Stale reference refusal atomic");
    check(!doc.commit(CadDocument::threadCandidate(stable,{{"featureId",threadId},{"lengthMm",20}}),"Too long").isEmpty(),"Overrun refused");
    check(doc.authored()==stable && doc.revision()==revision,"Interval refusal atomic");
    bool refused=false;try{CadDocument::threadCandidate(stable,{{"featureId",threadId},{"modeled",false}});}catch(const std::exception&){refused=true;}check(refused,"Cosmetic thread refused");
    refused=false;try{CadDocument::threadCandidate(stable,{{"featureId",threadId},{"unknown",true}});}catch(const std::exception&){refused=true;}check(refused,"Unknown argument refused");
    commit(doc,CadDocument::threadCandidate(stable,{{"featureId",threadId},{"fullLength",true},{"startOffsetMm",0},{"direction","left"}}));
    check(doc.bodies()[0].summary["thread"].toObject()["evaluatedLengthMm"]==10,"Full Length follows edited source");
    // Retarget an edited designation to a different earlier analytic boss.
    auto retarget=doc.authored();auto all=retarget["features"].toArray();auto other=cylinder;
    other["id"]=CadDocument::newId();other["bodyId"]=CadDocument::newId();auto op=other["parameters"].toObject();op["radiusMm"]=5;op["xMm"]=25;other["parameters"]=op;
    auto prefix=retarget;prefix["features"]=QJsonArray{all[0],other};const auto evaluatedPrefix=CadDocument::evaluateCandidate(prefix);check(evaluatedPrefix.ok,"Retarget prefix");
    QJsonObject otherSupport;for(const auto& b:evaluatedPrefix.evaluation->bodies)if(b.bodyId==other["bodyId"].toString())for(const auto& f:b.supportFaces)if(f.role=="cylinder.lateral")otherSupport={{"producerFeatureId",f.producerFeatureId},{"bodyId",f.bodyId},{"faceId",f.faceId}};
    retarget["features"]=QJsonArray{all[0],other,all[1]};
    commit(doc,CadDocument::threadCandidate(retarget,{{"featureId",threadId},{"support",otherSupport},{"nominalDiameterMm",10},{"pitchMm",1.5},{"fullLength",false},{"lengthMm",4},{"direction","right"}}));
    check(doc.bodies().size()==2 && doc.bodies()[1].summary["thread"].toObject()["nominalDiameterMm"]==10,"Size/pitch edit retargets selected boss");
    QTemporaryDir temp;auto path=temp.filePath("thread.alloy");check(doc.save(path).isEmpty(),"Save");CadDocument reopened;check(reopened.open(path).isEmpty(),"Reopen");
    check(reopened.authored()==doc.authored() && std::abs(reopened.bodies().back().summary["volumeMm3"].toDouble()-doc.bodies().back().summary["volumeMm3"].toDouble())<1e-7,"Exact retained reopen");
    std::cout<<"Retained Thread create/edit, worker adoption, upstream edit, refusal, undo/redo and reopen passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
