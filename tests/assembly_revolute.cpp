// SPDX-License-Identifier: MIT
#include "AssemblyRevolute.h"
#include "AssemblySlider.h"
#include "CadDocument.h"
#include <BRepAdaptor_Curve.hxx>
#include <GeomAbs_CurveType.hxx>
#include <QCoreApplication>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool valid,const char* message){if(!valid)throw std::runtime_error(message);}
void near(double actual,double expected,const char* message,double tolerance=1e-6){
    check(std::abs(actual-expected)<tolerance,message);
}
void commit(CadDocument& document,const QJsonObject& candidate){
    const auto error=document.commit(candidate,"Revolute test");check(error.isEmpty(),qPrintable(error));
}
QString definition(const QJsonObject& authored,const QString& name){
    for(const auto& value:authored["components"].toArray())if(value.toObject()["name"]==name)
        return value.toObject()["id"].toString();
    return {};
}
QString occurrence(const QJsonObject& authored,const QString& owner){
    for(const auto& value:authored["occurrences"].toArray())if(value.toObject()["definitionId"]==owner)
        return value.toObject()["id"].toString();
    return {};
}
const EvaluatedBody& bodyFor(const std::vector<EvaluatedBody>& bodies,const QString& owner){
    for(const auto& body:bodies)if(body.ownerId==owner)return body;
    throw std::runtime_error("Arm body unavailable");
}
QJsonObject sharedCircle(const EvaluatedBody& body){
    double farther=-1;QJsonObject chosen;
    for(const auto& edge:body.solidEdges)if(!edge.edge.IsNull()){
        const BRepAdaptor_Curve curve(edge.edge);if(curve.GetType()!=GeomAbs_Circle)continue;
        const auto center=curve.Circle().Location();
        if(std::abs(center.X())>1e-5 || std::abs(center.Z()-80)>1e-5)continue;
        if(std::abs(std::abs(curve.LastParameter()-curve.FirstParameter())-2*std::acos(-1.0))>1e-6)continue;
        if(std::abs(center.Y())>farther){farther=std::abs(center.Y());chosen={{"bodyId",body.bodyId},{"edgeId",edge.edgeId}};}
    }
    return chosen;
}
double yOf(const QJsonObject& assembly){return assembly["joints"].toArray()[0].toObject()["originFrameWorld"].toArray()[7].toDouble();}
}
int main(int argc,char** argv){QCoreApplication application(argc,argv);try{
    check(argc==2,"Pass accepted native piston fixture path");
    CadDocument document;check(document.open(QString::fromLocal8Bit(argv[1])).isEmpty(),"Open native fixture");
    const auto lower=definition(document.authored(),"Lower arm"),upper=definition(document.authored(),"Upper arm");
    const auto lowerOccurrence=occurrence(document.authored(),lower),upperOccurrence=occurrence(document.authored(),upper);
    check(!lowerOccurrence.isEmpty() && !upperOccurrence.isEmpty(),"Arm occurrences");
    const auto circle=sharedCircle(bodyFor(document.bodies(),lower));check(!circle.isEmpty(),"Shared hinge circle");
    const auto rest=document.authored()["features"].toArray();
    commit(document,AssemblyRevolute::candidate(document.authored(),document.evaluation(),
        {{"component1OccurrenceId",upperOccurrence},{"component2OccurrenceId",lowerOccurrence},{"position",circle},{"angleDeg",0}}));
    const auto jointId=document.authored()["joints"].toArray()[0].toObject()["id"].toString();
    check(!document.assemblyState()["joints"].toArray()[0].toObject()["pinAuthored"].toBool(),"Before Pin uses explicit temporary gauge");
    check(document.authored()["features"].toArray()==rest,"As-built joint did not alter definition history");
    check(bodyFor(document.worldBodies(),upper).shape.IsSame(bodyFor(document.bodies(),upper).shape),"Angle zero preserves rest BRep");
    const auto beforeDrive=document.authored();
    bool datumRefused=false;try{AssemblyRevolute::candidate(document.authored(),document.evaluation(),
        {{"jointId",jointId},{"position",QJsonObject{{"bodyId",circle["bodyId"]},{"edgeId",CadDocument::newId()}}}});
    }catch(const std::invalid_argument&){datumRefused=true;}
    check(datumRefused && document.authored()==beforeDrive,"Editing Position cannot silently recapture the as-built datum");
    commit(document,AssemblyRevolute::candidate(document.authored(),document.evaluation(),{{"jointId",jointId},{"angleDeg",15}}));
    const auto positive=document.assemblyState()["joints"].toArray()[0].toObject();
    near(positive["coordinateDeg"].toDouble(),15,"Positive prescribed coordinate");
    check(positive["driveErrorRadians"].toDouble()<1e-7,"Exact positive drive residual");
    check(!bodyFor(document.worldBodies(),upper).shape.IsSame(bodyFor(document.bodies(),upper).shape),"Upper arm world occurrence moved");
    check(bodyFor(document.worldBodies(),lower).shape.IsSame(bodyFor(document.bodies(),lower).shape),"Lower arm fixed");
    check(document.authored()["features"].toArray()==rest,"Drive changed definition geometry");
    commit(document,AssemblyRevolute::candidate(document.authored(),document.evaluation(),{{"jointId",jointId},{"angleDeg",-15}}));
    const auto negative=document.assemblyState()["joints"].toArray()[0].toObject();
    near(negative["coordinateDeg"].toDouble(),-15,"Negative prescribed coordinate");
    check(negative["driveErrorRadians"].toDouble()<1e-7,"Exact negative drive residual");
    document.history().undo();check(document.authored()["joints"].toArray()[0].toObject()["angleDeg"]==15,"Angle undo");
    document.history().redo();check(document.authored()["joints"].toArray()[0].toObject()["angleDeg"]==-15,"Angle redo");
    const auto unpinned=document.authored();
    commit(document,CadDocument::pinOccurrenceCandidate(document.authored(),lowerOccurrence));
    const auto pinned=document.authored();const auto pin=pinned["features"].toArray().last().toObject();
    check(pin["type"]=="assemblyPin" && document.evaluation().featureOrder.back()==pin["id"].toString(),"Pin is retained Timeline history");
    check(document.assemblyState()["joints"].toArray()[0].toObject()["pinAuthored"].toBool(),"Authored Pin distinguished from gauge");
    const auto prefix=document.evaluationBefore(pin["id"].toString());
    check(!AssemblyRevolute::inspect(pinned,prefix)["joints"].toArray()[0].toObject()["pinAuthored"].toBool(),"Prefix before Pin excludes authored Pin constraint");
    bool wrongPreviewRefused=false;try{AssemblySlider::inspect(pinned,document.evaluation(),12);}catch(const std::invalid_argument&){wrongPreviewRefused=true;}
    check(wrongPreviewRefused,"Slider preview cannot reinterpret Revolute degrees as millimeters");
    document.history().undo();check(document.authored()==unpinned && !document.assemblyState()["joints"].toArray()[0].toObject()["pinAuthored"].toBool(),"Pin undo");
    document.history().redo();check(document.authored()==pinned,"Pin redo exact");
    auto bad=pinned;auto rows=bad["joints"].toArray();auto row=rows[0].toObject();row["position"]=QJsonObject{{"bodyId",bodyFor(document.bodies(),upper).bodyId},{"edgeId",circle["edgeId"]}};rows[0]=row;bad["joints"]=rows;
    const auto revision=document.revision();check(!document.commit(bad,"Wrong owner").isEmpty() && document.authored()==pinned && document.revision()==revision,"Wrong-owner Position refusal atomic");
    bad=pinned;rows=bad["joints"].toArray();row=rows[0].toObject();row["angleDeg"]=181;rows[0]=row;bad["joints"]=rows;
    check(!document.commit(bad,"Unsupported angle").isEmpty() && document.authored()==pinned,"Angle limit refusal atomic");
    QTemporaryDir temporary;check(temporary.isValid(),"Temporary directory");const auto path=temporary.filePath("revolute.alloy");
    check(document.save(path).isEmpty(),"Save Revolute and Pin");CadDocument reopened;
    check(reopened.open(path).isEmpty() && reopened.authored()==pinned,"Reopen exact authored joint/Pin");
    check(reopened.assemblyState()["joints"].toArray()[0].toObject()["pinAuthored"].toBool(),"Reopen Pin solver state");
    const auto oldY=yOf(reopened.assemblyState());auto edited=reopened.authored();auto features=edited["features"].toArray();
    for(int i=0;i<features.size();++i){auto feature=features[i].toObject();if(feature["name"]!="Lower arm" || feature["type"]!="extrude")continue;
        auto parameters=feature["parameters"].toObject();parameters["distanceMm"]=parameters["distanceMm"].toDouble()+1;
        feature["parameters"]=parameters;features[i]=feature;break;}
    edited["features"]=features;commit(reopened,edited);
    check(reopened.authored()["joints"]==pinned["joints"],"Source edit retained joint IDs and marker");
    check(std::abs(yOf(reopened.assemblyState())-oldY)>0.5,"Circular origin follows meaningful arm thickness edit");
    reopened.history().undo();near(yOf(reopened.assemblyState()),oldY,"Source edit undo restores origin");
    std::cout<<"PASS: native circular-origin Revolute, signed exact drive, rest pose, Pin prefix/history, source edit and atomic refusal\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
