#include "CadDocument.h"
#include "SolidSplit.h"
#include "GeometryKernel.h"
#include <BRepAlgoAPI_Common.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <QCoreApplication>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
#include <stdexcept>
namespace {
void check(bool result,const char* message){if(!result)throw std::runtime_error(message);}
QJsonObject box(CadDocument& doc){
    const QJsonObject feature{{"id",CadDocument::newId()},{"bodyId",CadDocument::newId()},
        {"ownerId",doc.authored()["documentId"]},{"type","box"},{"name","Case"},{"visible",true},
        {"parameters",QJsonObject{{"widthMm",50},{"depthMm",80},{"heightMm",40},{"xMm",0},{"yMm",0},{"zMm",0}}}};
    auto candidate=doc.authored();candidate["features"]=QJsonArray{feature};check(doc.commit(candidate,"Box").isEmpty(),"Box commit");return feature;
}
QJsonObject source(const EvaluatedSupportFace& face){return {{"producerFeatureId",face.producerFeatureId},{"bodyId",face.bodyId},{"faceId",face.faceId}};}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    CadDocument doc;const auto stock=box(doc);const auto body=doc.bodies().at(0);
    QJsonObject top,bottom;for(const auto& face:body.supportFaces){if(face.role=="box.z1")top=source(face);if(face.role=="box.z0")bottom=source(face);}
    check(!top.isEmpty()&&!bottom.isEmpty(),"top and bottom supports");
    const QJsonObject mid{{"id",CadDocument::newId()},{"ownerId",doc.authored()["documentId"]},
        {"type","constructionPlane"},{"name","Midplane"},{"visible",true},
        {"parameters",QJsonObject{{"source",QJsonObject{{"faces",QJsonArray{top,bottom}}}}}}};
    auto before=doc.authored();auto features=before["features"].toArray();features.append(mid);before["features"]=features;
    check(doc.commit(before,"Midplane").isEmpty(),"retained Midplane commit");before=doc.authored();
    const auto candidate=CadDocument::splitBodyCandidate(before,{{"targetBodyId",body.bodyId},{"planeId",mid["id"]}});
    const auto split=candidate["features"].toArray().last().toObject();const auto positive=split["bodyId"].toString();
    check(positive==SolidSplit::positiveBodyId(split["id"].toString(),body.bodyId),"source-derived positive ID");
    check(doc.commit(candidate,"Split Body").isEmpty(),qPrintable(doc.lastEvaluationError()["message"].toString()));
    check(doc.bodies().size()==2,"two independently selectable bodies");
    const auto lower=doc.bodies()[0],upper=doc.bodies()[1];
    check(lower.bodyId==body.bodyId && upper.bodyId==positive,"source/derived body IDs");
    check(lower.ownerId==upper.ownerId && lower.ownerId==body.ownerId,"split outputs keep target owner");
    check(lower.summary["valid"].toBool() && upper.summary["valid"].toBool()
        && lower.summary["closed"].toBool() && upper.summary["closed"].toBool(),"two valid closed solids");
    check(std::abs(lower.summary["volumeMm3"].toDouble()-80000)<1e-5
        && std::abs(upper.summary["volumeMm3"].toDouble()-80000)<1e-5,"actual equal volume partition");
    BRepAlgoAPI_Common overlap(lower.shape,upper.shape);overlap.Build();check(overlap.IsDone(),"overlap intersection evaluates");
    GProp_GProps overlapMass;BRepGProp::VolumeProperties(overlap.Shape(),overlapMass);
    check(std::abs(overlapMass.Mass())<1e-6,"split halves have no shared interior");
    check(upper.featureId==split["id"].toString(),"new positive body belongs to split feature");
    auto cap=[&](const EvaluatedBody& half){for(const auto& face:half.supportFaces)if(face.role.startsWith("split."))return face;return EvaluatedSupportFace{};};
    check(!cap(lower).faceId.isEmpty() && !cap(upper).faceId.isEmpty(),"both retained cap supports");
    check(cap(lower).boundaryEdges.size()==4 && cap(upper).boundaryEdges.size()==4,"four exact cap boundaries per half");
    const auto resolvedLower=GeometryKernel::resolveSketchSupport(doc.evaluation(),source(cap(lower)));
    const auto resolvedUpper=GeometryKernel::resolveSketchSupport(doc.evaluation(),source(cap(upper)));
    check(resolvedLower.bodyId==lower.bodyId && resolvedUpper.bodyId==upper.bodyId,"both caps support downstream sketches");
    const auto authored=doc.authored();doc.history().undo();check(doc.authored()==before,"undo returns unsplit source");
    doc.history().redo();check(doc.authored()==authored,"redo restores two-body feature");
    auto resize=authored;features=resize["features"].toArray();auto changed=features[0].toObject();auto p=changed["parameters"].toObject();p["heightMm"]=50;changed["parameters"]=p;features[0]=changed;resize["features"]=features;
    check(doc.commit(resize,"Resize case").isEmpty(),qPrintable(doc.lastEvaluationError()["message"].toString()));
    check(doc.bodies().size()==2 && doc.bodies()[1].bodyId==positive,"output identities survive upstream resize");
    check(cap(doc.bodies()[0]).faceId==cap(lower).faceId && cap(doc.bodies()[1]).faceId==cap(upper).faceId,"cap IDs survive upstream resize");
    check(std::abs(doc.bodies()[0].summary["volumeMm3"].toDouble()-100000)<1e-5
        && std::abs(doc.bodies()[1].summary["volumeMm3"].toDouble()-100000)<1e-5,"midplane tracks case height");
    auto editedPlane=doc.authored();features=editedPlane["features"].toArray();auto plane=features[1].toObject();
    plane["parameters"]=QJsonObject{{"source",QJsonObject{{"plane","XY"}}},{"offsetMm",18}};
    features[1]=plane;editedPlane["features"]=features;
    check(doc.commit(editedPlane,"Move split plane").isEmpty(),"retained plane edit recomputes split");
    check(std::abs(doc.bodies()[0].summary["volumeMm3"].toDouble()-72000)<1e-5
        && std::abs(doc.bodies()[1].summary["volumeMm3"].toDouble()-128000)<1e-5,"plane edit partitions same source at new height");
    const auto stable=doc.authored();auto invalid=stable;features=invalid["features"].toArray();plane=features[1].toObject();
    plane["parameters"]=QJsonObject{{"source",QJsonObject{{"plane","XY"}}},{"offsetMm",60}};features[1]=plane;invalid["features"]=features;
    const auto revision=doc.revision();check(!doc.commit(invalid,"Miss case").isEmpty() && doc.authored()==stable && doc.revision()==revision,"missed split refuses atomically");
    QTemporaryDir temp;const auto path=temp.filePath("split.alloy");check(doc.save(path).isEmpty(),"save split");
    CadDocument reopened;check(reopened.open(path).isEmpty() && reopened.authored()==stable,"exact authored reopen");
    check(reopened.bodies().size()==2 && reopened.bodies()[1].bodyId==positive,"reopen retains both output IDs");
    CadDocument enclosed;const auto stock2=box(enclosed);const auto original=enclosed.bodies()[0];
    const auto shellCandidate=CadDocument::shellCandidate(enclosed.authored(),{{"targetBodyId",original.bodyId},{"thicknessMm",3.5}});
    check(enclosed.commit(shellCandidate,"Closed Shell").isEmpty(),"closed cavity shell commit");
    const auto cavity=enclosed.bodies()[0];check(cavity.summary["topology"].toObject()["shells"].toInt()==2,"source is a closed cavity");
    QJsonObject cavityTop,cavityBottom;for(const auto& face:cavity.supportFaces){
        if(face.role=="box.z1")cavityTop=source(face);if(face.role=="box.z0")cavityBottom=source(face);
    }
    check(!cavityTop.isEmpty()&&!cavityBottom.isEmpty(),"closed cavity keeps outer planar supports");
    const QJsonObject cavityMid{{"id",CadDocument::newId()},{"ownerId",enclosed.authored()["documentId"]},
        {"type","constructionPlane"},{"name","Cavity Midplane"},{"visible",true},
        {"parameters",QJsonObject{{"source",QJsonObject{{"faces",QJsonArray{cavityTop,cavityBottom}}}}}}};
    auto cavityCandidate=enclosed.authored();auto cavityFeatures=cavityCandidate["features"].toArray();cavityFeatures.append(cavityMid);
    cavityCandidate["features"]=cavityFeatures;check(enclosed.commit(cavityCandidate,"Cavity Midplane").isEmpty(),"closed cavity Midplane");
    cavityCandidate=CadDocument::splitBodyCandidate(enclosed.authored(),{{"targetBodyId",cavity.bodyId},{"planeId",cavityMid["id"]}});
    check(enclosed.commit(cavityCandidate,"Split closed cavity").isEmpty(),qPrintable(enclosed.lastEvaluationError()["message"].toString()));
    check(enclosed.bodies().size()==2,"closed cavity produces two solids");
    const double originalVolume=cavity.summary["volumeMm3"].toDouble();
    check(std::abs(enclosed.bodies()[0].summary["volumeMm3"].toDouble()+enclosed.bodies()[1].summary["volumeMm3"].toDouble()-originalVolume)<1e-5,
        "closed cavity halves conserve material volume");
    check(cap(enclosed.bodies()[0]).boundaryEdges.size()==8 && cap(enclosed.bodies()[1]).boundaryEdges.size()==8,
        "closed cavity annular split caps retain outer and inner boundaries");
    CadDocument retarget;const auto first=box(retarget);auto two=retarget.authored();auto twoFeatures=two["features"].toArray();
    auto second=first;second["id"]=CadDocument::newId();second["bodyId"]=CadDocument::newId();second["name"]="Second case";
    auto secondParameters=second["parameters"].toObject();secondParameters["xMm"]=70;second["parameters"]=secondParameters;
    twoFeatures.append(second);two["features"]=twoFeatures;check(retarget.commit(two,"Add second case").isEmpty(),"second candidate target");
    QJsonObject firstTop,firstBottom;for(const auto& face:retarget.bodies()[0].supportFaces){
        if(face.role=="box.z1")firstTop=source(face);if(face.role=="box.z0")firstBottom=source(face);
    }
    auto secondPlane=mid;secondPlane["id"]=CadDocument::newId();secondPlane["ownerId"]=retarget.authored()["documentId"];
    secondPlane["parameters"]=QJsonObject{{"source",QJsonObject{{"faces",QJsonArray{firstTop,firstBottom}}}}};
    two=retarget.authored();twoFeatures=two["features"].toArray();twoFeatures.append(secondPlane);two["features"]=twoFeatures;
    check(retarget.commit(two,"Midplane for both").isEmpty(),qPrintable(retarget.lastEvaluationError()["message"].toString()));
    const auto firstSplit=CadDocument::splitBodyCandidate(retarget.authored(),{{"targetBodyId",first["bodyId"]},{"planeId",secondPlane["id"]}});
    check(retarget.commit(firstSplit,"Split first").isEmpty(),"initial split target");
    const auto firstSplitFeature=firstSplit["features"].toArray().last().toObject();
    const auto swapped=CadDocument::splitBodyCandidate(retarget.authored(),{{"featureId",firstSplitFeature["id"]},
        {"targetBodyId",second["bodyId"]},{"planeId",secondPlane["id"]}});
    const auto swappedFeature=swapped["features"].toArray().last().toObject();
    check(swappedFeature["id"]==firstSplitFeature["id"] && swappedFeature["bodyId"]!=firstSplitFeature["bodyId"]
        && swappedFeature["bodyId"]==SolidSplit::positiveBodyId(swappedFeature["id"].toString(),second["bodyId"].toString()),
        "changing target recomputes source-derived output identity");
    check(retarget.commit(swapped,"Retarget split").isEmpty() && retarget.bodies().size()==3,"retargeted split evaluates both outputs");
    std::cout<<"PASS: retained Midplane yields two closed halves, exact volumes, caps, edit/history/reopen\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
