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
    std::vector<EvaluatedSolidEdge> result;
    for(const auto& input:inputs) {
        TopTools_IndexedMapOfShape candidates;
        if(present.Contains(input.edge))candidates.Add(input.edge);
        for(const auto& modified:operation.Modified(input.edge))if(modified.ShapeType()==TopAbs_EDGE && present.Contains(modified))candidates.Add(modified);
        auto mapped=input;
        if(candidates.Extent()==1){mapped.edge=TopoDS::Edge(candidates(1));mapped.provenance.remove("resolutionError");}
        else {mapped.edge.Nullify();mapped.provenance["resolutionError"]=candidates.IsEmpty() ? "deleted" : "ambiguous_split";}
        auto duplicate=std::find_if(result.begin(),result.end(),[&](const auto& edge){return edge.edgeId==mapped.edgeId;});
        if(duplicate==result.end())result.push_back(mapped);
        else if(!duplicate->edge.IsNull() && !mapped.edge.IsNull() && !duplicate->edge.IsSame(mapped.edge)){duplicate->edge.Nullify();duplicate->provenance["resolutionError"]="ambiguous_sources";}
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
