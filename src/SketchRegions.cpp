// SPDX-License-Identifier: MIT
#include "SketchProfiles.h"
#include "SketchGeometry.h"
#include <BOPAlgo_Builder.hxx>
#include <BOPAlgo_BuilderFace.hxx>
#include <BOPTools_AlgoTools2D.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepGProp.hxx>
#include <BRepTools_WireExplorer.hxx>
#include <BRepTools_ReShape.hxx>
#include <TopoDS_Iterator.hxx>
#include <BRepTopAdaptor_FClass2d.hxx>
#include <BRep_Tool.hxx>
#include <GProp_GProps.hxx>
#include <GeomAPI_ProjectPointOnCurve.hxx>
#include <gp_Ax3.hxx>
#include <gp_Pln.hxx>
#include <gp_Vec.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <QJsonArray>
#include <QMap>
#include <QSet>
#include <QUuid>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <limits>

namespace {
constexpr double contactTolerance=1e-6;
struct Source {QString id;QJsonObject entity;TopoDS_Edge edge;};
struct Segment {int source;QString lineage,start,end;int direction;QJsonObject detail;};
QJsonArray array(const QStringList& values){QJsonArray result;for(const auto& value:values)result.append(value);return result;}
QString rotation(QStringList values){QString best;for(int i=0;i<values.size();++i){QStringList rotated;for(int j=0;j<values.size();++j)rotated.append(values[(i+j)%values.size()]);const auto key=rotated.join('\n');if(best.isEmpty()||key<best)best=key;}return best;}
QMap<QString,QString> endpointEquivalence(const QJsonObject& parameters){
    QMap<QString,QString> representative;for(const auto& value:parameters["points"].toArray()){const auto id=value.toObject()["id"].toString();representative[id]=id;}
    for(const auto& value:parameters["constraints"].toArray()){const auto relation=value.toObject();const auto ids=relation["pointIds"].toArray();if(relation["type"]!="coincident" || ids.size()!=2)continue;const auto a=representative.value(ids[0].toString()),b=representative.value(ids[1].toString());if(a.isEmpty() || b.isEmpty())continue;const auto keep=std::min(a,b),remove=std::max(a,b);for(auto i=representative.begin();i!=representative.end();++i)if(i.value()==remove)i.value()=keep;}
    return representative;
}
// Diagnostics describe discovery; they never alter source geometry or identities.
SketchProfiles::RegionsResult enrich(const QJsonObject& feature,SketchProfiles::RegionsResult result) {
    const auto parameters=feature["parameters"].toObject();QMap<QString,QJsonObject> entities,points;
    for(const auto& p:parameters["points"].toArray())points[p.toObject()["id"].toString()]=p.toObject();
    for(const auto& e:parameters["entities"].toArray())if(!e.toObject()["construction"].toBool() && e.toObject()["type"]!="point")entities[e.toObject()["id"].toString()]=e.toObject();
    QSet<QString> used;for(auto& profile:result.profiles){for(const auto& id:profile.summary["sourceEntityIds"].toArray())used.insert(id.toString());int loops=0;for(TopExp_Explorer wire(profile.face,TopAbs_WIRE);wire.More();wire.Next())++loops;profile.summary["outerLoopCount"]=1;profile.summary["innerLoopCount"]=std::max(0,loops-1);}
    const auto representative=endpointEquivalence(parameters);
    QStringList ignored,guides;QMap<QString,int> degree;QMap<QString,QStringList> incident;
    for(auto e=entities.begin();e!=entities.end();++e){if(used.contains(e.key()))continue;const auto entity=e.value();if(entity["centerline"].toBool()){guides.append(e.key());continue;}ignored.append(e.key());if(entity["type"]=="line" || entity["type"]=="arc" || entity["type"]=="fitSpline" || entity["type"]=="controlSpline" || entity["type"]=="conic")for(const auto& key:{"startPointId","endPointId"}){const auto id=entity[key].toString();if(points.contains(id)){++degree[representative[id]];incident[representative[id]].append(e.key());}}}
    QJsonArray openEndpoints;QStringList openIds;for(auto i=degree.begin();i!=degree.end();++i)if(i.value()==1){openIds.append(i.key());const auto p=points[i.key()];openEndpoints.append(QJsonObject{{"pointId",i.key()},{"entityIds",array(incident[i.key()])},{"uMm",p["uMm"]},{"vMm",p["vMm"]}});}
    // Report nearest open endpoint pairs without inventing a closure threshold.
    QJsonArray gaps;QSet<QString> pairs;for(const auto& id:openIds){const auto a=points[id];double nearest=std::numeric_limits<double>::infinity();QString other;for(const auto& candidate:openIds){if(candidate==id)continue;const auto b=points[candidate];const double distance=std::hypot(a["uMm"].toDouble()-b["uMm"].toDouble(),a["vMm"].toDouble()-b["vMm"].toDouble());if(distance<nearest){nearest=distance;other=candidate;}}if(other.isEmpty())continue;QStringList ids{id,other};ids.sort();const auto key=ids.join('/');if(pairs.contains(key))continue;pairs.insert(key);gaps.append(QJsonObject{{"pointIds",array(ids)},{"distanceMm",nearest},{"closureApplied",false}});}
    auto diagnostics=result.diagnostics;diagnostics["ignoredEntityIds"]=array(ignored);diagnostics["ignoredCenterlineIds"]=array(guides);diagnostics["openEndpoints"]=openEndpoints;diagnostics["endpointGaps"]=gaps;
    auto notices=diagnostics["notices"].toArray();if(result.ok && !ignored.isEmpty())notices.append(QJsonObject{{"code","ignored_open_entities"},{"severity","warning"},{"message","Some sketch entities do not bound a closed region. They remain authored and are excluded from the selectable cells."},{"entityIds",array(ignored)}});if(!guides.isEmpty())notices.append(QJsonObject{{"code","unused_centerline"},{"severity","info"},{"message","Detached Centerline geometry is retained but does not bound a closed region."},{"entityIds",array(guides)}});diagnostics["notices"]=notices;result.diagnostics=diagnostics;return result;
}

}

SketchProfiles::RegionsResult SketchProfiles::deriveRegions(const QJsonObject& feature, const QJsonObject& resolvedFrame) {
    // Keep proven legacy identities and diagnostics for a simple boundary.
    const auto legacy=derive(feature,resolvedFrame);
    if(legacy.ok)return enrich(feature,{true,{legacy.profile},legacy.diagnostics,{}});
    RegionsResult result;result.diagnostics=legacy.diagnostics;result.error=legacy.error;
    if(legacy.diagnostics["code"]=="invalid_sketch" || legacy.diagnostics["code"]=="empty_sketch")return result;
    // This failure already identifies a single connected boundary. Do not turn
    // its crossing into unintended cells through the fallback.
    if(legacy.diagnostics["code"]=="self_intersecting_profile")return enrich(feature,result);
    const auto sketchId=feature["id"].toString();
    const auto parameters=feature["parameters"].toObject();
    const auto planeName=parameters["plane"].toString();
    auto refuse=[&](const QString& code,const QString& error,QJsonObject details=QJsonObject{}){details["code"]=code;details["message"]=error;details["sketchId"]=sketchId;details["profileCount"]=0;return enrich(feature,RegionsResult{false,{},details,error});};
    try {
        std::vector<Source> sources;TopTools_ListOfShape input;
        QSet<QString> endpointIds;
        for(const auto& value:parameters["entities"].toArray()) {
            const auto entity=value.toObject();if(entity["construction"].toBool() || entity["type"]=="point")continue;
            const auto edge=SketchGeometry::analyticEdge(parameters,entity,resolvedFrame);
            sources.push_back({entity["id"].toString(),entity,edge});input.Append(edge);
            for(const auto& field:{"startPointId","endPointId"})if(entity[field].isString())endpointIds.insert(entity[field].toString());
        }
        // A single open analytic edge retains the legacy open-profile diagnostic.
        if(sources.size()<2)return enrich(feature,result);
        const auto frame=SketchGeometry::evaluationFrame(parameters,resolvedFrame);const auto o=frame["origin"].toArray(),u=frame["uAxis"].toArray(),v=frame["vAxis"].toArray(),n=frame["normal"].toArray();
        const gp_Pnt origin(o[0].toDouble(),o[1].toDouble(),o[2].toDouble());
        const gp_Vec uAxis(u[0].toDouble(),u[1].toDouble(),u[2].toDouble()),vAxis(v[0].toDouble(),v[1].toDouble(),v[2].toDouble());
        QMap<QString,gp_Pnt> points;
        for(const auto& value:parameters["points"].toArray()){const auto point=value.toObject();points[point["id"].toString()]=origin.Translated(uAxis*point["uMm"].toDouble()+vAxis*point["vMm"].toDouble());}
        const gp_Pln plane(gp_Ax3(origin,gp_Dir(n[0].toDouble(),n[1].toDouble(),n[2].toDouble()),gp_Dir(uAxis)));
        auto generating=BRepBuilderAPI_MakeFace(plane).Face();
        BOPAlgo_Builder splitter;splitter.SetToFillHistory(true);splitter.SetArguments(input);splitter.Perform();
        if(splitter.HasErrors())return refuse("region_split_failed","OCCT could not split the analytic sketch arrangement.");
        TopTools_IndexedMapOfShape edges;TopExp::MapShapes(splitter.Shape(),TopAbs_EDGE,edges);
        QJsonArray overlaps;QSet<QString> overlapSets;
        std::vector<int> sourceByEdge(edges.Extent()+1,-1);
        std::vector<QStringList> lineageByEdge(edges.Extent()+1);TopTools_ListOfShape halfedges;
        for(int i=1;i<=edges.Extent();++i) {
            for(int j=0;j<int(sources.size());++j){bool mapped=edges(i).IsSame(sources[j].edge);
                for(const auto& modified:splitter.Modified(sources[j].edge))mapped|=edges(i).IsSame(modified);
                if(!mapped)continue;
                if(sourceByEdge[i]!=-1 && (sources[j].entity["type"]!="line" || sources[sourceByEdge[i]].entity["type"]!="line"))
                    return refuse("ambiguous_region_provenance","Coincident curved sources require explicit authored repair.",{{"entityIds",QJsonArray{sources[sourceByEdge[i]].id,sources[j].id}}});
                lineageByEdge[i].append(sources[j].id);
                if(sourceByEdge[i]<0 || sources[j].id<sources[sourceByEdge[i]].id)sourceByEdge[i]=j;}
            lineageByEdge[i].sort();
            if(lineageByEdge[i].size()>1 && !overlapSets.contains(lineageByEdge[i].join('/'))){overlapSets.insert(lineageByEdge[i].join('/'));overlaps.append(QJsonObject{{"code","coincident_boundary_sources"},{"severity","warning"},{"message","Coincident straight sources share one boundary segment; no duplicate region is created."},{"entityIds",array(lineageByEdge[i])}});}
            if(sourceByEdge[i]<0)return refuse("unmapped_region_provenance","A split edge has no authored source.");
            auto edge=TopoDS::Edge(edges(i));BOPTools_AlgoTools2D::BuildPCurveForEdgeOnFace(edge,generating);
            halfedges.Append(edge.Oriented(TopAbs_FORWARD));halfedges.Append(edge.Oriented(TopAbs_REVERSED));
        }
        // Observe actual splitter vertex sharing before exposing a healed cell.
        // Shared authored points, transitive Coincident and declared tangency
        // authorize existing contacts; kernel tolerance alone does not.
        struct Endpoint {QString pointId,entityId;gp_Pnt position;double tolerance;std::vector<TopoDS_Vertex> descendants;};
        std::vector<Endpoint> endpoints;
        TopTools_IndexedMapOfShape outputVertices;TopExp::MapShapes(splitter.Shape(),TopAbs_VERTEX,outputVertices);
        for(const auto& source:sources){
            if(source.entity["type"]=="circle")continue;TopoDS_Vertex first,last;TopExp::Vertices(source.edge,first,last,true);
            for(int end=0;end<2;++end){const auto id=source.entity[end ? "endPointId":"startPointId"].toString();const auto vertex=end ? last:first;Endpoint endpoint{id,source.id,points[id],BRep_Tool::Tolerance(vertex),{}};
                if(outputVertices.Contains(vertex))endpoint.descendants.push_back(vertex);
                for(const auto& modified:splitter.Modified(vertex))if(modified.ShapeType()==TopAbs_VERTEX && outputVertices.Contains(modified))endpoint.descendants.push_back(TopoDS::Vertex(modified));
                endpoints.push_back(endpoint);
            }
        }
        const auto equivalence=endpointEquivalence(parameters);
        for(size_t i=0;i<endpoints.size();++i)for(size_t j=i+1;j<endpoints.size();++j){const auto& a=endpoints[i];const auto& b=endpoints[j];if(a.entityId==b.entityId || equivalence.value(a.pointId)==equivalence.value(b.pointId))continue;bool declaredContact=false;for(const auto& value:parameters["constraints"].toArray()){const auto relation=value.toObject();const auto ids=relation["entityIds"].toArray();if((relation["type"]=="tangent" || relation["type"]=="arcTangent") && ids.contains(a.entityId) && ids.contains(b.entityId))declaredContact=true;}if(declaredContact)continue;
            const double distance=a.position.Distance(b.position);const double scale=std::max({1.,std::abs(a.position.X()),std::abs(a.position.Y()),std::abs(a.position.Z()),std::abs(b.position.X()),std::abs(b.position.Y()),std::abs(b.position.Z())});const double roundoff=64*std::numeric_limits<double>::epsilon()*scale;if(distance<=roundoff)continue;
            bool merged=false;for(const auto& first:a.descendants)for(const auto& last:b.descendants)merged|=first.IsSame(last);if(merged)return refuse("undeclared_endpoint_merge","The kernel merged separated sketch endpoints. Close them explicitly with Coincident or correct the geometry; no healed profile is exposed.",{{"pointIds",QJsonArray{a.pointId,b.pointId}},{"entityIds",QJsonArray{a.entityId,b.entityId}},{"distanceMm",distance},{"kernelVertexToleranceMm",std::max(a.tolerance,b.tolerance)},{"numericRoundoffMm",roundoff},{"closureApplied",false}});
        }
        BOPAlgo_BuilderFace builder;builder.SetFace(generating);builder.SetShapes(halfedges);builder.Perform();
        if(builder.HasErrors())return refuse("region_faces_failed","OCCT could not construct bounded planar regions.");
        // OCCT supplies the arrangement. Name new intersections from incident
        // authored source sets and their ordered branch on one retained curve.
        // Include multiplicity so a collapsing/changing contact cannot alias an
        // earlier branch; numeric coordinates/parameters never enter the ID.
        QMap<QString,std::vector<std::pair<double,gp_Pnt>>> intersectionGroups;
        TopTools_IndexedMapOfShape vertices;TopExp::MapShapes(splitter.Shape(),TopAbs_VERTEX,vertices);
        for(int vertexIndex=1;vertexIndex<=vertices.Extent();++vertexIndex) {
            const auto vertex=TopoDS::Vertex(vertices(vertexIndex));const auto location=BRep_Tool::Pnt(vertex);QStringList incident;
            for(int i=1;i<=edges.Extent();++i) {
                TopoDS_Vertex first,last;TopExp::Vertices(TopoDS::Edge(edges(i)),first,last);
                if(location.Distance(BRep_Tool::Pnt(first))<=contactTolerance || location.Distance(BRep_Tool::Pnt(last))<=contactTolerance)
                    incident.append(lineageByEdge[i]);
            }
            incident.removeDuplicates();incident.sort();if(incident.size()<2)continue;
            const auto source=std::find_if(sources.begin(),sources.end(),[&](const auto& source){return source.id==incident[0];});
            double firstParameter,lastParameter;const auto curve=BRep_Tool::Curve(source->edge,firstParameter,lastParameter);
            GeomAPI_ProjectPointOnCurve projection(location,curve,firstParameter,lastParameter);
            if(projection.NbPoints()==0)return refuse("ambiguous_region_provenance","An intersection cannot be placed on its retained source curve.");
            auto& group=intersectionGroups[incident.join(',')];
            if(std::none_of(group.begin(),group.end(),[&](const auto& entry){return entry.second.Distance(location)<=contactTolerance;}))
                group.push_back({projection.LowerDistanceParameter(),location});
        }
        QMap<QString,std::vector<gp_Pnt>> intersectionLocations;
        for(auto group=intersectionGroups.begin();group!=intersectionGroups.end();++group) {
            auto values=group.value();std::sort(values.begin(),values.end(),[](const auto& a,const auto& b){return a.first<b.first;});
            for(int i=0;i<int(values.size());++i)intersectionLocations["intersection:"+group.key()+":"+QString::number(values.size())+":"+QString::number(i)].push_back(values[i].second);
        }
        // Coordinates locate authored references; only their stable IDs enter signatures.
        auto contact=[&](const gp_Pnt& location){QStringList ids;
            for(const auto& id:endpointIds)if(points[id].Distance(location)<=contactTolerance)ids.append(id);
            if(!ids.isEmpty()){ids.sort();return QString("point:")+ids.join(',');}
            QStringList tangentIds;
            for(const auto& value:parameters["constraints"].toArray()){const auto relation=value.toObject();if(relation["type"]!="tangent")continue;
                const auto refs=relation["entityIds"].toArray();const Source* circle=nullptr;const Source* line=nullptr;
                for(const auto& source:sources){if(source.id==refs[0].toString())circle=&source;if(source.id==refs[1].toString())line=&source;}
                if(!circle||!line)continue;
                const auto center=points[circle->entity["centerPointId"].toString()],a=points[line->entity["startPointId"].toString()],b=points[line->entity["endPointId"].toString()];
                const gp_Vec delta(a,b),offset(a,center);const auto foot=a.Translated(delta*(offset.Dot(delta)/delta.SquareMagnitude()));
                if(foot.Distance(location)<=contactTolerance)tangentIds.append(relation["id"].toString());
            }
            tangentIds.sort();if(!tangentIds.isEmpty())return QString("tangent:")+tangentIds.join(',');
            QStringList contacts;for(auto entry=intersectionLocations.begin();entry!=intersectionLocations.end();++entry)
                for(const auto& point:entry.value())if(point.Distance(location)<=contactTolerance)contacts.append(entry.key());
            contacts.removeDuplicates();contacts.sort();return contacts.join('|');
        };
        std::vector<EvaluatedProfile> profiles;QSet<int> usedSegments;QSet<QString> signatures;
        for(const auto& shape:builder.Areas()) {
            auto face=TopoDS::Face(shape);
            // BuilderFace may retain an open guide as an INTERNAL wire. It is
            // not a planar region boundary and must not create an internal
            // prism face when the closed region is extruded.
            Handle(BRepTools_ReShape) boundaries=new BRepTools_ReShape;
            for(TopExp_Explorer wire(face,TopAbs_WIRE);wire.More();wire.Next()) {
                bool bounded=false;for(TopoDS_Iterator edge(wire.Current());edge.More();edge.Next())
                    bounded|=edge.Value().Orientation()==TopAbs_FORWARD || edge.Value().Orientation()==TopAbs_REVERSED;
                if(!bounded)boundaries->Remove(wire.Current());
            }
            face=TopoDS::Face(boundaries->Apply(face));
            if(BRepTopAdaptor_FClass2d(face,1e-7).PerformInfinitePoint()!=TopAbs_OUT)continue;
            if(!BRepCheck_Analyzer(face).IsValid())return refuse("invalid_region","OCCT constructed an invalid bounded face.");
            GProp_GProps properties;BRepGProp::SurfaceProperties(face,properties);const double area=properties.Mass();
            if(!std::isfinite(area)||area<=1e-8)return refuse("invalid_region","Bounded region has no usable positive area.");
            QStringList wires,sourceIds,pointIds;QJsonArray provenance;
            std::vector<EvaluatedBoundaryEdge> boundaryEdges;
            for(TopExp_Explorer wireIt(face,TopAbs_WIRE);wireIt.More();wireIt.Next()) {
                std::vector<Segment> segments;
                for(BRepTools_WireExplorer walk(TopoDS::Wire(wireIt.Current()),face);walk.More();walk.Next()) {
                    const auto edge=walk.Current();const int index=edges.FindIndex(edge);if(index>0)usedSegments.insert(index);
                    if(index==0)return refuse("unmapped_region_provenance","Face boundary contains an unmapped split edge.");
                    const int sourceIndex=sourceByEdge[index];const auto& source=sources[sourceIndex];
                    TopoDS_Vertex first,last;TopExp::Vertices(edge,first,last,true);
                    const auto a=contact(BRep_Tool::Pnt(first)),b=contact(BRep_Tool::Pnt(last));
                    int direction=edge.Orientation()==source.edge.Orientation()?1:-1;
                    if(source.entity["type"]=="line") {
                        TopoDS_Vertex sourceFirst,sourceLast;TopExp::Vertices(source.edge,sourceFirst,sourceLast,true);
                        direction=gp_Vec(BRep_Tool::Pnt(first),BRep_Tool::Pnt(last)).Dot(gp_Vec(BRep_Tool::Pnt(sourceFirst),BRep_Tool::Pnt(sourceLast)))>=0 ? 1 : -1;
                    }
                    BRepAdaptor_Curve curve(edge);
                    QJsonObject detail{{"entityId",source.id},{"sourceEntityIds",array(lineageByEdge[index])},{"type",source.entity["type"]},{"firstParameter",curve.FirstParameter()},
                        {"lastParameter",curve.LastParameter()},{"direction",direction},{"startContact",a},{"endContact",b}};
                    segments.push_back({sourceIndex,lineageByEdge[index].join(','),a,b,direction,detail});provenance.append(detail);sourceIds.append(lineageByEdge[index]);
                    boundaryEdges.push_back({sketchId,source.id,a,b,source.entity["type"].toString(),edge});
                }
                if(segments.empty())return refuse("invalid_region","A region wire is empty.");
                const bool oneSource=std::all_of(segments.begin(),segments.end(),[&](const auto& segment){return segment.source==segments[0].source;});
                if(oneSource){if(sources[segments[0].source].entity["type"]!="circle")return refuse("ambiguous_region_provenance","A closed single-source wire is not a circle.");
                    wires.append("full-circle:"+sources[segments[0].source].id);continue;}
                // Start at a source transition, so the circle seam never enters identity.
                size_t start=0;while(segments[start].lineage==segments[(start+segments.size()-1)%segments.size()].lineage)++start;
                QStringList forward,reverse;
                for(size_t offset=0;offset<segments.size();) {
                    const auto& first=segments[(start+offset)%segments.size()];auto last=first;++offset;
                    while(offset<segments.size() && segments[(start+offset)%segments.size()].lineage==first.lineage){
                        const auto& next=segments[(start+offset)%segments.size()];if(next.direction!=first.direction)return refuse("ambiguous_region_provenance","Same-source boundary reverses unexpectedly.");last=next;++offset;}
                    if(first.start.isEmpty()||last.end.isEmpty())return refuse("ambiguous_region_provenance","A region contact lacks a retained authored reference.");
                    const auto source=first.lineage;
                    forward.append(source+":"+first.start+":"+last.end+":"+QString::number(first.direction));
                    reverse.prepend(source+":"+last.end+":"+first.start+":"+QString::number(-first.direction));
                }
                wires.append(std::min(rotation(forward),rotation(reverse)));
            }
            wires.sort();const auto signature=wires.join("\nwire\n");
            if(signatures.contains(signature))return refuse("ambiguous_region_provenance","Two bounded regions have the same authored boundary signature.");signatures.insert(signature);
            sourceIds.removeDuplicates();sourceIds.sort();
            for(const auto& id:endpointIds)for(const auto& detail:provenance){const auto entry=detail.toObject();if(entry["startContact"].toString().contains(id)||entry["endContact"].toString().contains(id)){pointIds.append(id);break;}}
            pointIds.removeDuplicates();pointIds.sort();
            const auto profileId=QUuid::createUuidV5(QUuid(sketchId),QByteArray("alloy-regions/1\n")+signature.toUtf8()).toString(QUuid::WithoutBraces);
            QJsonObject summary{{"profileId",profileId},{"sketchId",sketchId},{"plane",planeName},{"frame",frame},{"kernel","OCCT"},{"valid",true},
                {"type","planar_region"},{"areaUnit","mm2"},{"areaMm2",area},{"sourceEntityIds",array(sourceIds)},
                {"sourcePointIds",array(pointIds)},{"edgeProvenance",provenance},{"boundarySignature",signature}};
            profiles.push_back({profileId,sketchId,planeName,face,summary,boundaryEdges});
        }
        if(profiles.empty())return enrich(feature,result);
        QJsonArray ignoredSegments;QSet<QString> boundedSources;for(const auto& profile:profiles)for(const auto& id:profile.summary["sourceEntityIds"].toArray())boundedSources.insert(id.toString());
        for(int i=1;i<=edges.Extent();++i)if(!usedSegments.contains(i)){BRepAdaptor_Curve curve(TopoDS::Edge(edges(i)));ignoredSegments.append(QJsonObject{{"sourceEntityIds",array(lineageByEdge[i])},{"firstParameter",curve.FirstParameter()},{"lastParameter",curve.LastParameter()}});bool partlyUsed=false;for(const auto& id:lineageByEdge[i])partlyUsed|=boundedSources.contains(id);if(partlyUsed)overlaps.append(QJsonObject{{"code","ignored_open_segments"},{"severity","warning"},{"message","Open portions of source curves are excluded from the bounded cells."},{"entityIds",array(lineageByEdge[i])}});}
        std::sort(profiles.begin(),profiles.end(),[](const auto& a,const auto& b){return a.profileId<b.profileId;});
        return enrich(feature,{true,profiles,{{"code","planar_regions"},{"profileCount",int(profiles.size())},{"sketchId",sketchId},{"provenance","authored_contacts"},{"notices",overlaps},{"ignoredSegments",ignoredSegments}}, {}});
    }catch(const Standard_Failure& error){return refuse("kernel_failure",QString::fromUtf8(error.GetMessageString()));}
    catch(const std::exception& error){return refuse("region_failure",QString::fromUtf8(error.what()));}
}
