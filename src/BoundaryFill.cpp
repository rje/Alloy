// SPDX-License-Identifier: MIT
#include "BoundaryFill.h"
#include "PlanarFaces.h"
#include "SolidFillet.h"
#include <BOPAlgo_MakerVolume.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepClass3d_SolidClassifier.hxx>
#include <BRepGProp.hxx>
#include <BRepBndLib.hxx>
#include <BRep_Tool.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <Bnd_Box.hxx>
#include <GProp_GProps.hxx>
#include <TopExp.hxx>
#include <TopoDS.hxx>
#include <TopoDS_AlertWithShape.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopTools_ListOfShape.hxx>
#include <QUuid>
#include <QSet>
#include <algorithm>
#include <sstream>

namespace {
QString sourceKey(const QJsonObject& ref){return ref["bodyId"].toString()+"/"+ref["producerFeatureId"].toString()+"/"+ref["faceId"].toString();}
QJsonArray orderedReferences(const QJsonArray& refs,bool faces){
    QMap<QString,QJsonObject> ordered;for(const auto& value:refs){const auto ref=value.toObject();
        const auto key=faces ? sourceKey(ref) : ref["bodyId"].toString()+"/"+ref["producerFeatureId"].toString();ordered[key]=ref;}
    QJsonArray result;for(const auto& ref:ordered)result.append(ref);return result;
}
QString referenceKey(const QJsonArray& faces){QStringList keys;for(const auto& v:faces)keys.append(sourceKey(v.toObject()));keys.sort();return keys.join('|');}
QString stableId(const QString& feature,const QByteArray& role){return QUuid::createUuidV5(QUuid(feature),QByteArray("alloy-boundary-fill/1/")+role).toString(QUuid::WithoutBraces);}
QString cellId(const QString& signature){return QUuid::createUuidV5(QUuid("c2abf903-1b49-4c62-86cc-1af0bc2aab11"),QByteArray("alloy-boundary-fill-cell/1/")+signature.toUtf8()).toString(QUuid::WithoutBraces);}
bool fromHistory(BOPAlgo_MakerVolume& maker,const TopoDS_Shape& source,const TopoDS_Shape& result){
    if(source.IsSame(result))return true;for(const auto& shape:maker.Modified(source))if(shape.IsSame(result))return true;
    for(const auto& shape:maker.Generated(source))if(shape.IsSame(result))return true;return false;
}
QJsonArray point(const gp_Pnt& p){return {p.X(),p.Y(),p.Z()};}
QJsonArray marker(const TopoDS_Solid& solid){
    auto inside=[&](const gp_Pnt& p){return BRepClass3d_SolidClassifier(solid,p,1e-7).State()==TopAbs_IN;};
    GProp_GProps props;BRepGProp::VolumeProperties(solid,props);const auto center=props.CentreOfMass();if(inside(center))return point(center);
    Bnd_Box bounds;BRepBndLib::Add(solid,bounds);double x1,y1,z1,x2,y2,z2;bounds.Get(x1,y1,z1,x2,y2,z2);
    for(int z=1;z<5;++z)for(int y=1;y<5;++y)for(int x=1;x<5;++x){const gp_Pnt p(x1+(x2-x1)*x/5,y1+(y2-y1)*y/5,z1+(z2-z1)*z/5);if(inside(p))return point(p);}return {};
}
}
BoundaryFillPartition BoundaryFill::partition(const QJsonObject& feature,const EvaluatedDocument& prior){
    const auto id=feature["id"].toString(),bodyId=feature["bodyId"].toString();const auto p=feature["parameters"].toObject();
    auto reject=[&](const QString& code,const QString& message){throw GeometryKernel::EvaluationError({{"featureId",id},{"featureType","boundaryFill"},{"code",code},{"message",message}});};
    const auto tools=p["tools"].toArray();if(tools.size()<2 || tools.size()>32)reject("boundary_fill_tools_required","Select 2–32 earlier local surface bodies");
    std::vector<const EvaluatedBody*> inputs;QSet<QString> selected;TopTools_ListOfShape arguments;
    struct FaceSource {const EvaluatedBody* body;const EvaluatedSupportFace* face;QJsonObject ref;};std::vector<FaceSource> sources;
    BoundaryFillPartition result;result.featureId=id;result.bodyId=bodyId;result.toolReferences=orderedReferences(tools,false);
    for(const auto& value:tools){const auto ref=value.toObject();if(!value.isObject() || ref.size()!=2)reject("boundary_fill_tool_unavailable","Tool references require exact body and producer fields");const auto sourceId=ref["bodyId"].toString();
        const auto found=std::find_if(prior.bodies.begin(),prior.bodies.end(),[&](const auto& b){return b.bodyId==sourceId;});
        if(found==prior.bodies.end() || found->featureId!=ref["producerFeatureId"].toString())reject("boundary_fill_tool_unavailable","Boundary Fill requires each exact earlier body and producer");
        // Full document evaluation finalizes owners after the feature loop.
        // Completed source-stage evaluations carry owners; authoritative full
        // recompute additionally passes CadDocument's authored ownership guard.
        if(!found->ownerId.isEmpty() && found->ownerId!=feature["ownerId"].toString())reject("boundary_fill_owner_unsupported","Boundary Fill tools must share its modeling owner");
        if(selected.contains(sourceId))reject("boundary_fill_tool_duplicate","Select each tool body once");selected.insert(sourceId);
        if(!found->occurrenceId.isEmpty() || found->summary["type"]!="surface" || found->summary["topology"].toObject()["solids"].toInt()!=0
            || found->shape.IsNull() || !BRepCheck_Analyzer(found->shape).IsValid())reject("boundary_fill_tool_unsupported","Boundary Fill requires valid local surface tools");
        TopTools_IndexedMapOfShape faces;TopExp::MapShapes(found->shape,TopAbs_FACE,faces);QSet<QString> names;
        if(faces.Extent()!=int(found->supportFaces.size()))reject("boundary_fill_tool_ancestry","Every selected tool face needs retained ancestry");
        for(const auto& f:found->supportFaces){const QJsonObject source{{"bodyId",sourceId},{"producerFeatureId",f.producerFeatureId},{"faceId",f.faceId}};const auto key=sourceKey(source);
            if(!faces.Contains(f.face) || names.contains(key))reject("boundary_fill_tool_ancestry","Tool face ancestry must be unique and current");names.insert(key);sources.push_back({&*found,&f,source});}
        inputs.push_back(&*found);arguments.Append(found->shape);result.toolBodyIds.append(sourceId);
    }
    BOPAlgo_MakerVolume maker;maker.SetArguments(arguments);maker.SetNonDestructive(true);maker.SetIntersect(true);maker.SetRunParallel(false);maker.SetToFillHistory(true);maker.Perform();
    std::ostringstream errors,warnings;maker.DumpErrors(errors);maker.DumpWarnings(warnings);QJsonArray details;
    for(const auto& alert:maker.GetReport()->GetAlerts(Message_Warning)){QJsonObject row{{"code",QString::fromLatin1(alert->DynamicType()->Name())}};const auto shaped=Handle(TopoDS_AlertWithShape)::DownCast(alert);
        if(!shaped.IsNull() && !shaped->GetShape().IsNull())row["shape"]=GeometryKernel::summarize(shaped->GetShape());details.append(row);}
    result.diagnostics={{"warnings",QString::fromStdString(warnings.str())},{"warningDetails",details},{"requestedFuzzyOverride","none"},{"effectiveFuzzyValue",maker.FuzzyValue()}};
    if(maker.HasErrors() || maker.Shape().IsNull())reject("boundary_fill_failed",QString::fromStdString(errors.str()).trimmed().isEmpty()?"OCCT could not build bounded cells":QString::fromStdString(errors.str()));
    TopTools_IndexedMapOfShape solids;TopExp::MapShapes(maker.Shape(),TopAbs_SOLID,solids);QSet<QString> cellSignatures;
    for(int n=1;n<=solids.Extent();++n){BoundaryFillCell cell;cell.shape=TopoDS::Solid(solids(n));cell.summary=GeometryKernel::summarize(cell.shape);
        if(!BRepCheck_Analyzer(cell.shape).IsValid() || cell.summary["topology"].toObject()["solids"].toInt()!=1 || cell.summary["volumeMm3"].toDouble()<=0)
            reject("boundary_fill_cell_invalid","Every bounded cell must be a valid positive-volume solid");
        TopTools_IndexedMapOfShape faces,edges;TopExp::MapShapes(cell.shape,TopAbs_FACE,faces);TopExp::MapShapes(cell.shape,TopAbs_EDGE,edges);
        QMap<int,QString> faceKeys;QJsonArray contributors;QSet<QString> signatures;
        for(int i=1;i<=faces.Extent();++i){const FaceSource* source=nullptr;
            for(const auto& candidate:sources)if(fromHistory(maker,candidate.face->face,faces(i))){if(source)reject("boundary_fill_face_ambiguous","A result face has multiple source contributors in this bounded slice");source=&candidate;}
            if(!source)reject("boundary_fill_face_unavailable","A result face has no exact named source history");const auto key=sourceKey(source->ref);
            if(signatures.contains(key))reject("boundary_fill_face_ambiguous","A named source face produces multiple current faces in one cell");signatures.insert(key);faceKeys[i]=key;contributors.append(source->ref);
            auto f=*source->face;f.face=TopoDS::Face(faces(i));f.faceId=stableId(id,QByteArray("face/")+key.toUtf8());f.bodyId=bodyId;f.producerFeatureId=id;f.role="boundaryFill.boundary";f.boundaryEdges.clear();
            if(!f.frame.isEmpty() && f.face.Orientation()!=source->face->face.Orientation()){
                for(const auto& axis:{"vAxis","normal"}){auto values=f.frame[axis].toArray();for(int k=0;k<values.size();++k)values[k]=-values[k].toDouble();f.frame[axis]=values;}
            }
            f.provenance={{"generatorFeatureId",id},{"sourceBodyId",source->body->bodyId},{"sourceProducerFeatureId",source->face->producerFeatureId},{"sourceFaceId",source->face->faceId},{"lineage","makerVolumeHistory"}};cell.supportFaces.push_back(std::move(f));
        }
        contributors=orderedReferences(contributors,true);const auto signature=referenceKey(contributors);
        if(cellSignatures.contains(signature))reject("boundary_fill_cell_ambiguous","Bounded cells have indistinguishable source-face signatures");cellSignatures.insert(signature);
        cell.reference={{"sourceFaces",contributors}};cell.cellId=cellId(signature);QSet<QString> edgeSignatures;
        for(int i=1;i<=edges.Extent();++i){const auto edge=TopoDS::Edge(edges(i));if(BRep_Tool::Degenerated(edge))continue;QStringList adjacent,edgeSources;
            for(int j=1;j<=faces.Extent();++j){TopTools_IndexedMapOfShape current;TopExp::MapShapes(faces(j),TopAbs_EDGE,current);if(current.Contains(edge))adjacent.append(faceKeys[j]);}
            for(const auto* b:inputs)for(const auto& e:b->solidEdges)if(fromHistory(maker,e.edge,edge))edgeSources.append(b->bodyId+"/"+b->featureId+"/"+e.edgeId);
            adjacent.sort();edgeSources.removeDuplicates();edgeSources.sort();if(adjacent.size()!=2)reject("boundary_fill_edge_ambiguous","Ordinary solid boundaries need two named incident faces");
            const auto signature=adjacent.join('|')+";sourceEdges="+edgeSources.join('|');if(edgeSignatures.contains(signature))reject("boundary_fill_edge_ambiguous","Current solid edges have indistinguishable source ancestry");edgeSignatures.insert(signature);
            const auto edgeId=stableId(id,QByteArray("edge/")+signature.toUtf8());const QJsonObject ancestry{{"generatorFeatureId",id},{"lineage","makerVolumeHistory"},{"sourceFaceSignatures",QJsonArray::fromStringList(adjacent)},{"sourceEdgeReferences",QJsonArray::fromStringList(edgeSources)}};
            cell.solidEdges.push_back({edgeId,edge,ancestry});for(auto& f:cell.supportFaces){TopTools_IndexedMapOfShape current;TopExp::MapShapes(f.face,TopAbs_EDGE,current);if(current.Contains(edge))f.boundaryEdges.push_back({edgeId,BRepAdaptor_Curve(edge).GetType()==GeomAbs_Line ? "line" : "curve",edge,ancestry});}
        }
        cell.selectionPointMm=marker(cell.shape);if(cell.selectionPointMm.isEmpty())reject("boundary_fill_cell_marker_unavailable","No qualified interior cell marker could be found");
        cell.summary["supportFaces"]=PlanarFaces::descriptors(cell.supportFaces);cell.summary["solidEdges"]=SolidFillet::descriptors(cell.solidEdges);cell.summary["cellId"]=cell.cellId;cell.summary["cell"]=cell.reference;result.cells.push_back(std::move(cell));
    }
    std::sort(result.cells.begin(),result.cells.end(),[](const auto& a,const auto& b){return a.cellId<b.cellId;});return result;
}
EvaluatedBody BoundaryFill::applyCell(const QJsonObject& feature,const BoundaryFillPartition& partition){
    const auto id=feature["id"].toString(),bodyId=feature["bodyId"].toString();const auto p=feature["parameters"].toObject();auto reject=[&](const QString& code,const QString& message){throw GeometryKernel::EvaluationError({{"featureId",id},{"featureType","boundaryFill"},{"code",code},{"message",message}});};
    if(id!=partition.featureId || bodyId!=partition.bodyId || orderedReferences(p["tools"].toArray(),false)!=partition.toolReferences)reject("boundary_fill_partition_stale","The cell partition does not match these exact feature tools");
    if(partition.cells.empty())reject("boundary_fill_no_cells","The selected sheets do not bound a solid cell");
    const auto ref=p["cell"].toObject();const auto requested=orderedReferences(ref["sourceFaces"].toArray(),true);if(ref.size()!=1 || requested.isEmpty() || requested.size()!=ref["sourceFaces"].toArray().size())reject("boundary_fill_cell_unavailable","Select one exact retained cell signature");const BoundaryFillCell* selected=nullptr;
    for(const auto& cell:partition.cells)if(cell.reference["sourceFaces"].toArray()==requested){if(selected)reject("boundary_fill_cell_ambiguous","The selected cell ancestry is ambiguous");selected=&cell;}
    if(!selected)reject("boundary_fill_cell_unavailable","The retained cell no longer exists; repair the source sheets or select a current cell");
    EvaluatedBody body;body.featureId=id;body.bodyId=bodyId;body.ownerId=feature["ownerId"].toString();body.name=feature["name"].toString();body.visible=feature["visible"].toBool(true);body.shape=selected->shape;body.supportFaces=selected->supportFaces;body.solidEdges=selected->solidEdges;body.summary=selected->summary;
    body.summary["operation"]="newBody";body.summary["removeTools"]=p["removeTools"];body.summary["tools"]=p["tools"];body.summary["makerDiagnostics"]=partition.diagnostics;return body;
}
EvaluatedBody BoundaryFill::evaluate(const QJsonObject& feature,const EvaluatedDocument& prior){return applyCell(feature,partition(feature,prior));}
QJsonArray BoundaryFill::descriptors(const BoundaryFillPartition& partition){QJsonArray result;for(const auto& c:partition.cells)result.append(QJsonObject{{"cellId",c.cellId},{"cell",c.reference},{"geometry",c.summary},{"selectionPointMm",c.selectionPointMm}});return result;}
