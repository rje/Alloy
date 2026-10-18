#include "RoundedLoopOffset.h"
#include "PlanarBoundaryInset.h"
#include <BRepOffsetAPI_MakeOffset.hxx>
#include <BRepAlgoAPI_Common.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepGProp.hxx>
#include <BRepBndLib.hxx>
#include <BRepTools.hxx>
#include <BRepTools_WireExplorer.hxx>
#include <BRep_Builder.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <GProp_GProps.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <TopExp.hxx>
#include <QCryptographicHash>
#include <QSet>
#include <QUuid>
#include <Bnd_Box.hxx>
#include <cmath>

namespace {
[[noreturn]] void fail(const QString& code,const QString& message) {
    throw GeometryKernel::EvaluationError({{"code",code},{"message",message}});
}
double area(const TopoDS_Face& face) {GProp_GProps props;BRepGProp::SurfaceProperties(face,props);return std::abs(props.Mass());}
int count(const TopoDS_Shape& shape,TopAbs_ShapeEnum kind) {int n=0;for(TopExp_Explorer e(shape,kind);e.More();e.Next())++n;return n;}
TopoDS_Wire offsetWire(const TopoDS_Face& face,double amount) {
    BRepOffsetAPI_MakeOffset offset(face,GeomAbs_Arc);offset.Perform(amount);
    if(!offset.IsDone())fail("offset_failed","The exact face boundary could not be offset");
    const auto shape=offset.Shape();
    if(count(shape,TopAbs_WIRE)!=1)fail("ambiguous_offset","Offset must produce exactly one closed loop");
    const auto wire=TopoDS::Wire(TopExp_Explorer(shape,TopAbs_WIRE).Current());
    if(wire.IsNull() || !wire.Closed() || !BRepCheck_Analyzer(wire).IsValid())fail("invalid_offset","Offset loop is not closed and valid");
    return wire;
}
TopoDS_Face oneLoopFace(const TopoDS_Wire& wire) {
    BRepBuilderAPI_MakeFace maker(wire,true);
    if(!maker.IsDone())fail("invalid_offset","Offset loop is not a planar face");
    auto face=maker.Face();if(!BRepCheck_Analyzer(face).IsValid())fail("invalid_offset","Offset loop is invalid");return face;
}
}

QString RoundedLoopOffset::outerLoopId(const EvaluatedSupportFace& support) {
    if(support.face.IsNull() || count(support.face,TopAbs_WIRE)!=1 || support.boundaryEdges.size()!=8)
        fail("unsupported_offset_boundary","Select one rounded-rectangle outer loop with four lines and four arcs");
    QSet<QString> identities;int lines=0,arcs=0;
    for(const auto& edge:support.boundaryEdges) {
        const auto id=edge.provenance["sourceEntityId"].toString(),sketch=edge.provenance["sourceSketchId"].toString();
        if(QUuid(id).isNull() || QUuid(sketch).isNull() || identities.contains(sketch+":"+id))
            fail("ambiguous_offset_boundary","Outer boundary lacks unique authored entity lineage");
        identities.insert(sketch+":"+id);
        if(edge.type=="line")++lines;else if(edge.type=="arc")++arcs;else fail("unsupported_offset_boundary","Only analytic line and arc boundaries are supported");
    }
    if(lines!=4 || arcs!=4)fail("unsupported_offset_boundary","This offset requires four lines and four tangent arcs");
    auto names=identities.values();names.sort();
    return QUuid::createUuidV5(QUuid(support.faceId),QByteArray("alloy-rounded-outer-loop/1\n")+names.join("\n").toUtf8()).toString(QUuid::WithoutBraces);
}

QString RoundedLoopOffset::insetLoopId(const EvaluatedSupportFace& support) {
    if(support.face.IsNull() || QUuid(support.faceId).isNull() || count(support.face,TopAbs_WIRE)!=1)
        fail("unsupported_inset_boundary","Select one retained planar face with a closed boundary");
    const auto wire=BRepTools::OuterWire(support.face);
    if(wire.IsNull() || !wire.Closed() || support.boundaryEdges.size()!=size_t(count(wire,TopAbs_EDGE)))
        fail("ambiguous_inset_lineage","Inset source boundary does not have complete retained edges");
    QSet<QString> identities;
    for(const auto& edge:support.boundaryEdges) {
        if(QUuid(edge.edgeId).isNull() || identities.contains(edge.edgeId))
            fail("ambiguous_inset_lineage","Inset source edge identities are missing or repeated");
        identities.insert(edge.edgeId);
    }
    auto names=identities.values();names.sort();
    return QUuid::createUuidV5(QUuid(support.faceId),
        QByteArray("alloy-planar-inset-loop/1\n")+names.join("\n").toUtf8()).toString(QUuid::WithoutBraces);
}

QString RoundedLoopOffset::validate(const QJsonObject& parameters) {
    if(!parameters.contains("loopOffsets"))return {};
    if(parameters["plane"]!="face" || !parameters["loopOffsets"].isArray() || parameters["loopOffsets"].toArray().size()!=1)
        return "One retained loop offset requires a face sketch";
    for(const auto& key:{"points","entities","constraints","dimensions","texts","projections","pointReferences"})
        if(!parameters.value(key).toArray().isEmpty())return "Retained loop offset requires its own generated-only sketch";
    const auto object=parameters["loopOffsets"].toArray().first().toObject();
    const bool inset=object.value("kind")=="planarInsetV1";
    if(object.size()!=(inset?5:4) || (object.contains("kind") && !inset)
        || QUuid(object["id"].toString()).isNull() || QUuid(object["outerLoopId"].toString()).isNull()
        || object["chain"]!=true || !object["distanceMm"].isDouble()
        || !std::isfinite(object["distanceMm"].toDouble()) || std::abs(object["distanceMm"].toDouble())<0.001
        || std::abs(object["distanceMm"].toDouble())>100000 || (inset && object["distanceMm"].toDouble()>0))
        return "Invalid retained closed-loop offset";
    return {};
}

EvaluatedSketch RoundedLoopOffset::evaluate(const QJsonObject& feature,const EvaluatedSupportFace& support,
    const QJsonObject& resolvedFrame) {
    const auto p=feature["parameters"].toObject();const auto error=validate(p);
    if(!error.isEmpty())fail("invalid_loop_offset",error);
    if(p["support"].toObject()!=QJsonObject{{"producerFeatureId",support.producerFeatureId},{"bodyId",support.bodyId},{"faceId",support.faceId}})
        fail("support_unavailable","Loop offset support identity changed");
    const auto object=p["loopOffsets"].toArray().first().toObject();const auto id=object["id"].toString();
    if(object.value("kind")=="planarInsetV1") {
        if(object["outerLoopId"]!=insetLoopId(support))
            fail("offset_lineage_unavailable","The retained inset source boundary changed");
        if(resolvedFrame.isEmpty())fail("support_unavailable","Inset requires the current resolved sketch frame");
        const auto inset=PlanarBoundaryInset::evaluate(support,-object["distanceMm"].toDouble(),id);
        const auto wire=BRepTools::OuterWire(inset.face);
        struct NamedEdge {TopoDS_Edge edge;QString id,type;QJsonObject ancestry;};
        std::vector<NamedEdge> ordered;QSet<QString> matched;
        for(BRepTools_WireExplorer it(wire,inset.face);it.More();it.Next()) {
            const auto edge=TopoDS::Edge(it.Current());
            const EvaluatedSupportEdge* found=nullptr;
            for(const auto& derived:inset.boundaryEdges)if(derived.edge.IsSame(edge)) {
                if(found)fail("ambiguous_inset_lineage","One inset edge has multiple retained identities");
                found=&derived;
            }
            if(!found || matched.contains(found->edgeId))
                fail("ambiguous_inset_lineage","Inset wire lost a unique generated edge identity");
            matched.insert(found->edgeId);ordered.push_back({edge,found->edgeId,found->type,found->provenance});
        }
        if(ordered.size()<3 || ordered.size()!=inset.boundaryEdges.size())
            fail("ambiguous_inset_lineage","Inset edge registry and closed wire differ");
        std::vector<QString> contacts;contacts.reserve(ordered.size());
        for(size_t i=0;i<ordered.size();++i) {
            const auto& previous=ordered[(i+ordered.size()-1)%ordered.size()];
            const auto& current=ordered[i];
            if(!TopExp::LastVertex(previous.edge,true).IsSame(TopExp::FirstVertex(current.edge,true)))
                fail("ambiguous_inset_lineage","Inset wire has a disconnected generated contact");
            QStringList pair{previous.id,current.id};pair.sort();
            contacts.push_back(QUuid::createUuidV5(QUuid(id),
                QByteArray("alloy-planar-inset-contact/1\n")+pair.join("\n").toUtf8()).toString(QUuid::WithoutBraces));
        }
        const auto profileId=QUuid::createUuidV5(QUuid(id),"alloy-inset-interior/1").toString(QUuid::WithoutBraces);
        EvaluatedProfile profile{profileId,feature["id"].toString(),"face",inset.face,{}, {}};
        QJsonArray edges;
        for(size_t i=0;i<ordered.size();++i) {
            const auto& entry=ordered[i];const auto end=contacts[(i+1)%ordered.size()];
            profile.boundaryEdges.push_back({feature["id"].toString(),entry.id,contacts[i],end,entry.type,entry.edge});
            edges.append(QJsonObject{{"edgeId",entry.id},{"type",entry.type},{"startContact",contacts[i]},
                {"endContact",end},{"ancestry",entry.ancestry}});
        }
        auto profileSummary=inset.summary;
        profileSummary["profileId"]=profileId;profileSummary["sketchId"]=feature["id"];
        profileSummary["plane"]="face";profileSummary["frame"]=resolvedFrame;
        profileSummary["innerLoopCount"]=0;profileSummary["sourceLoopId"]=object["outerLoopId"];
        profileSummary["offsetId"]=id;profileSummary["signedDistanceMm"]=object["distanceMm"];
        profileSummary["boundaryEdges"]=edges;profile.summary=profileSummary;
        TopoDS_Compound strokes;BRep_Builder builder;builder.MakeCompound(strokes);
        builder.Add(strokes,BRepTools::OuterWire(support.face));builder.Add(strokes,wire);
        Bnd_Box box;BRepBndLib::AddOptimal(strokes,box,false,false);
        double x0,y0,z0,x1,y1,z1;box.Get(x0,y0,z0,x1,y1,z1);
        QJsonObject summary{{"kernel","OCCT"},{"version",ALLOY_OCCT_VERSION},{"type","sketch"},{"valid",true},
            {"plane","face"},{"frame",resolvedFrame},{"solverIntegrated",false},
            {"solver",QJsonObject{{"status","retained generated inset"},{"dof",0}}},
            {"entities",QJsonArray{}},{"entityCount",0},{"pointCount",0},
            {"profiles",QJsonArray{profileSummary}},
            {"profileDiagnostics",QJsonObject{{"code","closed_loop_inset"},{"profileCount",1}}},
            {"loopOffsets",p["loopOffsets"]},
            {"boundsMm",QJsonObject{{"min",QJsonArray{x0,y0,z0}},
                {"max",QJsonArray{x1,y1,z1}},{"size",QJsonArray{x1-x0,y1-y0,z1-z0}}}}};
        return {feature["id"].toString(),feature["name"].toString(),"face",feature["visible"].toBool(),
            strokes,summary,{profile},resolvedFrame,feature["ownerId"].toString(),{}};
    }
    if(object["outerLoopId"]!=outerLoopId(support))fail("offset_lineage_unavailable","The retained source outer loop changed");
    const auto sourceWire=BRepTools::OuterWire(support.face);
    if(sourceWire.IsNull() || count(support.face,TopAbs_WIRE)!=1 || count(sourceWire,TopAbs_EDGE)!=8)
        fail("ambiguous_offset_boundary","The source face has more than one boundary or changed topology");
    const auto sourceArea=area(support.face),distance=object["distanceMm"].toDouble();
    if(sourceArea<=0)fail("invalid_offset","Source face has no positive area");
    // The kernel's offset sign follows wire orientation. Test both orientations
    // and retain only the candidate on the requested geometric side.
    TopoDS_Wire offset;TopoDS_Face offsetFace;int validCandidates=0;
    for(const double kernelDistance:{distance,-distance}) {
        try {
            auto candidate=offsetWire(support.face,kernelDistance);
            if(count(candidate,TopAbs_EDGE)!=8)continue;
            bool analytic=true;for(TopExp_Explorer e(candidate,TopAbs_EDGE);e.More();e.Next()) {
                const auto type=BRepAdaptor_Curve(TopoDS::Edge(e.Current())).GetType();
                if(type!=GeomAbs_Line && type!=GeomAbs_Circle){analytic=false;break;}
            }
            if(!analytic)continue;
            auto candidateFace=oneLoopFace(candidate);const double candidateArea=area(candidateFace);
            if(distance<0 ? candidateArea>=sourceArea-1e-7 : candidateArea<=sourceArea+1e-7)continue;
            // Common planar overlap gives an exact containment test; area alone
            // cannot distinguish a translated or self-intersecting result.
            BRepAlgoAPI_Common common(support.face,candidateFace);common.Build();
            if(!common.IsDone() || !BRepCheck_Analyzer(common.Shape()).IsValid())continue;
            GProp_GProps props;BRepGProp::SurfaceProperties(common.Shape(),props);
            const double expected=distance<0 ? candidateArea : sourceArea;
            if(std::abs(std::abs(props.Mass())-expected)>std::max(1e-5,expected*1e-7))continue;
            offset=candidate;offsetFace=candidateFace;++validCandidates;
        } catch(const GeometryKernel::EvaluationError&) {} catch(const Standard_Failure&) {}
    }
    if(validCandidates!=1)fail("ambiguous_offset","Offset did not yield one bounded loop on the requested side");
    const auto outer=distance<0 ? sourceWire : offset,inner=distance<0 ? offset : sourceWire;
    BRepBuilderAPI_MakeFace maker(outer,true);if(!maker.IsDone())fail("invalid_offset","Cannot form outer offset face");
    maker.Add(TopoDS::Wire(inner.Reversed()));if(!maker.IsDone() || !BRepCheck_Analyzer(maker.Face()).IsValid())
        fail("invalid_offset","Offset annulus is invalid or collapsed");
    const auto annulus=maker.Face();const double innerArea=distance<0 ? area(offsetFace) : sourceArea,outerArea=distance<0 ? sourceArea : area(offsetFace);
    if(innerArea<=1e-6 || outerArea-innerArea<=1e-6 || std::abs(area(annulus)-(outerArea-innerArea))>std::max(1e-5,outerArea*1e-7))
        fail("collapsed_offset","Offset produces no valid annular region");
    const auto profileId=QUuid::createUuidV5(QUuid(id),"alloy-offset-annulus/1").toString(QUuid::WithoutBraces);
    Bnd_Box innerBox;BRepBndLib::AddOptimal(inner,innerBox,false,false);
    double iu0,iv0,iz0,iu1,iv1,iz1;innerBox.Get(iu0,iv0,iz0,iu1,iv1,iz1);
    QJsonArray innerRadii;for(TopExp_Explorer edge(inner,TopAbs_EDGE);edge.More();edge.Next()){
        const BRepAdaptor_Curve curve(TopoDS::Edge(edge.Current()));
        if(curve.GetType()==GeomAbs_Circle)innerRadii.append(curve.Circle().Radius());
    }
    QJsonObject profileSummary{{"profileId",profileId},{"sketchId",feature["id"]},{"plane","face"},{"frame",support.frame},
        {"areaMm2",outerArea-innerArea},{"innerLoopCount",1},{"sourceLoopId",object["outerLoopId"]},{"offsetId",id},{"distanceMm",distance},
        {"innerBoundsMm",QJsonObject{{"min",QJsonArray{iu0,iv0,iz0}},{"max",QJsonArray{iu1,iv1,iz1}},{"size",QJsonArray{iu1-iu0,iv1-iv0,iz1-iz0}}}},
        {"innerArcRadiiMm",innerRadii}};
    EvaluatedProfile profile{profileId,feature["id"].toString(),"face",annulus,profileSummary,{}};
    TopoDS_Compound strokes;BRep_Builder builder;builder.MakeCompound(strokes);builder.Add(strokes,sourceWire);builder.Add(strokes,offset);
    Bnd_Box box;BRepBndLib::AddOptimal(strokes,box,false,false);double x0,y0,z0,x1,y1,z1;box.Get(x0,y0,z0,x1,y1,z1);
    QJsonObject summary{{"kernel","OCCT"},{"version",ALLOY_OCCT_VERSION},{"type","sketch"},{"valid",true},{"plane","face"},{"frame",support.frame},
        {"solverIntegrated",false},{"solver",QJsonObject{{"status","retained generated loop"},{"dof",0}}},{"entities",QJsonArray{}},
        {"entityCount",0},{"pointCount",0},{"profiles",QJsonArray{profileSummary}},{"profileDiagnostics",QJsonObject{{"code","closed_loop_offset"},{"profileCount",1}}},
        {"loopOffsets",p["loopOffsets"]},{"boundsMm",QJsonObject{{"min",QJsonArray{x0,y0,z0}},{"max",QJsonArray{x1,y1,z1}},{"size",QJsonArray{x1-x0,y1-y0,z1-z0}}}}};
    return {feature["id"].toString(),feature["name"].toString(),"face",feature["visible"].toBool(),strokes,summary,{profile},support.frame,feature["ownerId"].toString(),{}};
}

QJsonObject RoundedLoopOffset::deleteProfile(const QJsonObject& feature,const EvaluatedSupportFace& support,
    const QJsonArray& profileIds) {
    const auto parameters=feature.value("parameters").toObject();
    const auto error=validate(parameters);
    if(!parameters.contains("loopOffsets") || !error.isEmpty())fail("invalid_offset_owner",error.isEmpty() ? "Select a retained loop Offset result" : error);
    if(profileIds.size()!=1 || !profileIds.first().isString() || QUuid(profileIds.first().toString()).isNull())
        fail("invalid_profile_target","Select exactly one current loop Offset result profile");
    // Evaluation checks exact face and boundary lineage before resolving ownership.
    const auto evaluated=evaluate(feature,support,support.frame);
    const auto owner=parameters.value("loopOffsets").toArray().first().toObject();
    if(evaluated.profiles.size()!=1 || evaluated.profiles.front().profileId!=profileIds.first().toString()
        || evaluated.profiles.front().summary.value("offsetId")!=owner.value("id"))
        fail("stale_profile","The loop Offset result changed; cancel and reopen the sketch, then select its current result");
    auto candidate=feature;auto remaining=parameters;remaining.remove("loopOffsets");candidate["parameters"]=remaining;
    return candidate;
}
