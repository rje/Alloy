#include "PlanarFaces.h"
#include "SolidFillet.h"
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepAlgoAPI_BooleanOperation.hxx>
#include <BRepFilletAPI_MakeFillet.hxx>
#include <BRep_Tool.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <QUuid>
#include <QSet>
#include <algorithm>
namespace {
QString identity(const QString& feature,const QByteArray& signature) {
    return QUuid::createUuidV5(QUuid(feature),QByteArray("alloy-solid-edge/1\n")+signature).toString(QUuid::WithoutBraces);
}
}
std::vector<EvaluatedSolidEdge> SolidFillet::prismEdges(BRepPrimAPI_MakePrism& prism,const EvaluatedProfile& profile,const QString& generator) {
    std::vector<EvaluatedSolidEdge> result;QSet<QString> seen;
    const auto offer=[&](const QString& role,const QString& signature,const TopoDS_Shape& edge,QJsonObject provenance) {
        if(edge.IsNull() || edge.ShapeType()!=TopAbs_EDGE)return;
        const auto id=identity(generator,(profile.profileId+"\n"+role+"\n"+signature).toUtf8());
        auto existing=std::find_if(result.begin(),result.end(),[&](const auto& item){return item.edgeId==id;});
        if(existing!=result.end()) {if(!existing->edge.IsSame(edge))throw std::runtime_error("Ambiguous extrusion edge provenance");return;}
        provenance["generatorFeatureId"]=generator;provenance["sourceSketchId"]=profile.sketchId;provenance["sourceProfileId"]=profile.profileId;provenance["role"]=role;
        result.push_back({id,TopoDS::Edge(edge),provenance});
    };
    for(const auto& boundary:profile.boundaryEdges) {
        QStringList contacts{boundary.startContact,boundary.endContact};contacts.sort();
        QJsonObject provenance{{"sourceEntityId",boundary.sourceEntityId},{"startContact",contacts[0]},{"endContact",contacts[1]}};
        offer("endRim",boundary.sourceEntityId+"\n"+contacts.join("\n"),prism.LastShape(boundary.edge),provenance);
        offer("baseRim",boundary.sourceEntityId+"\n"+contacts.join("\n"),prism.FirstShape(boundary.edge),provenance);
        TopoDS_Vertex first,last;TopExp::Vertices(boundary.edge,first,last,true);
        for(const auto& endpoint:std::vector<std::pair<TopoDS_Vertex,QString>>{{first,boundary.startContact},{last,boundary.endContact}}) {
            if(endpoint.first.IsNull() || endpoint.second.isEmpty())continue;
            const auto& generated=prism.Generated(endpoint.first);TopoDS_Shape unique;int count=0;
            for(const auto& shape:generated)if(shape.ShapeType()==TopAbs_EDGE){unique=shape;++count;}
            if(count==1)offer("sideCorner",endpoint.second,unique,{{"contact",endpoint.second}});
        }
    }
    return result;
}
std::vector<EvaluatedSolidEdge> SolidFillet::transferEdges(BRepAlgoAPI_BooleanOperation& operation,const std::vector<EvaluatedSolidEdge>& inputs,const TopoDS_Shape& solid) {
    TopTools_IndexedMapOfShape present;TopExp::MapShapes(solid,TopAbs_EDGE,present);
    struct Resolution {EvaluatedSolidEdge source;TopTools_IndexedMapOfShape candidates;bool ambiguous=false;};
    QMap<QString,Resolution> groups;
    for(const auto& input:inputs) {
        auto& group=groups[input.edgeId];if(group.source.edgeId.isEmpty())group.source=input;
        group.ambiguous|=input.provenance["resolutionError"].toString().startsWith("ambiguous");
        if(input.edge.IsNull())continue;
        if(present.Contains(input.edge))group.candidates.Add(input.edge);
        for(const auto& modified:operation.Modified(input.edge))if(modified.ShapeType()==TopAbs_EDGE && present.Contains(modified))group.candidates.Add(modified);
    }
    std::vector<EvaluatedSolidEdge> result;
    for(const auto& group:groups) {
        auto mapped=group.source;
        if(!group.ambiguous && group.candidates.Extent()==1){mapped.edge=TopoDS::Edge(group.candidates(1));mapped.provenance.remove("resolutionError");}
        else {mapped.edge.Nullify();mapped.provenance["resolutionError"]=group.ambiguous || group.candidates.Extent()>1 ? "ambiguous_split" : "deleted";}
        result.push_back(mapped);
    }
    return result;
}
QJsonArray SolidFillet::descriptors(const std::vector<EvaluatedSolidEdge>& edges) {
    QJsonArray result;
    for(const auto& edge:edges) {
        QJsonObject value{{"edgeId",edge.edgeId},{"provenance",edge.provenance},{"available",!edge.edge.IsNull()}};
        if(!edge.edge.IsNull()) {
            BRepAdaptor_Curve curve(edge.edge);const auto a=curve.Value(curve.FirstParameter()),b=curve.Value(curve.LastParameter()),mid=curve.Value((curve.FirstParameter()+curve.LastParameter())/2);
            value["startMm"]=QJsonArray{a.X(),a.Y(),a.Z()};value["endMm"]=QJsonArray{b.X(),b.Y(),b.Z()};value["midpointMm"]=QJsonArray{mid.X(),mid.Y(),mid.Z()};
        }
        result.append(value);
    }
    return result;
}

void SolidFillet::evaluate(EvaluatedBody& body,const QJsonObject& parameters,const QString& featureId) {
    auto fail=[&](const QString& code,const QString& message,const QString& edgeId=QString{}) {
        throw GeometryKernel::EvaluationError({{"code",code},{"message",message},{"featureId",featureId},{"targetBodyId",body.bodyId},{"edgeId",edgeId}});
    };
    BRepFilletAPI_MakeFillet builder(body.shape);QSet<QString> seeds;QJsonArray contours;
    for(const auto& value:parameters["edgeIds"].toArray()) {
        const auto id=value.toString();
        const auto edge=std::find_if(body.solidEdges.begin(),body.solidEdges.end(),[&](const auto& item){return item.edgeId==id;});
        if(edge==body.solidEdges.end())fail("fillet_edge_unavailable","Authored edge provenance is no longer present",id);
        if(edge->edge.IsNull())fail("fillet_edge_"+edge->provenance["resolutionError"].toString("unavailable"),"Authored edge was deleted or has ambiguous descendants",id);
        builder.Add(parameters["radiusMm"].toDouble(),edge->edge);seeds.insert(id);
    }
    for(int contour=1;contour<=builder.NbContours();++contour) {
        QJsonArray chain;
        for(int index=1;index<=builder.NbEdges(contour);++index) {
            const auto actual=builder.Edge(contour,index);QStringList identities;
            for(const auto& edge:body.solidEdges)if(!edge.edge.IsNull() && edge.edge.IsSame(actual))identities.append(edge.edgeId);
            QJsonArray ids;for(const auto& id:identities)ids.append(id);
            chain.append(QJsonObject{{"edgeIds",ids},{"named",!ids.isEmpty()}});
        }
        contours.append(chain);
    }
    builder.Build();if(!builder.IsDone())fail("fillet_failed","The selected edges cannot be filleted at this radius");
    const auto result=builder.Shape();auto summary=GeometryKernel::summarize(result);
    if(!summary["valid"].toBool() || !summary["closed"].toBool() || summary["topology"].toObject()["solids"].toInt()!=1 || summary["volumeMm3"].toDouble()<=0)
        fail("fillet_invalid_result","Fillet must produce one valid closed solid");
    auto caps=PlanarFaces::transfer(builder,body.supportFaces,result);
    TopTools_IndexedMapOfShape edges;TopExp::MapShapes(result,TopAbs_EDGE,edges);
    for(auto& edge:body.solidEdges)if(!edge.edge.IsNull() && !edges.Contains(edge.edge)) {edge.edge.Nullify();edge.provenance["resolutionError"]="modified_by_fillet";}
    body.shape=result;body.supportFaces=caps;
    QJsonArray capValues;for(const auto& cap:caps)capValues.append(QJsonObject{{"faceId",cap.faceId},{"bodyId",cap.bodyId},{"producerFeatureId",cap.producerFeatureId},{"role",cap.role},{"planar",true},{"frame",cap.frame},{"edges",GeometryKernel::supportEdgeDescriptors(cap)}});
    summary["supportFaces"]=capValues;summary["solidEdges"]=descriptors(body.solidEdges);summary["lastModifierFeatureId"]=featureId;summary["producerFeatureId"]=body.featureId;summary["operation"]="fillet";summary["radiusMm"]=parameters["radiusMm"];summary["selectedEdgeIds"]=parameters["edgeIds"];summary["tangentContours"]=contours;body.summary=summary;
}
