#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>

static void check(bool okay,const char* message){if(!okay)throw std::runtime_error(message);}
static QJsonObject ref(const EvaluatedSupportFace& face) {
    return {{"producerFeatureId",face.producerFeatureId},{"bodyId",face.bodyId},{"faceId",face.faceId}};
}
int main(int argc,char** argv) {
    QCoreApplication app(argc,argv);
    try {
        CadDocument document;
        auto box=QJsonObject{{"id",CadDocument::newId()},{"bodyId",CadDocument::newId()},
            {"ownerId",document.authored()["documentId"]},{"type","box"},{"name","Box"},{"visible",true},
            {"parameters",QJsonObject{{"widthMm",50},{"depthMm",80},{"heightMm",40},
                {"xMm",0},{"yMm",0},{"zMm",0}}}};
        auto candidate=document.authored();candidate["features"]=QJsonArray{box};
        check(document.commit(candidate,"Box").isEmpty(),"box commit");
        auto body=document.bodies().at(0);
        check(body.supportFaces.size()==6 && body.solidEdges.size()==12,"six faces and twelve edges");
        check(body.summary["supportFaces"].toArray().size()==6 && body.summary["solidEdges"].toArray().size()==12,
            "shared inspection topology");
        const auto top=std::find_if(body.supportFaces.begin(),body.supportFaces.end(),[](const auto& face){return face.role=="box.z1";});
        check(top!=body.supportFaces.end() && top->boundaryEdges.size()==4,"named top face and four real edges");
        check(GeometryKernel::resolveSketchSupport(document.evaluation(),ref(*top)).face.IsSame(top->face),"resolved top face");
        const auto sourceFace=ref(*top);
        const auto sourceEdge=top->boundaryEdges.at(0).edgeId;
        auto sketch=SketchGeometry::newSupportedFeature(document.authored()["documentId"].toString(),sourceFace,"Top",{});
        check(document.commit(SketchGeometry::replaceFeature(document.authored(),sketch),"Top sketch").isEmpty(),
            "box top supports sketch");
        check(GeometryKernel::projectionContext(document.evaluation(),sketch)["lines"].toArray().isEmpty(),
            "unselected edges are not projected");
        check(document.commit(CadDocument::shellCandidate(document.authored(),{{"targetBodyId",body.bodyId},
            {"removedFace",sourceFace},{"thicknessMm",4}}),"Shell").isEmpty(),"top-face Shell4");
        body=document.bodies().at(0);
        check(body.summary["valid"].toBool() && body.summary["closed"].toBool(),"closed shell");
        check(std::abs(body.summary["volumeMm3"].toDouble()-(50*80*40-42*72*36))<1e-3,"box shell volume");
        check(GeometryKernel::resolveSketchSupport(document.evaluation(),sourceFace).role=="shell-rim", "stable opening face");
        const auto inner=std::find_if(body.supportFaces.begin(),body.supportFaces.end(),[](const auto& face){
            return face.role=="shell-inner" && face.provenance["boxFace"]=="y1";
        });
        check(inner!=body.supportFaces.end(),"named inner back face");
        check(std::abs(inner->frame["origin"].toArray()[1].toDouble()-76)<1e-5
            && inner->frame["normal"].toArray()[1].toDouble()==-1,"back wall at y76 normal -Y");
        check(inner->boundaryEdges.size()==4,"four retained straight inner edges");
        const auto innerSketch=SketchGeometry::newSupportedFeature(document.authored()["documentId"].toString(),
            ref(*inner),"Inner back",GeometryKernel::supportEdgeDescriptors(*inner));
        check(GeometryKernel::projectionContext(document.evaluation(),innerSketch)["lines"].toArray().size()==4,
            "inner back line projections resolve from actual edges");
        const auto front=std::find_if(body.supportFaces.begin(),body.supportFaces.end(),[](const auto& face){return face.role=="box.y0";});
        check(front!=body.supportFaces.end() && front->boundaryEdges.size()==4,"front outer face and boundary survive Shell");
        int innerFaces=0;
        for(const auto& face:body.supportFaces)if(face.role=="shell-inner") {
            ++innerFaces;check(face.boundaryEdges.size()==4,"inner face has four retained straight boundaries");
        }
        check(innerFaces==5,"five inner planar faces");
        const auto innerId=inner->faceId,innerEdge=inner->boundaryEdges.at(0).edgeId;
        const auto innerSupport=ref(*inner);
        auto shellFeature=document.authored()["features"].toArray().last().toObject();
        auto args=shellFeature["parameters"].toObject();args["featureId"]=shellFeature["id"];args["thicknessMm"]=3;
        check(document.commit(CadDocument::shellCandidate(document.authored(),args),"Edit Shell").isEmpty(),"shell edit");
        body=document.bodies().at(0);
        const auto editedInner=std::find_if(body.supportFaces.begin(),body.supportFaces.end(),[&](const auto& face){return face.faceId==innerId;});
        check(editedInner!=body.supportFaces.end() && editedInner->boundaryEdges.size()==4,"inner face survives edit");
        check(std::abs(editedInner->frame["origin"].toArray()[1].toDouble()-77)<1e-5,
            "back wall tracks thickness edit");
        check(std::any_of(editedInner->boundaryEdges.begin(),editedInner->boundaryEdges.end(),[&](const auto& edge){return edge.edgeId==innerEdge;}),"inner edge identity survives edit");
        auto resized=box["parameters"].toObject();resized["widthMm"]=54;resized["depthMm"]=90;
        auto revised=document.authored();auto features=revised["features"].toArray();
        auto first=features[0].toObject();first["parameters"]=resized;features[0]=first;revised["features"]=features;
        check(document.commit(revised,"Resize Box").isEmpty(),"upstream box edit");
        body=document.bodies().at(0);
        check(std::abs(body.summary["volumeMm3"].toDouble()-(54*90*40-48*84*37))<1e-3,
            "upstream box and shell recompute");
        check(GeometryKernel::resolveSketchSupport(document.evaluation(),sourceFace).faceId==sourceFace["faceId"],
            "top support identity survives box edit");
        const auto resizedInner=std::find_if(body.supportFaces.begin(),body.supportFaces.end(),[&](const auto& face){return face.faceId==innerId;});
        check(resizedInner!=body.supportFaces.end() && resizedInner->boundaryEdges.size()==4,
            "inner back support survives upstream edit");
        check(std::abs(resizedInner->frame["origin"].toArray()[1].toDouble()-87)<1e-5,
            "inner back wall follows depth edit");
        check(std::any_of(resizedInner->boundaryEdges.begin(),resizedInner->boundaryEdges.end(),[&](const auto& edge){return edge.edgeId==innerEdge;}),
            "inner edge identity survives upstream edit");
        const auto stable=document.authored();document.history().undo();document.history().redo();
        check(document.authored()==stable,"shell edit exact undo redo");
        QTemporaryDir directory;check(directory.isValid(),"temp directory");
        const auto path=directory.filePath("box-shell.alloy");
        check(document.save(path).isEmpty(),"save");CadDocument restored;
        check(restored.open(path).isEmpty() && restored.authored()==stable,"reopen");
        check(GeometryKernel::resolveSketchSupport(restored.evaluation(),innerSupport).faceId==innerId,"reopened inner face");
        const auto before=restored.authored();auto bad=sourceFace;bad["faceId"]=CadDocument::newId();
        check(!restored.commit(CadDocument::shellCandidate(before,{{"targetBodyId",body.bodyId},
            {"removedFace",bad},{"thicknessMm",3},{"featureId",shellFeature["id"]}}),"Bad Shell").isEmpty(),
            "missing face refused");
        check(restored.authored()==before,"refusal atomic");
        check(!sourceEdge.isEmpty(),"outer edge identity");
        std::cout<<"PASS: real box faces/edges, top Shell, inner lines, edit and storage\n";
        return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
