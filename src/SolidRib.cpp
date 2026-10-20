// SPDX-License-Identifier: MIT
#include "SolidRib.h"
#include "PlanarFaces.h"
#include <BRepAdaptor_Surface.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepAlgoAPI_Common.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <gp_Pln.hxx>
#include <algorithm>
#include <cmath>

namespace {
double mass(const TopoDS_Shape& s,bool area=false){GProp_GProps props;if(area)BRepGProp::SurfaceProperties(s,props);else BRepGProp::VolumeProperties(s,props);return std::abs(props.Mass());}
TopoDS_Shape common(const TopoDS_Shape& a,const TopoDS_Shape& b){BRepAlgoAPI_Common op;TopTools_ListOfShape args,tools;args.Append(a);tools.Append(b);op.SetArguments(args);op.SetTools(tools);op.SetNonDestructive(true);op.SetRunParallel(false);op.Build();if(!op.IsDone() || op.HasErrors())throw GeometryKernel::EvaluationError({{"code","rib_certificate_failed"},{"message","Could not certify the entire Rib section and span"}});return op.Shape();}
}
void SolidRib::evaluate(const QJsonObject& feature,EvaluatedDocument& prior){
 const auto p=feature["parameters"].toObject();const auto id=feature["id"].toString();
 auto reject=[&](const QString& code,const QString& message){throw GeometryKernel::EvaluationError({{"code",code},{"message",message},{"featureId",id},{"featureType","rib"},{"sketchId",p["sketchId"]},{"sourceEntityId",p["sourceEntityId"]},{"targetBodyId",p["targetBodyId"]}});};
 auto target=std::find_if(prior.bodies.begin(),prior.bodies.end(),[&](const auto& b){return b.bodyId==p["targetBodyId"];});
 const auto sketch=std::find_if(prior.sketches.begin(),prior.sketches.end(),[&](const auto& s){return s.featureId==p["sketchId"];});
 if(target==prior.bodies.end() || sketch==prior.sketches.end())reject("rib_source_unavailable","Select one earlier retained sketch Line and active solid target");
 if((!target->ownerId.isEmpty() && target->ownerId!=feature["ownerId"]) || (!sketch->ownerId.isEmpty() && sketch->ownerId!=feature["ownerId"]) || !target->occurrenceId.isEmpty())reject("unsupported_instance_edit","Rib requires a sketch and solid in the active component");
 QJsonObject entity;for(const auto& q:sketch->summary["entities"].toArray())if(q.toObject()["id"]==p["sourceEntityId"])entity=q.toObject();
 if(entity["type"]!="line" || entity["construction"].toBool())reject("rib_profile_unsupported","Rib requires one nonconstruction straight open sketch Line");
 for(const auto& profile:sketch->profiles)if(profile.summary["sourceEntityIds"].toArray().contains(p["sourceEntityId"]))reject("rib_profile_unsupported","Select an open Line outside a closed sketch profile");
 QMap<QString,gp_Pnt> points;for(const auto& q:sketch->summary["worldPoints"].toArray()){const auto point=q.toObject();const auto xyz=point["positionMm"].toArray();if(xyz.size()==3)points[point["id"].toString()]={xyz[0].toDouble(),xyz[1].toDouble(),xyz[2].toDouble()};}
 if(!points.contains(entity["startPointId"].toString()) || !points.contains(entity["endPointId"].toString()))reject("rib_source_unavailable","Rib Line endpoints are unavailable");
 const auto a=points.value(entity["startPointId"].toString()),b=points.value(entity["endPointId"].toString());gp_Vec tangent(a,b);const double length=tangent.Magnitude();
 const auto n=sketch->resolvedFrame["normal"].toArray();if(length<1e-6 || n.size()!=3)reject("rib_profile_invalid","Rib Line or sketch plane is degenerate");
 tangent.Normalize();gp_Vec normal(n[0].toDouble(),n[1].toDouble(),n[2].toDouble());normal.Normalize();auto span=normal.Crossed(tangent);if(span.Magnitude()<1e-8)reject("rib_profile_invalid","Rib Line is perpendicular to its sketch plane");span.Normalize();if(p["flip"].toBool())span.Reverse();
 const double t=p["thicknessMm"].toDouble(),area=length*t,tolerance=std::max(1e-6,area*1e-8);
 auto section=[&](double d){const auto aa=a.Translated(span*d),bb=b.Translated(span*d);BRepBuilderAPI_MakePolygon wire;wire.Add(aa.Translated(normal*(-t/2)));wire.Add(bb.Translated(normal*(-t/2)));wire.Add(bb.Translated(normal*(t/2)));wire.Add(aa.Translated(normal*(t/2)));wire.Close();return BRepBuilderAPI_MakeFace(wire.Wire()).Face();};
 auto prism=[&](double lo,double hi){return BRepPrimAPI_MakePrism(section(lo),span*(hi-lo),true).Shape();};
 struct Wall{double distance;const EvaluatedSupportFace* face;double orientation;};std::vector<Wall> walls;
 // Find only exact retained planar faces. Full face/section Common proves
 // complete coverage, including holes; a midpoint ray cannot establish this.
 for(const auto& face:target->supportFaces){if(face.face.IsNull())continue;BRepAdaptor_Surface surface(face.face);if(surface.GetType()!=GeomAbs_Plane)continue;auto axis=gp_Vec(surface.Plane().Axis().Direction());if(face.face.Orientation()==TopAbs_REVERSED)axis.Reverse();const double dot=axis.Dot(span);if(std::abs(dot)<1-1e-9)continue;const double distance=gp_Vec(a,surface.Plane().Location()).Dot(span);if(distance<=1e-6)continue;const double covered=mass(common(section(distance),face.face),true);if(std::abs(covered-area)<=tolerance)walls.push_back({distance,&face,dot});}
 std::sort(walls.begin(),walls.end(),[](const auto& x,const auto& y){return x.distance<y.distance;});
 const Wall* exit=nullptr;const Wall* stop=nullptr;
 for(const auto& wall:walls){if(!exit && wall.orientation>0)exit=&wall;else if(exit && wall.distance>exit->distance+1e-6 && wall.orientation<0){stop=&wall;break;}}
 if(!exit || !stop)reject("rib_support_unavailable","To Next requires a source wall and a unique full-cover planar stop perpendicular to Rib span");
 for(const auto& wall:walls)if((&wall!=exit && std::abs(wall.distance-exit->distance)<1e-6) || (&wall!=stop && std::abs(wall.distance-stop->distance)<1e-6))reject("rib_support_ambiguous","More than one retained face resolves a Rib support");
 const QJsonArray supports{exit->face->faceId,stop->face->faceId};if(!p["supportFaceIds"].toArray().isEmpty() && p["supportFaceIds"]!=supports)reject("rib_support_changed","The retained Rib support changed; edit the Profile or Target to resolve a new support");
 const double overlap=mass(common(prism(0,exit->distance),target->shape));
 if(std::abs(overlap-area*exit->distance)>std::max(1e-5,area*exit->distance*1e-8))reject("rib_source_partial","The entire Rib section must start inside one source wall and reach its planar exit without gaps");
 const double obstruction=mass(common(prism(exit->distance,stop->distance),target->shape));
 if(obstruction>std::max(1e-5,area*(stop->distance-exit->distance)*1e-8))reject("rib_span_obstructed","Rib span contains a partial or unsupported intervening wall; select an unobstructed full-cover planar stop");
 const double stopDistance=stop->distance,exitDistance=exit->distance;
 auto joined=GeometryKernel::booleanOperation("join",target->shape,{prism(0,stop->distance)},&target->supportFaces,&target->solidEdges);
 target->shape=joined;target->summary=GeometryKernel::summarize(joined);target->summary["lastModifierFeatureId"]=id;target->summary["producerFeatureId"]=target->featureId;target->summary["operation"]="rib";target->summary["ribSpanMm"]=stopDistance;target->summary["ribSourceExitMm"]=exitDistance;target->summary["ribSupportFaceIds"]=supports;target->summary["thicknessMm"]=t;target->summary["flip"]=p["flip"];target->summary["supportFaces"]=PlanarFaces::descriptors(target->supportFaces);
}
