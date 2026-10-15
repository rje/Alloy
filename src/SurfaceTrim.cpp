// SPDX-License-Identifier: MIT
#include "SurfaceTrim.h"
#include "PlanarFaces.h"
#include "SolidFillet.h"
#include <BRepAlgoAPI_Common.hxx>
#include <BRepAlgoAPI_Splitter.hxx>
#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepGProp.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <gp_Pln.hxx>
#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <Bnd_Box.hxx>
#include <GeomAPI_ProjectPointOnCurve.hxx>
#include <GProp_GProps.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopTools_ListOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <QUuid>
#include <algorithm>
#include <cmath>

namespace {
constexpr double mmTolerance=1e-5;
QString stableId(const QString& feature,const QByteArray& role){
    return QUuid::createUuidV5(QUuid(feature),QByteArray("alloy-surface-trim/1/")+role)
        .toString(QUuid::WithoutBraces);
}
bool contains(const TopoDS_Shape& parent,const TopoDS_Shape& child){
    for(TopExp_Explorer walk(parent,child.ShapeType());walk.More();walk.Next())
        if(walk.Current().IsSame(child))return true;
    return false;
}
double area(const TopoDS_Shape& shape){
    if(shape.IsNull())return 0;GProp_GProps result;BRepGProp::SurfaceProperties(shape,result);return result.Mass();
}
double length(const TopoDS_Shape& shape){
    if(shape.IsNull())return 0;GProp_GProps result;BRepGProp::LinearProperties(shape,result);return result.Mass();
}
struct NamedEdge {QString id;TopoDS_Edge edge;QJsonObject provenance;};

SurfaceTrimSplit splitPlanarSheet(const QJsonObject& feature,const EvaluatedBody& target,
    const EvaluatedDocument& prior){
    const auto p=feature["parameters"].toObject();const auto id=feature["id"].toString();
    auto reject=[&](const QString& code,const QString& message){throw GeometryKernel::EvaluationError({
        {"featureId",id},{"featureType","surfaceTrim"},{"code",code},{"message",message}});};
    const EvaluatedBody* tool=nullptr;
    for(const auto& body:prior.bodies)if(body.bodyId==p["toolBodyId"])tool=&body;
    if(!tool || tool->bodyId==target.bodyId)
        reject("trim_tool_unavailable","Select a distinct earlier retained planar sheet tool");
    // This ingredient accepts the exact parallelograms produced by a single
    // straight sketch line's Surface Extrude. No arbitrary planar-face claim.
    auto qualify=[&](const EvaluatedBody& body){
        if(body.shape.IsNull() || body.summary["type"]!="surface" || body.supportFaces.size()!=1
            || body.solidEdges.size()!=4 || !body.occurrenceId.isEmpty()
            || body.summary["topology"].toObject()["solids"].toInt()!=0
            || body.supportFaces[0].role!="surfaceExtrude.sheet")
            reject("trim_sheet_unsupported","Sheet tools require two local single-line planar Surface Extrudes");
        const auto& face=body.supportFaces[0];
        if(BRepAdaptor_Surface(face.face).GetType()!=GeomAbs_Plane || face.boundaryEdges.size()!=4)
            reject("trim_sheet_unsupported","Each crossing sheet must be a planar four-line extrusion");
        for(const auto& edge:face.boundaryEdges)
            if(BRepAdaptor_Curve(edge.edge).GetType()!=GeomAbs_Line || edge.provenance["lineage"]!="surfacePrismBuilder")
                reject("trim_sheet_unsupported","Curved or modified sheet boundaries are outside this Trim ingredient");
    };
    qualify(target);qualify(*tool);
    const auto& source=target.supportFaces[0];const auto& cutter=tool->supportFaces[0];
    auto plane=BRepAdaptor_Surface(cutter.face).Plane();auto normal=plane.Axis().Direction();
    if(cutter.face.Orientation()==TopAbs_REVERSED)normal.Reverse();
    if(cutter.face.Orientation()!=TopAbs_FORWARD && cutter.face.Orientation()!=TopAbs_REVERSED)
        reject("trim_tool_unsupported","Tool face orientation is unavailable");
    BRepAlgoAPI_Splitter splitter;TopTools_ListOfShape arguments,tools;
    arguments.Append(target.shape);tools.Append(tool->shape);
    splitter.SetArguments(arguments);splitter.SetTools(tools);splitter.SetNonDestructive(true);splitter.Build();
    if(!splitter.IsDone() || splitter.HasErrors() || !BRepCheck_Analyzer(splitter.Shape()).IsValid())
        reject("trim_split_failed","OCCT could not split these finite crossing sheets");
    TopTools_IndexedMapOfShape faces,edges,descendants;
    TopExp::MapShapes(splitter.Shape(),TopAbs_FACE,faces);TopExp::MapShapes(splitter.Shape(),TopAbs_EDGE,edges);
    for(const auto& shape:splitter.Modified(source.face))
        if(shape.ShapeType()==TopAbs_FACE && faces.Contains(shape))descendants.Add(shape);
    if(faces.Extent()!=2 || descendants.Extent()!=2)
        reject("trim_region_ambiguous","The finite sheet tool must divide the target into exactly two regions");
    SurfaceTrimSplit result;result.splitShape=splitter.Shape();
    result.projection={{"mode","surfaceIntersection"},{"toolBodyId",tool->bodyId},
        {"toolFaceId",cutter.faceId},{"normal",QJsonArray{normal.X(),normal.Y(),normal.Z()}}};
    QSet<QString> sides;
    for(int i=1;i<=descendants.Extent();++i){
        const auto face=TopoDS::Face(descendants(i));GProp_GProps mass;BRepGProp::SurfaceProperties(face,mass);
        const auto distance=gp_Vec(plane.Location(),mass.CentreOfMass()).Dot(gp_Vec(normal));
        if(mass.Mass()<mmTolerance || std::abs(distance)<mmTolerance)
            reject("trim_region_ambiguous","A region has no unambiguous tool-plane side");
        const auto side=distance<0?QString("negative"):QString("positive");
        for(TopExp_Explorer walk(face,TopAbs_EDGE);walk.More();walk.Next())
            if(BRepAdaptor_Curve(TopoDS::Edge(walk.Current())).GetType()!=GeomAbs_Line)
                reject("trim_region_ambiguous","Split regions must remain straight-edged convex polygons");
        for(TopExp_Explorer walk(face,TopAbs_VERTEX);walk.More();walk.Next()){
            const auto d=gp_Vec(plane.Location(),BRep_Tool::Pnt(TopoDS::Vertex(walk.Current()))).Dot(gp_Vec(normal));
            if((distance<0 && d>mmTolerance) || (distance>0 && d< -mmTolerance))
                reject("trim_region_ambiguous","A split region crosses both tool-plane sides");
        }
        if(sides.contains(side))reject("trim_region_ambiguous","Expected one region on each tool-plane side");
        sides.insert(side);
        result.regions.push_back({stableId(id,(source.faceId+"/sheet/"+cutter.faceId+"/"+side).toUtf8()),
            source.faceId,false,face,mass.Mass(),side});
    }
    std::vector<NamedEdge> named;
    auto offer=[&](const NamedEdge& edge){for(const auto& current:named)
        if(current.edge.IsSame(edge.edge) || current.id==edge.id)
            reject("trim_lineage_ambiguous","A split boundary has competing retained identities");
        named.push_back(edge);};
    for(const auto& original:target.solidEdges){
        TopTools_IndexedMapOfShape modified;
        for(const auto& shape:splitter.Modified(original.edge))
            if(shape.ShapeType()==TopAbs_EDGE && edges.Contains(shape))modified.Add(shape);
        if(modified.IsEmpty() && edges.Contains(original.edge))modified.Add(original.edge);
        std::vector<std::pair<double,TopoDS_Edge>> ordered;
        Standard_Real first,last;const auto curve=BRep_Tool::Curve(original.edge,first,last);
        for(int i=1;i<=modified.Extent();++i){const auto edge=TopoDS::Edge(modified(i));BRepAdaptor_Curve c(edge);
            GeomAPI_ProjectPointOnCurve projection(c.Value((c.FirstParameter()+c.LastParameter())/2),curve,first,last);
            if(projection.NbPoints()!=1 || projection.LowerDistance()>mmTolerance)
                reject("trim_lineage_ambiguous","A source boundary moved off its retained curve");
            ordered.emplace_back(projection.LowerDistanceParameter(),edge);
        }
        std::sort(ordered.begin(),ordered.end(),[](const auto& a,const auto& b){return a.first<b.first;});
        for(int i=0;i<int(ordered.size());++i){auto provenance=original.provenance;provenance["trimModifierFeatureId"]=id;
            offer({ordered.size()==1?original.edgeId:stableId(id,(original.edgeId+"/segment/"+QString::number(i)).toUtf8()),
                ordered[i].second,provenance});}
    }
    int intersections=0;
    for(int i=1;i<=edges.Extent();++i){const auto edge=TopoDS::Edge(edges(i));
        if(std::any_of(named.begin(),named.end(),[&](const auto& current){return current.edge.IsSame(edge);}))continue;
        if(!contains(result.regions[0].face,edge) || !contains(result.regions[1].face,edge) || length(edge)<mmTolerance)
            reject("trim_lineage_ambiguous","Expected a nondegenerate shared intersection boundary");
        BRepAlgoAPI_Common common;common.SetNonDestructive(true);TopTools_ListOfShape a,b;
        a.Append(edge);b.Append(tool->shape);common.SetArguments(a);common.SetTools(b);common.Build();
        if(!common.IsDone() || common.HasErrors() || std::abs(length(common.Shape())-length(edge))>mmTolerance)
            reject("trim_lineage_ambiguous","New boundary does not match the actual finite tool sheet");
        ++intersections;
        offer({stableId(id,(source.faceId+"/intersection/"+cutter.faceId).toUtf8()),edge,
            {{"generatorFeatureId",id},{"sourceFaceId",source.faceId},{"toolBodyId",tool->bodyId},
             {"toolFaceId",cutter.faceId},{"role","trimBoundary"},{"lineage","splitterSurfaceIntersection"}}});
    }
    if(intersections!=1)reject("trim_region_ambiguous","Expected exactly one shared surface intersection edge");
    for(const auto& region:result.regions){
        EvaluatedSupportFace support{region.regionId,target.bodyId,id,"surfaceTrim."+region.side,
            region.face,source.frame,{},{{"sourceFaceId",source.faceId},{"toolBodyId",tool->bodyId},
                {"toolFaceId",cutter.faceId},{"side",region.side}}};
        for(const auto& edge:named)if(contains(region.face,edge.edge))
            support.boundaryEdges.push_back({edge.id,"line",edge.edge,edge.provenance});
        TopTools_IndexedMapOfShape boundaries;TopExp::MapShapes(region.face,TopAbs_EDGE,boundaries);
        if(boundaries.Extent()!=int(support.boundaryEdges.size()))
            reject("trim_lineage_ambiguous","A split boundary lacks a unique retained ID");
        result.faces.push_back(std::move(support));
    }
    for(const auto& edge:named)result.edges.push_back({edge.id,edge.edge,edge.provenance});
    return result;
}
}

QJsonArray SurfaceTrim::descriptors(const SurfaceTrimSplit& split){
    QJsonArray result;
    for(const auto& region:split.regions)result.append(QJsonObject{{"regionId",region.regionId},
        {"sourceFaceId",region.sourceFaceId},{"inside",region.inside},{"removable",region.removable()},
        {"side",region.side},{"areaMm2",region.areaMm2}});
    return result;
}

SurfaceTrimSplit SurfaceTrim::split(const QJsonObject& feature,const EvaluatedBody& target,
    const EvaluatedDocument& prior){
    const auto p=feature["parameters"].toObject();const auto featureId=feature["id"].toString();
    auto reject=[&](const QString& code,const QString& message){throw GeometryKernel::EvaluationError({
        {"featureId",featureId},{"featureType","surfaceTrim"},{"targetBodyId",target.bodyId},
        {"code",code},{"message",message}});};
    if(p.contains("toolBodyId"))return splitPlanarSheet(feature,target,prior);
    if(target.shape.IsNull() || target.summary["type"]!="surface" ||
        target.summary["topology"].toObject()["solids"].toInt()!=0 || target.supportFaces.size()!=2 ||
        !target.occurrenceId.isEmpty())reject("trim_target_unsupported","Select one local two-face open rail sheet");
    const auto sketchId=p["toolSketchId"].toString();
    const EvaluatedSketch* sketch=nullptr;for(const auto& current:prior.sketches)
        if(current.featureId==sketchId){sketch=&current;break;}
    if(!sketch)reject("trim_tool_unavailable","Selected tool sketch must be earlier and solved");
    const auto frame=sketch->resolvedFrame;const auto normalValues=frame["normal"].toArray(),originValues=frame["origin"].toArray();
    if(normalValues.size()!=3 || originValues.size()!=3)
        reject("trim_tool_unavailable","Tool sketch has no resolved plane and normal");
    gp_Vec normal(normalValues[0].toDouble(),normalValues[1].toDouble(),normalValues[2].toDouble());
    if(normal.Magnitude()<1e-8)reject("trim_tool_unavailable","Tool sketch normal is degenerate");
    normal.Normalize();const gp_Pnt origin(originValues[0].toDouble(),originValues[1].toDouble(),originValues[2].toDouble());
    const auto refs=p["toolEntityIds"].toArray();
    if(refs.size()!=2 || refs[0]==refs[1])reject("trim_tool_required","Select two distinct ordinary fit splines");
    std::array<TopoDS_Edge,2> toolEdges;
    std::array<QJsonObject,2> entities;
    for(int i=0;i<2;++i){
        const auto entityId=refs[i].toString();
        if(!sketch->entityEdges.contains(entityId))reject("trim_tool_unavailable","Selected spline is no longer solved");
        for(const auto& value:sketch->summary["entities"].toArray())if(value.toObject()["id"]==entityId){entities[i]=value.toObject();break;}
        if(entities[i]["type"]!="fitSpline" || entities[i]["construction"].toBool())
            reject("trim_tool_unsupported","Sketch Trim tool requires two ordinary fit splines");
        toolEdges[i]=TopoDS::Edge(BRepBuilderAPI_Copy(sketch->entityEdges.value(entityId),false,false).Shape());
        BRepAdaptor_Curve curve(toolEdges[i]);
        if(curve.GetType()!=GeomAbs_BSplineCurve || curve.IsClosed())
            reject("trim_tool_unsupported","Tool splines must remain open");
    }
    auto endpoints=[](const TopoDS_Edge& edge){BRepAdaptor_Curve curve(edge);
        return std::pair<gp_Pnt,gp_Pnt>{curve.Value(curve.FirstParameter()),curve.Value(curve.LastParameter())};};
    const auto a=endpoints(toolEdges[0]),b=endpoints(toolEdges[1]);
    if(a.first.Distance(a.second)<mmTolerance ||
        !((a.first.Distance(b.first)<mmTolerance && a.second.Distance(b.second)<mmTolerance)
            || (a.first.Distance(b.second)<mmTolerance && a.second.Distance(b.first)<mmTolerance)))
        reject("trim_tool_open","The two solved splines must meet at both endpoints");
    Bnd_Box bounds;BRepBndLib::Add(target.shape,bounds);
    if(bounds.IsVoid())reject("trim_target_unsupported","Target has no finite bounds");
    Standard_Real xmin,ymin,zmin,xmax,ymax,zmax;bounds.Get(xmin,ymin,zmin,xmax,ymax,zmax);
    const auto margin=p["throughMarginMm"].toDouble();
    double low=1e100,high=-1e100;
    for(double x:{xmin,xmax})for(double y:{ymin,ymax})for(double z:{zmin,zmax}){
        const auto projection=gp_Vec(origin,gp_Pnt(x,y,z)).Dot(normal);
        low=std::min(low,projection);high=std::max(high,projection);
    }
    if(!std::isfinite(low) || !std::isfinite(high) || margin<.001 || margin>1000)
        reject("trim_projection_invalid","Sketch-normal through-target reach is unavailable");
    const double sideOne=std::max(0.0,-low)+margin,sideTwo=std::max(0.0,high)+margin;
    gp_Trsf shift;shift.SetTranslation(-normal*sideOne);
    std::array<TopoDS_Edge,2> shifted;
    std::array<TopoDS_Shape,2> tools;
    for(int i=0;i<2;++i){
        BRepBuilderAPI_Transform translated(toolEdges[i],shift,true);
        if(!translated.IsDone())reject("trim_projection_failed","Could not place sketch-normal cutter");
        shifted[i]=TopoDS::Edge(translated.Shape());
        BRepPrimAPI_MakePrism prism(shifted[i],normal*(sideOne+sideTwo),false,true);prism.Build();
        if(!prism.IsDone() || !BRepCheck_Analyzer(prism.Shape()).IsValid())
            reject("trim_projection_failed","Could not sweep a valid cutter sheet through the target");
        tools[i]=prism.Shape();
    }
    const auto second=TopoDS::Edge((a.second.Distance(b.second)<mmTolerance ? shifted[1].Reversed() : shifted[1]));
    BRepBuilderAPI_MakeWire loop(shifted[0],second);
    if(!loop.IsDone() || !loop.Wire().Closed())reject("trim_tool_open","Selected splines do not form one closed loop");
    BRepBuilderAPI_MakeFace loopFace(loop.Wire());
    if(!loopFace.IsDone() || !BRepCheck_Analyzer(loopFace.Face()).IsValid())
        reject("trim_tool_invalid","Selected spline loop is not a valid planar face");
    BRepPrimAPI_MakePrism regionPrism(loopFace.Face(),normal*(sideOne+sideTwo),false,true);regionPrism.Build();
    if(!regionPrism.IsDone() || !BRepCheck_Analyzer(regionPrism.Shape()).IsValid())
        reject("trim_projection_failed","Sketch-normal removal prism is invalid");
    BRepAlgoAPI_Splitter splitter;TopTools_ListOfShape arguments,cutters;
    arguments.Append(target.shape);for(const auto& tool:tools)cutters.Append(tool);
    splitter.SetArguments(arguments);splitter.SetTools(cutters);splitter.SetNonDestructive(true);splitter.Build();
    if(!splitter.IsDone() || splitter.HasErrors() || !BRepCheck_Analyzer(splitter.Shape()).IsValid())
        reject("trim_split_failed","OCCT could not split the surface with these tool splines");
    TopTools_IndexedMapOfShape currentFaces,currentEdges;
    TopExp::MapShapes(splitter.Shape(),TopAbs_FACE,currentFaces);
    TopExp::MapShapes(splitter.Shape(),TopAbs_EDGE,currentEdges);
    if(currentFaces.Extent()!=4)reject("trim_region_ambiguous","Expected one inner and one outer region on each source face");
    SurfaceTrimSplit result;result.splitShape=splitter.Shape();
    result.projection={{"mode","sketchNormalThroughTarget"},{"sourcePlane",p["sourcePlane"]},
        {"normal",normalValues},{"sideOneMm",sideOne},{"sideTwoMm",sideTwo},{"marginMm",margin}};
    for(const auto& source:target.supportFaces){
        TopTools_IndexedMapOfShape descendants;
        for(const auto& shape:splitter.Modified(source.face))if(shape.ShapeType()==TopAbs_FACE && currentFaces.Contains(shape))descendants.Add(shape);
        if(descendants.Extent()!=2)reject("trim_region_ambiguous","Each source face must yield exactly two regions");
        int inner=0,outer=0;
        for(int index=1;index<=descendants.Extent();++index){
            const auto face=TopoDS::Face(descendants(index));const auto faceArea=area(face);
            BRepAlgoAPI_Common overlap;overlap.SetNonDestructive(true);TopTools_ListOfShape a,b;
            a.Append(face);b.Append(regionPrism.Shape());overlap.SetArguments(a);overlap.SetTools(b);overlap.Build();
            if(!overlap.IsDone() || overlap.HasErrors() || faceArea<mmTolerance)
                reject("trim_region_ambiguous","A split region could not be classified");
            const auto commonArea=area(overlap.Shape());
            const auto inside=std::abs(commonArea-faceArea)<std::max(mmTolerance,faceArea*1e-6);
            if(!inside && commonArea>std::max(mmTolerance,faceArea*1e-6))
                reject("trim_region_ambiguous","A split region overlaps its sketch cutter only partially");
            inside?++inner:++outer;
            result.regions.push_back({stableId(featureId,(source.faceId+(inside?"/inside":"/outside")).toUtf8()),
                source.faceId,inside,face,faceArea});
        }
        if(inner!=1 || outer!=1)reject("trim_region_ambiguous","Both source faces need a unique inner and outer region");
    }
    // Transfer original edge identity through OCCT history, ordered by source
    // curve parameter if a boundary splits. New boundaries are certified by
    // their full intersection with exactly one selected cutter sheet.
    std::vector<NamedEdge> named;
    auto offer=[&](const NamedEdge& edge){for(const auto& existing:named){
        if(existing.edge.IsSame(edge.edge)){
            if(existing.id!=edge.id)reject("trim_lineage_ambiguous","A current edge has competing source identities");return;}
        if(existing.id==edge.id)reject("trim_lineage_ambiguous","One retained edge ID resolves to multiple current edges");}
        named.push_back(edge);};
    for(const auto& original:target.solidEdges){
        if(original.edge.IsNull())continue;
        TopTools_IndexedMapOfShape modified;
        for(const auto& shape:splitter.Modified(original.edge))if(shape.ShapeType()==TopAbs_EDGE && currentEdges.Contains(shape))modified.Add(shape);
        if(modified.IsEmpty() && currentEdges.Contains(original.edge))modified.Add(original.edge);
        if(modified.IsEmpty())continue;
        std::vector<std::pair<double,TopoDS_Edge>> ordered;
        Standard_Real first,last;const auto curve=BRep_Tool::Curve(original.edge,first,last);
        for(int i=1;i<=modified.Extent();++i){const auto edge=TopoDS::Edge(modified(i));BRepAdaptor_Curve adaptor(edge);
            const auto midpoint=adaptor.Value((adaptor.FirstParameter()+adaptor.LastParameter())/2);
            GeomAPI_ProjectPointOnCurve projection(midpoint,curve,first,last);
            if(projection.NbPoints()!=1 || projection.LowerDistance()>mmTolerance)
                reject("trim_lineage_ambiguous","A source boundary descendant moved off its curve");
            ordered.emplace_back(projection.LowerDistanceParameter(),edge);
        }
        std::sort(ordered.begin(),ordered.end(),[](const auto& a,const auto& b){return a.first<b.first;});
        for(int i=0;i<int(ordered.size());++i){auto provenance=original.provenance;
            provenance["trimModifierFeatureId"]=featureId;
            const auto edgeId=ordered.size()==1 ? original.edgeId
                : stableId(featureId,(original.edgeId+"/segment/"+QString::number(i)).toUtf8());
            offer({edgeId,ordered[i].second,provenance});}
    }
    for(int i=1;i<=currentEdges.Extent();++i){const auto edge=TopoDS::Edge(currentEdges(i));
        if(BRep_Tool::Degenerated(edge))continue; // Collapsed tips remain topology, never selectable curves.
        if(std::any_of(named.begin(),named.end(),[&](const auto& value){return value.edge.IsSame(edge);}))continue;
        const auto edgeLength=length(edge);if(edgeLength<mmTolerance)reject("trim_lineage_ambiguous","Degenerate trim boundary");
        int tool=-1;for(int k=0;k<2;++k){BRepAlgoAPI_Common overlap;overlap.SetNonDestructive(true);TopTools_ListOfShape a,b;
            a.Append(edge);b.Append(tools[k]);overlap.SetArguments(a);overlap.SetTools(b);overlap.Build();
            if(!overlap.IsDone() || overlap.HasErrors())reject("trim_lineage_ambiguous","Cannot resolve cutter boundary");
            if(std::abs(length(overlap.Shape())-edgeLength)<std::max(mmTolerance,edgeLength*1e-6)){
                if(tool>=0)reject("trim_lineage_ambiguous","New edge belongs to two tool splines");tool=k;}}
        if(tool<0)reject("trim_lineage_ambiguous","New edge has no retained cutter source");
        QString faceId;
        for(const auto& region:result.regions)if(contains(region.face,edge)){
            if(faceId.isEmpty())faceId=region.sourceFaceId;
            else if(faceId!=region.sourceFaceId)reject("trim_lineage_ambiguous","New edge spans two source faces");}
        if(faceId.isEmpty())reject("trim_lineage_ambiguous","New edge has no source face");
        QJsonObject provenance{{"generatorFeatureId",featureId},{"sourceSketchId",sketchId},
            {"sourceEntityId",refs[tool]},{"sourceFaceId",faceId},{"role","trimBoundary"},
            {"lineage","splitterCutterIntersection"}};
        offer({stableId(featureId,(faceId+"/edge/"+refs[tool].toString()).toUtf8()),edge,provenance});
    }
    for(const auto& region:result.regions){
        EvaluatedSupportFace support{region.regionId,target.bodyId,featureId,
            region.inside?"surfaceTrim.inner":"surfaceTrim.outer",region.face,{}, {},
            {{"sourceFaceId",region.sourceFaceId},{"region","inside"}}};
        support.provenance["region"]=region.inside?"inside":"outside";
        for(const auto& edge:named)if(contains(region.face,edge.edge)){
            const BRepAdaptor_Curve curve(edge.edge);
            support.boundaryEdges.push_back({edge.id,curve.GetType()==GeomAbs_Line?"line":"curve",edge.edge,edge.provenance});}
        TopTools_IndexedMapOfShape boundaries;TopExp::MapShapes(region.face,TopAbs_EDGE,boundaries);
        int ordinaryBoundaries=0;for(int i=1;i<=boundaries.Extent();++i)
            if(!BRep_Tool::Degenerated(TopoDS::Edge(boundaries(i))))++ordinaryBoundaries;
        if(ordinaryBoundaries!=int(support.boundaryEdges.size()))
            reject("trim_lineage_ambiguous","An ordinary region boundary has no unique retained ID");
        result.faces.push_back(std::move(support));
    }
    for(const auto& edge:named)result.edges.push_back({edge.id,edge.edge,edge.provenance});
    return result;
}

void SurfaceTrim::evaluate(EvaluatedBody& target,const QJsonObject& feature,const EvaluatedDocument& prior){
    const auto partition=split(feature,target,prior);
    applySelection(target,feature,partition);
}

void SurfaceTrim::applySelection(EvaluatedBody& target,const QJsonObject& feature,const SurfaceTrimSplit& splitResult){
    const auto parameters=feature["parameters"].toObject();
    const auto removed=parameters["removedRegionIds"].toArray();
    if(removed.isEmpty() || removed.size()>(parameters.contains("toolBodyId")?1:2))throw GeometryKernel::EvaluationError({
        {"featureId",feature["id"]},{"code","trim_region_required"},
        {"message","Select one or two inner regions to remove"}});
    QSet<QString> chosen;for(const auto& value:removed)chosen.insert(value.toString());
    if(chosen.size()!=removed.size())throw GeometryKernel::EvaluationError({
        {"featureId",feature["id"]},{"code","trim_region_duplicate"},{"message","Select each region once"}});
    TopoDS_Compound kept;BRep_Builder builder;builder.MakeCompound(kept);
    std::vector<EvaluatedSupportFace> supports;
    for(size_t i=0;i<splitResult.regions.size();++i){const auto& region=splitResult.regions[i];
        if(chosen.contains(region.regionId)){
            if(!region.removable())throw GeometryKernel::EvaluationError({{"featureId",feature["id"]},
                {"code","trim_region_unsupported"},{"message","This bounded Trim removes inner regions only"}});
            chosen.remove(region.regionId);continue;}
        builder.Add(kept,region.face);supports.push_back(splitResult.faces[i]);
    }
    if(!chosen.isEmpty())throw GeometryKernel::EvaluationError({{"featureId",feature["id"]},
        {"code","trim_region_unavailable"},{"message","Selected region no longer matches the current split"}});
    auto summary=GeometryKernel::summarize(kept);
    if(!summary["valid"].toBool() || summary["topology"].toObject()["solids"].toInt()!=0 ||
        summary["topology"].toObject()["faces"].toInt()!=int(supports.size()))
        throw GeometryKernel::EvaluationError({{"featureId",feature["id"]},
            {"code","trim_invalid_result"},{"message","Trim must leave a valid open sheet"}});
    target.shape=kept;target.supportFaces=std::move(supports);target.solidEdges.clear();
    for(const auto& edge:splitResult.edges)if(contains(kept,edge.edge))target.solidEdges.push_back(edge);
    summary["supportFaces"]=PlanarFaces::descriptors(target.supportFaces);
    summary["solidEdges"]=SolidFillet::descriptors(target.solidEdges);
    summary["lastModifierFeatureId"]=feature["id"];
    summary["producerFeatureId"]=target.featureId;
    summary["operation"]="surfaceTrim";summary["projection"]=splitResult.projection;
    summary["regions"]=descriptors(splitResult);summary["removedRegionIds"]=removed;
    target.summary=summary;
}
