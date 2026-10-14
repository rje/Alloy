#include "SolidChamfer.h"
#include "SolidFillet.h"
#include "PlanarFaces.h"
#include <BRepFilletAPI_MakeChamfer.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <algorithm>
void SolidChamfer::evaluate(EvaluatedBody& body,const QJsonObject& parameters,const QString& featureId) {
    auto fail=[&](const QString& code,const QString& message,const QString& edgeId=QString{}) {
        throw GeometryKernel::EvaluationError({{"code",code},{"message",message},{"featureId",featureId},{"targetBodyId",body.bodyId},{"edgeId",edgeId}});
    };
    BRepFilletAPI_MakeChamfer builder(body.shape);
    for(const auto& value:parameters["edgeIds"].toArray()) {
        const auto id=value.toString();
        const auto edge=std::find_if(body.solidEdges.begin(),body.solidEdges.end(),[&](const auto& item){return item.edgeId==id;});
        if(edge==body.solidEdges.end())fail("chamfer_edge_unavailable","Authored edge provenance is no longer present",id);
        if(edge->edge.IsNull())fail("chamfer_edge_"+edge->provenance["resolutionError"].toString("unavailable"),"Authored edge was deleted or has ambiguous descendants",id);
        builder.Add(parameters["distanceMm"].toDouble(),edge->edge);
    }
    builder.Build();if(!builder.IsDone())fail("chamfer_failed","The selected edges cannot be chamfered at this distance");
    const auto result=builder.Shape();auto summary=GeometryKernel::summarize(result);
    if(!summary["valid"].toBool() || !summary["closed"].toBool() || summary["topology"].toObject()["solids"].toInt()!=1 || summary["volumeMm3"].toDouble()<=0)
        fail("chamfer_invalid_result","Chamfer must produce one valid closed solid");
    auto caps=PlanarFaces::transfer(builder,body.supportFaces,result);
    auto edges=SolidFillet::transferEdges(builder,body.solidEdges,result,true);
    body.shape=result;body.supportFaces=caps;body.solidEdges=std::move(edges);
    QJsonArray capValues;for(const auto& cap:caps)capValues.append(QJsonObject{{"faceId",cap.faceId},{"bodyId",cap.bodyId},{"producerFeatureId",cap.producerFeatureId},{"role",cap.role},{"planar",true},{"frame",cap.frame},{"edges",GeometryKernel::supportEdgeDescriptors(cap)}});
    summary["supportFaces"]=capValues;summary["solidEdges"]=SolidFillet::descriptors(body.solidEdges);summary["lastModifierFeatureId"]=featureId;summary["producerFeatureId"]=body.featureId;summary["operation"]="chamfer";summary["distanceMm"]=parameters["distanceMm"];summary["selectedEdgeIds"]=parameters["edgeIds"];body.summary=summary;
}
