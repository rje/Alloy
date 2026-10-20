// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "BodyCopy.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QCryptographicHash>
#include <BRepTools.hxx>
#include <BRep_Builder.hxx>
#include <TopoDS_Compound.hxx>
#include <sstream>
#include <iostream>
#include <cmath>
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
void volume(const EvaluatedBody& b,double expected){check(std::abs(b.summary["volumeMm3"].toDouble()-expected)<1e-6,"exact independent volume");}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    CadDocument doc;auto authored=doc.authored();const auto sourceId=CadDocument::newId(),sourceFeature=CadDocument::newId();
    authored["features"]=QJsonArray{QJsonObject{{"id",sourceFeature},{"ownerId",authored["documentId"]},{"bodyId",sourceId},{"type","box"},{"name","Source"},{"visible",true},{"parameters",QJsonObject{{"widthMm",20},{"depthMm",20},{"heightMm",10},{"xMm",-10},{"yMm",-10},{"zMm",0}}}}};
    check(doc.commit(authored,"Source").isEmpty(),"source commit");
    const auto snapshot=BodyCopy::capture(doc.bodies()[0]);
    auto candidate=CadDocument::bodyCopyCandidate(authored,snapshot,{{"xMm",30},{"name","Snapshot"}});
    const auto copyFeature=candidate["features"].toArray().last().toObject();const auto copyId=copyFeature["bodyId"].toString();
    check(copyId!=sourceId && copyFeature["id"]!=sourceFeature,"fresh identities");
    auto preview=CadDocument::evaluateCandidate(candidate);check(preview.ok && preview.evaluation->bodies.size()==2,"two preview solids");check(doc.authored()==authored,"preview disposable");
    volume(preview.evaluation->bodies[1],4000);check(std::abs(preview.evaluation->bodies[1].summary["boundsMm"].toObject()["min"].toArray()[0].toDouble()-20)<1e-6,"explicit placement");
    check(doc.commit(candidate,"Copy").isEmpty(),"copy commit");doc.history().undo();check(doc.authored()==authored && doc.bodies().size()==1,"undo copy");doc.history().redo();check(doc.authored()==candidate,"redo identity");
    auto features=candidate["features"].toArray();auto source=features[0].toObject();auto p=source["parameters"].toObject();p["heightMm"]=14;source["parameters"]=p;features[0]=source;candidate["features"]=features;
    check(doc.commit(candidate,"Source edit").isEmpty(),"source edit");volume(doc.bodies()[0],5600);volume(doc.bodies()[1],4000);
    const auto sourceEdited=doc.authored();doc.history().undo();volume(doc.bodies()[0],4000);volume(doc.bodies()[1],4000);doc.history().redo();check(doc.authored()==sourceEdited,"source redo");
    // A separate root tool targets the copy; no copied-face lineage is needed.
    const auto tool=CadDocument::newId();features=candidate["features"].toArray();features.append(QJsonObject{{"id",CadDocument::newId()},{"ownerId",authored["documentId"]},{"bodyId",tool},{"type","cylinder"},{"name","Cut tool"},{"visible",true},{"parameters",QJsonObject{{"radiusMm",2},{"heightMm",20},{"xMm",30},{"yMm",0},{"zMm",-5}}}});candidate["features"]=features;
    check(doc.commit(candidate,"Tool").isEmpty(),"tool commit");candidate=CadDocument::booleanCandidate(candidate,{{"targetBodyId",copyId},{"toolBodyIds",QJsonArray{tool}},{"operation","cut"},{"keepTools",false}});
    check(doc.commit(candidate,"Cut copy").isEmpty(),"editable copied solid");check(doc.bodies().size()==2,"tool consumed only");volume(doc.bodies()[0],5600);volume(doc.bodies()[1],4000-40*std::acos(-1.0));
    doc.history().undo();check(doc.bodies().size()==3,"undo Cut");volume(doc.bodies()[1],4000);doc.history().redo();check(doc.authored()==candidate,"redo Cut");
    QTemporaryDir dir;check(doc.save(dir.filePath("copy.alloy")).isEmpty(),"save");CadDocument reopened;check(reopened.open(dir.filePath("copy.alloy")).isEmpty(),"reopen");check(reopened.authored()==candidate,"authored snapshot retained");volume(reopened.bodies()[0],5600);volume(reopened.bodies()[1],4000-40*std::acos(-1.0));
    auto rejected=[&](QJsonObject params){auto invalid=candidate;auto f=invalid["features"].toArray();auto c=f[1].toObject();c["parameters"]=params;f[1]=c;invalid["features"]=f;check(!CadDocument::evaluateCandidate(invalid).ok,"corrupt snapshot must refuse");check(doc.authored()==candidate,"atomic refusal");};
    auto params=copyFeature["parameters"].toObject();auto bad=snapshot;bad["sha256"]=QString(64,'0');params["snapshot"]=bad;rejected(params);
    bad=snapshot;bad["byteCount"]=bad["byteCount"].toInteger()+1;params["snapshot"]=bad;rejected(params);
    bad=snapshot;bad["bytesBase64"]=bad["bytesBase64"].toString()+"\n";params["snapshot"]=bad;rejected(params);
    bad=snapshot;bad["byteCount"]=33554433;params["snapshot"]=bad;rejected(params);
    params=copyFeature["parameters"].toObject();params["sourceBodyId"]=sourceId;rejected(params);
    // A hash-correct non-BRep payload still refuses at geometry evaluation.
    bad=snapshot;const QByteArray junk("not a BRep");bad["bytesBase64"]=QString::fromLatin1(junk.toBase64());bad["byteCount"]=junk.size();bad["sha256"]=QString::fromLatin1(QCryptographicHash::hash(junk,QCryptographicHash::Sha256).toHex());params=copyFeature["parameters"].toObject();params["snapshot"]=bad;rejected(params);
    BRep_Builder compoundBuilder;TopoDS_Compound compound;compoundBuilder.MakeCompound(compound);compoundBuilder.Add(compound,doc.bodies()[0].shape);compoundBuilder.Add(compound,doc.bodies()[1].shape);
    auto multiple=doc.bodies()[0];multiple.shape=compound;bool multiRefused=false;try{BodyCopy::capture(multiple);}catch(const std::exception&){multiRefused=true;}check(multiRefused,"multi-solid source refuses");
    bool refused=false;try{BodyCopy::capture(EvaluatedBody{});}catch(const std::exception&){refused=true;}check(refused,"null source refuses");
    std::cout<<"Body snapshot isolation/Cut/history/storage/integrity passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
