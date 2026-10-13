#include "CadDocument.h"
#include "SketchGeometry.h"
#include "SolidFillet.h"
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <QCoreApplication>
#include <QTemporaryDir>
#include <iostream>
#include <stdexcept>
#include <cmath>
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try {
    CadDocument doc;auto feature=SketchGeometry::newFeature(doc.authored()["documentId"].toString(),"XY");feature=SketchGeometry::addRectangle(feature,{{"u1Mm",29},{"v1Mm",0},{"u2Mm",32},{"v2Mm",-10}});
    const auto sketch=SketchGeometry::evaluateFeature(feature);check(sketch.profiles.size()==1,"one profile");const auto& profile=sketch.profiles[0];const auto generator=CadDocument::newId();
    auto build=[&](double height){BRepPrimAPI_MakePrism prism(profile.face,gp_Vec(0,0,height),true,true);auto edges=SolidFillet::prismEdges(prism,profile,generator);EvaluatedBody body{generator,CadDocument::newId(),"Prong",true,prism.Shape(),GeometryKernel::summarize(prism.Shape()),{},edges};return body;};
    auto body=build(24);const auto descriptors=SolidFillet::descriptors(body.solidEdges);check(descriptors.size()==12,"four top rims/four base rims/four generated corners");QJsonArray selected;
    for(const auto& value:descriptors){const auto edge=value.toObject(),p=edge["provenance"].toObject();const auto a=edge["startMm"].toArray(),b=edge["endMm"].toArray();if(std::abs(a[0].toDouble()-32)>1e-7 || std::abs(b[0].toDouble()-32)>1e-7)continue;
        if(p["role"]=="sideCorner" && (std::abs(a[1].toDouble()+10)<1e-7 || std::abs(a[1].toDouble())<1e-7))selected.append(edge["edgeId"]);
    }
    check(selected.size()==2,"video two vertical corner provenance");const auto original=body;SolidFillet::evaluate(body,{{"edgeIds",selected},{"radiusMm",1}},CadDocument::newId());check(body.summary["valid"].toBool() && body.summary["volumeMm3"].toDouble()<720,"real rounded prong");
    auto resized=build(26);for(const auto& id:selected){bool found=false;for(const auto& edge:resized.solidEdges)found|=edge.edgeId==id.toString();check(found,"height edit retains edge identity");}SolidFillet::evaluate(resized,{{"edgeIds",selected},{"radiusMm",.5}},CadDocument::newId());check(resized.summary["valid"].toBool(),"edited radius/height valid");
    auto impossible=original;bool rejected=false;try{SolidFillet::evaluate(impossible,{{"edgeIds",selected},{"radiusMm",10}},CadDocument::newId());}catch(const std::exception&){rejected=true;}check(rejected && impossible.shape.IsSame(original.shape),"impossible radius atomic");
    // A descriptor scattered into another result may be unavailable there.
    // Its later merge must recover the unique live descendant independent of order.
    auto missing=original.solidEdges.front();missing.edge.Nullify();missing.provenance["resolutionError"]="deleted";
    BRepAlgoAPI_Fuse same(original.shape,original.shape);same.Build();auto forward=SolidFillet::transferEdges(same,{missing,original.solidEdges.front()},same.Shape());auto reverse=SolidFillet::transferEdges(same,{original.solidEdges.front(),missing},same.Shape());
    check(forward.size()==1 && reverse.size()==1 && !forward[0].edge.IsNull() && forward[0].edge.IsSame(reverse[0].edge),"merged live descriptor wins over other-result absence independent of order");
    missing.provenance["resolutionError"]="ambiguous_split";auto ambiguous=SolidFillet::transferEdges(same,{missing,original.solidEdges.front()},same.Shape());check(ambiguous[0].edge.IsNull() && ambiguous[0].provenance["resolutionError"]=="ambiguous_split","live candidate cannot clear established ambiguity");
    auto deleted=original;deleted.solidEdges[0].edge.Nullify();deleted.solidEdges[0].provenance["resolutionError"]="deleted";bool unavailable=false;try{SolidFillet::evaluate(deleted,{{"edgeIds",QJsonArray{deleted.solidEdges[0].edgeId}},{"radiusMm",1}},CadDocument::newId());}catch(const GeometryKernel::EvaluationError& e){unavailable=e.diagnostics()["code"]=="fillet_edge_deleted";}check(unavailable,"specific deleted edge diagnostic");
    if(argc>1) {
        CadDocument source;check(source.open(QString::fromLocal8Bit(argv[1])).isEmpty(),"open joined support fixture");check(source.bodies().size()==1,"joined source one body");
        const auto before=source.authored();const auto target=source.bodies()[0].bodyId;QJsonArray ids;QString generator;
        for(const auto& value:SolidFillet::descriptors(source.bodies()[0].solidEdges)) {
            const auto edge=value.toObject(),p=edge["provenance"].toObject();if(!edge["available"].toBool())continue;
            const auto a=edge["startMm"].toArray(),b=edge["endMm"].toArray();if(std::abs(a[0].toDouble()-32)>1e-6 || std::abs(b[0].toDouble()-32)>1e-6)continue;
            if(p["role"]=="sideCorner" && (std::abs(a[1].toDouble()+10)<1e-6 || std::abs(a[1].toDouble())<1e-6) && std::max(a[2].toDouble(),b[2].toDouble())>23.9) {ids.append(edge["edgeId"]);generator=p["generatorFeatureId"].toString();}
        }
        check(ids.size()==2,"video joined two vertical corner seeds");QJsonObject support;
        for(const auto& cap:source.bodies()[0].supportFaces)if(std::abs(cap.frame["origin"].toArray()[2].toDouble()-20)<1e-6)support={{"bodyId",cap.bodyId},{"faceId",cap.faceId},{"producerFeatureId",cap.producerFeatureId}};
        check(!support.isEmpty(),"original disk cap20");QJsonObject args{{"targetBodyId",target},{"edgeIds",ids},{"radiusMm",1}};
        check(source.commit(CadDocument::filletCandidate(before,args),"Source Fillet").isEmpty(),"joined source fillet commit");auto filleted=source.authored();const auto featureId=filleted["features"].toArray().last().toObject()["id"].toString();
        check(source.bodies()[0].bodyId==target && source.bodies()[0].summary["tangentContours"].toArray().size()>0,"retained body/actual contours");
        source.history().undo();check(source.authored()==before,"fillet one undo");source.history().redo();check(source.authored()==filleted,"fillet exact redo");
        args["featureId"]=featureId;args["radiusMm"]=.5;check(source.commit(CadDocument::filletCandidate(filleted,args),"Edit radius").isEmpty(),"fillet radius edit");source.history().undo();
        QJsonObject upstream;for(const auto& entry:before["features"].toArray())if(entry.toObject()["id"].toString()==generator){upstream=entry.toObject()["parameters"].toObject();upstream["featureId"]=generator;break;}upstream["distanceMm"]=26;
        check(source.commit(CadDocument::extrusionCandidate(filleted,upstream),"Edit outer height").isEmpty(),"source upstream height retains provenance");source.history().undo();
        auto attached=SketchGeometry::newSupportedFeature(source.authored()["documentId"].toString(),support,"Original disk hole");attached=SketchGeometry::addCircle(attached,{{"centerUMm",7.5},{"centerVMm",-5},{"diameterMm",5.2}});
        check(source.commit(SketchGeometry::replaceFeature(source.authored(),attached),"Disk supported circle").isEmpty(),"original disk attachment after fillet");check(source.sketches().back().resolvedFrame["origin"].toArray()[2].toDouble()==20,"circle remains disk20");
        const auto stable=source.authored();const auto revision=source.revision();args["radiusMm"]=10;check(!source.commit(CadDocument::filletCandidate(stable,args),"Impossible radius").isEmpty() && source.authored()==stable && source.revision()==revision,"joined impossible radius atomic");
        QTemporaryDir tmp;const auto path=tmp.filePath("support-filleted.alloy");check(source.save(path).isEmpty(),"fillet save");CadDocument reopened;check(reopened.open(path).isEmpty() && reopened.authored()==stable,"fillet exact reopen");
    }
    std::cout<<"Generated fillet provenance/radius/upstream/failure passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
