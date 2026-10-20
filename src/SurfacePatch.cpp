// SPDX-License-Identifier: MIT
#include "SurfacePatch.h"
#include "PlanarFaces.h"
#include "SolidFillet.h"
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepAlgoAPI_Check.hxx>
#include <BRepGProp.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <Geom_BezierSurface.hxx>
#include <Geom_BezierCurve.hxx>
#include <Geom_BSplineCurve.hxx>
#include <Geom_Line.hxx>
#include <Geom2d_Line.hxx>
#include <TColgp_Array2OfPnt.hxx>
#include <array>
#include <algorithm>
#include <BRep_Tool.hxx>
#include <GProp_GProps.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <QUuid>
#include <cmath>

namespace {
constexpr double joinTolerance=1e-7;
constexpr double boundaryTolerance=0.01;
QJsonArray xyz(const auto& p){return {p.X(),p.Y(),p.Z()};}

EvaluatedBody fourEdgePatch(const QJsonObject& feature,const EvaluatedDocument& prior){
    const auto params=feature.value("parameters").toObject();const auto featureId=feature.value("id").toString();
    auto reject=[&](const QString& code,const QString& message){auto details=params;details["featureId"]=featureId;details["featureType"]="surfacePatch";details["code"]=code;details["message"]=message;throw GeometryKernel::EvaluationError(details);};
    struct Boundary{QString sketchId,entityId;TopoDS_Edge edge;gp_Pnt a,b;};
    std::array<Boundary,4> selected;const auto refs=params.value("boundaryEdges").toArray();
    if(refs.size()!=4 || params.value("continuity")!="g0")reject("patch_invalid_loop","Select exactly four distinct ordered ordinary Lines with Connected (G0)");
    QSet<QString> unique;
    for(int i=0;i<4;++i){const auto ref=refs[i].toObject();auto& b=selected[i];b.sketchId=ref.value("sketchId").toString();b.entityId=ref.value("entityId").toString();
        const auto key=b.sketchId+"/"+b.entityId;
        if(unique.contains(key))reject("patch_invalid_loop","Boundary Lines must be distinct");unique.insert(key);
        const EvaluatedSketch* sketch=nullptr;for(const auto& s:prior.sketches)if(s.featureId==b.sketchId)sketch=&s;
        if(!sketch || !sketch->entityEdges.contains(b.entityId))reject("surface_source_unavailable","A selected Patch Line is unavailable in its earlier source sketch");
        bool ordinary=false;for(const auto& value:sketch->summary.value("entities").toArray()){const auto e=value.toObject();if(e.value("id")==b.entityId)ordinary=e.value("type")=="line" && !e.value("construction").toBool();}
        if(!ordinary)reject("unsupported_patch_boundary","Four-edge Patch supports ordinary straight Lines only");
        b.edge=sketch->entityEdges.value(b.entityId);const BRepAdaptor_Curve curve(b.edge);
        if(curve.GetType()!=GeomAbs_Line)reject("unsupported_patch_boundary","Selected solved boundaries must remain exact straight Lines");
        b.a=curve.Value(curve.FirstParameter());b.b=curve.Value(curve.LastParameter());
        if(b.a.Distance(b.b)<=joinTolerance)reject("patch_invalid_loop","Patch boundary Lines must have nonzero length");
    }
    // Retain the authored cyclic edge order. Only traversal direction may reverse.
    std::array<gp_Pnt,4> corners;bool connected=false;
    for(bool reverse:{false,true}){
        corners[0]=reverse ? selected[0].b:selected[0].a;corners[1]=reverse ? selected[0].a:selected[0].b;bool ok=true;
        auto end=corners[1];
        for(int i=1;i<4;++i){const auto& b=selected[i];gp_Pnt next;
            if(end.Distance(b.a)<=joinTolerance)next=b.b;
            else if(end.Distance(b.b)<=joinTolerance)next=b.a;
            else {ok=false;break;}
            if(i<3)corners[i+1]=next;else if(next.Distance(corners[0])>joinTolerance)ok=false;end=next;
        }
        if(ok){connected=true;break;}
    }
    if(!connected)reject("patch_invalid_loop","Select four Lines in cyclic order with exact shared endpoints; the boundary is open or disconnected");
    for(int i=0;i<4;++i)for(int j=i+1;j<4;++j)if(corners[i].Distance(corners[j])<=joinTolerance)reject("patch_invalid_loop","Patch requires four distinct corners without branches");
    for(int i=0;i<2;++i){BRepExtrema_DistShapeShape separation(selected[i].edge,selected[i+2].edge);separation.Perform();
        if(!separation.IsDone() || separation.Value()<=joinTolerance)reject("patch_invalid_loop","Opposite boundary Lines must not intersect or overlap");}
    const gp_Vec ab(corners[0],corners[1]),ad(corners[0],corners[3]);const auto baseNormal=ab.Crossed(ad);
    if(baseNormal.Magnitude()<=joinTolerance || std::abs(gp_Vec(corners[0],corners[2]).Dot(baseNormal))/baseNormal.Magnitude()<=joinTolerance)
        reject("patch_nonplanar_required","Four-edge mode requires a nonplanar loop; use Boundary Profile for a planar Patch");
    // Nonzero corner determinant also proves regularity and injectivity of
    // S=A+u*AB+v*AD+u*v*T: AB, AD and T are linearly independent. Equal
    // surface points therefore require equal u and v; parallel derivatives
    // would contradict that same independence. A twisted sheet may have
    // corner normals more than 90 degrees apart without folding.
    // Four straight boundaries have an exact degree-(1,1) Coons span. No fitting,
    // endpoint healing, triangulated substitute or tolerance approximation is used.
    TColgp_Array2OfPnt poles(1,2,1,2);poles(1,1)=corners[0];poles(2,1)=corners[1];poles(2,2)=corners[2];poles(1,2)=corners[3];
    Handle(Geom_BezierSurface) surface=new Geom_BezierSurface(poles);
    BRepBuilderAPI_MakeFace builder(surface,joinTolerance);if(!builder.IsDone())reject("patch_build_failed","OCCT could not construct the four-Line bilinear sheet");
    const auto face=builder.Face();
    const auto actualSurface=Handle(Geom_BezierSurface)::DownCast(BRep_Tool::Surface(face));
    if(actualSurface.IsNull() || actualSurface->UDegree()!=1 || actualSurface->VDegree()!=1 || actualSurface->IsURational() || actualSurface->IsVRational())
        reject("patch_boundary_tolerance","Output support must remain a nonrational degree-(1,1) surface");
    EvaluatedBody body;body.featureId=featureId;body.bodyId=feature.value("bodyId").toString();body.name=feature.value("name").toString();body.visible=feature.value("visible").toBool(true);body.shape=face;body.summary=GeometryKernel::summarize(body.shape);
    TopTools_IndexedMapOfShape wires,edges;TopExp::MapShapes(face,TopAbs_WIRE,wires);TopExp::MapShapes(face,TopAbs_EDGE,edges);const auto topology=body.summary.value("topology").toObject();
    if(!body.summary.value("valid").toBool() || topology.value("faces")!=1 || topology.value("solids")!=0 || edges.Extent()!=4 || wires.Extent()!=1 || !BRep_Tool::IsClosed(wires(1)) || !body.summary.value("volumeMm3").isNull() || body.summary.value("areaMm2").toDouble()<=0)
        reject("patch_invalid","Patch must produce one valid nonplanar face, one closed four-edge wire and zero solids");
    BRepAlgoAPI_Check interference(face,false,true);if(!interference.IsValid())reject("patch_invalid_loop","Patch boundary or sheet has geometric self-interference");
    QByteArray lineage;for(const auto& b:selected)lineage+=b.sketchId.toUtf8()+"/"+b.entityId.toUtf8()+"/";
    auto stable=[&](const QByteArray& role){return QUuid::createUuidV5(QUuid(featureId),QByteArray("alloy-surface-patch-four-lines/1/")+lineage+role).toString(QUuid::WithoutBraces);};
    QJsonObject provenance{{"generatorFeatureId",featureId},{"lineage","surfacePatchFourLines"},{"boundaryEdges",refs},{"continuity","g0"}};
    EvaluatedSupportFace support{stable("face"),body.bodyId,featureId,"surfacePatch.sheet",face,{},{},provenance};
    TopTools_IndexedMapOfShape retained;QJsonArray certificates;double maxDeviation=0,perimeter=0;
    for(int i=0;i<4;++i){const auto& source=selected[i];const auto a=corners[i],b=corners[(i+1)%4];TopoDS_Edge current;QJsonObject certificate;
        for(int e=1;e<=edges.Extent();++e){if(retained.Contains(edges(e)))continue;const auto candidate=TopoDS::Edge(edges(e));const BRepAdaptor_Curve curve(candidate);const auto first=curve.Value(curve.FirstParameter()),last=curve.Value(curve.LastParameter());
            if((first.Distance(a)<=joinTolerance && last.Distance(b)<=joinTolerance) || (first.Distance(b)<=joinTolerance && last.Distance(a)<=joinTolerance)){if(!current.IsNull())reject("patch_lineage_unavailable","More than one output boundary matches a source Line");current=candidate;}
        }
        if(current.IsNull())reject("patch_lineage_unavailable","Could not identify every actual output boundary from its source Line");retained.Add(current);
        double first,last;const auto curve=BRep_Tool::Curve(current,first,last);QJsonArray control;QString curveType;
        const auto bezier=Handle(Geom_BezierCurve)::DownCast(curve);const auto bspline=Handle(Geom_BSplineCurve)::DownCast(curve);const auto line=Handle(Geom_Line)::DownCast(curve);
        if(!bezier.IsNull() && !bezier->IsRational() && bezier->Degree()==1){curveType="bezier";for(int j=1;j<=bezier->NbPoles();++j)control.append(xyz(bezier->Pole(j)));}
        else if(!bspline.IsNull() && !bspline->IsRational() && bspline->Degree()==1 && bspline->NbPoles()==2){curveType="bspline";for(int j=1;j<=bspline->NbPoles();++j)control.append(xyz(bspline->Pole(j)));}
        else if(!line.IsNull()){curveType="line";control={xyz(curve->Value(first)),xyz(curve->Value(last))};}
        else reject("patch_boundary_tolerance","An output boundary lacks an exact linear whole-curve certificate");
        // Convex-hull bound: every point of a degree-1 nonrational curve is in
        // the segment between these poles. Distance to a finite segment is
        // convex, so the maximum pole distance bounds the entire curve.
        double deviation=0;const gp_Vec span(source.a,source.b);for(const auto& value:control){const auto q=value.toArray();const gp_Pnt pole(q[0].toDouble(),q[1].toDouble(),q[2].toDouble());const double t=std::clamp(gp_Vec(source.a,pole).Dot(span)/span.SquareMagnitude(),0.,1.);deviation=std::max(deviation,pole.Distance(source.a.Translated(span*t)));}
        // Certify that the actual edge's pcurve traverses one complete natural
        // iso of the same degree-(1,1) support, rather than trusting 3D endpoints.
        double pf,pl;const auto pc=BRep_Tool::CurveOnSurface(current,face,pf,pl);const auto pcLine=Handle(Geom2d_Line)::DownCast(pc);
        if(pcLine.IsNull() || std::abs(pf)>joinTolerance || std::abs(pl-1.)>joinTolerance)reject("patch_boundary_tolerance","Output edge is not a complete natural surface iso");
        const auto pa=pc->Value(pf),pb=pc->Value(pl);const bool uIso=std::abs(pa.X()-pb.X())<=joinTolerance;
        const bool vIso=std::abs(pa.Y()-pb.Y())<=joinTolerance;
        const double fixed=uIso?pa.X():pa.Y(),lo=uIso?std::min(pa.Y(),pb.Y()):std::min(pa.X(),pb.X()),hi=uIso?std::max(pa.Y(),pb.Y()):std::max(pa.X(),pb.X());
        if(uIso==vIso || std::min(std::abs(fixed),std::abs(fixed-1.))>joinTolerance || std::abs(lo)>joinTolerance || std::abs(hi-1.)>joinTolerance)
            reject("patch_boundary_tolerance","Output boundary does not cover a single complete natural iso");
        const auto iso=actualSurface->Value(pa.X(),pa.Y()),isoEnd=actualSurface->Value(pb.X(),pb.Y());
        if(iso.Distance(curve->Value(first))>joinTolerance || isoEnd.Distance(curve->Value(last))>joinTolerance || deviation>boundaryTolerance)
            reject("patch_boundary_tolerance","Complete Patch boundary exceeds the 0.01 mm G0 deviation limit");
        maxDeviation=std::max(maxDeviation,deviation);const BRepAdaptor_Curve actual(current);const auto aa=actual.Value(actual.FirstParameter()),bb=actual.Value(actual.LastParameter());
        certificate={{"sourceSketchId",source.sketchId},{"sourceEntityId",source.entityId},{"sourceStartMm",xyz(source.a)},{"sourceEndMm",xyz(source.b)},{"curveType",curveType},{"degree",1},{"rational",false},{"controlPointsMm",control},{"parameterRange",QJsonArray{first,last}},{"surfaceIso",uIso?"u":"v"},{"isoParameter",fixed},{"pcurveStartUV",QJsonArray{pa.X(),pa.Y()}},{"pcurveEndUV",QJsonArray{pb.X(),pb.Y()}},{"maxDeviationBoundMm",deviation},{"method","linear-control-hull-to-finite-source-segment"}};
        auto detail=provenance;detail["sourceSketchId"]=source.sketchId;detail["sourceEntityId"]=source.entityId;detail["startMm"]=xyz(aa);detail["endMm"]=xyz(bb);detail["boundaryCertificate"]=certificate;
        const auto id=stable("edge/"+source.sketchId.toUtf8()+"/"+source.entityId.toUtf8());body.solidEdges.push_back({id,current,detail});support.boundaryEdges.push_back({id,"line",current,detail});certificates.append(certificate);
        GProp_GProps length;BRepGProp::LinearProperties(current,length);perimeter+=length.Mass();
    }
    body.supportFaces.push_back(std::move(support));body.summary["supportFaces"]=PlanarFaces::descriptors(body.supportFaces);body.summary["solidEdges"]=SolidFillet::descriptors(body.solidEdges);
    for(auto it=params.begin();it!=params.end();++it)body.summary[it.key()]=it.value();
    body.summary["boundaryLoopCount"]=1;body.summary["closedBoundary"]=true;body.summary["perimeterMm"]=perimeter;body.summary["boundaryToleranceMm"]=boundaryTolerance;body.summary["maxBoundaryDeviationBoundMm"]=maxDeviation;body.summary["boundaryCertificates"]=certificates;
    body.summary["surfaceCertificate"]=QJsonObject{{"type","bezier"},{"uDegree",actualSurface->UDegree()},{"vDegree",actualSurface->VDegree()},{"rational",false},{"cornersMm",QJsonArray{xyz(actualSurface->Pole(1,1)),xyz(actualSurface->Pole(2,1)),xyz(actualSurface->Pole(2,2)),xyz(actualSurface->Pole(1,2))}},{"nonplanar",true},{"cornerPlaneDeviationMm",std::abs(gp_Vec(corners[0],corners[2]).Dot(baseNormal))/baseNormal.Magnitude()}};
    return body;
}
}

EvaluatedBody SurfacePatch::evaluate(const QJsonObject& feature,const EvaluatedDocument& prior) {
    const auto p=feature.value("parameters").toObject();const auto featureId=feature.value("id").toString();
    if(p.value("boundaryMode")=="fourEdges")return fourEdgePatch(feature,prior);
    const auto sketchId=p.value("sketchId").toString(),profileId=p.value("profileId").toString();
    auto reject=[&](const QString& code,const QString& message){auto d=p;d["featureId"]=featureId;d["featureType"]="surfacePatch";d["code"]=code;d["message"]=message;throw GeometryKernel::EvaluationError(d);};
    const EvaluatedSketch* sketch=nullptr;for(const auto& s:prior.sketches)if(s.featureId==sketchId)sketch=&s;
    if(!sketch)reject("surface_source_unavailable","Patch requires an earlier solved planar sketch");
    if(sketch->profiles.size()!=1 || sketch->profiles.front().profileId!=profileId)reject("patch_profile_unavailable","Select exactly one closed profile in a single-loop sketch");
    const auto& profile=sketch->profiles.front();
    if(profile.summary.value("type")!="closed_line_profile" || profile.summary.value("innerLoopCount").toInt()!=0 || profile.boundaryEdges.size()<3)
        reject("unsupported_patch_profile","Patch currently requires one simple closed ordinary-Line loop without holes or arrangement cells");
    QSet<QString> sources;
    for(const auto& boundary:profile.boundaryEdges){
        if(boundary.type!="line" || boundary.sourceEntityId.isEmpty() || sources.contains(boundary.sourceEntityId))reject("unsupported_patch_profile","Patch boundaries must retain distinct ordinary Lines");
        bool ordinary=false;for(const auto& q:sketch->summary.value("entities").toArray()){const auto e=q.toObject();if(e.value("id")==boundary.sourceEntityId)ordinary=e.value("type")=="line" && !e.value("construction").toBool();}
        if(!ordinary)reject("unsupported_patch_profile","Patch boundary source must be an ordinary Line");sources.insert(boundary.sourceEntityId);
    }
    EvaluatedBody body;body.featureId=featureId;body.bodyId=feature.value("bodyId").toString();body.name=feature.value("name").toString();body.visible=feature.value("visible").toBool(true);body.shape=profile.face;body.summary=GeometryKernel::summarize(body.shape);
    TopTools_IndexedMapOfShape wires;TopExp::MapShapes(body.shape,TopAbs_WIRE,wires);const int wireCount=wires.Extent();
    const auto topology=body.summary.value("topology").toObject();const auto area=body.summary.value("areaMm2").toDouble();const BRepAdaptor_Surface surface(profile.face);
    if(!body.summary.value("valid").toBool() || topology.value("faces").toInt()!=1 || topology.value("solids").toInt()!=0 || (wireCount!=1 || !BRep_Tool::IsClosed(wires(1))) || topology.value("edges").toInt()!=sources.size() || surface.GetType()!=GeomAbs_Plane || !std::isfinite(area) || area<=1e-12 || !body.summary.value("volumeMm3").isNull())reject("patch_invalid","Patch must produce one valid positive-area planar face, one wire and zero solids");
    BRepAlgoAPI_Check interference(body.shape,false,true);if(!interference.IsValid())reject("patch_self_intersection","Patch boundary has geometric self-interference");
    auto xyz=[](const auto& q){return QJsonArray{q.X(),q.Y(),q.Z()};};
    auto stable=[&](const QByteArray& role){return QUuid::createUuidV5(QUuid(featureId),QByteArray("alloy-surface-patch/1/")+sketchId.toUtf8()+"/"+profileId.toUtf8()+"/"+role).toString(QUuid::WithoutBraces);};
    QJsonObject provenance{{"generatorFeatureId",featureId},{"sourceSketchId",sketchId},{"sourceProfileId",profileId},{"lineage","surfacePatchProfile"}};
    const auto plane=surface.Plane();auto u=plane.Position().XDirection(),v=plane.Position().YDirection();auto n=u.Crossed(v);if(profile.face.Orientation()==TopAbs_REVERSED){v.Reverse();n.Reverse();}
    EvaluatedSupportFace support{stable("face"),body.bodyId,featureId,"surfacePatch.sheet",profile.face,{{"origin",xyz(plane.Location())},{"uAxis",xyz(u)},{"vAxis",xyz(v)},{"normal",xyz(n)}},{},provenance};
    TopTools_IndexedMapOfShape retained;double perimeter=0;
    for(const auto& boundary:profile.boundaryEdges){
        if(retained.Contains(boundary.edge))reject("patch_lineage_unavailable","Patch boundary sources must identify distinct exact edges");retained.Add(boundary.edge);
        const BRepAdaptor_Curve curve(boundary.edge);const auto a=curve.Value(curve.FirstParameter()),b=curve.Value(curve.LastParameter());
        if(curve.GetType()!=GeomAbs_Line || plane.Distance(a)>1e-7 || plane.Distance(b)>1e-7 || a.Distance(b)<=1e-7)reject("patch_nonplanar_or_degenerate","Patch requires nondegenerate coplanar Line boundaries within 1e-7 mm");
        auto detail=provenance;detail["sourceEntityId"]=boundary.sourceEntityId;detail["startMm"]=xyz(a);detail["endMm"]=xyz(b);
        const auto id=stable("edge/"+boundary.sourceEntityId.toUtf8());body.solidEdges.push_back({id,boundary.edge,detail});support.boundaryEdges.push_back({id,"line",boundary.edge,detail});GProp_GProps length;BRepGProp::LinearProperties(boundary.edge,length);perimeter+=length.Mass();
    }
    body.summary["boundaryLoopCount"]=wireCount;body.summary["closedBoundary"]=BRep_Tool::IsClosed(wires(1));
    body.supportFaces.push_back(std::move(support));body.summary["supportFaces"]=PlanarFaces::descriptors(body.supportFaces);body.summary["solidEdges"]=SolidFillet::descriptors(body.solidEdges);
    for(auto i=p.begin();i!=p.end();++i)body.summary[i.key()]=i.value();body.summary["sourceSketchId"]=sketchId;body.summary["sourceProfileId"]=profileId;body.summary["perimeterMm"]=perimeter;body.summary["normal"]=xyz(n);return body;
}
