// SPDX-License-Identifier: MIT
#include "PlanarFaces.h"
#include <BRepBuilderAPI_MakeShape.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRep_Tool.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopExp.hxx>
#include <TopoDS.hxx>
#include <QUuid>
#include <QMap>
#include <QSet>
#include <algorithm>
#include <cmath>
namespace {
QJsonArray point(const gp_Pnt& p) {return {p.X(),p.Y(),p.Z()};}
QJsonArray axis(const gp_Dir& p) {return {p.X(),p.Y(),p.Z()};}
TopoDS_Shape descendant(BRepBuilderAPI_MakeShape& op,const TopoDS_Shape& source,
    const TopTools_IndexedMapOfShape& present,TopAbs_ShapeEnum type) {
    TopTools_IndexedMapOfShape choices;
    if(present.Contains(source))choices.Add(source);
    for(const auto& next:op.Modified(source))if(next.ShapeType()==type && present.Contains(next))choices.Add(next);
    return choices.Extent()==1 ? choices(1) : TopoDS_Shape{};
}
}
bool PlanarFaces::updateStraightBoundary(const TopoDS_Edge& source,
    const TopoDS_Edge& next,QJsonObject& provenance) {
    if(source.IsNull() || next.IsNull())return false;
    BRepAdaptor_Curve before(source),after(next);
    if(before.GetType()!=GeomAbs_Line || after.GetType()!=GeomAbs_Line)return false;
    auto a=after.Value(after.FirstParameter()),b=after.Value(after.LastParameter());
    const auto oldA=provenance["startMm"].toArray(),oldB=provenance["endMm"].toArray();
    // Support boundaries already carry their oriented endpoints. Solid edges
    // initially use the curve parameter direction, as their public descriptors do.
    const auto first=oldA.size()==3 ? gp_Pnt(oldA[0].toDouble(),oldA[1].toDouble(),oldA[2].toDouble()) : before.Value(before.FirstParameter());
    const auto last=oldB.size()==3 ? gp_Pnt(oldB[0].toDouble(),oldB[1].toDouble(),oldB[2].toDouble()) : before.Value(before.LastParameter());
    const gp_Vec direction(first,last),current(a,b);
    if(direction.Magnitude()<1e-6 || current.Magnitude()<1e-6
        || direction.Crossed(current).Magnitude()>1e-9*direction.Magnitude()*current.Magnitude()
        || before.Line().Distance(a)>1e-6 || before.Line().Distance(b)>1e-6)return false;
    if(direction.Dot(current)<0)std::swap(a,b);
    if(provenance.contains("startContact") || provenance.contains("endContact")) {
        provenance["sourceBoundaryContacts"]=QJsonObject{{"start",provenance["startContact"]},{"end",provenance["endContact"]}};
        provenance.remove("startContact");provenance.remove("endContact");
    }
    provenance["endpointProvenance"]="currentKernelBoundary";
    provenance["lineage"]="uniqueModifiedStraight";
    provenance["startMm"]=point(a);provenance["endMm"]=point(b);
    return true;
}
std::vector<EvaluatedSupportFace> PlanarFaces::transfer(BRepBuilderAPI_MakeShape& op,
    const std::vector<EvaluatedSupportFace>& sources,const TopoDS_Shape& result,bool retainModifiedStraight) {
    TopTools_IndexedMapOfShape present;TopExp::MapShapes(result,TopAbs_FACE,present);
    std::vector<EvaluatedSupportFace> output;QSet<QString> seen;
    for(auto source:sources) {
        const auto next=descendant(op,source.face,present,TopAbs_FACE);
        if(next.IsNull() || BRepAdaptor_Surface(TopoDS::Face(next)).GetType()!=GeomAbs_Plane)continue;
        // A retained frame must still lie on the result plane. This verifies
        // kernel history; it is not used to search for replacement faces.
        const auto origin=source.frame["origin"].toArray(),normal=source.frame["normal"].toArray();
        const auto plane=BRepAdaptor_Surface(TopoDS::Face(next)).Plane();
        if(origin.size()!=3 || normal.size()!=3 || plane.Distance(gp_Pnt(origin[0].toDouble(),origin[1].toDouble(),origin[2].toDouble()))>1e-6
            || std::abs(plane.Axis().Direction().Dot(gp_Dir(normal[0].toDouble(),normal[1].toDouble(),normal[2].toDouble())))<1-1e-9)continue;
        source.face=TopoDS::Face(next);
        TopTools_IndexedMapOfShape edges;TopExp::MapShapes(next,TopAbs_EDGE,edges);
        std::vector<EvaluatedSupportEdge> retained;
        for(auto edge:source.boundaryEdges) {
            const auto mapped=descendant(op,edge.edge,edges,TopAbs_EDGE);if(mapped.IsNull())continue;
            // Kernel history alone chooses the edge; coordinates only verify
            // straight-line scope and preserve endpoint roles on that choice.
            if(!mapped.IsSame(edge.edge)
                && (!retainModifiedStraight || edge.type!="line" || !updateStraightBoundary(edge.edge,TopoDS::Edge(mapped),edge.provenance)))continue;
            edge.edge=TopoDS::Edge(mapped);retained.push_back(edge);
        }
        source.boundaryEdges=retained;
        const auto key=source.producerFeatureId+"/"+source.faceId;
        if(seen.contains(key)) {
            const auto prior=std::find_if(output.begin(),output.end(),[&](const auto& f){return f.producerFeatureId==source.producerFeatureId && f.faceId==source.faceId;});
            if(!prior->face.IsSame(source.face))throw std::runtime_error("Ambiguous retained planar face identity");
        } else {output.push_back(source);seen.insert(key);}
    }
    return output;
}
void PlanarFaces::retainLinearCutBoundaries(BRepBuilderAPI_MakeShape& op,
    const std::vector<EvaluatedSupportFace>& sources,
    const std::vector<EvaluatedSolidEdge>& tools,
    std::vector<EvaluatedSupportFace>& retained,const QString& feature) {
    for(auto& face:retained) {
        TopTools_IndexedMapOfShape present;TopExp::MapShapes(face.face,TopAbs_EDGE,present);
        struct Candidate {TopoDS_Edge edge;QStringList contributors;};
        std::vector<Candidate> candidates;
        const auto offer=[&](const TopoDS_Shape& shape,const QString& signature) {
            if(shape.IsNull() || shape.ShapeType()!=TopAbs_EDGE || !present.Contains(shape)
                || BRepAdaptor_Curve(TopoDS::Edge(shape)).GetType()!=GeomAbs_Line)return;
            for(auto& c:candidates)if(c.edge.IsSame(shape)){c.contributors.append(signature);return;}
            candidates.push_back({TopoDS::Edge(shape),{signature}});
        };
        // Tool base/end rims name the newly exposed cut boundary. Simplification
        // can merge collinear inputs; the sorted contributor set names that
        // exact resulting edge rather than choosing the first source entity.
        for(const auto& tool:tools)if(tool.provenance["role"]=="baseRim" || tool.provenance["role"]=="endRim") {
            const auto mapped=descendant(op,tool.edge,present,TopAbs_EDGE);
            offer(mapped,tool.provenance["role"]=="endRim" ? "tool/end/"+tool.edgeId : "tool/"+tool.edgeId);
        }
        const auto source=std::find_if(sources.begin(),sources.end(),[&](const auto& f){return f.faceId==face.faceId && f.producerFeatureId==face.producerFeatureId;});
        if(source!=sources.end())for(const auto& edge:source->boundaryEdges) {
            if(edge.type!="line")continue;
            TopTools_IndexedMapOfShape children;
            for(const auto& child:op.Modified(edge.edge))if(child.ShapeType()==TopAbs_EDGE && present.Contains(child))children.Add(child);
            if(children.Extent()<2)continue;
            TopoDS_Vertex first,last;TopExp::Vertices(edge.edge,first,last,true);
            const auto start=edge.provenance["startMm"].toArray(),end=edge.provenance["endMm"].toArray();
            if(first.IsNull() || last.IsNull() || start.size()!=3 || end.size()!=3)continue;
            const gp_Pnt a(start[0].toDouble(),start[1].toDouble(),start[2].toDouble()),
                b(end[0].toDouble(),end[1].toDouble(),end[2].toDouble());
            // Coordinates verify the semantic roles of these two original
            // vertices only; result candidates still come solely from history.
            if(BRep_Tool::Pnt(first).Distance(a)>1e-7)std::swap(first,last);
            if(BRep_Tool::Pnt(first).Distance(a)>1e-7 || BRep_Tool::Pnt(last).Distance(b)>1e-7)continue;
            for(const auto& endpoint:std::vector<std::pair<TopoDS_Vertex,QString>>{{first,"start"},{last,"end"}}) {
                if(endpoint.first.IsNull())continue;
                TopoDS_Shape match;int matches=0;
                for(int n=1;n<=children.Extent();++n) {
                    TopTools_IndexedMapOfShape vertices;TopExp::MapShapes(children(n),TopAbs_VERTEX,vertices);
                    if(!descendant(op,endpoint.first,vertices,TopAbs_VERTEX).IsNull()){match=children(n);++matches;}
                }
                if(matches==1)offer(match,"split/"+edge.edgeId+"/"+endpoint.second);
            }
        }
        for(auto& candidate:candidates) {
            if(std::any_of(face.boundaryEdges.begin(),face.boundaryEdges.end(),[&](const auto& e){return e.edge.IsSame(candidate.edge);}))continue;
            candidate.contributors.removeDuplicates();candidate.contributors.sort();
            const auto id=QUuid::createUuidV5(QUuid(feature),("alloy-cut-boundary/1\n"+candidate.contributors.join("\n")).toUtf8()).toString(QUuid::WithoutBraces);
            BRepAdaptor_Curve curve(candidate.edge);
            QJsonArray contributors;for(const auto& c:candidate.contributors)contributors.append(c);
            face.boundaryEdges.push_back({id,"line",candidate.edge,{{"generatorFeatureId",feature},{"lineage","exactLinearCutHistory"},
                {"contributors",contributors},{"startMm",point(curve.Value(curve.FirstParameter()))},{"endMm",point(curve.Value(curve.LastParameter()))}}});
        }
    }
}
std::vector<EvaluatedSupportFace> PlanarFaces::prismSides(BRepPrimAPI_MakePrism& prism,
    const EvaluatedProfile& profile,const QString& generator,double distance) {
    std::vector<EvaluatedSupportFace> result;
    TopTools_IndexedMapOfShape solidFaces;TopExp::MapShapes(prism.Shape(),TopAbs_FACE,solidFaces);
    const auto offer=[&](const QString& role,const QByteArray& signature,const TopoDS_Shape& shape,const QJsonObject& frame) {
        if(shape.IsNull() || shape.ShapeType()!=TopAbs_FACE || BRepAdaptor_Surface(TopoDS::Face(shape)).GetType()!=GeomAbs_Plane)return;
        const auto id=QUuid::createUuidV5(QUuid(generator),QByteArray("alloy-planar-face/1\n")+signature).toString(QUuid::WithoutBraces);
        const auto prior=std::find_if(result.begin(),result.end(),[&](const auto& value){return value.faceId==id;});
        if(prior!=result.end()){if(!prior->face.IsSame(shape))throw std::runtime_error("Ambiguous planar face provenance");return;}
        result.push_back({id,{},generator,role,TopoDS::Face(shape),frame,{}});
    };
    auto base=profile.summary["frame"].toObject();
    auto n=base["normal"].toArray(),v=base["vAxis"].toArray();
    if(distance>0){for(int i=0;i<3;++i){n[i]=-n[i].toDouble();v[i]=-v[i].toDouble();}base["normal"]=n;base["vAxis"]=v;}
    offer("extrude.base",profile.profileId.toUtf8()+"/base",prism.FirstShape(),base);
    if(!result.empty() && result.back().role=="extrude.base") {
        auto& cap=result.back();TopTools_IndexedMapOfShape capEdges;TopExp::MapShapes(cap.face,TopAbs_EDGE,capEdges);
        QSet<QString> ids;
        for(const auto& boundary:profile.boundaryEdges) {
            if(boundary.type!="line")continue;
            const auto generated=prism.FirstShape(boundary.edge);
            if(generated.IsNull() || generated.ShapeType()!=TopAbs_EDGE || !capEdges.Contains(generated))
                throw std::runtime_error("Base boundary has no exact prism history");
            TopoDS_Vertex first,last,originalFirst,originalLast;
            TopExp::Vertices(TopoDS::Edge(generated),first,last,true);TopExp::Vertices(boundary.edge,originalFirst,originalLast,true);
            auto a=BRep_Tool::Pnt(first),b=BRep_Tool::Pnt(last);
            const auto expectedA=BRep_Tool::Pnt(originalFirst),expectedB=BRep_Tool::Pnt(originalLast);
            if(a.Distance(expectedA)>1e-6 || b.Distance(expectedB)>1e-6)std::swap(a,b);
            if(a.Distance(expectedA)>1e-6 || b.Distance(expectedB)>1e-6)
                throw std::runtime_error("Base boundary endpoint lineage mismatch");
            QStringList contacts{boundary.startContact,boundary.endContact};contacts.sort();
            const auto signature=boundary.sourceSketchId+"/"+boundary.sourceEntityId+"/"+contacts.join('/');
            const auto id=QUuid::createUuidV5(QUuid(generator),("alloy-base-edge/1\n"+signature).toUtf8()).toString(QUuid::WithoutBraces);
            if(ids.contains(id) || std::any_of(cap.boundaryEdges.begin(),cap.boundaryEdges.end(),[&](const auto& e){return e.edge.IsSame(generated);}))
                throw std::runtime_error("Ambiguous base boundary lineage");
            ids.insert(id);if(boundary.startContact>boundary.endContact)std::swap(a,b);
            cap.boundaryEdges.push_back({id,"line",TopoDS::Edge(generated),{{"sourceSketchId",boundary.sourceSketchId},
                {"sourceEntityId",boundary.sourceEntityId},{"startContact",contacts[0]},{"endContact",contacts[1]},
                {"startMm",point(a)},{"endMm",point(b)}}});
        }
    }
    for(const auto& boundary:profile.boundaryEdges) {
        if(boundary.type!="line")continue;
        TopTools_IndexedMapOfShape generated;
        for(const auto& shape:prism.Generated(boundary.edge))if(shape.ShapeType()==TopAbs_FACE)generated.Add(shape);
        if(generated.Extent()!=1)continue;
        const auto face=TopoDS::Face(solidFaces(solidFaces.FindIndex(generated(1))));const auto plane=BRepAdaptor_Surface(face).Plane();
        auto normal=plane.Axis().Direction();if(face.Orientation()==TopAbs_REVERSED)normal.Reverse();
        const auto sourceNormal=profile.summary["frame"].toObject()["normal"].toArray();
        gp_Dir vertical(sourceNormal[0].toDouble(),sourceNormal[1].toDouble(),sourceNormal[2].toDouble());
        gp_Dir horizontal=vertical.Crossed(normal);
        BRepAdaptor_Curve curve(boundary.edge);const auto origin=curve.Value(curve.FirstParameter());
        QJsonObject frame{{"origin",point(origin)},{"uAxis",axis(horizontal)},{"vAxis",axis(vertical)},{"normal",axis(normal)}};
        QStringList contacts{boundary.startContact,boundary.endContact};contacts.sort();
        offer("extrude.side",(profile.profileId+"/"+boundary.sourceEntityId+"/"+contacts.join('/')).toUtf8(),face,frame);
    }
    return result;
}
std::vector<EvaluatedSolidEdge> PlanarFaces::solidLinearCutRims(const std::vector<EvaluatedSupportFace>& faces,
    const std::vector<EvaluatedSolidEdge>& toolEdges,const TopoDS_Shape& result,const QString& feature) {
    QMap<QString,EvaluatedSolidEdge> tools;
    for(const auto& tool:toolEdges)if(tool.provenance["role"]=="baseRim" || tool.provenance["role"]=="endRim") {
        if(tools.contains(tool.edgeId))throw std::runtime_error("Repeated linear Cut tool edge identity");
        tools.insert(tool.edgeId,tool);
    }
    TopTools_IndexedMapOfShape current;TopExp::MapShapes(result,TopAbs_EDGE,current);
    struct Candidate {EvaluatedSolidEdge edge;QString semantic;int uses=0;};
    std::vector<Candidate> candidates;
    QMap<QString,int> semanticUses;
    for(const auto& face:faces) {
        TopTools_IndexedMapOfShape faceEdges;TopExp::MapShapes(face.face,TopAbs_EDGE,faceEdges);
        for(const auto& boundary:face.boundaryEdges) {
            if(boundary.provenance["generatorFeatureId"]!=feature ||
               boundary.provenance["lineage"]!="exactLinearCutHistory" ||
               boundary.type!="line" || boundary.edge.IsNull() ||
               !current.Contains(boundary.edge) || !faceEdges.Contains(boundary.edge))continue;
            const auto contributors=boundary.provenance["contributors"].toArray();
            if(contributors.size()!=1)continue; // Merged or split source roles require an explicit repair.
            const auto reference=contributors.first().toString();
            const bool endRim=reference.startsWith("tool/end/");
            if(!endRim && !reference.startsWith("tool/"))continue;
            const auto toolId=reference.mid(endRim ? 9 : 5);
            if(!tools.contains(toolId))continue;
            const auto& tool=tools[toolId];
            if(tool.provenance["role"]!=(endRim ? "endRim" : "baseRim"))continue;
            const auto sketchId=tool.provenance["sourceSketchId"].toString();
            const auto entityId=tool.provenance["sourceEntityId"].toString();
            if(QUuid(sketchId).isNull() || QUuid(entityId).isNull())continue;
            const auto signature=face.producerFeatureId+"\n"+face.faceId+"\n"+sketchId+"\n"+entityId+(endRim ? "\nend" : "");
            const auto id=QUuid::createUuidV5(QUuid(feature),QByteArray(endRim ? "alloy-solid-linear-cut-end-rim/1\n" : "alloy-solid-linear-cut-rim/1\n")+signature.toUtf8())
                .toString(QUuid::WithoutBraces);
            QJsonObject provenance{{"generatorFeatureId",feature},{"role",endRim ? "cutEndRim" : "cutRim"},
                {"sourceSketchId",sketchId},{"sourceEntityId",entityId},
                {"sourceToolEdgeId",toolId},{"targetFaceId",face.faceId},
                {"targetFaceProducerFeatureId",face.producerFeatureId},
                {"lineage","exactLinearCutHistory"},{"endpointProvenance","currentKernelBoundary"},
                {"startMm",boundary.provenance["startMm"]},{"endMm",boundary.provenance["endMm"]}};
            auto prior=std::find_if(candidates.begin(),candidates.end(),[&](const auto& c){return c.edge.edge.IsSame(boundary.edge);});
            if(prior!=candidates.end()) {++prior->uses;continue;}
            ++semanticUses[signature];
            candidates.push_back({{id,boundary.edge,provenance},signature,1});
        }
    }
    std::vector<EvaluatedSolidEdge> named;
    for(const auto& candidate:candidates)if(candidate.uses==1 && semanticUses[candidate.semantic]==1)
        named.push_back(candidate.edge);
    return named;
}
QJsonArray PlanarFaces::descriptors(const std::vector<EvaluatedSupportFace>& faces) {
    QJsonArray output;
    for(const auto& f:faces){QJsonObject descriptor{{"faceId",f.faceId},{"bodyId",f.bodyId},{"producerFeatureId",f.producerFeatureId},{"role",f.role},{"planar",true},{"frame",f.frame},{"edges",GeometryKernel::supportEdgeDescriptors(f)}};if(!f.provenance.isEmpty())descriptor["provenance"]=f.provenance;output.append(descriptor);}
    return output;
}
