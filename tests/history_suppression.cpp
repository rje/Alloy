// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
#include <stdexcept>
static void check(bool value,const char* why){if(!value)throw std::runtime_error(why);}
static QJsonObject box(const QString& owner,const QString& name,double w,double d,double h,double x=0){return {{"id",CadDocument::newId()},{"bodyId",CadDocument::newId()},{"ownerId",owner},{"type","box"},{"name",name},{"visible",true},{"parameters",QJsonObject{{"widthMm",w},{"depthMm",d},{"heightMm",h},{"xMm",x},{"yMm",0},{"zMm",0}}}};}
static QJsonObject suppress(QJsonObject a,int index,bool value){auto rows=a.value("features").toArray();auto f=rows[index].toObject();f["suppressed"]=value;rows[index]=f;a["features"]=rows;return a;}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    CadDocument doc;auto a=doc.authored();auto source=box(doc.activeOwnerId(),"Box A",20,15,10);a["features"]=QJsonArray{source};check(doc.commit(a,"Box A").isEmpty(),"Source");
    const auto& body=doc.bodies()[0];QJsonObject support;
    for(const auto& face:body.supportFaces)if(face.role=="box.z1")support={{"producerFeatureId",face.producerFeatureId},{"bodyId",face.bodyId},{"faceId",face.faceId}};
    check(!support.isEmpty(),"Top face");a=CadDocument::faceOffsetCandidate(doc.authored(),{{"targetBodyId",source.value("bodyId")},{"face",support},{"distanceMm",5}});check(doc.commit(a,"Offset").isEmpty(),"Offset");
    a=doc.authored();auto rows=a.value("features").toArray();const auto offset=rows[1].toObject();const auto independent=box(doc.activeOwnerId(),"Box B",8,6,4,40);rows.append(independent);a["features"]=rows;
    a["bodyNames"]=QJsonObject{{source.value("bodyId").toString(),"Named A"}};
    a["objectOwners"]=QJsonObject{{"bodies",QJsonObject{{source.value("bodyId").toString(),doc.activeOwnerId()}}},{"sketches",QJsonObject{}}};
    check(doc.commit(a,"Independent").isEmpty(),"Independent and metadata");const auto full=doc.authored();const auto geometry=doc.state().value("evaluation").toObject().value("bodies");
    check(doc.bodies().size()==2 && std::abs(doc.bodies()[0].summary.value("volumeMm3").toDouble()-4500)<1e-6,"Initial volume");
    const auto index=doc.history().index();check(doc.commit(suppress(full,0,true),"Suppress").isEmpty(),"Suppress source");
    const auto suppressed=doc.authored();check(doc.bodies().size()==1 && doc.bodies()[0].bodyId==independent.value("bodyId"),"Independent remains");
    const auto states=doc.evaluation().featureStates;check(states.value(source.value("id").toString()).toObject().value("explicitSuppressed").toBool(),"Explicit flag");
    check(states.value(offset.value("id").toString()).toObject().value("effectiveSuppressed").toBool() && !states.value(offset.value("id").toString()).toObject().value("explicitSuppressed").toBool(),"Implicit dependent");
    check(doc.evaluation().featureOrder.size()==3 && doc.history().index()==index+1,"Order and transaction");
    doc.history().undo();check(doc.authored()==full && doc.state().value("evaluation").toObject().value("bodies")==geometry,"Undo exact");doc.history().redo();check(doc.authored()==suppressed && doc.bodies().size()==1,"Redo");
    QTemporaryDir temp;const auto path=temp.path()+"/suppressed.alloy";check(doc.save(path).isEmpty(),"Save suppressed");CadDocument reopened;check(reopened.open(path).isEmpty() && reopened.authored()==suppressed && reopened.bodies().size()==1,"Reopen suppressed");
    check(reopened.commit(suppress(reopened.authored(),0,false),"Restore").isEmpty(),"Restore");check(reopened.state().value("evaluation").toObject().value("bodies")==geometry,"Exact restored geometry");
    check(reopened.save(path).isEmpty(),"Save false");CadDocument restored;check(restored.open(path).isEmpty() && restored.authored().value("features").toArray()[0].toObject().value("suppressed")==false,"Persist false");
    auto unsupported=full;auto fs=unsupported.value("features").toArray();auto cylinder=independent;cylinder["type"]="cylinder";cylinder["parameters"]=QJsonObject{{"radiusMm",4},{"heightMm",4},{"xMm",40},{"yMm",0},{"zMm",0}};fs[2]=cylinder;unsupported["features"]=fs;check(doc.commit(unsupported,"Cylinder").isEmpty(),"Unsupported baseline");const auto before=doc.authored();const auto rev=doc.revision();const auto undo=doc.history().index();
    check(!doc.commit(suppress(before,0,true),"Refuse").isEmpty() && doc.lastEvaluationError().value("featureId")==cylinder.value("id"),"Unsupported refusal");check(doc.authored()==before && doc.revision()==rev && doc.history().index()==undo,"Atomic refusal");
    check(!CadDocument::evaluateCandidate(suppress(before,2,false)).ok,"Unsupported false field refused");auto invalid=suppress(full,0,true);auto ifs=invalid.value("features").toArray();auto bad=ifs[0].toObject();bad["suppressed"]="true";ifs[0]=bad;invalid["features"]=ifs;check(!CadDocument::evaluateCandidate(invalid).ok,"Malformed flag refused");
    std::cout<<"PASS source/dependent/independent; identities/metadata; atomic family refusal; Undo/Redo; suppressed and restored persistence\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
