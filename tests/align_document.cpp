// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <iostream>
#include <stdexcept>
#include <cmath>
void check(bool b,const char* m){if(!b)throw std::runtime_error(m);}
QJsonObject ref(const EvaluatedSupportFace& f){return {{"producerFeatureId",f.producerFeatureId},{"bodyId",f.bodyId},{"faceId",f.faceId}};}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
 CadDocument d;auto authored=d.authored();QJsonArray features;for(int i=0;i<2;++i)features.append(QJsonObject{{"id",CadDocument::newId()},{"ownerId",authored["documentId"]},{"bodyId",CadDocument::newId()},{"type","box"},{"name",i ? "Target" : "Source"},{"visible",true},{"parameters",QJsonObject{{"widthMm",10},{"depthMm",15},{"heightMm",20},{"xMm",i*50},{"yMm",0},{"zMm",i*30}}}});authored["features"]=features;check(d.commit(authored,"boxes").isEmpty(),"boxes");
 const auto source=d.authored();const auto before=d.bodies();const auto sf=before[0].supportFaces[0],tf=before[1].supportFaces[3];QJsonObject args{{"sourceFace",ref(sf)},{"targetFace",ref(tf)}};
 auto candidate=CadDocument::alignCandidate(source,args);const auto evaluation=CadDocument::evaluateCandidate(candidate);if(!evaluation.ok)throw std::runtime_error(evaluation.error.toStdString());check(d.authored()==source,"disposable preview");
 auto verify=[&](const EvaluatedDocument& e,bool flip){check(e.bodies.size()==2,"two independent solids");check(e.bodies[0].bodyId==before[0].bodyId,"source id");check(e.bodies[1].summary==before[1].summary,"target fixed");const auto a=GeometryKernel::resolveSketchSupport(e,ref(sf)),b=GeometryKernel::resolveSketchSupport(e,ref(tf));for(int i=0;i<3;++i){check(std::abs(a.frame["origin"].toArray()[i].toDouble()-b.frame["origin"].toArray()[i].toDouble())<1e-8,"frame origins");check(std::abs(a.frame["uAxis"].toArray()[i].toDouble()-b.frame["uAxis"].toArray()[i].toDouble())<1e-8,"frame U");check(std::abs(a.frame["normal"].toArray()[i].toDouble()-(flip ? -1 : 1)*b.frame["normal"].toArray()[i].toDouble())<1e-8,"normal flip");}check(e.bodies[0].solidEdges.size()==before[0].solidEdges.size(),"edge lineage");};
 verify(*evaluation.evaluation,false);check(d.commit(candidate,"Align").isEmpty(),"commit");d.history().undo();check(d.authored()==source,"undo");d.history().redo();check(d.authored()==candidate,"redo");
 args["flip"]=true;args["featureId"]=candidate["features"].toArray().last().toObject()["id"];candidate=CadDocument::alignCandidate(d.authored(),args);check(d.commit(candidate,"Flip").isEmpty(),"edit flip");verify(d.evaluation(),true);
 auto edited=candidate;features=edited["features"].toArray();auto box=features[0].toObject(),p=box["parameters"].toObject();p["widthMm"]=14;box["parameters"]=p;features[0]=box;edited["features"]=features;check(d.commit(edited,"upstream").isEmpty(),"upstream recompute");verify(d.evaluation(),true);
 const auto good=d.authored();args["targetFace"]=ref(sf);bool rejected=false;try{auto bad=CadDocument::alignCandidate(good,args);rejected=!d.commit(bad,"bad").isEmpty();}catch(...){rejected=true;}check(rejected && d.authored()==good,"same-body atomic refusal");
 args["targetFace"]=ref(tf);auto missing=ref(sf);missing["faceId"]=CadDocument::newId();args["sourceFace"]=missing;check(!d.commit(CadDocument::alignCandidate(good,args),"missing").isEmpty() && d.authored()==good,"lost face atomic");
 QTemporaryDir temp;auto path=temp.filePath("align.alloy");check(d.save(path).isEmpty(),"save");CadDocument reopened;check(reopened.open(path).isEmpty() && reopened.authored()==good,"reopen");verify(reopened.evaluation(),true);
 std::cout<<"Align exact frame/flip/lineage/upstream/history/reopen/refusal passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
