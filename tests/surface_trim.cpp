// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include "SketchSpline.h"
#include "SurfaceTrim.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <BRepTools.hxx>
#include <BRep_Tool.hxx>
#include <TopoDS.hxx>
#include <TopExp_Explorer.hxx>
#include <Prs3d_Drawer.hxx>
#include <StdPrs_ToolTriangulatedShape.hxx>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void append(CadDocument& document,const QJsonObject& feature){
    auto candidate=document.authored();auto rows=candidate["features"].toArray();rows.append(feature);candidate["features"]=rows;
    const auto error=document.commit(candidate,"Add source");if(!error.isEmpty())throw std::runtime_error(error.toStdString());
}
QJsonObject editPoint(const QJsonObject& authored,const QString& sketchId,const QString& pointId,double value){
    auto candidate=authored;auto rows=candidate["features"].toArray();
    for(int i=0;i<rows.size();++i)if(rows[i].toObject()["id"]==sketchId){
        auto sketch=rows[i].toObject();auto p=sketch["parameters"].toObject();auto points=p["points"].toArray();
        for(int j=0;j<points.size();++j)if(points[j].toObject()["id"]==pointId){auto point=points[j].toObject();point["vMm"]=value;points[j]=point;}
        p["points"]=points;sketch["parameters"]=p;rows[i]=sketch;break;
    }
    candidate["features"]=rows;return candidate;
}
}

void crossingSheetCheck(const QString& fixturePath){
    CadDocument doc;const auto owner=doc.authored()["documentId"].toString();
    auto makeSheet=[&](const QString& plane,double u1,double v1,double u2,double v2,double reach,const QString& name){
        auto sketch=SketchGeometry::newFeature(owner,plane,name+" source");
        sketch=SketchGeometry::addLine(sketch,{{"u1Mm",u1},{"v1Mm",v1},{"u2Mm",u2},{"v2Mm",v2}});append(doc,sketch);
        const auto entity=sketch["parameters"].toObject()["entities"].toArray()[0].toObject()["id"];
        const auto candidate=CadDocument::surfaceExtrudeCandidate(doc.authored(),{{"sketchId",sketch["id"]},
            {"entityIds",QJsonArray{entity}},{"sideOneMm",reach},{"sideTwoMm",reach},{"name",name}});
        check(doc.commit(candidate,"Crossing sheet").isEmpty(),"Crossing-sheet fixture failed");return sketch;
    };
    const auto targetSketch=makeSheet("XY",-20,0,20,0,10,"Target sheet");const auto target=doc.bodies().back();
    const auto toolSketch=makeSheet("XZ",-30,0,30,0,20,"Tool sheet");const auto tool=doc.bodies().back();
    const auto before=doc.authored();const auto toolArea=tool.summary["areaMm2"].toDouble();
    if(!fixturePath.isEmpty())check(doc.save(fixturePath).isEmpty(),"Save crossing-sheet source fixture");
    const auto id=CadDocument::newId();const QJsonObject p{{"targetBodyId",target.bodyId},{"toolBodyId",tool.bodyId},
        {"projectionMode","surfaceIntersection"},{"removedRegionIds",QJsonArray{}}};
    const QJsonObject feature{{"id",id},{"type","surfaceTrim"},{"parameters",p}};
    const auto split=SurfaceTrim::split(feature,target,doc.evaluation());
    check(split.regions.size()==2 && split.regions[0].side!=split.regions[1].side,"Two signed sheet regions unavailable");
    QSet<QString> regionIds,edgeIds;for(const auto& row:split.regions){regionIds.insert(row.regionId);
        check(!row.inside && row.removable() && std::abs(row.areaMm2-400)<1e-4,"Sheet region semantics/area");}
    for(const auto& edge:split.edges)edgeIds.insert(edge.edgeId);
    for(const auto& region:split.regions){
        auto candidate=CadDocument::surfaceTrimCandidate(before,{{"newFeatureId",id},{"targetBodyId",target.bodyId},
            {"toolBodyId",tool.bodyId},{"removedRegionIds",QJsonArray{region.regionId}}});
        const auto evaluation=CadDocument::evaluateCandidate(candidate);check(evaluation.ok,"Either sheet side must be removable");
    }
    const auto chosen=split.regions[0].regionId;
    auto candidate=CadDocument::surfaceTrimCandidate(before,{{"newFeatureId",id},{"targetBodyId",target.bodyId},
        {"toolBodyId",tool.bodyId},{"removedRegionIds",QJsonArray{chosen}}});
    check(doc.commit(candidate,"Trim planar sheet").isEmpty(),"Retained sheet-tool Trim commit");
    const auto trimmed=doc.authored();check(doc.bodies().size()==2,"Trim consumed tool");
    auto find=[&](const CadDocument& document,const QString& bodyId)->const EvaluatedBody&{
        for(const auto& body:document.bodies())if(body.bodyId==bodyId)return body;throw std::runtime_error("Body unavailable");};
    check(std::abs(find(doc,target.bodyId).summary["areaMm2"].toDouble()-400)<1e-4,"Trim kept wrong area");
    check(find(doc,tool.bodyId).supportFaces[0].faceId==tool.supportFaces[0].faceId
        && std::abs(find(doc,tool.bodyId).summary["areaMm2"].toDouble()-toolArea)<1e-6,"Tool changed");
    // Translate both retained tool-line endpoints in its sketch plane: stable
    // signed identities persist while the exact intersection moves by 2 mm.
    auto changed=doc.authored();const auto pointRows=toolSketch["parameters"].toObject()["points"].toArray();
    for(const auto& point:pointRows)changed=editPoint(changed,toolSketch["id"].toString(),point.toObject()["id"].toString(),2);
    check(doc.commit(changed,"Translate tool line").isEmpty(),"Tool translation edit failed");
    const auto edited=doc.authored();const auto keptId=find(doc,target.bodyId).supportFaces[0].faceId;
    check(regionIds.contains(keptId) && std::abs(find(doc,target.bodyId).summary["areaMm2"].toDouble()-400)>10,"Stable side under translation");
    for(const auto& edge:find(doc,target.bodyId).solidEdges)check(edgeIds.contains(edge.edgeId),"Stable boundary under translation");
    doc.history().undo();check(doc.authored()==trimmed,"Undo tool translation");
    doc.history().redo();check(doc.authored()==edited,"Redo tool translation");
    QTemporaryDir temporary;const auto path=temporary.filePath("sheet-trim.alloy");check(doc.save(path).isEmpty(),"Save sheet Trim");
    CadDocument reopened;check(reopened.open(path).isEmpty() && reopened.authored()==edited
        && find(reopened,target.bodyId).supportFaces[0].faceId==keptId,"Reopen retained sheet Trim");
    auto broken=edited;for(const auto& point:pointRows)broken=editPoint(broken,toolSketch["id"].toString(),point.toObject()["id"].toString(),30);
    check(!doc.commit(broken,"Move tool beyond target").isEmpty() && doc.authored()==edited,"No crossing must refuse atomically");
    doc.history().undo();doc.history().undo();check(doc.authored()==before,"Undo returns pre-trim branch");
    std::cout<<"SURFACE_SHEET_TRIM_OK kept=400 tool="<<toolArea<<'\n';
}

int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    crossingSheetCheck(argc>3?QString::fromLocal8Bit(argv[3]):QString{});
    CadDocument document;const auto root=document.authored()["documentId"].toString();
    auto back=SketchGeometry::newFeature(root,"XZ","Back open arc");
    back=SketchGeometry::addArc(back,{{"startUMm",-25},{"startVMm",0},{"throughUMm",0},{"throughVMm",10},
        {"endUMm",25},{"endVMm",0}});append(document,back);
    const auto arc=back["parameters"].toObject()["entities"].toArray()[0].toObject();
    const QJsonObject plane{{"id",CadDocument::newId()},{"ownerId",root},{"type","constructionPlane"},
        {"name","Lower section plane"},{"visible",true},
        {"parameters",QJsonObject{{"source",QJsonObject{{"plane","XZ"}}},{"offsetMm",-80.0}}}};
    append(document,plane);
    auto lower=SketchGeometry::newConstructionPlaneFeature(root,plane["id"].toString(),"Mirrored lower chain");
    lower=SketchGeometry::addLine(lower,{{"u1Mm",0},{"v1Mm",-5},{"u2Mm",-25},{"v2Mm",0}});
    lower=SketchGeometry::addLine(lower,{{"u1Mm",0},{"v1Mm",-5},{"u2Mm",25},{"v2Mm",0}});
    append(document,lower);
    const auto lowerEntities=lower["parameters"].toObject()["entities"].toArray();
    auto rail=SketchGeometry::newFeature(root,"XY","Exterior guide rail");
    const auto railResult=SketchSpline::add(rail,{{"points",QJsonArray{
        QJsonObject{{"uMm",-25},{"vMm",0}},QJsonObject{{"uMm",-31},{"vMm",40}},
        QJsonObject{{"uMm",-25},{"vMm",80}}}}});
    check(railResult.ok,"Guide rail sketch failed");rail=railResult.feature;append(document,rail);
    const auto railEntity=rail["parameters"].toObject()["entities"].toArray()[0].toObject();
    QJsonArray sections{QJsonObject{{"sketchId",back["id"]},{"entityIds",QJsonArray{arc["id"]}}},
        QJsonObject{{"sketchId",lower["id"]},{"entityIds",QJsonArray{lowerEntities[0].toObject()["id"],lowerEntities[1].toObject()["id"]}}}};
    const QJsonObject railRef{{"sketchId",rail["id"]},{"entityId",railEntity["id"]}};
    auto loft=CadDocument::loftCandidate(document.authored(),{{"operation","newComponent"},{"guideType","rail"},
        {"componentName","Iron upper"},{"sections",sections},{"rail",railRef}});
    check(document.commit(loft,"Outer sheet").isEmpty(),"Outer rail sheet failed");
    const auto source=document.bodies().back();const auto sourceArea=source.summary["areaMm2"].toDouble();
    auto tool=SketchGeometry::newFeature(root,"XZ","Paired handle splines");
    const auto upper=SketchSpline::add(tool,{{"points",QJsonArray{
        QJsonObject{{"uMm",-20},{"vMm",3}},QJsonObject{{"uMm",0},{"vMm",10}},
        QJsonObject{{"uMm",20},{"vMm",3}}}}});check(upper.ok,"Upper trim spline failed");tool=upper.feature;
    const auto lowerTool=SketchSpline::add(tool,{{"points",QJsonArray{
        QJsonObject{{"uMm",-20},{"vMm",3}},QJsonObject{{"uMm",0},{"vMm",0}},
        QJsonObject{{"uMm",20},{"vMm",3}}}}});check(lowerTool.ok,"Lower trim spline failed");tool=lowerTool.feature;
    append(document,tool);
    const auto entities=tool["parameters"].toObject()["entities"].toArray();
    const QJsonArray toolIds{entities[0].toObject()["id"],entities[1].toObject()["id"]};
    const auto featureId=CadDocument::newId();
    const QJsonObject feature{{"id",featureId},{"type","surfaceTrim"},{"ownerId",source.ownerId},
        {"name","Handle opening"},{"visible",true},{"parameters",QJsonObject{{"targetBodyId",source.bodyId},
            {"toolSketchId",tool["id"]},{"toolEntityIds",toolIds},{"removedRegionIds",QJsonArray{}},
            {"throughMarginMm",10.0},{"projectionMode","sketchNormalThroughTarget"},
            {"sourcePlane","XZ"},{"sourceNormal",QJsonArray{0,-1,0}}}}};
    auto triangulatedFaces=[](const TopoDS_Shape& shape){int count=0;TopLoc_Location location;
        for(TopExp_Explorer face(shape,TopAbs_FACE);face.More();face.Next())
            if(!BRep_Tool::Triangulation(TopoDS::Face(face.Current()),location).IsNull())++count;
        return count;
    };
    check(triangulatedFaces(source.shape)==0,"Fresh Trim source unexpectedly already tessellated");
    const auto split=SurfaceTrim::split(feature,source,document.evaluation());
    check(split.regions.size()==4,"Two separate inner region picks unavailable");
    QJsonArray removed;double removedArea=0;
    for(const auto& region:split.regions)if(region.inside){removed.append(region.regionId);removedArea+=region.areaMm2;}
    check(removed.size()==2 && removedArea>1000,"Expected two visible inner regions");
    // One immutable partition supports changing selections without repeating
    // intersections/classification or modifying its original source/regions.
    const auto partitionRegions=SurfaceTrim::descriptors(split);
    auto selectedFeature=feature;auto selectedParameters=feature["parameters"].toObject();
    selectedParameters["removedRegionIds"]=QJsonArray{removed[0]};selectedFeature["parameters"]=selectedParameters;
    auto oneRemoved=source;SurfaceTrim::applySelection(oneRemoved,selectedFeature,split);
    check(oneRemoved.summary["valid"].toBool() && oneRemoved.supportFaces.size()==3,
        "Prepared partition could not remove one inner region");
    selectedParameters["removedRegionIds"]=removed;selectedFeature["parameters"]=selectedParameters;
    auto bothRemoved=source;SurfaceTrim::applySelection(bothRemoved,selectedFeature,split);
    check(bothRemoved.summary["valid"].toBool() && bothRemoved.supportFaces.size()==2,
        "Prepared partition could not remove both inner regions");
    // Reproduce the live viewport's mesh on this private synchronous fixture.
    // Projection, regions and selected geometry must retain the exact canonical
    // unmeshed result, including lineage; never accept a wider area tolerance.
    Handle(Prs3d_Drawer) drawer=new Prs3d_Drawer;
    drawer->SetDeviationCoefficient(.0005);drawer->SetDeviationAngle(.10);
    StdPrs_ToolTriangulatedShape::Tessellate(source.shape,drawer);
    check(triangulatedFaces(source.shape)>0,"Viewport-parameter tessellation did not mesh the test source");
    const auto meshedSplit=SurfaceTrim::split(feature,source,document.evaluation());
    check(meshedSplit.projection==split.projection && SurfaceTrim::descriptors(meshedSplit)==partitionRegions,
        "Viewport tessellation changed Trim projection or exact region descriptors");
    auto meshedRemoved=source;SurfaceTrim::applySelection(meshedRemoved,selectedFeature,meshedSplit);
    check(meshedRemoved.summary==bothRemoved.summary,"Viewport tessellation changed selected Trim geometry or lineage");
    check(meshedRemoved.supportFaces.size()==bothRemoved.supportFaces.size() && meshedRemoved.solidEdges.size()==bothRemoved.solidEdges.size(),
        "Viewport tessellation changed named Trim topology");
    for(size_t i=0;i<bothRemoved.supportFaces.size();++i)check(meshedRemoved.supportFaces[i].faceId==bothRemoved.supportFaces[i].faceId,"Meshed Trim face identity changed");
    for(size_t i=0;i<bothRemoved.solidEdges.size();++i)check(meshedRemoved.solidEdges[i].edgeId==bothRemoved.solidEdges[i].edgeId,"Meshed Trim edge identity changed");
    std::cout<<"SURFACE_TRIM_MESH_INDEPENDENT_OK exactProjectionRegionsSelectedGeometryAndIDs\n";
    check(source.shape.IsSame(document.bodies().back().shape) && source.supportFaces.size()==2
        && SurfaceTrim::descriptors(split)==partitionRegions,"Selection mutated the source or partition");
    auto invalid=selectedParameters;invalid["removedRegionIds"]=QJsonArray{CadDocument::newId()};
    selectedFeature["parameters"]=invalid;auto refused=source;bool rejected=false;
    try{SurfaceTrim::applySelection(refused,selectedFeature,split);}catch(const GeometryKernel::EvaluationError& e){
        rejected=e.diagnostics()["code"]=="trim_region_unavailable";}
    check(rejected && refused.shape.IsSame(source.shape),"Unknown region must refuse without changing the source");
    auto args=QJsonObject{{"newFeatureId",featureId},{"ownerId",source.ownerId},{"targetBodyId",source.bodyId},
        {"toolSketchId",tool["id"]},{"toolEntityIds",toolIds},{"removedRegionIds",removed}};
    if(argc>1)check(document.save(QString::fromLocal8Bit(argv[1])).isEmpty(),"Save pre-Trim fixture");
    auto candidate=CadDocument::surfaceTrimCandidate(document.authored(),args);
    // The earlier root-sketch route remains the main regression below. A
    // cutter authored in the target's identity component uses that same local
    // frame and must agree with the already prepared exact geometry.
    auto sameOwner=candidate;auto ownerFeatures=sameOwner["features"].toArray();
    for(int i=0;i<ownerFeatures.size();++i){auto f=ownerFeatures[i].toObject();if(f["id"]==tool["id"]){f["ownerId"]=source.ownerId;ownerFeatures[i]=f;}}
    sameOwner["features"]=ownerFeatures;const auto ownerEvaluation=CadDocument::evaluateCandidate(sameOwner);
    check(ownerEvaluation.ok,"Same-owner identity-component sketch cutter must evaluate");
    const auto& ownerResult=ownerEvaluation.evaluation->bodies.back();
    check(ownerResult.summary["valid"].toBool() && ownerResult.summary["topology"]==bothRemoved.summary["topology"]
        && std::abs(ownerResult.summary["areaMm2"].toDouble()-bothRemoved.summary["areaMm2"].toDouble())<1e-6,
        "Same-owner preview and authoritative commit geometry disagree");
    for(size_t i=0;i<ownerResult.supportFaces.size();++i)check(ownerResult.supportFaces[i].faceId==bothRemoved.supportFaces[i].faceId,
        "Same-owner cutter changed retained region identities");
    auto foreign=CadDocument::componentCandidate(sameOwner,"Foreign cutter owner");
    const auto foreignOwner=foreign["components"].toArray().last().toObject()["id"].toString();auto foreignFeatures=foreign["features"].toArray();
    for(int i=0;i<foreignFeatures.size();++i){auto f=foreignFeatures[i].toObject();if(f["id"]==tool["id"]){f["ownerId"]=foreignOwner;foreignFeatures[i]=f;}}
    foreign["features"]=foreignFeatures;const auto refusedOwner=CadDocument::evaluateCandidate(foreign);
    check(!refusedOwner.ok && refusedOwner.diagnostics["code"]=="cross_component_dependency" && document.authored()!=foreign,
        "Foreign-owner cutter must refuse before changing the document");
    std::cout<<"SURFACE_TRIM_OWNER_OK sameIdentityOwnerGeometry retainedIDs rootPreserved foreignRefusal\n";
    if(const auto error=document.commit(candidate,"Trim two inner regions");!error.isEmpty())
        throw std::runtime_error((error+" / "+document.lastEvaluationError()["code"].toString()).toStdString());
    const auto trimmed=document.bodies().back();const auto accepted=document.authored();
    check(trimmed.summary["topology"]==bothRemoved.summary["topology"]
        && std::abs(trimmed.summary["areaMm2"].toDouble()-bothRemoved.summary["areaMm2"].toDouble())<1e-6,
        "Shared selection application disagrees with authoritative committed geometry");
    for(size_t i=0;i<trimmed.supportFaces.size();++i)
        check(trimmed.supportFaces[i].faceId==bothRemoved.supportFaces[i].faceId,"Prepared selection changed retained face identity");
    check(trimmed.solidEdges.size()==bothRemoved.solidEdges.size(),"Prepared selection changed named edge count");
    for(size_t i=0;i<trimmed.solidEdges.size();++i)
        check(trimmed.solidEdges[i].edgeId==bothRemoved.solidEdges[i].edgeId,"Prepared selection changed retained edge identity");
    check(trimmed.bodyId==source.bodyId && trimmed.summary["topology"].toObject()["faces"]==2
        && trimmed.summary["topology"].toObject()["solids"]==0 && trimmed.summary["valid"].toBool(),
        "Trim must leave the same valid open sheet body");
    check(std::abs((sourceArea-removedArea)-trimmed.summary["areaMm2"].toDouble())<.05,
        "Retained sheet area disagrees with selected inner regions");
    check(trimmed.supportFaces.size()==2 && trimmed.solidEdges.size()>=4,"Trim boundary lineage unavailable");
    if(argc>2)BRepTools::Write(trimmed.shape,argv[2]);
    const auto stableFace=trimmed.supportFaces[0].faceId;const auto stableEdges=trimmed.solidEdges.size();
    auto changed=editPoint(accepted,tool["id"].toString(),entities[0].toObject()["pointIds"].toArray()[1].toString(),11);
    check(document.commit(changed,"Edit upper handle spline").isEmpty(),"Tool sketch edit failed");
    check(document.bodies().back().supportFaces[0].faceId==stableFace
        && document.bodies().back().solidEdges.size()==stableEdges,"Trim lineage changed under source edit");
    const auto edited=document.authored();document.history().undo();check(document.authored()==accepted,"Undo tool edit");
    document.history().redo();check(document.authored()==edited,"Redo tool edit");
    QTemporaryDir temporary;const auto path=temporary.filePath("trim.alloy");
    check(document.save(path).isEmpty(),"Save Trim");CadDocument reopened;
    check(reopened.open(path).isEmpty() && reopened.authored()==edited
        && reopened.bodies().back().supportFaces[0].faceId==stableFace,"Reopen retained Trim");
    const auto broken=editPoint(edited,tool["id"].toString(),entities[0].toObject()["pointIds"].toArray()[0].toString(),4);
    check(!document.commit(broken,"Break tool loop").isEmpty() && document.authored()==edited,
        "Broken shared endpoint must refuse atomically");
    std::cout<<"SURFACE_TRIM_OK removed="<<removedArea<<" kept="<<trimmed.summary["areaMm2"].toDouble()<<'\n';
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
