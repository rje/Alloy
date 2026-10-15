#include "PlanarFaces.h"
#include "SolidFillet.h"
#include "RuledCornerFillet.h"
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepAlgoAPI_BooleanOperation.hxx>
#include <BRepAlgoAPI_Common.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepFilletAPI_MakeFillet.hxx>
#include <ChFiDS_ErrorStatus.hxx>
#include <ElCLib.hxx>
#include <BRep_Tool.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <gp_Circ.hxx>
#include <gp_Lin.hxx>
#include <gp_Pln.hxx>
#include <QUuid>
#include <QMap>
#include <QSet>
#include <algorithm>
#include <optional>
#include <cmath>
#include <numbers>
namespace {
QString identity(const QString& feature,const QByteArray& signature) {
    return QUuid::createUuidV5(QUuid(feature),QByteArray("alloy-solid-edge/1\n")+signature).toString(QUuid::WithoutBraces);
}

struct FilletResult {
    TopoDS_Shape shape;
    std::vector<EvaluatedSupportFace> caps;
    std::vector<EvaluatedSolidEdge> edges;
};
bool validSolid(const TopoDS_Shape& shape) {
    if(shape.IsNull() || !TopExp_Explorer(shape,TopAbs_SOLID).More())return false;
    const auto s=GeometryKernel::summarize(shape);
    return s.value("valid").toBool() && s.value("closed").toBool()
        && s.value("topology").toObject().value("solids").toInt()==1 && s.value("volumeMm3").toDouble()>0;
}

// Compose kernel history, never search for replacement identities by coordinates.
// Multiple distinct descendants remain visible to the existing ambiguity checks.
class PairedHistory final : public BRepBuilderAPI_MakeShape {
public:
    PairedHistory(BRepFilletAPI_MakeFillet& first,BRepFilletAPI_MakeFillet& second,BRepAlgoAPI_Common& common)
        : branches{&first,&second},intersection(common) {
        TopExp::MapShapes(first.Shape(),present[0]);TopExp::MapShapes(second.Shape(),present[1]);
        TopExp::MapShapes(common.Shape(),result);
    }
    const TopTools_ListOfShape& Modified(const TopoDS_Shape& source) override {
        changes.Clear();TopTools_IndexedMapOfShape candidates;
        for(int i=0;i<2;++i) {
            TopTools_IndexedMapOfShape intermediate;
            if(present[i].Contains(source))intermediate.Add(source);
            for(const auto& next:branches[i]->Modified(source))if(present[i].Contains(next))intermediate.Add(next);
            for(int j=1;j<=intermediate.Extent();++j) {
                const auto& next=intermediate(j);
                if(result.Contains(next))candidates.Add(next);
                for(const auto& final:intersection.Modified(next))if(result.Contains(final))candidates.Add(final);
            }
        }
        for(int i=1;i<=candidates.Extent();++i)changes.Append(candidates(i));
        return changes;
    }
private:
    BRepFilletAPI_MakeFillet* branches[2];BRepAlgoAPI_Common& intersection;
    TopTools_IndexedMapOfShape present[2],result;TopTools_ListOfShape changes;
};

std::optional<FilletResult> pairedRidge(const EvaluatedBody& body,BRepFilletAPI_MakeFillet& failed,
                                      const QMap<int,double>& radii) {
    // Only the full-width limit of two open analytic, coplanar extrusion rims.
    // Other contours, variable widths, unrelated provenance and stripe failures
    // keep the existing refusal. No radius or tolerance retry is performed.
    constexpr double tolerance=1e-7;
    if(failed.NbContours()!=2 || radii.size()!=2 || failed.NbFaultyContours()!=0
        || failed.NbFaultyVertices()==0 || failed.StripeStatus(1)!=ChFiDS_Ok || failed.StripeStatus(2)!=ChFiDS_Ok
        || failed.Closed(1) || failed.Closed(2) || failed.NbEdges(1)!=failed.NbEdges(2))return {};
    const double radius=radii.value(1);
    if(radius<=0 || std::abs(radius-radii.value(2))>tolerance)return {};
    QString profile,generator;std::vector<TopoDS_Edge> chains[2];
    std::optional<gp_Pln> plane;
    for(int c=1;c<=2;++c)for(int i=1;i<=failed.NbEdges(c);++i) {
        const auto edge=failed.Edge(c,i);
        const auto named=std::find_if(body.solidEdges.begin(),body.solidEdges.end(),[&](const auto& e){return !e.edge.IsNull() && e.edge.IsSame(edge);});
        if(named==body.solidEdges.end() || named->provenance.value("role")!="endRim")return {};
        const auto p=named->provenance.value("sourceProfileId").toString(),g=named->provenance.value("generatorFeatureId").toString();
        if(p.isEmpty() || g.isEmpty())return {};
        if(profile.isEmpty()){profile=p;generator=g;}else if(profile!=p || generator!=g)return {};
        const BRepAdaptor_Curve curve(edge);
        if(curve.GetType()!=GeomAbs_Line && curve.GetType()!=GeomAbs_Circle)return {};
        chains[c-1].push_back(edge);
        if(curve.GetType()==GeomAbs_Circle) {
            const auto circle=curve.Circle();
            if(!plane)plane=gp_Pln(circle.Location(),circle.Axis().Direction());
            else if(std::abs(plane->Axis().Direction().Dot(circle.Axis().Direction()))<1-1e-9)return {};
        }
    }
    if(!plane)return {}; // A curved ridge is the qualified workflow; no box-edge fallback.
    for(const auto& chain:chains)for(const auto& edge:chain) {
        const BRepAdaptor_Curve curve(edge);
        for(double fraction:{0.0,0.5,1.0})if(plane->Distance(curve.Value(curve.FirstParameter()+(curve.LastParameter()-curve.FirstParameter())*fraction))>tolerance)return {};
    }
    // Both rims must bound one planar strip. Its remaining boundary must be
    // exactly two unbranched original terminal paths connecting opposite rims.
    // A terminal may comprise several faces (the ported mouth does); mere
    // coincidence on infinite offset supports does not establish this contract.
    TopoDS_Face cap;TopTools_IndexedMapOfShape capEdges;
    for(TopExp_Explorer ex(body.shape,TopAbs_FACE);ex.More();ex.Next()) {
        const auto face=TopoDS::Face(ex.Current());if(BRepAdaptor_Surface(face).GetType()!=GeomAbs_Plane)continue;
        TopTools_IndexedMapOfShape present;TopExp::MapShapes(face,TopAbs_EDGE,present);bool all=true;
        for(const auto& chain:chains)for(const auto& edge:chain)all&=present.Contains(edge);
        if(all){if(!cap.IsNull())return {};cap=face;capEdges=present;}
    }
    if(cap.IsNull())return {};
    std::vector<TopoDS_Edge> ends;
    for(int i=1;i<=capEdges.Extent();++i) {
        const auto edge=TopoDS::Edge(capEdges(i));bool selected=false;
        for(const auto& chain:chains)for(const auto& rim:chain)selected|=rim.IsSame(edge);
        if(!selected)ends.push_back(edge);
    }
    const TopoDS_Vertex starts[]{failed.FirstVertex(1),failed.LastVertex(1)};
    const TopoDS_Vertex targets[]{failed.FirstVertex(2),failed.LastVertex(2)};
    QSet<int> usedEnds,usedTargets;
    for(const auto& start:starts) {
        if(start.IsNull())return {};auto current=start;bool reached=false;
        for(size_t step=0;step<=ends.size();++step) {
            int next=-1;TopoDS_Vertex destination;
            for(int i=0;i<static_cast<int>(ends.size());++i)if(!usedEnds.contains(i)) {
                TopoDS_Vertex a,b;TopExp::Vertices(ends[i],a,b);
                if(a.IsNull() || b.IsNull())return {};
                if(a.IsSame(current) || b.IsSame(current)) {
                    if(next!=-1)return {};next=i;destination=a.IsSame(current) ? b : a;
                }
            }
            if(next<0)return {};usedEnds.insert(next);current=destination;
            for(int t=0;t<2;++t)if(!targets[t].IsNull() && current.IsSame(targets[t])) {
                if(usedTargets.contains(t))return {};usedTargets.insert(t);reached=true;
            }
            if(reached)break;
        }
        if(!reached)return {};
    }
    if(usedEnds.size()!=static_cast<int>(ends.size()) || usedTargets.size()!=2)return {};
    QSet<int> paired;
    for(const auto& edge:chains[0]) {
        const BRepAdaptor_Curve a(edge);int match=-1;
        for(int j=0;j<static_cast<int>(chains[1].size());++j) {
            const BRepAdaptor_Curve b(chains[1][j]);if(a.GetType()!=b.GetType())continue;
            bool compatible=false;
            if(a.GetType()==GeomAbs_Circle) {
                const double spanA=a.LastParameter()-a.FirstParameter(),spanB=b.LastParameter()-b.FirstParameter();
                const double middleB=ElCLib::Parameter(a.Circle(),b.Value((b.FirstParameter()+b.LastParameter())/2));
                const double separation=std::abs(std::remainder(middleB-(a.FirstParameter()+a.LastParameter())/2,2*std::numbers::pi));
                compatible=spanA<std::numbers::pi && spanB<std::numbers::pi && separation<(spanA+spanB)/2-1e-9
                    && a.Circle().Location().Distance(b.Circle().Location())<tolerance
                    && std::abs(std::abs(a.Circle().Radius()-b.Circle().Radius())-2*radius)<tolerance;
            } else {
                const auto origin=a.Value(a.FirstParameter());const gp_Vec direction(a.Line().Direction());
                const double aEnd=gp_Vec(origin,a.Value(a.LastParameter())).Dot(direction);
                const double b0=gp_Vec(origin,b.Value(b.FirstParameter())).Dot(direction),b1=gp_Vec(origin,b.Value(b.LastParameter())).Dot(direction);
                const double overlap=std::min(std::max(0.0,aEnd),std::max(b0,b1))-std::max(std::min(0.0,aEnd),std::min(b0,b1));
                compatible=overlap>tolerance && std::abs(a.Line().Direction().Dot(b.Line().Direction()))>1-1e-9
                    && std::abs(a.Line().Distance(b.Line().Location())-2*radius)<tolerance;
            }
            if(compatible) {
                // Supporting lines/circles alone can pair disjoint finite arcs.
                BRepExtrema_DistShapeShape separation(edge,chains[1][j]);
                compatible=separation.IsDone() && std::abs(separation.Value()-2*radius)<tolerance;
            }
            if(compatible){if(match!=-1)return {};match=j;}
        }
        if(match<0 || paired.contains(match))return {};paired.insert(match);
    }
    BRepFilletAPI_MakeFillet first(body.shape),second(body.shape);
    BRepFilletAPI_MakeFillet* branches[]{&first,&second};
    for(int i=0;i<2;++i) {
        branches[i]->Add(radius,chains[i].front());branches[i]->SetRadius(radius,1,1);branches[i]->Build();
        if(!branches[i]->IsDone() || !validSolid(branches[i]->Shape()))return {};
        BRepAlgoAPI_Cut outside(branches[i]->Shape(),body.shape);
        if(!outside.IsDone() || TopExp_Explorer(outside.Shape(),TopAbs_FACE).More())return {};
    }
    BRepAlgoAPI_Common common(first.Shape(),second.Shape());
    if(!common.IsDone() || !validSolid(common.Shape()))return {};
    PairedHistory history(first,second,common);
    return FilletResult{common.Shape(),PlanarFaces::transfer(history,body.supportFaces,common.Shape(),true),
        SolidFillet::transferEdges(history,body.solidEdges,common.Shape(),true)};
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
    QMap<int,QString> contourOwners;QMap<int,double> contourRadii;QMap<QString,double> seedRadii;
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
            seedRadii[id]=radius;
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
    builder.Build();std::optional<FilletResult> paired;std::optional<RuledCornerFillet::Result> ruled;
    const bool ordinaryValid=builder.IsDone() && validSolid(builder.Shape());
    if(!ordinaryValid && !builder.IsDone()) {
        paired=pairedRidge(body,builder,contourRadii);
    }
    if(!ordinaryValid && !paired)ruled=RuledCornerFillet::evaluate(body,seedRadii,featureId);
    if(!ordinaryValid && !paired && !ruled)fail("fillet_failed","The selected edges cannot be filleted at this radius");
    const auto result=paired ? paired->shape : ruled ? ruled->shape : builder.Shape();auto summary=GeometryKernel::summarize(result);
    if(!summary["valid"].toBool() || !summary["closed"].toBool() || summary["topology"].toObject()["solids"].toInt()!=1 || summary["volumeMm3"].toDouble()<=0)
        fail("fillet_invalid_result","Fillet must produce one valid closed solid");
    auto caps=paired ? std::move(paired->caps) : ruled ? std::move(ruled->supportFaces) : PlanarFaces::transfer(builder,body.supportFaces,result,true);
    auto edges=paired ? std::move(paired->edges) : ruled ? std::move(ruled->solidEdges) : transferEdges(builder,body.solidEdges,result,true);
    body.solidEdges=std::move(edges);
    body.shape=result;body.supportFaces=caps;
    summary["supportFaces"]=PlanarFaces::descriptors(caps);summary["solidEdges"]=descriptors(body.solidEdges);summary["lastModifierFeatureId"]=featureId;summary["producerFeatureId"]=body.featureId;summary["operation"]="fillet";
    if(paired)summary["construction"]="pairedFullRoundCommon";
    if(ruled)summary["construction"]="ruledCornerCut";
    if(grouped)summary["radiusGroups"]=groupSummaries;else summary["radiusMm"]=parameters["radiusMm"];
    summary["selectedEdgeIds"]=allSeeds;summary["tangentContours"]=contours;body.summary=summary;
}
