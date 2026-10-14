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
#include <QMap>
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
std::vector<EvaluatedSolidEdge> SolidFillet::transferEdges(BRepBuilderAPI_MakeShape& operation,const std::vector<EvaluatedSolidEdge>& inputs,const TopoDS_Shape& solid,bool straightModifiedOnly) {
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
        if(!group.ambiguous && group.candidates.Extent()==1){
            const auto next=TopoDS::Edge(group.candidates(1));
            if(straightModifiedOnly && !next.IsSame(mapped.edge)
                && !PlanarFaces::updateStraightBoundary(mapped.edge,next,mapped.provenance)) {
                mapped.edge.Nullify();mapped.provenance["resolutionError"]="modified_by_fillet";
            } else {
                // A subsequent Boolean may modify a retained fillet descendant.
                // Refresh oriented metadata or remove it before exposing a new
                // curve's own endpoints; never publish stale current endpoints.
                if(!straightModifiedOnly && !next.IsSame(mapped.edge)
                    && mapped.provenance.contains("endpointProvenance")
                    && !PlanarFaces::updateStraightBoundary(mapped.edge,next,mapped.provenance)) {
                    mapped.provenance.remove("endpointProvenance");mapped.provenance.remove("lineage");
                    mapped.provenance.remove("startMm");mapped.provenance.remove("endMm");
                }
                mapped.edge=next;mapped.provenance.remove("resolutionError");
            }
        }
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
            value["startMm"]=edge.provenance.contains("endpointProvenance") ? edge.provenance["startMm"] : QJsonValue(QJsonArray{a.X(),a.Y(),a.Z()});value["endMm"]=edge.provenance.contains("endpointProvenance") ? edge.provenance["endMm"] : QJsonValue(QJsonArray{b.X(),b.Y(),b.Z()});value["midpointMm"]=QJsonArray{mid.X(),mid.Y(),mid.Z()};
        }
        result.append(value);
    }
    return result;
}

void SolidFillet::evaluate(EvaluatedBody& body,const QJsonObject& parameters,const QString& featureId) {
    auto fail=[&](const QString& code,const QString& message,const QString& edgeId=QString{}) {
        throw GeometryKernel::EvaluationError({{"code",code},{"message",message},{"featureId",featureId},{"targetBodyId",body.bodyId},{"edgeId",edgeId}});
    };
    BRepFilletAPI_MakeFillet builder(body.shape);QJsonArray contours,allSeeds,groupSummaries;
    const auto grouped=parameters.contains("groups");
    const auto groups=grouped ? parameters["groups"].toArray() : QJsonArray{parameters};
    QMap<int,QString> contourOwners;QMap<int,double> contourRadii;
    for(const auto& value:groups){
        const auto group=value.toObject();
        const auto groupId=grouped ? group["id"].toString() : QStringLiteral("legacy");
        const double radius=group["radiusMm"].toDouble();QJsonArray groupContours;
        for(const auto& seed:group["edgeIds"].toArray()){
            const auto id=seed.toString();
            const auto edge=std::find_if(body.solidEdges.begin(),body.solidEdges.end(),[&](const auto& item){return item.edgeId==id;});
            if(edge==body.solidEdges.end())fail("fillet_edge_unavailable","Authored edge provenance is no longer present",id);
            if(edge->edge.IsNull())fail("fillet_edge_"+edge->provenance["resolutionError"].toString("unavailable"),"Authored edge was deleted or has ambiguous descendants",id);
            builder.Add(radius,edge->edge);allSeeds.append(id);
            const int contour=builder.Contour(edge->edge);
            if(contour==0)fail("fillet_contour_unavailable","Selected edge has no tangent contour",id);
            if(contourOwners.contains(contour) && contourOwners[contour]!=groupId)
                fail("fillet_group_overlap","Two radius groups select the same tangent contour",id);
            contourOwners[contour]=groupId;contourRadii[contour]=radius;
            if(!groupContours.contains(contour))groupContours.append(contour);
        }
        groupSummaries.append(QJsonObject{{"id",groupId},{"radiusMm",radius},{"seedEdgeIds",group["edgeIds"]},{"contourIndices",groupContours}});
    }
    for(int contour=1;contour<=builder.NbContours();++contour) {
        if(!contourOwners.contains(contour))fail("fillet_contour_unavailable","Tangent contour has no radius group");
        builder.SetRadius(contourRadii[contour],contour,1);
        QJsonArray chain;
        for(int index=1;index<=builder.NbEdges(contour);++index) {
            const auto actual=builder.Edge(contour,index);QStringList identities;
            for(const auto& edge:body.solidEdges)if(!edge.edge.IsNull() && edge.edge.IsSame(actual))identities.append(edge.edgeId);
            QJsonArray ids;for(const auto& id:identities)ids.append(id);
            chain.append(QJsonObject{{"edgeIds",ids},{"named",!ids.isEmpty()},{"radiusGroupId",contourOwners[contour]}});
        }
        contours.append(chain);
    }
    builder.Build();if(!builder.IsDone())fail("fillet_failed","The selected edges cannot be filleted at this radius");
    const auto result=builder.Shape();auto summary=GeometryKernel::summarize(result);
    if(!summary["valid"].toBool() || !summary["closed"].toBool() || summary["topology"].toObject()["solids"].toInt()!=1 || summary["volumeMm3"].toDouble()<=0)
        fail("fillet_invalid_result","Fillet must produce one valid closed solid");
    auto caps=PlanarFaces::transfer(builder,body.supportFaces,result,true);
    auto edges=transferEdges(builder,body.solidEdges,result,true);
    body.solidEdges=std::move(edges);
    body.shape=result;body.supportFaces=caps;
    QJsonArray capValues;for(const auto& cap:caps)capValues.append(QJsonObject{{"faceId",cap.faceId},{"bodyId",cap.bodyId},{"producerFeatureId",cap.producerFeatureId},{"role",cap.role},{"planar",true},{"frame",cap.frame},{"edges",GeometryKernel::supportEdgeDescriptors(cap)}});
    summary["supportFaces"]=capValues;summary["solidEdges"]=descriptors(body.solidEdges);summary["lastModifierFeatureId"]=featureId;summary["producerFeatureId"]=body.featureId;summary["operation"]="fillet";
    if(grouped)summary["radiusGroups"]=groupSummaries;else summary["radiusMm"]=parameters["radiusMm"];
    summary["selectedEdgeIds"]=allSeeds;summary["tangentContours"]=contours;body.summary=summary;
}
