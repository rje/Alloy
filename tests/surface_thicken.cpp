// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "MeshExport.h"
#include "SolidFillet.h"
#include <BRepAdaptor_Surface.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepOffsetAPI_MakeThickSolid.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <QCoreApplication>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool v,const char* message){if(!v)throw std::runtime_error(message);}
QJsonObject ref(const QJsonObject& authored,const QString& sketchName){
    for(const auto& row:authored["features"].toArray()){
        const auto f=row.toObject();if(f["type"]!="sketch" || f["name"]!=sketchName)continue;
        const auto entities=f["parameters"].toObject()["entities"].toArray();
        check(entities.size()==1,"Expected one retained source entity");
        return {{"sketchId",f["id"]},{"entityId",entities[0].toObject()["id"]}};
    }
    throw std::runtime_error("Source sketch unavailable");
}
QJsonArray faceIds(const EvaluatedBody& body){QJsonArray ids;for(const auto& face:body.supportFaces)ids.append(face.faceId);return ids;}
QJsonArray edgeIds(const EvaluatedBody& body){QJsonArray ids;for(const auto& edge:body.solidEdges)ids.append(edge.edgeId);return ids;}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    check(argc==2,"Pass the actual pre-Loft document path");CadDocument document;
    check(document.open(QString::fromLocal8Bit(argv[1])).isEmpty(),"Open source sketches");
    const auto loft=CadDocument::loftCandidate(document.authored(),{{"operation","newComponent"},{"componentName","Shoehorn"},
        {"sections",QJsonArray{ref(document.authored(),"Arc"),ref(document.authored(),"Line")}},
        {"centerline",ref(document.authored(),"Path")}});
    check(document.commit(loft,"Surface Loft").isEmpty(),"Commit source sheet");
    const auto sheet=document.bodies().back();const QJsonObject face{{"bodyId",sheet.bodyId},{"faceId",sheet.supportFaces[0].faceId},
        {"producerFeatureId",sheet.featureId}};
    auto candidate=CadDocument::thickenCandidate(document.authored(),{{"sourceFace",face},{"signedThicknessMm",3.0}});
    const auto preview=CadDocument::evaluateCandidate(candidate);
    if(!preview.ok)throw std::runtime_error(("Thicken preview: "+preview.error).toStdString());
    check(preview.evaluation->bodies.size()==2 && !preview.evaluation->bodies.front().visible,
        "New Body retains but hides its source sheet");
    check(document.commit(candidate,"Thicken").isEmpty(),"Commit Thicken");
    const auto solid=document.bodies().back();
    check(document.bodies().size()==2 && document.bodies().front().bodyId==sheet.bodyId
        && !document.bodies().front().visible && solid.ownerId==document.authored()["documentId"]
        && solid.summary["type"]=="solid" && solid.summary["closed"]==true
        && solid.summary["valid"]==true && solid.summary["topology"].toObject()["solids"]==1
        && solid.supportFaces.size()==6 && solid.solidEdges.size()==12,
        "Closed root solid, complete topology and retained hidden component sheet");
    document.history().undo();check(document.bodies().size()==1 && document.bodies().front().visible,
        "Undo New Body restores visible Loft sheet");document.history().redo();
    auto shown=document.authored();auto shownFeatures=shown["features"].toArray();
    for(int i=0;i<shownFeatures.size();++i)if(shownFeatures[i].toObject()["id"]==sheet.featureId){
        auto loftFeature=shownFeatures[i].toObject();loftFeature["visible"]=true;shownFeatures[i]=loftFeature;
    }
    shown["features"]=shownFeatures;
    check(document.commit(shown,"Show retained sheet").isEmpty() && document.bodies().front().visible,
        "Retained source sheet could not be shown again");
    document.history().undo();check(!document.bodies().front().visible,"Undo visibility must restore hidden sheet");
    const auto solidId=solid.bodyId,sourceId=sheet.bodyId;
    const auto firstFaces=faceIds(solid),firstEdges=edgeIds(solid);const double volume=solid.summary["volumeMm3"].toDouble();
    check(volume>10000 && firstFaces.size()==6,"Thicken volume and retained faces");
    check(solid.shape.Orientation()==TopAbs_FORWARD && BRepCheck_Analyzer(solid.shape).IsValid(),
        "Positive Thicken material must use a valid forward solid wrapper");
    BRepOffsetAPI_MakeThickSolid sourceOffset;sourceOffset.MakeThickSolidBySimple(sheet.shape,3.0);
    check(sourceOffset.IsDone(),"Source offset unavailable for orientation comparison");
    auto previousWrapper=sourceOffset.Shape();previousWrapper.Reverse();
    BRepAlgoAPI_Cut oldMinusNew(previousWrapper,solid.shape),newMinusOld(solid.shape,previousWrapper);
    oldMinusNew.Build();newMinusOld.Build();
    check(oldMinusNew.IsDone() && newMinusOld.IsDone(),"Material equivalence check failed");
    GProp_GProps removed,added;BRepGProp::VolumeProperties(oldMinusNew.Shape(),removed,true);
    BRepGProp::VolumeProperties(newMinusOld.Shape(),added,true);
    check(std::abs(removed.Mass())<1e-6 && std::abs(added.Mass())<1e-6,
        "Forward wrapper changed the physical Thicken material");
    QString corner;
    for(const auto& edge:solid.solidEdges)if(edge.provenance["role"].toString().startsWith("corner/")){
        corner=edge.edgeId;break;
    }
    check(!corner.isEmpty(),"Retained Thicken corner unavailable");
    auto rounded=solid;
    SolidFillet::evaluate(rounded,{{"radiusMm",1.0},{"edgeIds",QJsonArray{corner}}},CadDocument::newId());
    check(rounded.summary["valid"]==true && rounded.summary["closed"]==true
        && rounded.summary["volumeMm3"].toDouble()>0 && rounded.summary["volumeMm3"].toDouble()<volume,
        "Downstream corner Fillet must produce valid removed material");
    for(const auto& support:solid.supportFaces)if(support.role.startsWith("side/")){
        if(BRepAdaptor_Surface(support.face).GetType()!=GeomAbs_Plane)continue;
        check(support.frame.contains("origin") && support.frame.contains("uAxis")
            && support.frame.contains("vAxis") && support.frame.contains("normal"),
            "Planar side has a truthful attachment frame");
    }
    auto edited=CadDocument::thickenCandidate(document.authored(),{{"featureId",solid.featureId},{"signedThicknessMm",-3.0}});
    check(document.commit(edited,"Reverse Thicken").isEmpty(),"Signed side edit");
    check(document.bodies().back().bodyId==solidId && faceIds(document.bodies().back())==firstFaces
        && edgeIds(document.bodies().back())==firstEdges
        && document.bodies().back().shape.Orientation()==TopAbs_FORWARD
        && BRepCheck_Analyzer(document.bodies().back().shape).IsValid()
        && std::abs(document.bodies().back().summary["volumeMm3"].toDouble()-volume)>100,
        "Signed side edit did not retain identity and change geometry");
    document.history().undo();check(document.bodies().back().summary["signedThicknessMm"]==3.0,"Undo signed edit");
    document.history().redo();check(document.bodies().back().summary["signedThicknessMm"]==-3.0,"Redo signed edit");
    QTemporaryDir directory;const auto path=directory.filePath("thickened.alloy");check(document.save(path).isEmpty(),"Save solid");
    CadDocument reopened;check(reopened.open(path).isEmpty() && reopened.bodies().size()==2
        && !reopened.bodies().front().visible
        && reopened.bodies().back().bodyId==solidId && faceIds(reopened.bodies().back())==firstFaces,
        "Reopen retained solid");
    const auto mesh=MeshExport::writeStl(reopened.bodies().back(),directory.filePath("shoehorn-mm.stl"),"mm",true,0.1,10);
    check(mesh["bodyCount"]==1,"Strict solid STL export");
    const auto predecessor=reopened.evaluationBefore(solid.featureId);
    check(predecessor.bodies.size()==1 && predecessor.bodies[0].bodyId==sourceId
        && predecessor.bodies[0].summary["type"]=="surface","Prefix did not recover original sheet");
    auto pathEdit=reopened.authored();auto features=pathEdit["features"].toArray();
    for(int i=0;i<features.size();++i)if(features[i].toObject()["name"]=="Path"){
        auto sketch=features[i].toObject();auto parameters=sketch["parameters"].toObject();auto points=parameters["points"].toArray();
        const auto ids=parameters["entities"].toArray()[0].toObject()["pointIds"].toArray();
        for(int j=0;j<points.size();++j)if(points[j].toObject()["id"]==ids[1]){
            auto point=points[j].toObject();point["vMm"]=20.0;points[j]=point;
        }
        parameters["points"]=points;sketch["parameters"]=parameters;features[i]=sketch;
    }
    pathEdit["features"]=features;
    const double beforePath=reopened.bodies().back().summary["volumeMm3"].toDouble();
    check(reopened.commit(pathEdit,"Edit source path").isEmpty(),"Source spline edit through Thicken");
    check(reopened.bodies().back().bodyId==solidId && faceIds(reopened.bodies().back())==firstFaces
        && edgeIds(reopened.bodies().back())==firstEdges
        && std::abs(reopened.bodies().back().summary["volumeMm3"].toDouble()-beforePath)>100,
        "Thicken did not follow source path with stable solid topology IDs");
    CadDocument named;check(named.open(QString::fromLocal8Bit(argv[1])).isEmpty(),"Open named-sheet source");
    const auto namedLoft=CadDocument::loftCandidate(named.authored(),{{"operation","newComponent"},{"componentName","Shoehorn"},
        {"sections",QJsonArray{ref(named.authored(),"Arc"),ref(named.authored(),"Line")}},
        {"centerline",ref(named.authored(),"Path")}});
    check(named.commit(namedLoft,"Surface Loft").isEmpty(),"Build named source");
    const auto namedSheet=named.bodies().back();
    check(named.commit(CadDocument::renameBodyCandidate(named.authored(),named.evaluation(),namedSheet.bodyId,"Named sheet"),"Name sheet").isEmpty(),"Name source sheet");
    const QJsonObject namedFace{{"bodyId",namedSheet.bodyId},{"faceId",namedSheet.supportFaces[0].faceId},
        {"producerFeatureId",namedSheet.featureId}};
    const auto namedCandidate=CadDocument::thickenCandidate(named.authored(),{{"sourceFace",namedFace},{"signedThicknessMm",3.0}});
    check(named.commit(namedCandidate,"Thicken named sheet").isEmpty() && named.bodies().size()==2
        && !named.bodies().front().visible && named.bodies().front().name=="Named sheet",
        "Named source sheet was not retained with its stable name");
    std::cout<<"SURFACE_THICKEN_OK signed=3 volume="<<volume<<" body="<<solidId.toStdString()<<"\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
