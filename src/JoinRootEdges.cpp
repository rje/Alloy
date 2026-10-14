// SPDX-License-Identifier: MIT
#include "JoinRootEdges.h"
#include <BRepAdaptor_Curve.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <QUuid>
#include <algorithm>

namespace {
bool descendant(BRepBuilderAPI_MakeShape& operation,const TopoDS_Shape& source,
    const TopoDS_Shape& current) {
    if(source.IsSame(current))return true;
    for(const auto& shape:operation.Modified(source))if(shape.ShapeType()==current.ShapeType()
        && shape.IsSame(current))return true;
    return false;
}
}

std::vector<EvaluatedSolidEdge> JoinRootEdges::publish(BRepBuilderAPI_MakeShape& operation,
    const std::vector<EvaluatedSupportFace>& targetFaces,
    const std::vector<ToolBase>& toolBases,
    const std::vector<EvaluatedSolidEdge>& toolEdges,
    const TopoDS_Shape& result,
    const std::vector<EvaluatedSupportFace>& retainedFaces,
    const QString& featureId) {
    TopTools_IndexedMapOfShape present;TopExp::MapShapes(result,TopAbs_EDGE,present);
    TopTools_IndexedDataMapOfShapeListOfShape incident;
    TopExp::MapShapesAndAncestors(result,TopAbs_EDGE,TopAbs_FACE,incident);
    std::vector<EvaluatedSolidEdge> output;
    for(const auto& source:toolEdges){
        if(source.provenance["role"]!="baseRim" || source.edge.IsNull())continue;
        const auto profileId=source.provenance["sourceProfileId"].toString();
        if(profileId.isEmpty())continue;
        TopTools_IndexedMapOfShape currentEdges;
        if(present.Contains(source.edge))currentEdges.Add(present(present.FindIndex(source.edge)));
        for(const auto& modified:operation.Modified(source.edge))if(modified.ShapeType()==TopAbs_EDGE
            && present.Contains(modified))currentEdges.Add(present(present.FindIndex(modified)));
        struct Candidate {TopoDS_Edge edge;const EvaluatedSupportFace* target;};
        std::vector<Candidate> matches;
        for(int i=1;i<=currentEdges.Extent();++i){
            const auto shape=currentEdges(i);const auto edge=TopoDS::Edge(shape);
            if(BRepAdaptor_Curve(edge).GetType()!=GeomAbs_Line || !incident.Contains(shape))continue;
            const auto& faces=incident.FindFromKey(shape);
            TopTools_IndexedMapOfShape pair;for(const auto& face:faces)pair.Add(face);
            if(pair.Extent()!=2)continue;
            for(int side=1;side<=2;++side){
                const auto& currentTarget=pair(side),&currentTool=pair(3-side);
                const EvaluatedSupportFace* target=nullptr;int targetCount=0;
                for(const auto& retained:retainedFaces){
                    if(!retained.face.IsSame(currentTarget))continue;
                    const auto original=std::find_if(targetFaces.begin(),targetFaces.end(),[&](const auto& item){
                        return item.faceId==retained.faceId && item.producerFeatureId==retained.producerFeatureId;});
                    if(original==targetFaces.end() || !descendant(operation,original->face,currentTarget))continue;
                    target=&retained;++targetCount;
                }
                int toolCount=0;
                for(const auto& base:toolBases)if(base.sourceProfileId==profileId
                    && descendant(operation,base.face,currentTool))++toolCount;
                if(targetCount==1 && toolCount==1)matches.push_back({edge,target});
            }
        }
        if(matches.empty())continue;
        if(matches.size()!=1)throw GeometryKernel::EvaluationError({{"code","join_root_ambiguous"},
            {"message","Join source rim has multiple target-face intersection descendants"},
            {"featureId",featureId},{"sourceEdgeId",source.edgeId}});
        const auto& match=matches.front();
        if(std::any_of(output.begin(),output.end(),[&](const auto& prior){return prior.edge.IsSame(match.edge);}))
            throw GeometryKernel::EvaluationError({{"code","join_root_ambiguous"},
                {"message","Two Join source rims claim one current target intersection"},
                {"featureId",featureId},{"sourceEdgeId",source.edgeId}});
        const auto faceKey=match.target->producerFeatureId+"/"+match.target->faceId;
        const auto edgeId=QUuid::createUuidV5(QUuid(featureId),
            ("alloy-join-root/1\n"+source.edgeId+"\n"+faceKey).toUtf8()).toString(QUuid::WithoutBraces);
        auto provenance=source.provenance;
        provenance["role"]="join.root";provenance["lineage"]="exactModifiedBaseRimTargetIncidence";
        provenance["sourceToolEdgeId"]=source.edgeId;
        provenance["targetFaceId"]=match.target->faceId;
        provenance["targetProducerFeatureId"]=match.target->producerFeatureId;
        provenance.remove("startMm");provenance.remove("endMm");
        output.push_back({edgeId,match.edge,provenance});
    }
    return output;
}
