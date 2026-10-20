// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "Interference.h"
#include <BRepAlgoAPI_Common.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <gp_Pln.hxx>
#include <QCoreApplication>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
namespace {
QString idA,idB;
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void close(double actual,double expected){check(std::abs(actual-expected)<1e-7,"Numeric mismatch");}
QJsonObject box(const QJsonObject& authored,const QString& bodyId,double x){return {{"id",CadDocument::newId()},{"bodyId",bodyId},{"ownerId",authored.value("documentId")},{"type","box"},{"name",bodyId==idA?"A":"B"},{"visible",true},{"parameters",QJsonObject{{"xMm",x},{"yMm",bodyId==idA?0:2},{"zMm",bodyId==idA?0:3},{"widthMm",bodyId==idA?10:4},{"depthMm",bodyId==idA?10:4},{"heightMm",bodyId==idA?10:4}}}};}
QString code(const QJsonObject& result){return result.value("error").toObject().value("code").toString();}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    idA=CadDocument::newId();idB=CadDocument::newId();
    CadDocument document;auto authored=document.authored();authored["features"]=QJsonArray{box(authored,idA,0),box(authored,idB,8)};
    const auto error=document.commit(authored,"Fixture");if(!error.isEmpty())throw std::runtime_error(error.toStdString());
    const QJsonArray pair{QJsonObject{{"bodyId",idA}},QJsonObject{{"bodyId",idB}}};
    const auto before=document.authored();const auto history=document.history().count();
    const auto overlap=Interference::evaluate(document.bodies(),pair,"mm");check(overlap.json.value("kind")=="overlap","Overlap classification");close(overlap.json.value("canonical").toObject().value("overlapVolumeMm3").toDouble(),32);
    BRepAlgoAPI_Common independentCommon(document.bodies()[0].shape,document.bodies()[1].shape);check(independentCommon.IsDone(),"Independent common");GProp_GProps volume;BRepGProp::VolumeProperties(independentCommon.Shape(),volume);close(overlap.json.value("canonical").toObject().value("overlapVolumeMm3").toDouble(),volume.Mass());check(!overlap.feedback.IsNull(),"Overlap feedback");
    for(const auto [x,kind,distance]:{std::tuple{12.,"clearance",2.},std::tuple{10.,"touching",0.}}){
        authored=document.authored();auto features=authored.value("features").toArray();auto b=features[1].toObject();auto p=b.value("parameters").toObject();p["xMm"]=x;b["parameters"]=p;features[1]=b;authored["features"]=features;check(document.commit(authored,"Move source").isEmpty(),"Source edit");
        const auto result=Interference::evaluate(document.bodies(),pair,"in").json;check(result.value("kind")==kind,"Distance classification");close(result.value("canonical").toObject().value("clearanceMm").toDouble(),distance);close(result.value("display").toObject().value("length").toDouble(),distance/25.4);
        BRepExtrema_DistShapeShape independentDistance(document.bodies()[0].shape,document.bodies()[1].shape);check(independentDistance.IsDone(),"Independent distance");close(result.value("canonical").toObject().value("clearanceMm").toDouble(),independentDistance.Value());
        const auto pts=result.value("canonical").toObject().value("closestPointsMm").toArray();double square=0;for(int axis=0;axis<3;++axis)square+=std::pow(pts[0].toArray()[axis].toDouble()-pts[1].toArray()[axis].toDouble(),2);close(std::sqrt(square),distance);
    }
    document.history().undo();document.history().undo();check(document.authored()==before && document.history().count()==history+2,"Inspection adds no history or authored state");
    const auto inch=Interference::evaluate(document.bodies(),pair,"in").json;close(inch.value("display").toObject().value("volume").toDouble(),32/std::pow(25.4,3));
    auto bodies=document.bodies();check(code(Interference::evaluate(bodies,{},"mm").json)=="interference_selection_count","No pair refusal");check(code(Interference::evaluate(bodies,{pair[0]},"mm").json)=="interference_selection_count","One refusal");check(code(Interference::evaluate(bodies,{pair[0],pair[1],pair[0]},"mm").json)=="interference_selection_count","Three refusal");check(code(Interference::evaluate(bodies,{pair[0],pair[0]},"mm").json)=="interference_duplicate","Duplicate refusal");check(code(Interference::evaluate(bodies,{pair[0],QJsonObject{{"bodyId","missing"}}},"mm").json)=="interference_unavailable","Stale refusal");
    bodies[1].visible=false;check(code(Interference::evaluate(bodies,pair,"mm").json)=="interference_unavailable","Hidden refusal");bodies[1].visible=true;
    auto instance=bodies[0];bodies[0].occurrenceId="original";instance.occurrenceId="copy";bodies.push_back(instance);check(code(Interference::evaluate(bodies,pair,"mm").json)=="interference_ambiguous","Ambiguous occurrence refusal");
    check(Interference::evaluate(bodies,{QJsonObject{{"bodyId",idA},{"occurrenceId","copy"}},pair[1]},"mm").json.value("first").toObject().value("occurrenceId")=="copy","Explicit occurrence resolution");
    bodies=document.bodies();bodies[1].shape=BRepBuilderAPI_MakeFace(gp_Pln(),0,10,0,10).Shape();check(code(Interference::evaluate(bodies,pair,"mm").json)=="interference_solid_required","Surface refusal");
    QTemporaryDir directory;check(document.save(directory.path()+"/pair.alloy").isEmpty(),"Save");CadDocument reopened;check(reopened.open(directory.path()+"/pair.alloy").isEmpty(),"Reopen");close(Interference::evaluate(reopened.bodies(),pair,"mm").json.value("canonical").toObject().value("overlapVolumeMm3").toDouble(),32);
    std::cout<<"Committed exact pair overlap, clearance, touching, units, identity, refusals, history and reopen passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
