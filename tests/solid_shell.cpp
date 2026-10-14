#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <numbers>
#include <BRepAdaptor_Curve.hxx>

static void check(bool okay,const char* message){if(!okay)throw std::runtime_error(message);}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    {
        CadDocument closed;
        QJsonObject parameters{{"widthMm",50},{"depthMm",80},{"heightMm",40},{"xMm",0},{"yMm",0},{"zMm",0}};
        auto authored=closed.authored();
        auto f=QJsonObject{{"id",CadDocument::newId()},{"bodyId",CadDocument::newId()},
            {"ownerId",authored["documentId"]},{"type","box"},{"name","Box"},{"visible",true},{"parameters",parameters}};
        authored["features"]=QJsonArray{f};check(closed.commit(authored,"Box").isEmpty(),"closed source");
        const auto result=closed.commit(CadDocument::shellCandidate(closed.authored(),
            {{"targetBodyId",closed.bodies()[0].bodyId},{"thicknessMm",3.5}}),"Closed Shell");
        if(!result.isEmpty())throw std::runtime_error(result.toStdString());
        check(std::abs(closed.bodies()[0].summary["volumeMm3"].toDouble()-(50*80*40-43*73*33))<1e-4,"closed cavity volume");
        check(closed.bodies()[0].supportFaces.size()==12,"all six closed cavity planes retained");
        const auto stable=closed.authored();const auto id=stable["features"].toArray().last().toObject()["id"];
        check(!closed.commit(CadDocument::shellCandidate(stable,{{"featureId",id},
            {"targetBodyId",f["bodyId"]},{"thicknessMm",100}}),"Impossible cavity").isEmpty(),"oversized cavity refused");
        check(closed.authored()==stable,"closed failure atomic");
        check(closed.commit(CadDocument::shellCandidate(stable,{{"featureId",id},
            {"targetBodyId",f["bodyId"]},{"thicknessMm",4}}),"Wall edit").isEmpty(),"closed thickness edit");
        closed.history().undo();check(closed.authored()==stable,"closed undo");closed.history().redo();
        QTemporaryDir temp;const auto path=temp.filePath("closed.alloy");check(closed.save(path).isEmpty(),"closed save");
        CadDocument reopened;check(reopened.open(path).isEmpty() && reopened.authored()==closed.authored(),"closed reopen");
        CadDocument rounded;authored=rounded.authored();f["ownerId"]=authored["documentId"];authored["features"]=QJsonArray{f};
        check(rounded.commit(authored,"Box").isEmpty(),"rounded shell source");
        QJsonArray vertical,rims;for(const auto& edge:rounded.bodies()[0].solidEdges)
            (edge.provenance["boxEdge"].toString().contains("z*") ? vertical : rims).append(edge.edgeId);
        auto group=[](const QJsonArray& edges,double r){return QJsonObject{{"id",CadDocument::newId()},
            {"edgeIds",edges},{"radiusMm",r},{"tangentChain",true}};};
        check(rounded.commit(CadDocument::filletCandidate(rounded.authored(),{{"targetBodyId",f["bodyId"]},
            {"groups",QJsonArray{group(vertical,5),group(rims,2.5)}}}),"Round stock").isEmpty(),"rounded source");
        const auto oldVolume=rounded.bodies()[0].summary["volumeMm3"].toDouble();
        auto error=rounded.commit(CadDocument::shellCandidate(rounded.authored(),{{"targetBodyId",f["bodyId"]},
            {"thicknessMm",3.5}}),"Closed rounded shell");
        if(!error.isEmpty())throw std::runtime_error(error.toStdString());
        check(rounded.authored()["features"].toArray().size()==3 &&
            rounded.bodies()[0].summary["volumeMm3"].toDouble()<oldVolume,"rounded cavity retained");
        QSet<QString> boxInnerIds;for(const auto& face:rounded.bodies()[0].supportFaces)
            if(face.role=="shell-inner")boxInnerIds.insert(face.faceId);
        auto moved=rounded.authored();auto movedFeatures=moved["features"].toArray();
        auto movedBox=movedFeatures[0].toObject();auto movedValues=movedBox["parameters"].toObject();
        movedValues["xMm"]=10;movedValues["yMm"]=-6;movedValues["zMm"]=4;
        movedValues["widthMm"]=52;movedValues["depthMm"]=83;movedValues["heightMm"]=41;
        movedBox["parameters"]=movedValues;movedFeatures[0]=movedBox;moved["features"]=movedFeatures;
        check(rounded.commit(moved,"Move and resize source").isEmpty(),"translated resized rounded Box replay");
        QSet<QString> movedInnerIds;for(const auto& face:rounded.bodies()[0].supportFaces)
            if(face.role=="shell-inner")movedInnerIds.insert(face.faceId);
        check(movedInnerIds==boxInnerIds,"translated Box inner references stable");
        rounded.history().undo();
        CadDocument source;
        auto center=SketchGeometry::addCenterRectangle(
            SketchGeometry::newFeature(source.authored()["documentId"].toString(),"XY","Centered stock"),
            {{"centerUMm",0},{"centerVMm",0},{"widthMm",93},{"heightMm",64},
                {"centerSnapTarget",QJsonObject{{"kind","origin"}}}});
        check(center.ok,"centered stock sketch");
        check(source.commit(SketchGeometry::replaceFeature(source.authored(),center.feature),"Center Rectangle").isEmpty(),"source sketch commit");
        const auto sourceProfile=source.sketches()[0].profiles[0].profileId;
        check(source.commit(CadDocument::extrusionCandidate(source.authored(),
            {{"sketchId",center.feature["id"]},{"profileId",sourceProfile},{"distanceMm",30}}),"Extrude").isEmpty(),"source prism");
        QJsonArray sourceVertical,sourceRims;
        for(const auto& edge:source.bodies()[0].solidEdges){
            const auto role=edge.provenance["role"].toString();
            (role=="sideCorner"?sourceVertical:sourceRims).append(edge.edgeId);
        }
        check(sourceVertical.size()==4 && sourceRims.size()==8,"source prism provenance");
        const auto sourceBody=source.bodies()[0].bodyId;
        check(source.commit(CadDocument::filletCandidate(source.authored(),{{"targetBodyId",sourceBody},
            {"groups",QJsonArray{group(sourceVertical,5),group(sourceRims,2.5)}}}),"Grouped Fillet").isEmpty(),"source grouped blend");
        const auto sourceOuterVolume=source.bodies()[0].summary["volumeMm3"].toDouble();
        const auto sourceShell=source.commit(CadDocument::shellCandidate(source.authored(),
            {{"targetBodyId",sourceBody},{"thicknessMm",3.5}}),"Closed Shell");
        if(!sourceShell.isEmpty())throw std::runtime_error(sourceShell.toStdString());
        const double cavity=(86*57-(4-std::numbers::pi)*1.5*1.5)*23;
        check(std::abs(source.bodies()[0].summary["volumeMm3"].toDouble()
            -(sourceOuterVolume-cavity))<1e-4,"source cavity volume");
        check(std::count_if(source.bodies()[0].supportFaces.begin(),source.bodies()[0].supportFaces.end(),
            [](const auto& face){return face.role=="shell-inner";})==6,"source six inner planes");
        QMap<QString,TopoDS_Edge> sharedInnerEdges;int sharedCount=0;
        for(const auto& face:source.bodies()[0].supportFaces)if(face.role=="shell-inner")
            for(const auto& edge:face.boundaryEdges){
                if(sharedInnerEdges.contains(edge.edgeId)){
                    check(sharedInnerEdges.value(edge.edgeId).IsSame(edge.edge),"shared inner edge identity is one kernel edge");
                    ++sharedCount;
                }else sharedInnerEdges.insert(edge.edgeId,edge.edge);
            }
        check(sharedCount>=8,"inner planar faces share semantic edge IDs");
        const auto floorIt=std::find_if(source.bodies()[0].supportFaces.begin(),source.bodies()[0].supportFaces.end(),
            [](const auto& face){return face.role=="shell-inner" && face.frame["normal"].toArray()[2].toDouble()>.99;});
        check(floorIt!=source.bodies()[0].supportFaces.end(),"exact inner floor support is available");
        const auto floorFaceId=floorIt->faceId;
        const QJsonObject floorRef{{"producerFeatureId",floorIt->producerFeatureId},
            {"bodyId",floorIt->bodyId},{"faceId",floorIt->faceId}};
        auto floorSketch=SketchGeometry::newSupportedFeature(source.authored()["documentId"].toString(),floorRef,
            "Floor mounting sketch",GeometryKernel::supportEdgeDescriptors(*floorIt));
        const auto floorProjection=GeometryKernel::projectionContext(source.evaluation(),floorSketch);
        check(floorProjection["points"].toArray().isEmpty() && !floorProjection["unsupportedEdges"].toArray().isEmpty(),
            "Generated inner fillet arcs became false authored circle centers");
        check(source.commit(SketchGeometry::replaceFeature(source.authored(),floorSketch),"Sketch on inner floor").isEmpty(),
            "actual closed rounded inner floor refused face-supported sketch");
        source.history().undo();
        auto conflicting=source.evaluation();
        bool assignedFirst=false,assignedSecond=false;
        for(auto& body:conflicting.bodies)if(body.bodyId==sourceBody)
            for(auto& face:body.supportFaces)if(face.faceId==floorFaceId)
                for(auto& edge:face.boundaryEdges)if(edge.type=="circle" && !assignedSecond){
                    edge.provenance["sourceSketchId"]=center.feature["id"];
                    edge.provenance["sourceEntityId"]=QString("11111111-1111-4111-8111-111111111111");
                    if(assignedFirst)assignedSecond=true;else assignedFirst=true;
                }
        check(assignedSecond,"inner floor needs two distinct circular descendants for refusal proof");
        bool refusedConflict=false;
        try{GeometryKernel::projectionContext(conflicting,floorSketch);}
        catch(const GeometryKernel::EvaluationError& error){refusedConflict=error.diagnostics()["code"]=="ambiguous_projection";}
        check(refusedConflict,"two different circle centers with one retained source identity did not refuse");
        const auto shellStage=source.authored();
        const auto shellStageId=shellStage["features"].toArray().last().toObject()["id"];
        check(source.commit(CadDocument::shellCandidate(shellStage,
            {{"featureId",shellStageId},{"targetBodyId",sourceBody},{"thicknessMm",4}}),
            "Edit cavity").isEmpty(),"source cavity wall edit");
        QSet<QString> stageEdgeIds;for(const auto& face:source.bodies()[0].supportFaces)
            if(face.role=="shell-inner")for(const auto& edge:face.boundaryEdges)stageEdgeIds.insert(edge.edgeId);
        check(stageEdgeIds==QSet<QString>(sharedInnerEdges.keyBegin(),sharedInnerEdges.keyEnd()),
            "source inner edge IDs survive Shell edit");
        source.history().undo();check(source.authored()==shellStage,"restore source wall before split");
        auto reference=[](const EvaluatedSupportFace& face){return QJsonObject{
            {"producerFeatureId",face.producerFeatureId},{"bodyId",face.bodyId},{"faceId",face.faceId}};};
        QJsonObject top,bottom;for(const auto& face:source.bodies()[0].supportFaces){
            if(face.role=="extrude.endCap")top=reference(face);
            if(face.role=="extrude.base")bottom=reference(face);
        }
        check(!top.isEmpty()&&!bottom.isEmpty(),"source outer caps retained for Midplane");
        const QJsonObject plane{{"id",CadDocument::newId()},{"ownerId",source.authored()["documentId"]},
            {"type","constructionPlane"},{"name","Middle"},{"visible",true},
            {"parameters",QJsonObject{{"source",QJsonObject{{"faces",QJsonArray{top,bottom}}}}}}};
        auto withPlane=source.authored();auto sourceFeatures=withPlane["features"].toArray();
        sourceFeatures.append(plane);withPlane["features"]=sourceFeatures;
        check(source.commit(withPlane,"Midplane").isEmpty(),"rounded cavity Midplane");
        check(source.commit(CadDocument::splitBodyCandidate(source.authored(),
            {{"targetBodyId",sourceBody},{"planeId",plane["id"]}}),"Split Body").isEmpty(),"rounded cavity split");
        check(source.bodies().size()==2 && std::abs(source.bodies()[0].summary["volumeMm3"].toDouble()
            +source.bodies()[1].summary["volumeMm3"].toDouble()-(sourceOuterVolume-cavity))<1e-4,
            "rounded cavity split conserves material");
        QSet<QString> innerIds;for(const auto& face:source.bodies()[0].supportFaces)
            if(face.role=="shell-inner")innerIds.insert(face.faceId);
        const auto beforeWallEdit=source.authored();
        const auto shellId=beforeWallEdit["features"].toArray()[3].toObject()["id"];
        check(source.commit(CadDocument::shellCandidate(beforeWallEdit,
            {{"featureId",shellId},{"targetBodyId",sourceBody},{"thicknessMm",4}}),
            "Edit closed wall").isEmpty(),"source wall edit with downstream split");
        QSet<QString> editedIds;for(const auto& face:source.bodies()[0].supportFaces)
            if(face.role=="shell-inner")editedIds.insert(face.faceId);
        check(editedIds==innerIds,"source inner face IDs survive wall edit");
        const auto validWall=source.authored();
        check(!source.commit(CadDocument::shellCandidate(validWall,
            {{"featureId",shellId},{"targetBodyId",sourceBody},{"thicknessMm",100}}),
            "Impossible wall").isEmpty() && source.authored()==validWall,
            "impossible thickness refuses atomically through existing Shell authority");
        source.history().undo();check(source.authored()==beforeWallEdit,"source wall edit undo");
        source.history().redo();
        QTemporaryDir sourceDir;check(sourceDir.isValid(),"source save directory");
        const auto sourcePath=sourceDir.filePath("rounded-source.alloy");
        check(source.save(sourcePath).isEmpty(),"save rounded source");
        CadDocument sourceReopened;
        check(sourceReopened.open(sourcePath).isEmpty() && sourceReopened.authored()==source.authored()
            && sourceReopened.bodies().size()==2,"reopen rounded source and split");
    }
    CadDocument document;
    auto sketch=SketchGeometry::newFeature(document.authored()["documentId"].toString(),"XY","Cube base");
    sketch=SketchGeometry::addRectangle(sketch,{{"u1Mm",0},{"v1Mm",0},{"u2Mm",25},{"v2Mm",25}});
    check(document.commit(SketchGeometry::replaceFeature(document.authored(),sketch),"Rectangle").isEmpty(),"sketch commit");
    const auto profile=document.sketches()[0].profiles[0].profileId;
    check(document.commit(CadDocument::extrusionCandidate(document.authored(),{{"sketchId",sketch["id"]},
        {"profileId",profile},{"distanceMm",12.5}}),"Extrude").isEmpty(),"extrude commit");
    const auto bodyId=document.bodies()[0].bodyId;
    const auto cap=document.bodies()[0].supportFaces[0];
    QJsonObject support{{"producerFeatureId",cap.producerFeatureId},{"bodyId",cap.bodyId},{"faceId",cap.faceId}};
    check(document.commit(CadDocument::lengthParameterCandidate(document.authored(),{{"name","Wall"},{"value",2},{"units","mm"}}),"Add Wall").isEmpty(),"Wall parameter created");
    const auto wallId=document.authored()["userParameters"].toArray()[0].toObject()["id"];
    const auto before=document.authored();
    QJsonObject arguments{{"targetBodyId",bodyId},{"removedFace",support},{"thicknessMm",2},{"thicknessParameterId",wallId}};
    const auto candidate=CadDocument::shellCandidate(before,arguments);
    check(document.commit(candidate,"Shell").isEmpty(),"shell commit");
    check(document.bodies().size()==1 && document.bodies()[0].bodyId==bodyId,"body identity");
    check(std::abs(document.bodies()[0].summary["volumeMm3"].toDouble()-3182)<1e-3,"cube shell volume");
    check(document.bodies()[0].summary["valid"].toBool() && document.bodies()[0].summary["closed"].toBool(),"valid closed shell");
    const auto rim=GeometryKernel::resolveSketchSupport(document.evaluation(),support);
    check(rim.role=="shell-rim" && rim.frame["origin"].toArray()[2].toDouble()==12.5,"retained top rim");
    bool innerBase=false;
    for(const auto& face:document.bodies()[0].supportFaces)
        if(face.role=="shell-inner" && std::abs(face.frame["origin"].toArray()[2].toDouble()-2)<1e-5)innerBase=true;
    check(innerBase,"kernel-mapped inner base support");
    check(document.authored()["features"].toArray().last().toObject()["parameters"].toObject()["thicknessParameterId"]==wallId,"Shell retains Wall ID");
    check(document.commit(CadDocument::lengthParameterCandidate(document.authored(),{{"parameterId",wallId},{"name","WallSize"},{"value",2.5}}),"Edit Wall").isEmpty(),"bound Shell rebuilds after rename/value edit");
    check(document.authored()["features"].toArray().last().toObject()["parameters"].toObject()["thicknessParameterId"]==wallId,"Shell binding survives rename");
    const auto boundStable=document.authored();const auto boundRevision=document.revision();
    check(!document.commit(CadDocument::lengthParameterCandidate(document.authored(),{{"parameterId",wallId},{"value",20}}),"Impossible bound Shell").isEmpty(),"bound Shell refuses impossible thickness");
    check(document.authored()==boundStable && document.revision()==boundRevision,"failed bound recompute atomic");
    document.history().undo();check(document.authored()==candidate,"undo Wall edit restores Shell cache");
    const auto shellFeature=document.authored()["features"].toArray().last().toObject();
    arguments["featureId"]=shellFeature["id"];arguments["thicknessMm"]=1.5;arguments.remove("thicknessParameterId");
    check(document.commit(CadDocument::shellCandidate(document.authored(),arguments),"Edit Shell").isEmpty(),"shell edit");
    check(!document.authored()["features"].toArray().last().toObject()["parameters"].toObject().contains("thicknessParameterId"),"numeric Shell edit replaces binding");
    check(std::abs(document.bodies()[0].summary["volumeMm3"].toDouble()-(7812.5-22*22*11))<1e-3,"edited thickness volume");
    const auto extrude=document.authored()["features"].toArray()[1].toObject();
    check(document.commit(CadDocument::extrusionCandidate(document.authored(),{{"featureId",extrude["id"]},
        {"sketchId",sketch["id"]},{"profileId",profile},{"distanceMm",13.5}}),"Resize source").isEmpty(),"upstream edit");
    check(std::abs(document.bodies()[0].summary["volumeMm3"].toDouble()-(25*25*13.5-22*22*12))<1e-3,"upstream shell rebuild");
    check(GeometryKernel::resolveSketchSupport(document.evaluation(),support).frame["origin"].toArray()[2].toDouble()==13.5,"rim follows upstream edit");
    document.history().undo();
    document.history().undo();check(document.authored()==candidate,"shell edit undo");
    document.history().redo();check(document.authored()!=candidate,"shell edit redo");
    const auto stable=document.authored();const auto revision=document.revision();const auto history=document.history().count();
    arguments["removedFace"]=QJsonObject{{"producerFeatureId",cap.producerFeatureId},{"bodyId",bodyId},{"faceId",CadDocument::newId()}};
    check(!document.commit(CadDocument::shellCandidate(stable,arguments),"Missing face").isEmpty(),"missing face rejected");
    arguments["removedFace"]=support;arguments["thicknessMm"]=20;
    check(!document.commit(CadDocument::shellCandidate(stable,arguments),"Impossible thickness").isEmpty(),"impossible thickness rejected");
    check(document.authored()==stable && document.revision()==revision && document.history().count()==history,"failure atomic");
    QTemporaryDir directory;check(directory.isValid(),"temporary directory");const auto path=directory.filePath("shell.alloy");
    check(document.save(path).isEmpty(),"save shell");CadDocument reopened;check(reopened.open(path).isEmpty(),"reopen shell");
    check(reopened.authored()==stable && std::abs(reopened.bodies()[0].summary["volumeMm3"].toDouble()-
        document.bodies()[0].summary["volumeMm3"].toDouble())<1e-6,"reopened exact shell");
    CadDocument stud;
    const auto owner=stud.authored()["documentId"].toString();
    auto base=SketchGeometry::newFeature(owner,"XY","Block");
    base=SketchGeometry::addRectangle(base,{{"u1Mm",0},{"v1Mm",0},{"u2Mm",31.8},{"v2Mm",15.8}});
    check(stud.commit(SketchGeometry::replaceFeature(stud.authored(),base),"Block").isEmpty(),"block sketch");
    check(stud.commit(CadDocument::extrusionCandidate(stud.authored(),{{"sketchId",base["id"]},
        {"profileId",stud.sketches()[0].profiles[0].profileId},{"distanceMm",9.6}}),"Block").isEmpty(),"block extrude");
    const auto block=stud.bodies()[0];
    const auto roofSource=*std::find_if(block.supportFaces.begin(),block.supportFaces.end(),[](const auto& face){return face.role=="extrude.endCap";});
    auto topSketch=SketchGeometry::newSupportedFeature(owner,{{"producerFeatureId",roofSource.producerFeatureId},
        {"bodyId",roofSource.bodyId},{"faceId",roofSource.faceId}},"Stud",{});
    topSketch=SketchGeometry::addCircle(topSketch,{{"centerUMm",3.9},{"centerVMm",3.9},{"diameterMm",5}});
    check(stud.commit(SketchGeometry::replaceFeature(stud.authored(),topSketch),"Stud").isEmpty(),"stud sketch");
    check(stud.commit(CadDocument::extrusionCandidate(stud.authored(),{{"sketchId",topSketch["id"]},
        {"profileId",stud.sketches().back().profiles[0].profileId},{"distanceMm",1.7},{"operation","join"},
        {"targetBodyIds",QJsonArray{block.bodyId}}}),"Stud").isEmpty(),"stud join");
    const auto beforeStudShell=stud.bodies()[0];
    const auto bottom=*std::find_if(beforeStudShell.supportFaces.begin(),beforeStudShell.supportFaces.end(),[](const auto& face){return face.role=="extrude.base";});
    check(stud.commit(CadDocument::shellCandidate(stud.authored(),{{"targetBodyId",block.bodyId},
        {"removedFace",QJsonObject{{"producerFeatureId",bottom.producerFeatureId},{"bodyId",bottom.bodyId},{"faceId",bottom.faceId}}},
        {"thicknessMm",1.49}}),"Shell" ).isEmpty(),"stud bottom shell");
    const auto roof=std::find_if(stud.bodies()[0].supportFaces.begin(),stud.bodies()[0].supportFaces.end(),[](const auto& face){
        return face.role=="shell-inner" && std::abs(face.frame["origin"].toArray()[2].toDouble()-8.11)<1e-5;
    });
    check(roof!=stud.bodies()[0].supportFaces.end(),"source-mapped inner roof");
    check(roof->boundaryEdges.size()==1,"one actual inner circular mouth");
    check(roof->frame["normal"].toArray()[2].toDouble()==-1,"inner roof outward normal");
    const BRepAdaptor_Curve mouth(roof->boundaryEdges[0].edge);
    check(mouth.GetType()==GeomAbs_Circle && std::abs(mouth.Circle().Radius()-1.01)<1e-5,"inner circular mouth radius");
    const QJsonObject roofRef{{"producerFeatureId",roof->producerFeatureId},{"bodyId",roof->bodyId},{"faceId",roof->faceId}};
    auto roofSketch=SketchGeometry::newSupportedFeature(owner,roofRef,"Inner roof",{});
    const auto centerContext=GeometryKernel::projectionContext(stud.evaluation(),roofSketch);
    check(centerContext["points"].toArray().size()==1,"one retained inner mouth center");
    const auto center=centerContext["points"].toArray()[0].toObject();
    check(std::abs(center["uMm"].toDouble()-3.9)<1e-5 && std::abs(center["vMm"].toDouble()+3.9)<1e-5,"mouth center in roof frame");
    const auto snapped=SketchGeometry::addPointerCircle(roofSketch,{{"centerUMm",0},{"centerVMm",0},
        {"diameterMm",6.3},{"centerSnapTarget",QJsonObject{{"kind","point"},{"pointId",center["id"]}}}},centerContext);
    check(snapped.ok,"snap circle to inner mouth center");roofSketch=snapped.feature;
    check(roofSketch["parameters"].toObject()["pointReferences"].toArray().size()==1,"retained mouth center reference");
    check(stud.commit(SketchGeometry::replaceFeature(stud.authored(),roofSketch),"Roof sketch").isEmpty(),"commit roof sketch");
    const auto pointReferences=roofSketch["parameters"].toObject()["pointReferences"].toArray();
    const auto shellId=stud.authored()["features"].toArray()[4].toObject()["id"].toString();
    check(!shellId.isEmpty(),"shell feature ID");
    check(stud.commit(CadDocument::shellCandidate(stud.authored(),{{"featureId",shellId},
        {"targetBodyId",block.bodyId},{"removedFace",QJsonObject{{"producerFeatureId",bottom.producerFeatureId},{"bodyId",bottom.bodyId},{"faceId",bottom.faceId}}},
        {"thicknessMm",1.3}}),"Edit stud shell").isEmpty(),"edit shell with dependent center sketch");
    const auto moved=GeometryKernel::projectionContext(stud.evaluation(),roofSketch)["points"].toArray()[0].toObject();
    check(moved["id"]==center["id"] && moved["source"]==center["source"] && moved["retained"].toBool(),"stable retained center identity");
    const auto changedRoof=GeometryKernel::resolveSketchSupport(stud.evaluation(),roofRef);
    check(std::abs(changedRoof.frame["origin"].toArray()[2].toDouble()-8.3)<1e-5,"roof follows thickness edit");
    check(stud.sketches().back().summary["valid"].toBool(),"dependent sketch evaluates after shell edit");
    const auto edited=stud.authored();stud.history().undo();stud.history().redo();
    check(stud.authored()==edited && stud.sketches().back().summary["valid"].toBool(),"shell edit undo redo");
    const auto studPath=directory.filePath("stud-roof.alloy");
    check(stud.save(studPath).isEmpty(),"save roof sketch");CadDocument restored;
    check(restored.open(studPath).isEmpty() && restored.authored()==edited,"reopen roof sketch");
    check(SketchGeometry::findFeature(restored.authored(),roofSketch["id"].toString())["parameters"].toObject()["pointReferences"].toArray()==pointReferences,"reopen retained center");
    check(GeometryKernel::projectionContext(restored.evaluation(),roofSketch)["points"].toArray()[0].toObject()["id"]==center["id"],"reopen center identity");
    auto missing=roofSketch;auto missingParameters=missing["parameters"].toObject();
    auto missingReferences=missingParameters["pointReferences"].toArray();
    auto missingReference=missingReferences[0].toObject();auto missingSource=missingReference["source"].toObject();
    missingSource["occurrenceId"]=CadDocument::newId();
    missingReference["source"]=missingSource;
    missingReferences[0]=missingReference;missingParameters["pointReferences"]=missingReferences;missing["parameters"]=missingParameters;
    const auto safe=restored.authored();const auto safeRevision=restored.revision();const auto safeHistory=restored.history().count();
    check(!restored.commit(SketchGeometry::replaceFeature(safe,missing),"Missing occurrence").isEmpty()
        && restored.lastEvaluationError()["code"]=="projection_unavailable","missing occurrence did not fail explicitly");
    check(restored.authored()==safe && restored.revision()==safeRevision && restored.history().count()==safeHistory,"missing occurrence changed accepted work");
    std::cout<<"One-face sharp inward Shell document and geometry passed\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
