// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include "SketchOpenChainOffset.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool value,const char* message) {if(!value)throw std::runtime_error(message);}
void commit(CadDocument& document,const QJsonObject& candidate) {
    const auto error=document.commit(candidate,"Model parameter test");
    if(!error.isEmpty())throw std::runtime_error(error.toStdString());
}
QJsonObject named(const QJsonArray& rows,const QString& name,const QString& type) {
    for(const auto& value:rows) {
        const auto row=value.toObject();
        if(row["name"]==name && row["featureType"]==type)return row;
    }
    return {};
}
QJsonObject user(const QJsonObject& authored,const QString& name) {
    for(const auto& value:authored["userParameters"].toArray())if(value.toObject()["name"]==name)return value.toObject();
    return {};
}
}
int main(int argc,char** argv) {QCoreApplication app(argc,argv);try {
    CadDocument document;
    const auto owner=document.authored()["documentId"];
    QJsonObject box{{"id",CadDocument::newId()},{"bodyId",CadDocument::newId()},{"ownerId",owner},
        {"type","box"},{"name","Box"},{"visible",true},
        {"parameters",QJsonObject{{"widthMm",50},{"depthMm",80},{"heightMm",40},{"xMm",0},{"yMm",0},{"zMm",0}}}};
    auto candidate=document.authored();candidate["features"]=QJsonArray{box};commit(document,candidate);
    QJsonObject top;
    for(const auto& face:document.bodies().front().supportFaces)if(face.role=="box.z1")
        top={{"producerFeatureId",face.producerFeatureId},{"bodyId",face.bodyId},{"faceId",face.faceId}};
    check(!top.isEmpty(),"Box top support");
    commit(document,CadDocument::shellCandidate(document.authored(),{{"targetBodyId",box["bodyId"]},
        {"removedFace",top},{"thicknessMm",4}}));
    auto sketch=SketchGeometry::newFeature(document.activeOwnerId(),"XY","Angled length");
    sketch=SketchGeometry::addLine(sketch,{{"u1Mm",100},{"v1Mm",0},{"u2Mm",102},{"v2Mm",3.464101615}});
    const auto lineId=sketch["parameters"].toObject()["entities"].toArray().last().toObject()["id"];
    const auto length=SketchGeometry::addDimension(sketch,{{"kind","linear"},{"entityId",lineId},{"valueMm",4}});
    check(length.ok,"driving slanted line length");sketch=length.feature;
    commit(document,SketchGeometry::replaceFeature(document.authored(),sketch));
    const auto boxId=box["id"],shellId=document.authored()["features"].toArray()[1].toObject()["id"],sketchId=sketch["id"];
    auto rows=CadDocument::modelLengthRows(document.authored());
    check(rows.size()==5,"literal Box Shell and sketch rows");
    const auto depthTarget=named(rows,"Depth","box")["target"].toObject();
    const auto shellTarget=named(rows,"Thickness","shell")["target"].toObject();
    const auto lineTarget=named(rows,"linear Dimension","sketch")["target"].toObject();
    check(depthTarget["featureId"]==boxId && shellTarget["featureId"]==shellId &&
        lineTarget["featureId"]==sketchId && lineTarget.contains("dimensionId"),"stable target identities");
    for(const auto& entry:QVector<QPair<QString,double>>{{"Length",80},{"Width",50},{"Height",40},{"Thickness",4}})
        commit(document,CadDocument::lengthParameterCandidate(document.authored(),{{"name",entry.first},{"value",entry.second},{"units","mm"}}));
    auto bind=[&](const QJsonObject& target,const QString& expression){
        commit(document,CadDocument::modelLengthCandidate(document.authored(),{{"target",target},{"expression",expression}}));
    };
    rows=CadDocument::modelLengthRows(document.authored());
    bind(depthTarget,"Length");bind(named(rows,"Width","box")["target"].toObject(),"Width");
    bind(named(rows,"Height","box")["target"].toObject(),"Height");
    bind(shellTarget,"Thickness");bind(lineTarget,"Thickness");
    rows=CadDocument::modelLengthRows(document.authored());
    check(named(rows,"Depth","box")["sourceName"]=="Length" &&
        named(rows,"linear Dimension","sketch")["sourceName"]=="Thickness","source names shown");
    check(document.authored()["features"].toArray()[0].toObject()["id"]==boxId,"rebind recreated Box");
    commit(document,CadDocument::lengthParameterCandidate(document.authored(),{{"parameterId",user(document.authored(),"Width")["id"]},{"name","Wide"},{"value",74}}));
    rows=CadDocument::modelLengthRows(document.authored());
    check(named(rows,"Width","box")["expression"]=="Wide","rename follows stable parameter ID");
    check(std::abs(document.bodies().front().summary["boundsMm"].toObject()["size"].toArray()[0].toDouble()-74)<1e-6,
        "bound Box recomputed after width update");
    commit(document,CadDocument::lengthParameterCandidate(document.authored(),{{"parameterId",user(document.authored(),"Thickness")["id"]},{"value",3.5}}));
    rows=CadDocument::modelLengthRows(document.authored());
    check(std::abs(named(rows,"Thickness","shell")["valueMm"].toDouble()-3.5)<1e-6 &&
        std::abs(named(rows,"linear Dimension","sketch")["valueMm"].toDouble()-3.5)<1e-6,
        "Shell and slanted length share Thickness");
    const auto stable=document.authored();const auto revision=document.revision();
    check(!document.commit(CadDocument::lengthParameterCandidate(document.authored(),
        {{"parameterId",user(document.authored(),"Thickness")["id"]},{"value",45}}),"Impossible Shell").isEmpty(),
        "invalid downstream Shell refused");
    check(document.authored()==stable && document.revision()==revision,"invalid update changed authored state");
    try {CadDocument::modelLengthCandidate(document.authored(),{{"target",lineTarget},{"expression","Missing"}});
        check(false,"missing user name accepted");}catch(const std::invalid_argument&){}
    bind(lineTarget,"3.5 mm");
    check(!named(CadDocument::modelLengthRows(document.authored()),"linear Dimension","sketch").contains("parameterId"),
        "same-value literal did not remove binding");
    document.history().undo();
    check(named(CadDocument::modelLengthRows(document.authored()),"linear Dimension","sketch")["sourceName"]=="Thickness",
        "undo restored binding");
    document.history().redo();
    check(!named(CadDocument::modelLengthRows(document.authored()),"linear Dimension","sketch").contains("parameterId"),
        "redo restored numeric replacement");
    auto offsetSketch=SketchGeometry::newFeature(document.activeOwnerId(),"XY","Lid clearance");
    const double edges[][4]={{-12,10,-8,10},{-8,10,-6,0},{-6,0,6,0},{6,0,8,10},{8,10,12,10},{-8,10,8,10}};
    for(const auto& edge:edges)offsetSketch=SketchGeometry::addLine(offsetSketch,
        {{"u1Mm",edge[0]},{"v1Mm",edge[1]},{"u2Mm",edge[2]},{"v2Mm",edge[3]}});
    const auto entities=offsetSketch["parameters"].toObject()["entities"].toArray();QJsonArray sourceIds;
    for(int i=0;i<5;++i)sourceIds.append(entities[i].toObject()["id"]);
    offsetSketch=SketchOpenChainOffset::add(offsetSketch,offsetSketch["parameters"].toObject(),
        {{"sourceEntityIds",sourceIds},{"topEntityId",entities[5].toObject()["id"]},{"signedDistanceMm",-0.25}});
    commit(document,SketchGeometry::replaceFeature(document.authored(),offsetSketch));
    commit(document,CadDocument::lengthParameterCandidate(document.authored(),{{"name","Clearance"},{"value",0.25},{"units","mm"}}));
    auto offsetRow=named(CadDocument::modelLengthRows(document.authored()),"Chain Offset","sketch");
    const auto offsetTarget=offsetRow["target"].toObject();
    check(offsetTarget.contains("offsetGroupId") && offsetRow["valueMm"].toDouble()==-0.25,
        "authored Offset row identity and inward value");
    bind(offsetTarget,"Clearance");
    offsetRow=named(CadDocument::modelLengthRows(document.authored()),"Chain Offset","sketch");
    check(offsetRow["parameterId"]==user(document.authored(),"Clearance")["id"] &&
        offsetRow["valueMm"].toDouble()==-0.25,"Offset binding lost inward side");
    commit(document,CadDocument::lengthParameterCandidate(document.authored(),
        {{"parameterId",user(document.authored(),"Clearance")["id"]},{"value",0.4}}));
    check(std::abs(named(CadDocument::modelLengthRows(document.authored()),"Chain Offset","sketch")["valueMm"].toDouble()+0.4)<1e-9,
        "bound Offset did not recompute from Clearance");
    bind(offsetTarget,"-0.4 mm");
    check(!named(CadDocument::modelLengthRows(document.authored()),"Chain Offset","sketch").contains("parameterId"),
        "same-value Offset literal did not unbind");
    QTemporaryDir temporary;check(temporary.isValid(),"temporary directory");
    const auto path=temporary.path()+"/model-table.alloy";check(document.save(path).isEmpty(),"save");
    CadDocument reopened;check(reopened.open(path).isEmpty() && reopened.authored()==document.authored(),"reopen");
    check(named(CadDocument::modelLengthRows(reopened.authored()),"Depth","box")["sourceName"]=="Length",
        "reopen stable model row");
    std::cout<<"PASS: existing model lengths rebind through stable targets, recompute, unbind, undo and reopen\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
