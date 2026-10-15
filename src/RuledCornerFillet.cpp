// SPDX-License-Identifier: MIT
#include "RuledCornerFillet.h"
#include "PlanarFaces.h"
#include "SolidFillet.h"
#include <BRepAlgoAPI_Common.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeSolid.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepBuilderAPI_Sewing.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepFilletAPI_MakeFillet.hxx>
#include <BRepGProp.hxx>
#include <BRepLib.hxx>
#include <BRepOffsetAPI_ThruSections.hxx>
#include <BRepTools.hxx>
#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <GProp_GProps.hxx>
#include <Geom_BSplineSurface.hxx>
#include <GeomTools.hxx>
#include <ShapeAnalysis_FreeBounds.hxx>
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
#include <memory>
#include <sstream>

namespace {
constexpr double tolerance=1e-6;
struct Wall {const EvaluatedSupportFace* source;TopoDS_Face extension;Handle(Geom_BSplineSurface) surface;};
struct Corner {QString seedId,firstWall,secondWall;double radius;TopoDS_Edge extendedEdge;TopoDS_Face roundFace;TopoDS_Solid cutter;};
QString pairKey(QString a,QString b) {if(b<a)std::swap(a,b);return a+"\n"+b;}
QString generatedId(const QString& feature,const QByteArray& role) {
    return QUuid::createUuidV5(QUuid(feature),QByteArray("alloy-ruled-corner/1\n")+role).toString(QUuid::WithoutBraces);
}
QJsonArray point(const gp_Pnt& p) {return {p.X(),p.Y(),p.Z()};}
double volume(const TopoDS_Shape& shape) {GProp_GProps props;BRepGProp::VolumeProperties(shape,props,true);return props.Mass();}
bool oneValidSolid(const TopoDS_Shape& shape) {
    if(shape.IsNull() || !BRepCheck_Analyzer(shape,true,false,true).IsValid())return false;
    int solids=0;for(TopExp_Explorer ex(shape,TopAbs_SOLID);ex.More();ex.Next())++solids;
    return solids==1 && std::isfinite(volume(shape)) && volume(shape)>0;
}
TopoDS_Shape uniqueDescendant(BRepBuilderAPI_MakeShape& operation,const TopoDS_Shape& source,
                              const TopTools_IndexedMapOfShape& present,TopAbs_ShapeEnum kind) {
    TopTools_IndexedMapOfShape candidates;
    if(present.Contains(source))candidates.Add(source);
    for(const auto& child:operation.Modified(source))if(child.ShapeType()==kind && present.Contains(child))candidates.Add(child);
    for(const auto& child:operation.Generated(source))if(child.ShapeType()==kind && present.Contains(child))candidates.Add(child);
    return candidates.Extent()==1 ? candidates(1) : TopoDS_Shape{};
}
std::string exactSupport(const TopoDS_Face& face) {
    TopLoc_Location location;const auto surface=BRep_Tool::Surface(face,location);
    if(surface.IsNull() || !location.IsIdentity())return {};
    std::ostringstream stream;GeomTools::Write(surface,stream);return stream.str();
}
TopoDS_Shape copied(BRepBuilderAPI_Copy& copy,const TopoDS_Shape& source) {
    return copy.ModifiedShape(source);
}
bool validExtendedWall(const EvaluatedSupportFace& face,Wall& output) {
    if(face.face.IsNull() || !face.role.startsWith("side/"))return false;
    TopLoc_Location location;
    const auto original=Handle(Geom_BSplineSurface)::DownCast(BRep_Tool::Surface(face.face,location));
    if(original.IsNull() || !location.IsIdentity() || original->VDegree()!=1 || original->NbVKnots()!=2
        || original->NbVPoles()!=2 || original->IsURational() || original->IsVRational())return false;
    auto surface=Handle(Geom_BSplineSurface)::DownCast(original->Copy());
    double u0,u1,v0,v1;original->Bounds(u0,u1,v0,v1);
    const double span=v1-v0;if(!std::isfinite(span) || span<=0 || !std::isfinite(u0) || !std::isfinite(u1))return false;
    for(int u=1;u<=surface->NbUPoles();++u) {
        const auto start=original->Pole(u,1),end=original->Pole(u,2);const gp_Vec span(start,end);
        surface->SetPole(u,1,start.Translated(-span));surface->SetPole(u,2,end.Translated(span));
    }
    surface->SetVKnot(1,v0-span);surface->SetVKnot(2,v1+span);
    // The affine extension must preserve the entire original support domain.
    for(int u=0;u<=16;++u)for(int v=0;v<=8;++v)
        if(original->Value(u0+(u1-u0)*u/16.,v0+span*v/8.).Distance(surface->Value(u0+(u1-u0)*u/16.,v0+span*v/8.))>1e-9)return false;
    auto extended=BRepBuilderAPI_MakeFace(surface,1e-7).Face();
    extended.Orientation(face.face.Orientation());output={&face,extended,surface};return true;
}
bool makeCutter(const TopoDS_Shape& source,Corner& corner,const TopoDS_Face& a,const TopoDS_Face& b,
                const TopoDS_Edge& shared) {
    BRep_Builder builder;TopoDS_Shell pair;builder.MakeShell(pair);builder.Add(pair,a);builder.Add(pair,b);
    BRepFilletAPI_MakeFillet fillet(pair);fillet.Add(corner.radius,shared);fillet.Build();
    if(!fillet.IsDone() || !BRepCheck_Analyzer(fillet.Shape(),true,false,true).IsValid())return false;
    BRepBuilderAPI_Sewing open(1e-6);
    for(const auto& wall:{a,b}) {
        const auto& modified=fillet.Modified(wall);if(modified.IsEmpty())return false;
        TopoDS_Compound parts;builder.MakeCompound(parts);
        for(const auto& part:modified)builder.Add(parts,part);
        BRepAlgoAPI_Cut difference(wall,parts);if(!difference.IsDone())return false;
        open.Add(difference.Shape());
    }
    TopTools_IndexedMapOfShape rounds;
    for(const auto& shape:fillet.Generated(shared))if(shape.ShapeType()==TopAbs_FACE)rounds.Add(shape);
    if(rounds.Extent()!=1)return false;
    corner.roundFace=TopoDS::Face(rounds(1));open.Add(corner.roundFace.Reversed());open.Perform();
    TopTools_IndexedMapOfShape openFaces;TopExp::MapShapes(open.SewedShape(),TopAbs_FACE,openFaces);
    if(openFaces.Extent()!=3 || open.NbFreeEdges()!=6)return false;
    Handle(TopTools_HSequenceOfShape) freeEdges=new TopTools_HSequenceOfShape,loops=new TopTools_HSequenceOfShape;
    for(int index=1;index<=open.NbFreeEdges();++index)freeEdges->Append(open.FreeEdge(index));
    ShapeAnalysis_FreeBounds::ConnectEdgesToWires(freeEdges,1e-6,false,loops);
    if(loops->Length()!=2)return false;
    BRepBuilderAPI_Sewing closed(1e-6);closed.Add(open.SewedShape());
    for(int index=1;index<=2;++index) {
        const auto loop=TopoDS::Wire(loops->Value(index));TopTools_IndexedMapOfShape vertices;
        TopExp::MapShapes(loop,TopAbs_VERTEX,vertices);if(vertices.Extent()<2)return false;
        gp_XYZ sum(0,0,0);for(int vertex=1;vertex<=vertices.Extent();++vertex)
            sum+=BRep_Tool::Pnt(TopoDS::Vertex(vertices(vertex))).XYZ();
        const gp_Pnt center(sum/vertices.Extent());
        BRepExtrema_DistShapeShape distance(BRepBuilderAPI_MakeVertex(center),source);distance.Perform();
        if(!distance.IsDone() || distance.Value()<1e-4)return false;
        gp_Vec outward(distance.PointOnShape2(1),center);if(outward.Magnitude()<1e-9)return false;
        outward.Normalize();const auto apex=BRepBuilderAPI_MakeVertex(center.Translated(5*outward)).Vertex();
        BRepOffsetAPI_ThruSections cap(false,true,1e-6);cap.SetMutableInput(false);
        cap.AddWire(loop);cap.AddVertex(apex);cap.Build();if(!cap.IsDone())return false;
        BRepExtrema_DistShapeShape separation(cap.Shape(),source);separation.Perform();
        if(!separation.IsDone() || separation.Value()<1e-4 || !BRepCheck_Analyzer(cap.Shape(),true,false,true).IsValid())return false;
        closed.Add(cap.Shape());
    }
    closed.Perform();if(closed.SewedShape().ShapeType()!=TopAbs_SHELL)return false;
    corner.cutter=BRepBuilderAPI_MakeSolid(TopoDS::Shell(closed.SewedShape())).Solid();
    BRepLib::OrientClosedSolid(corner.cutter);
    return oneValidSolid(corner.cutter);
}
}

std::optional<RuledCornerFillet::Result> RuledCornerFillet::evaluate(const EvaluatedBody& body,
    const QMap<QString,double>& seedRadii,const QString& featureId) {
    try {
        if(seedRadii.size()!=4 || body.supportFaces.size()!=6 || body.solidEdges.size()!=12
            || body.summary.value("sourceFace").toObject().isEmpty() || !oneValidSolid(body.shape))return {};
        const auto topology=body.summary.value("topology").toObject();
        if(topology.value("faces").toInt()!=6 || topology.value("edges").toInt()!=12)return {};
        QMap<QString,Wall> walls;std::vector<const EvaluatedSupportFace*> broad;
        for(const auto& face:body.supportFaces) {
            if(face.role.startsWith("side/")) {
                Wall wall;if(!validExtendedWall(face,wall) || walls.contains(face.faceId))return {};
                walls.insert(face.faceId,wall);
            } else if(face.role=="source" || face.role=="offset")broad.push_back(&face);
            else return {};
        }
        if(walls.size()!=4 || broad.size()!=2 || broad[0]->role==broad[1]->role)return {};
        TopTools_IndexedDataMapOfShapeListOfShape originalAdjacency;
        TopExp::MapShapesAndAncestors(body.shape,TopAbs_EDGE,TopAbs_FACE,originalAdjacency);
        QMap<QString,Corner> corners;QSet<QString> usedPairs;QMap<QString,int> wallUses;
        for(auto it=seedRadii.begin();it!=seedRadii.end();++it) {
            if(!std::isfinite(it.value()) || it.value()<=0)return {};
            const auto named=std::find_if(body.solidEdges.begin(),body.solidEdges.end(),[&](const auto& edge){return edge.edgeId==it.key();});
            if(named==body.solidEdges.end() || named->edge.IsNull() || !named->provenance.value("role").toString().startsWith("corner/"))return {};
            const int index=originalAdjacency.FindIndex(named->edge);if(index<1)return {};
            const auto neighbors=originalAdjacency.FindFromIndex(index);if(neighbors.Extent()!=2)return {};
            QStringList ids;
            for(const auto& neighbor:neighbors)for(auto face=walls.begin();face!=walls.end();++face)
                if(face->source->face.IsSame(neighbor))ids.append(face.key());
            if(ids.size()!=2 || ids[0]==ids[1])return {};
            const auto key=pairKey(ids[0],ids[1]);if(usedPairs.contains(key))return {};
            usedPairs.insert(key);++wallUses[ids[0]];++wallUses[ids[1]];
            if(ids[1]<ids[0])std::swap(ids[0],ids[1]);
            corners.insert(key,{it.key(),ids[0],ids[1],it.value(),{}, {}, {}});
        }
        if(corners.size()!=4) return {};
        for(auto it=walls.begin();it!=walls.end();++it)if(wallUses.value(it.key())!=2)return {};
        // Operation order follows the input topology, never authored UUIDs.
        // OCCT's rolling-ball approximation and Boolean trimming can depend on
        // input order even when the exact source supports are equivalent.
        TopTools_IndexedMapOfShape inputFaces,inputEdges;
        TopExp::MapShapes(body.shape,TopAbs_FACE,inputFaces);
        TopExp::MapShapes(body.shape,TopAbs_EDGE,inputEdges);
        QStringList orderedWalls=walls.keys(),orderedCorners=corners.keys();
        auto wallRank=[&](const QString& id){return inputFaces.FindIndex(walls[id].source->face);};
        auto cornerRank=[&](const QString& id){
            const auto seed=std::find_if(body.solidEdges.begin(),body.solidEdges.end(),[&](const auto& edge){return edge.edgeId==corners[id].seedId;});
            return inputEdges.FindIndex(seed->edge);
        };
        for(const auto& id:orderedWalls)if(wallRank(id)<1)return {};
        for(const auto& id:orderedCorners)if(cornerRank(id)<1)return {};
        std::sort(orderedWalls.begin(),orderedWalls.end(),[&](const auto& a,const auto& b){return wallRank(a)<wallRank(b);});
        std::sort(orderedCorners.begin(),orderedCorners.end(),[&](const auto& a,const auto& b){return cornerRank(a)<cornerRank(b);});
        BRepBuilderAPI_Sewing extensions(1e-6);
        for(const auto& id:orderedWalls)extensions.Add(walls[id].extension);
        extensions.Perform();if(!BRepCheck_Analyzer(extensions.SewedShape(),true,false,true).IsValid())return {};
        TopTools_IndexedDataMapOfShapeListOfShape extendedAdjacency;
        TopExp::MapShapesAndAncestors(extensions.SewedShape(),TopAbs_EDGE,TopAbs_FACE,extendedAdjacency);
        QSet<QString> matched;
        for(int index=1;index<=extendedAdjacency.Extent();++index) {
            const auto neighbors=extendedAdjacency.FindFromIndex(index);if(neighbors.Extent()!=2)continue;
            QStringList ids;TopoDS_Face pairFaces[2];int n=0;
            for(const auto& neighbor:neighbors) {
                QString id;const auto surface=BRep_Tool::Surface(TopoDS::Face(neighbor));
                for(auto wall=walls.begin();wall!=walls.end();++wall)if(surface==wall->surface){if(!id.isEmpty())return {};id=wall.key();}
                if(id.isEmpty() || n==2)return {};ids.append(id);pairFaces[n++]=TopoDS::Face(neighbor);
            }
            const auto key=pairKey(ids[0],ids[1]);if(!corners.contains(key) || matched.contains(key))return {};
            if(wallRank(ids[1])<wallRank(ids[0]))std::swap(pairFaces[0],pairFaces[1]);
            matched.insert(key);corners[key].extendedEdge=TopoDS::Edge(extendedAdjacency.FindKey(index));
            if(!makeCutter(body.shape,corners[key],pairFaces[0],pairFaces[1],corners[key].extendedEdge))return {};
        }
        if(matched.size()!=4)return {};
        BRepBuilderAPI_Copy copy(body.shape,false,false);auto normalized=copy.Shape();
        BRep_Builder builder;
        for(auto it=walls.begin();it!=walls.end();++it) {
            auto face=TopoDS::Face(copied(copy,it->source->face));if(face.IsNull())return {};
            std::vector<TopoDS_Edge> edges;std::vector<Handle(Geom2d_Curve)> curves;
            std::vector<double> first,last;
            for(TopExp_Explorer ex(face,TopAbs_EDGE);ex.More();ex.Next()) {
                auto edge=TopoDS::Edge(ex.Current());double a,b;
                auto curve=BRep_Tool::CurveOnSurface(edge,face,a,b);if(curve.IsNull())return {};
                edges.push_back(edge);curves.push_back(curve);first.push_back(a);last.push_back(b);
            }
            builder.UpdateFace(face,it->surface,TopLoc_Location(),BRep_Tool::Tolerance(face));
            for(size_t i=0;i<edges.size();++i){builder.UpdateEdge(edges[i],curves[i],face,BRep_Tool::Tolerance(edges[i]));builder.Range(edges[i],face,first[i],last[i]);}
        }
        if(!oneValidSolid(normalized) || std::abs(volume(normalized)-volume(body.shape))>1e-5)return {};
        TopTools_ListOfShape arguments,tools;arguments.Append(normalized);
        for(const auto& id:orderedCorners) {
            const auto& corner=corners[id];
            BRepAlgoAPI_Common common;
            TopTools_ListOfShape commonArguments,commonTools;
            commonArguments.Append(normalized);commonTools.Append(corner.cutter);
            common.SetArguments(commonArguments);common.SetTools(commonTools);
            common.SetNonDestructive(true);common.Build();
            if(!common.IsDone() || !std::isfinite(volume(common.Shape())) || volume(common.Shape())<1e-5)return {};
            tools.Append(corner.cutter);
        }
        BRepAlgoAPI_Cut cut;cut.SetArguments(arguments);cut.SetTools(tools);cut.SetNonDestructive(true);cut.Build();
        if(!cut.IsDone() || !oneValidSolid(cut.Shape()) || volume(cut.Shape())>=volume(normalized)-1e-5)return {};
        BRepAlgoAPI_Cut outside;
        TopTools_ListOfShape outsideArguments,outsideTools;
        outsideArguments.Append(cut.Shape());outsideTools.Append(normalized);
        outside.SetArguments(outsideArguments);outside.SetTools(outsideTools);
        outside.SetNonDestructive(true);outside.Build();
        if(!outside.IsDone() || TopExp_Explorer(outside.Shape(),TopAbs_SOLID).More())return {};
        TopTools_IndexedMapOfShape finalFaces,finalEdges;
        TopExp::MapShapes(cut.Shape(),TopAbs_FACE,finalFaces);TopExp::MapShapes(cut.Shape(),TopAbs_EDGE,finalEdges);
        if(finalFaces.Extent()!=10)return {};
        Result result;result.shape=cut.Shape();TopTools_IndexedMapOfShape claimed;
        for(const auto& source:body.supportFaces) {
            const auto onCopy=copied(copy,source.face);
            const auto current=uniqueDescendant(cut,onCopy,finalFaces,TopAbs_FACE);
            if(current.IsNull() || claimed.Contains(current))return {};
            claimed.Add(current);auto retained=source;retained.face=TopoDS::Face(current);retained.boundaryEdges.clear();
            TopTools_IndexedMapOfShape faceEdges;TopExp::MapShapes(current,TopAbs_EDGE,faceEdges);
            for(auto edge:source.boundaryEdges) {
                const auto copiedEdge=copied(copy,edge.edge);
                const auto next=uniqueDescendant(cut,copiedEdge,faceEdges,TopAbs_EDGE);
                if(!next.IsNull()){edge.edge=TopoDS::Edge(next);retained.boundaryEdges.push_back(edge);}
            }
            result.supportFaces.push_back(std::move(retained));
        }
        QMap<QString,TopoDS_Face> rounded;
        for(auto it=corners.begin();it!=corners.end();++it) {
            auto current=uniqueDescendant(cut,it->roundFace,finalFaces,TopAbs_FACE);
            if(current.IsNull()) {
                // Sewing the three cutter walls can replace a face's TShape.
                // Accept only one exact OCCT support representation in the
                // final solid; this is curve data identity, never proximity.
                const auto key=exactSupport(it->roundFace);int matches=0;
                if(key.empty())return {};
                for(int i=1;i<=finalFaces.Extent();++i)if(exactSupport(TopoDS::Face(finalFaces(i)))==key){
                    current=finalFaces(i);++matches;
                }
                if(matches!=1)return {};
            }
            if(current.IsNull() || claimed.Contains(current))return {};
            claimed.Add(current);rounded[it.key()]=TopoDS::Face(current);
            const auto id=generatedId(featureId,("face\n"+it->seedId+"\n"+it.key()).toUtf8());
            result.supportFaces.push_back({id,body.bodyId,featureId,"fillet.round",TopoDS::Face(current),{}, {},
                {{"sourceSeedEdgeId",it->seedId},{"sourceWallFaceIds",QJsonArray{it->firstWall,it->secondWall}},{"radiusMm",it->radius}}});
        }
        if(claimed.Extent()!=10)return {}; // No auxiliary cap or unknown support may survive.
        std::vector<EvaluatedSolidEdge> copiedEdges;copiedEdges.reserve(body.solidEdges.size());
        for(auto edge:body.solidEdges){edge.edge=TopoDS::Edge(copied(copy,edge.edge));copiedEdges.push_back(edge);}
        result.solidEdges=SolidFillet::transferEdges(cut,copiedEdges,result.shape,true);
        TopTools_IndexedDataMapOfShapeListOfShape edgeFaces,vertexFaces;
        TopExp::MapShapesAndAncestors(result.shape,TopAbs_EDGE,TopAbs_FACE,edgeFaces);
        TopExp::MapShapesAndAncestors(result.shape,TopAbs_VERTEX,TopAbs_FACE,vertexFaces);
        for(auto it=corners.begin();it!=corners.end();++it) {
            auto roundedFace=std::find_if(result.supportFaces.begin(),result.supportFaces.end(),[&](const auto& face){return face.role=="fillet.round" && face.provenance.value("sourceSeedEdgeId")==it->seedId;});
            if(roundedFace==result.supportFaces.end())return {};
            for(const auto* broadFace:broad) {
                auto retainedBroad=std::find_if(result.supportFaces.begin(),result.supportFaces.end(),[&](const auto& face){return face.faceId==broadFace->faceId;});
                if(retainedBroad==result.supportFaces.end())return {};
                std::vector<TopoDS_Edge> joinEdges;
                for(int i=1;i<=edgeFaces.Extent();++i) {
                    bool round=false,broadMatch=false;
                    for(const auto& neighbor:edgeFaces.FindFromIndex(i)) {
                        round|=neighbor.IsSame(roundedFace->face);broadMatch|=neighbor.IsSame(retainedBroad->face);
                    }
                    if(round && broadMatch)joinEdges.push_back(TopoDS::Edge(edgeFaces.FindKey(i)));
                }
                if(joinEdges.empty() || joinEdges.size()>3)return {};
                auto endpointRole=[&](const TopoDS_Vertex& vertex)->QString {
                    const int index=vertexFaces.FindIndex(vertex);if(index<1)return {};
                    QString match;
                    for(const auto& neighbor:vertexFaces.FindFromIndex(index))for(const auto& face:result.supportFaces)
                        if((face.faceId==it->firstWall || face.faceId==it->secondWall) && face.face.IsSame(neighbor)) {
                            if(!match.isEmpty() && match!=face.faceId)return {};match=face.faceId;
                        }
                    return match;
                };
                const auto chainId=generatedId(featureId,("chain\n"+roundedFace->faceId+"\n"+broadFace->role).toUtf8());
                struct Segment {TopoDS_Edge edge;QString role;TopoDS_Vertex start,end;};
                std::vector<Segment> segments;QMap<QString,TopoDS_Vertex> terminalInteriors;
                for(const auto& joining:joinEdges) {
                    TopoDS_Vertex first,last;TopExp::Vertices(joining,first,last,true);
                    if(first.IsNull() || last.IsNull() || first.IsSame(last))return {};
                    const auto a=endpointRole(first),b=endpointRole(last);
                    QString role;TopoDS_Vertex start=first,end=last;
                    if(joinEdges.size()==1) {
                        if(a.isEmpty() || b.isEmpty() || a==b)return {};
                        role="whole";if(a!=it->firstWall){start=last;end=first;}
                    } else if(!a.isEmpty() != !b.isEmpty()) {
                        role=a.isEmpty()?b:a;
                        if(terminalInteriors.contains(role))return {};
                        if(a.isEmpty()){start=last;end=first;}
                        terminalInteriors[role]=end;
                    } else if(a.isEmpty() && b.isEmpty() && joinEdges.size()==3)role="middle";
                    else return {};
                    segments.push_back({joining,role,start,end});
                }
                if(joinEdges.size()>1) {
                    if(terminalInteriors.size()!=2 || !terminalInteriors.contains(it->firstWall)
                        || !terminalInteriors.contains(it->secondWall))return {};
                    if(joinEdges.size()==2) {
                        if(!terminalInteriors[it->firstWall].IsSame(terminalInteriors[it->secondWall]))return {};
                    } else {
                        auto middle=std::find_if(segments.begin(),segments.end(),[](const auto& s){return s.role=="middle";});
                        if(middle==segments.end() || std::count_if(segments.begin(),segments.end(),[](const auto& s){return s.role=="middle";})!=1
                            || terminalInteriors[it->firstWall].IsSame(terminalInteriors[it->secondWall]))return {};
                        if(middle->start.IsSame(terminalInteriors[it->secondWall]))std::swap(middle->start,middle->end);
                        if(!middle->start.IsSame(terminalInteriors[it->firstWall])
                            || !middle->end.IsSame(terminalInteriors[it->secondWall]))return {};
                    }
                }
                for(const auto& segment:segments) {
                    const auto& joining=segment.edge;const auto& role=segment.role;
                    const auto id=generatedId(featureId,("edge\n"+chainId+"\n"+role).toUtf8());
                    QJsonObject provenance{{"role","roundBoundary"},{"sourceSeedEdgeId",it->seedId},
                        {"sourceWallFaceIds",QJsonArray{it->firstWall,it->secondWall}},
                        {"broadFaceRole",broadFace->role},{"boundaryChainId",chainId},
                        {"terminalWallFaceId",role=="whole" || role=="middle"?QJsonValue():QJsonValue(role)},
                        {"startWallFaceId",role=="middle"?QJsonValue():QJsonValue(role=="whole"?it->firstWall:role)},
                        {"endWallFaceId",role=="whole"?QJsonValue(it->secondWall):QJsonValue()},
                        {"endpointProvenance","incidentSideFaces"},
                        {"startMm",point(BRep_Tool::Pnt(segment.start))},{"endMm",point(BRep_Tool::Pnt(segment.end))}};
                    result.solidEdges.push_back({id,joining,provenance});
                    EvaluatedSupportEdge named{id,"curve",joining,provenance};
                    roundedFace->boundaryEdges.push_back(named);retainedBroad->boundaryEdges.push_back(named);
                }
            }
        }
        return result;
    } catch(const std::exception&) {return {};}
}
