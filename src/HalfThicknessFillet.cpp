// SPDX-License-Identifier: MIT
#include "HalfThicknessFillet.h"
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeSolid.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_Sewing.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepClass_FaceClassifier.hxx>
#include <BRepFilletAPI_MakeFillet.hxx>
#include <BRepGProp.hxx>
#include <BRepLib.hxx>
#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <GProp_GProps.hxx>
#include <Geom2dAPI_InterCurveCurve.hxx>
#include <Geom2d_Line.hxx>
#include <Geom2d_TrimmedCurve.hxx>
#include <GeomAPI_IntSS.hxx>
#include <GeomAPI_ProjectPointOnCurve.hxx>
#include <GeomAPI_ProjectPointOnSurf.hxx>
#include <GeomFill_Pipe.hxx>
#include <GeomProjLib.hxx>
#include <GeomTools.hxx>
#include <Geom_Circle.hxx>
#include <Geom_OffsetSurface.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <ShapeAnalysis_FreeBounds.hxx>
#include <ShapeFix_Shell.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_HSequenceOfShape.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <QUuid>
#include <QSet>
#include <algorithm>
#include <cmath>
#include <numbers>
#include <sstream>

namespace {
constexpr double kernelTolerance=1e-7,joinTolerance=1e-6;
QString id(const QString& feature,const QString& role) {
    return QUuid::createUuidV5(QUuid(feature),QByteArray("alloy-half-thickness/1\n")+role.toUtf8()).toString(QUuid::WithoutBraces);
}
QJsonArray point(const gp_Pnt& value){return {value.X(),value.Y(),value.Z()};}
std::string supportKey(const Handle(Geom_Surface)& surface) {
    if(surface.IsNull())return {};std::ostringstream stream;GeomTools::Write(surface,stream);return stream.str();
}
Handle(Geom_Surface) surfaceOf(const TopoDS_Face& face) {
    TopLoc_Location location;auto surface=BRep_Tool::Surface(face,location);
    return location.IsIdentity()?surface:Handle(Geom_Surface){};
}
double volume(const TopoDS_Shape& shape){GProp_GProps p;BRepGProp::VolumeProperties(shape,p,true);return p.Mass();}
struct Sector {
    const EvaluatedSupportFace* source;
    Handle(Geom_Curve) curve;
    double first,last;
    bool corner;
    TopoDS_Face patch;
};
bool adjacent(const TopoDS_Face& a,const TopoDS_Face& b) {
    TopTools_IndexedMapOfShape edges;TopExp::MapShapes(a,TopAbs_EDGE,edges);
    for(TopExp_Explorer ex(b,TopAbs_EDGE);ex.More();ex.Next())if(edges.Contains(ex.Current()))return true;
    return false;
}
bool finiteSector(const EvaluatedSupportFace& side,const Handle(Geom_Surface)& middle,double radius,Sector& result) {
    const auto surface=surfaceOf(side.face);if(surface.IsNull())return false;
    Handle(Geom_Surface) inward=new Geom_OffsetSurface(surface,side.face.Orientation()==TopAbs_REVERSED?radius:-radius);
    GeomAPI_IntSS intersection(middle,inward,kernelTolerance);
    if(!intersection.IsDone() || intersection.NbLines()!=1)return false;
    auto curve=intersection.Line(1);double first=curve->FirstParameter(),last=curve->LastParameter(),tolerance=kernelTolerance;
    if(!std::isfinite(first) || !std::isfinite(last) || first>=last)return false;
    const auto projected=GeomProjLib::Curve2d(curve,first,last,inward,tolerance);
    if(projected.IsNull())return false;
    std::vector<double> parameters{first,last};
    for(TopExp_Explorer ex(side.face,TopAbs_EDGE);ex.More();ex.Next()) {
        double a,b;auto boundary=BRep_Tool::CurveOnSurface(TopoDS::Edge(ex.Current()),side.face,a,b);
        if(boundary.IsNull())return false;
        Handle(Geom2d_Curve) finite=new Geom2d_TrimmedCurve(boundary,a,b);
        Geom2dAPI_InterCurveCurve hits(projected,finite,kernelTolerance);
        if(hits.NbSegments())return false;
        for(int i=1;i<=hits.NbPoints();++i) {
            const auto parameter=hits.Intersector().Point(i).ParamOnFirst();
            if(parameter>=first && parameter<=last)parameters.push_back(parameter);
        }
    }
    std::sort(parameters.begin(),parameters.end());
    parameters.erase(std::unique(parameters.begin(),parameters.end(),[](double a,double b){return std::abs(a-b)<1e-8;}),parameters.end());
    std::vector<std::pair<double,double>> intervals;
    for(size_t i=1;i<parameters.size();++i) {
        BRepClass_FaceClassifier classifier(side.face,projected->Value((parameters[i-1]+parameters[i])/2),kernelTolerance);
        if(classifier.State()==TopAbs_IN)intervals.emplace_back(parameters[i-1],parameters[i]);
    }
    if(intervals.size()!=1)return false;
    result={&side,curve,intervals[0].first,intervals[0].second,side.role=="fillet.round",{}};return true;
}
bool reconcile(std::vector<Sector>& sectors) {
    for(auto& sector:sectors)if(!sector.corner) {
        for(auto* parameter:{&sector.first,&sector.last}) {
            gp_Pnt target;int matches=0;
            for(const auto& corner:sectors)if(corner.corner && adjacent(sector.source->face,corner.source->face))
                for(const auto endpoint:{corner.first,corner.last}) {
                    const auto candidate=corner.curve->Value(endpoint);
                    if(candidate.Distance(sector.curve->Value(*parameter))<1e-5){target=candidate;++matches;}
                }
            if(matches!=1)return false;
            GeomAPI_ProjectPointOnCurve projection(target,sector.curve);
            if(projection.NbPoints()<1 || projection.LowerDistance()>joinTolerance)return false;
            int minima=0;
            for(int i=1;i<=projection.NbPoints();++i)if(projection.Distance(i)<=projection.LowerDistance()+kernelTolerance)++minima;
            if(minima!=1)return false;
            *parameter=projection.LowerDistanceParameter();
        }
        if(sector.first>=sector.last)return false;
    }
    return true;
}
bool makePatch(Sector& sector,const Handle(Geom_Surface)& original,const Handle(Geom_Surface)& offset,
    const Handle(Geom_Surface)& middle,double radius,double offsetSign,
    const Handle(TopTools_HSequenceOfShape) contacts[2]) {
    double tolerance=kernelTolerance;
    auto projected=GeomProjLib::Curve2d(sector.curve,sector.first,sector.last,middle,tolerance);
    if(projected.IsNull())return false;
    Handle(Geom2d_Curve) finite=new Geom2d_TrimmedCurve(projected,sector.first,sector.last);
    const auto uv=finite->Value(sector.first);gp_Pnt center,p;gp_Vec du,dv,tangent;
    middle->D1(uv.X(),uv.Y(),center,du,dv);auto normal=du.Crossed(dv);
    sector.curve->D1(sector.first,p,tangent);
    if(normal.Magnitude()<1e-10 || tangent.Magnitude()<1e-10)return false;
    normal.Normalize();normal*=offsetSign;tangent.Normalize();
    GeomAPI_ProjectPointOnSurf onSide(center,surfaceOf(sector.source->face));
    if(onSide.NbPoints()<1)return false;
    auto outward=gp_Vec(center,onSide.NearestPoint());if(outward.Magnitude()<1e-10)return false;outward.Normalize();
    if(tangent.Crossed(-normal).Dot(outward)<0)tangent.Reverse();
    Handle(Geom_Curve) circle=new Geom_Circle(gp_Ax2(center,gp_Dir(tangent),gp_Dir(-normal)),radius);
    Handle(Geom_Curve) arc=new Geom_TrimmedCurve(circle,0,std::numbers::pi);
    GeomFill_Pipe pipe(finite,middle,arc);pipe.Perform(kernelTolerance,false,GeomAbs_C1,11,30);
    const auto surface=pipe.Surface();if(surface.IsNull())return false;
    double u0,u1,v0,v1;surface->Bounds(u0,u1,v0,v1);
    if(std::abs(v0-sector.first)>kernelTolerance || std::abs(v1-sector.last)>kernelTolerance)return false;
    // Bounded construction witnesses complement the kernel's requested pipe
    // tolerance. Dense independent geometric certification stays out of preview.
    for(int i=0;i<=2;++i) {
        const double t=sector.first+(sector.last-sector.first)*i/2.;const auto puv=finite->Value(t);
        const auto c=middle->Value(puv.X(),puv.Y());
        for(int j=0;j<=4;++j)if(std::abs(surface->Value(u0+(u1-u0)*j/4.,t).Distance(c)-radius)>joinTolerance)return false;
        if(surface->Value(u0,t).Distance(original->Value(puv.X(),puv.Y()))>joinTolerance
            || surface->Value(u1,t).Distance(offset->Value(puv.X(),puv.Y()))>joinTolerance)return false;
    }
    BRep_Builder builder;TopoDS_Edge edges[2];
    for(int j=0;j<2;++j) {
        const auto support=j?offset:original;
        edges[j]=BRepBuilderAPI_MakeEdge(finite,support,sector.first,sector.last).Edge();
        if(!BRepLib::BuildCurve3d(edges[j],kernelTolerance))return false;
        Handle(Geom2d_Curve) pc=new Geom2d_Line(gp_Pnt2d(j?u1:u0,0),gp_Dir2d(0,1));
        builder.UpdateEdge(edges[j],pc,surface,TopLoc_Location(),kernelTolerance);
        builder.Range(edges[j],surface,TopLoc_Location(),sector.first,sector.last);contacts[j]->Append(edges[j]);
    }
    const auto a=BRepBuilderAPI_MakeEdge(surface->VIso(sector.first),u0,u1).Edge();
    const auto b=BRepBuilderAPI_MakeEdge(surface->VIso(sector.last),u0,u1).Edge();
    Handle(Geom2d_Curve) pa=new Geom2d_Line(gp_Pnt2d(0,sector.first),gp_Dir2d(1,0));
    Handle(Geom2d_Curve) pb=new Geom2d_Line(gp_Pnt2d(0,sector.last),gp_Dir2d(1,0));
    builder.UpdateEdge(a,pa,surface,TopLoc_Location(),kernelTolerance);builder.UpdateEdge(b,pb,surface,TopLoc_Location(),kernelTolerance);
    BRepBuilderAPI_MakeWire wire;wire.Add(edges[0]);wire.Add(b);wire.Add(TopoDS::Edge(edges[1].Reversed()));wire.Add(TopoDS::Edge(a.Reversed()));
    if(!wire.IsDone())return false;
    sector.patch=BRepBuilderAPI_MakeFace(surface,wire.Wire(),true).Face();
    return BRepCheck_Analyzer(sector.patch,true,false,true).IsValid();
}
}

std::optional<RuledCornerFillet::Result> HalfThicknessFillet::evaluate(const EvaluatedBody& body,
    const BRepFilletAPI_MakeFillet& contours,const QMap<int,double>& radii,const QString& featureId) {
    try {
        if(body.summary.value("construction")!="ruledCornerCut" || body.supportFaces.size()!=10
            || contours.NbContours()!=2 || radii.size()!=2 || !BRepCheck_Analyzer(body.shape,true,false,true).IsValid())return {};
        const EvaluatedSupportFace *originalFace=nullptr,*offsetFace=nullptr;
        QMap<QString,const EvaluatedSupportFace*> named;int corners=0,walls=0;
        for(const auto& face:body.supportFaces) {
            if(face.face.IsNull() || named.contains(face.faceId) || surfaceOf(face.face).IsNull())return {};
            named[face.faceId]=&face;
            if(face.role=="source"){if(originalFace)return {};originalFace=&face;}
            else if(face.role=="offset"){if(offsetFace)return {};offsetFace=&face;}
            else if(face.role.startsWith("side/"))++walls;
            else if(face.role=="fillet.round" && !face.provenance.value("sourceSeedEdgeId").toString().isEmpty())++corners;
            else return {};
        }
        if(!originalFace || !offsetFace || walls!=4 || corners!=4)return {};
        const auto original=surfaceOf(originalFace->face),offset=surfaceOf(offsetFace->face);
        const auto offsetSurface=Handle(Geom_OffsetSurface)::DownCast(offset);
        if(offsetSurface.IsNull() || supportKey(original)!=supportKey(offsetSurface->BasisSurface()))return {};
        const double displacement=offsetSurface->Offset(),radius=std::abs(displacement)/2;
        if(!std::isfinite(radius) || radius<kernelTolerance)return {};
        TopTools_IndexedDataMapOfShapeListOfShape edgeFaces;
        TopExp::MapShapesAndAncestors(body.shape,TopAbs_EDGE,TopAbs_FACE,edgeFaces);
        TopTools_IndexedMapOfShape expectedRims,selectedRims;
        for(const auto* broad:{originalFace,offsetFace})for(TopExp_Explorer ex(broad->face,TopAbs_EDGE);ex.More();ex.Next())expectedRims.Add(ex.Current());
        for(int contour=1;contour<=contours.NbContours();++contour) {
            if(!radii.contains(contour) || std::abs(radii.value(contour)-radius)>1e-9)return {};
            int broadRole=-1;
            for(int i=1;i<=contours.NbEdges(contour);++i) {
                const auto edge=contours.Edge(contour,i);if(!expectedRims.Contains(edge) || selectedRims.Contains(edge))return {};
                const int index=edgeFaces.FindIndex(edge);if(index<1)return {};
                int currentRole=-1;
                for(const auto& face:edgeFaces.FindFromIndex(index)) {
                    if(face.IsSame(originalFace->face))currentRole=0;
                    if(face.IsSame(offsetFace->face)){if(currentRole!=-1)return {};currentRole=1;}
                }
                if(currentRole<0 || (broadRole>=0 && currentRole!=broadRole))return {};
                broadRole=currentRole;selectedRims.Add(edge);
            }
        }
        if(selectedRims.Extent()!=expectedRims.Extent())return {};
        Handle(Geom_Surface) middle=new Geom_OffsetSurface(original,displacement/2);
        std::vector<Sector> sectors;
        // Shape traversal controls kernel operation order; UUIDs only name the result.
        TopTools_IndexedMapOfShape inputFaces;TopExp::MapShapes(body.shape,TopAbs_FACE,inputFaces);
        if(inputFaces.Extent()!=10)return {};
        for(int i=1;i<=inputFaces.Extent();++i) {
            const EvaluatedSupportFace* source=nullptr;
            for(const auto& candidate:body.supportFaces)if(candidate.face.IsSame(inputFaces(i))){if(source)return {};source=&candidate;}
            if(!source)return {};
            if(source==originalFace || source==offsetFace)continue;
            Sector sector;if(!finiteSector(*source,middle,radius,sector))return {};sectors.push_back(sector);
        }
        if(sectors.size()!=8 || !reconcile(sectors))return {};
        Handle(TopTools_HSequenceOfShape) contacts[2]={new TopTools_HSequenceOfShape,new TopTools_HSequenceOfShape};
        BRepBuilderAPI_Sewing sewing(joinTolerance,true,true,true,false);
        for(auto& sector:sectors){if(!makePatch(sector,original,offset,middle,radius,displacement>0?1:-1,contacts))return {};sewing.Add(sector.patch);}
        for(int i=0;i<2;++i) {
            Handle(TopTools_HSequenceOfShape) wires=new TopTools_HSequenceOfShape;
            ShapeAnalysis_FreeBounds::ConnectEdgesToWires(contacts[i],joinTolerance,false,wires);
            if(wires->Length()!=1 || !wires->Value(1).Closed())return {};
            auto broad=BRepBuilderAPI_MakeFace(i?offset:original,TopoDS::Wire(wires->Value(1)),true).Face();
            if(!BRepCheck_Analyzer(broad,true,false,true).IsValid())return {};sewing.Add(broad);
        }
        sewing.Perform();const auto sewn=sewing.SewedShape();
        if(sewn.ShapeType()!=TopAbs_SHELL || sewing.NbFreeEdges() || sewing.NbMultipleEdges())return {};
        ShapeFix_Shell orientation(TopoDS::Shell(sewn));orientation.FixFaceOrientation(TopoDS::Shell(sewn));
        auto solid=BRepBuilderAPI_MakeSolid(orientation.Shell()).Solid();BRepLib::OrientClosedSolid(solid);
        if(!BRepCheck_Analyzer(solid,true,false,true).IsValid() || !std::isfinite(volume(solid))
            || volume(solid)<=0 || volume(solid)>=volume(body.shape))return {};
        TopTools_IndexedMapOfShape finalFaces,finalEdges;TopExp::MapShapes(solid,TopAbs_FACE,finalFaces);TopExp::MapShapes(solid,TopAbs_EDGE,finalEdges);
        if(finalFaces.Extent()!=10 || finalEdges.Extent()!=24)return {};
        RuledCornerFillet::Result result;result.shape=solid;QSet<int> claimed;
        auto currentFace=[&](const Handle(Geom_Surface)& surface)->TopoDS_Face {
            const auto key=supportKey(surface);int match=0;
            for(int i=1;i<=finalFaces.Extent();++i)if(supportKey(surfaceOf(TopoDS::Face(finalFaces(i))))==key){if(match)return {};match=i;}
            if(!match || claimed.contains(match))return {};claimed.insert(match);return TopoDS::Face(finalFaces(match));
        };
        for(const auto* source:{originalFace,offsetFace}) {
            auto retained=*source;retained.face=currentFace(surfaceOf(source->face));retained.boundaryEdges.clear();
            if(retained.face.IsNull())return {};result.supportFaces.push_back(retained);
        }
        for(const auto& sector:sectors) {
            const auto face=currentFace(surfaceOf(sector.patch));if(face.IsNull())return {};
            result.supportFaces.push_back({id(featureId,"face\n"+sector.source->faceId),body.bodyId,featureId,"fillet.perimeter",face,{}, {},
                {{"sourceWallFaceId",sector.source->faceId},{"sourceWallRole",sector.source->role},{"radiusMm",radius}}});
        }
        if(claimed.size()!=10)return {};
        TopTools_IndexedDataMapOfShapeListOfShape finalAdjacency,vertexFaces;
        TopExp::MapShapesAndAncestors(solid,TopAbs_EDGE,TopAbs_FACE,finalAdjacency);
        TopExp::MapShapesAndAncestors(solid,TopAbs_VERTEX,TopAbs_FACE,vertexFaces);
        for(int i=1;i<=finalEdges.Extent();++i) {
            const auto edge=TopoDS::Edge(finalEdges(i));const int index=finalAdjacency.FindIndex(edge);if(index<1)return {};
            std::vector<EvaluatedSupportFace*> neighbors;
            for(const auto& shape:finalAdjacency.FindFromIndex(index))for(auto& face:result.supportFaces)if(face.face.IsSame(shape))neighbors.push_back(&face);
            if(neighbors.size()!=2 || neighbors[0]==neighbors[1])return {};
            TopoDS_Vertex start,end;TopExp::Vertices(edge,start,end,true);if(start.IsNull() || end.IsNull() || start.IsSame(end))return {};
            auto endpointNeighbor=[&](const TopoDS_Vertex& vertex)->QString {
                const int v=vertexFaces.FindIndex(vertex);if(v<1)return {};QSet<QString> others;
                for(const auto& shape:vertexFaces.FindFromIndex(v))for(const auto& face:result.supportFaces)
                    if(face.faceId!=neighbors[0]->faceId && face.faceId!=neighbors[1]->faceId && face.face.IsSame(shape))others.insert(face.faceId);
                return others.size()==1?*others.begin():QString{};
            };
            auto startNeighbor=endpointNeighbor(start),endNeighbor=endpointNeighbor(end);
            if(startNeighbor.isEmpty() || endNeighbor.isEmpty() || startNeighbor==endNeighbor)return {};
            if(endNeighbor<startNeighbor){std::swap(start,end);std::swap(startNeighbor,endNeighbor);}
            QStringList pair{neighbors[0]->faceId,neighbors[1]->faceId};pair.sort();
            const auto edgeId=id(featureId,"edge\n"+pair.join('\n'));
            auto startFaces=pair,endFaces=pair;startFaces.append(startNeighbor);endFaces.append(endNeighbor);startFaces.sort();endFaces.sort();
            QJsonObject provenance{{"role","perimeterBoundary"},{"adjacentFaceIds",QJsonArray{pair[0],pair[1]}},
                {"startAdjacentFaceId",startNeighbor},{"endAdjacentFaceId",endNeighbor},
                {"startPointId",id(featureId,"point\n"+startFaces.join('\n'))},
                {"endPointId",id(featureId,"point\n"+endFaces.join('\n'))},
                {"endpointProvenance","incidentPerimeterFaces"},{"startMm",point(BRep_Tool::Pnt(start))},{"endMm",point(BRep_Tool::Pnt(end))}};
            for(const auto* neighbor:neighbors)if(neighbor->role=="source" || neighbor->role=="offset")provenance["broadFaceRole"]=neighbor->role;
            if(std::any_of(result.solidEdges.begin(),result.solidEdges.end(),[&](const auto& existing){return existing.edgeId==edgeId;}))return {};
            result.solidEdges.push_back({edgeId,edge,provenance});
            for(auto* neighbor:neighbors)neighbor->boundaryEdges.push_back({edgeId,"curve",edge,provenance});
        }
        return result;
    } catch(const std::exception&) {return {};}
}
