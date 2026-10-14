// SPDX-License-Identifier: MIT
#include "FeaturePattern.h"
#include "SolidFillet.h"
#include "PlanarFaces.h"
#include "SolidHole.h"
#include <BRepAdaptor_Curve.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <TopoDS.hxx>
#include <QUuid>
#include <algorithm>
#include <cmath>

namespace {
[[noreturn]] void holeFailure(const QString& code,const QString& message) {
    throw GeometryKernel::EvaluationError({{"code",code},{"message",message}});
}
gp_Pnt point(const QJsonArray& values) {
    if(values.size()!=3 || !values[0].isDouble() || !values[1].isDouble() || !values[2].isDouble())
        holeFailure("pattern_point_unavailable","The retained Hole point has no world placement");
    return {values[0].toDouble(),values[1].toDouble(),values[2].toDouble()};
}
void evaluateHole(EvaluatedBody& target,const QJsonObject& feature,const QJsonObject& seed,const EvaluatedDocument& prior) {
    const auto p=feature["parameters"].toObject(),source=seed["parameters"].toObject();
    if(source["targetBodyId"]!=target.bodyId || p["targetBodyId"]!=target.bodyId
        || p["distribution"]!="spacing" || p["computeType"]!="adjust")
        holeFailure("unsupported_pattern_seed","Hole Pattern requires its retained target, Spacing and Adjust");
    const EvaluatedSketch* sketch=nullptr;
    for(const auto& item:prior.sketches)if(item.featureId==source["sourceSketchId"].toString()){sketch=&item;break;}
    if(!sketch)holeFailure("pattern_point_unavailable","Hole seed sketch is unavailable");
    QJsonObject retained;
    for(const auto& value:sketch->summary["standalonePoints"].toArray()) {
        const auto item=value.toObject();
        if(item["id"]==source["sourceEntityId"] && item["pointId"]==source["sourcePointId"]){retained=item;break;}
    }
    if(retained.isEmpty())holeFailure("pattern_point_unavailable","Hole seed point identity is unavailable");
    const auto entry=point(retained["positionMm"].toArray());
    const auto normal=sketch->resolvedFrame["normal"].toArray();
    if(normal.size()!=3)holeFailure("pattern_support_unavailable","Hole seed support frame is unavailable");
    const gp_Dir outward(normal[0].toDouble(),normal[1].toDouble(),normal[2].toDouble());
    std::vector<gp_Vec> directions;std::vector<int> quantities;std::vector<double> spacings;QJsonArray resolvedAxes;
    for(const auto& value:p["axes"].toArray()) {
        auto axis=value.toObject();
        const auto edge=std::find_if(target.solidEdges.begin(),target.solidEdges.end(),[&](const auto& item){return item.edgeId==axis["edgeId"].toString();});
        if(edge==target.solidEdges.end() || edge->edge.IsNull())holeFailure("pattern_axis_unavailable","Select a current retained straight model edge");
        BRepAdaptor_Curve curve(edge->edge);if(curve.GetType()!=GeomAbs_Line)holeFailure("unsupported_pattern_axis","Hole Pattern directions require straight edges");
        const auto start=edge->provenance["startMm"].toArray(),end=edge->provenance["endMm"].toArray();
        const auto a=point(start),b=point(end),first=curve.Value(curve.FirstParameter()),last=curve.Value(curve.LastParameter());
        if(!((a.Distance(first)<1e-6 && b.Distance(last)<1e-6) || (a.Distance(last)<1e-6 && b.Distance(first)<1e-6)))
            holeFailure("pattern_axis_unavailable","Oriented edge endpoints do not match the current kernel edge");
        gp_Vec direction(a,b);if(direction.Magnitude()<1e-6)holeFailure("pattern_axis_unavailable","Pattern axis is degenerate");direction.Normalize();
        if(!directions.empty() && std::abs(direction.Dot(directions.front()))>1e-8)
            holeFailure("unsupported_pattern_axes","Hole Pattern requires perpendicular directions");
        directions.push_back(direction);quantities.push_back(axis["quantity"].toInt());spacings.push_back(axis["spacingMm"].toDouble());
        axis["orientedStartMm"]=start;axis["orientedEndMm"]=end;
        axis["directionMode"]="one";axis["direction"]=QJsonArray{direction.X(),direction.Y(),direction.Z()};
        axis["totalSpanMm"]=axis["spacingMm"].toDouble()*(axis["quantity"].toInt()-1);
        resolvedAxes.append(axis);
    }
    if(directions.size()!=2)holeFailure("pattern_axis_required","Hole Pattern requires two directions");
    const double sink=source["sinkDiameterMm"].toDouble();
    if(std::abs(spacings[0])+1e-7<sink || std::abs(spacings[1])+1e-7<sink)
        holeFailure("pattern_overlapping_openings","This Hole Pattern requires separated countersink openings");
    const int q1=quantities[0],q2=quantities[1];
    std::vector<TopoDS_Shape> tools;QJsonArray instances;const auto id=feature["id"].toString();
    for(int j=0;j<q2;++j)for(int i=0;i<q1;++i) {
        const gp_Vec translation=directions[0]*(spacings[0]*i)+directions[1]*(spacings[1]*j);
        const auto center=entry.Translated(translation);
        const auto instanceId=QUuid::createUuidV5(QUuid(id),QByteArray("alloy-pattern-instance/1/")+QByteArray::number(i)+"/"+QByteArray::number(j)).toString(QUuid::WithoutBraces);
        instances.append(QJsonObject{{"instanceId",instanceId},{"index1",i},{"index2",j},
            {"translationMm",QJsonArray{translation.X(),translation.Y(),translation.Z()}},
            {"centerMm",QJsonArray{center.X(),center.Y(),center.Z()}},{"seed",i==0 && j==0}});
        if(i==0 && j==0)continue; // The retained Hole already cut the seed.
        tools.push_back(SolidHole::tool(target.shape,center,outward,source["boreDiameterMm"].toDouble(),
            source["sinkDiameterMm"].toDouble(),source["includedAngleDeg"].toDouble()));
    }
    if(tools.size()!=size_t(q1*q2-1))holeFailure("pattern_instance_unavailable","Hole occurrence generation is incomplete");
    const auto before=GeometryKernel::summarize(target.shape)["volumeMm3"].toDouble();
    auto faces=target.supportFaces;auto edges=target.solidEdges;
    const auto result=GeometryKernel::booleanOperation("cut",target.shape,tools,&faces,&edges);
    auto summary=GeometryKernel::summarize(result);
    const auto removed=before-summary["volumeMm3"].toDouble();
    if(!std::isfinite(removed) || removed<=1e-7)holeFailure("pattern_no_effect","Hole Pattern removed no target material");
    summary["operation"]="rectangularPattern";summary["seedFeatureId"]=seed["id"];
    summary["lastModifierFeatureId"]=id;summary["instances"]=instances;
    summary["instanceCount"]=q1*q2;summary["seedCenterMm"]=retained["positionMm"];
    summary["patternAxes"]=resolvedAxes;summary["computeType"]="Adjust";summary["distribution"]="Spacing";
    summary["removedVolumeMm3"]=removed;summary["supportFaces"]=PlanarFaces::descriptors(faces);
    summary["solidEdges"]=SolidFillet::descriptors(edges);
    target.shape=result;target.summary=summary;target.supportFaces=std::move(faces);target.solidEdges=std::move(edges);
}
}

void FeaturePattern::evaluate(EvaluatedBody& target,const QJsonObject& feature,const QJsonObject& seed,const EvaluatedDocument& prior) {
    if(seed["type"]=="hole")return evaluateHole(target,feature,seed,prior);
    auto reject=[](const QString& code,const QString& message){throw GeometryKernel::EvaluationError({{"code",code},{"message",message}});};
    const auto p=feature["parameters"].toObject(),source=seed["parameters"].toObject();
    if(seed["type"]!="extrude" || source["operation"]!="join" || source["targetBodyIds"].toArray()!=QJsonArray{target.bodyId})reject("unsupported_pattern_seed","Pattern supports one-target additive Extrude Join features");
    QJsonArray ids=source["profileIds"].toArray();if(source.contains("profileId"))ids=QJsonArray{source["profileId"]};
    if(ids.size()!=1)reject("unsupported_pattern_seed","Pattern requires one connected additive profile");
    const EvaluatedProfile* profile=nullptr;for(const auto& sketch:prior.sketches)if(sketch.featureId==source["sketchId"].toString())for(const auto& region:sketch.profiles)if(region.profileId==ids[0].toString())profile=&region;
    if(!profile)reject("pattern_profile_unavailable","The retained seed profile is unavailable");
    std::vector<gp_Vec> directions;std::vector<int> quantities;std::vector<double> extents;
    for(const auto& value:p["axes"].toArray()){
        const auto axis=value.toObject();const auto edge=std::find_if(target.solidEdges.begin(),target.solidEdges.end(),[&](const auto& candidate){return candidate.edgeId==axis["edgeId"].toString();});
        if(edge==target.solidEdges.end() || edge->edge.IsNull())reject("pattern_axis_unavailable","Choose a current retained straight model edge");
        BRepAdaptor_Curve curve(edge->edge);if(curve.GetType()!=GeomAbs_Line)reject("unsupported_pattern_axis","Pattern axes must be straight model edges");
        auto direction=gp_Vec(curve.Value(curve.FirstParameter()),curve.Value(curve.LastParameter()));if(direction.Magnitude()<1e-6)reject("pattern_axis_unavailable","The selected axis edge is degenerate");direction.Normalize();
        if(!directions.empty() && std::abs(direction.Dot(directions.front()))>1e-8)reject("unsupported_pattern_axes","This pattern supports perpendicular axes");
        directions.push_back(direction);quantities.push_back(axis["quantity"].toInt());extents.push_back(axis["extentMm"].toDouble());
    }
    if(directions.empty())reject("pattern_axis_required","Choose one or two axis edges");
    const int q1=quantities[0],q2=directions.size()==2 ? quantities[1] : 1;
    if(q1*q2>1000)reject("pattern_instance_limit","This pattern supports at most 1000 total instances");
    GProp_GProps properties;BRepGProp::SurfaceProperties(profile->face,properties);auto seedCenter=properties.CentreOfMass();const auto normal=profile->summary["frame"].toObject()["normal"].toArray();seedCenter.Translate(gp_Vec(normal[0].toDouble(),normal[1].toDouble(),normal[2].toDouble())*(source["startOffsetMm"].toDouble()+source["distanceMm"].toDouble()/2));
    QJsonArray resolvedAxes;const auto authoredAxes=p["axes"].toArray();for(size_t i=0;i<directions.size();++i){auto axis=authoredAxes[int(i)].toObject();axis["direction"]=QJsonArray{directions[i].X(),directions[i].Y(),directions[i].Z()};resolvedAxes.append(axis);}
    std::vector<TopoDS_Shape> tools;QJsonArray instances;const auto id=feature["id"].toString();
    const auto volumeBefore=target.summary["volumeMm3"].toDouble();
    for(int j=0;j<q2;++j)for(int i=0;i<q1;++i){
        gp_Vec translation=directions[0]*(extents[0]*i/(q1-1));if(directions.size()==2)translation+=directions[1]*(extents[1]*j/(q2-1));
        const auto instanceId=QUuid::createUuidV5(QUuid(id),QByteArray("alloy-pattern-instance/1/")+QByteArray::number(i)+"/"+QByteArray::number(j)).toString(QUuid::WithoutBraces);
        instances.append(QJsonObject{{"instanceId",instanceId},{"index1",i},{"index2",j},{"translationMm",QJsonArray{translation.X(),translation.Y(),translation.Z()}},{"centerMm",QJsonArray{seedCenter.X()+translation.X(),seedCenter.Y()+translation.Y(),seedCenter.Z()+translation.Z()}},{"seed",i==0 && j==0}});
        if(i==0 && j==0)continue;
        EvaluatedSupportFace cap;std::vector<EvaluatedSolidEdge> edges;
        const auto prism=GeometryKernel::extrusion(*profile,source["distanceMm"].toDouble(),&cap,&edges,instanceId,nullptr,source["startOffsetMm"].toDouble());
        gp_Trsf transform;transform.SetTranslation(translation);BRepBuilderAPI_Transform moved(prism,transform,true);
        if(!moved.IsDone())reject("pattern_transform_failed","Pattern instance translation failed");tools.push_back(moved.Shape());
        for(auto& edge:edges){const auto transformed=moved.ModifiedShape(edge.edge);if(transformed.IsNull() || transformed.ShapeType()!=TopAbs_EDGE)reject("pattern_edge_unavailable","Instance edge provenance is unavailable");edge.edge=TopoDS::Edge(transformed);edge.provenance["patternFeatureId"]=id;edge.provenance["seedFeatureId"]=seed["id"];edge.provenance["instanceId"]=instanceId;edge.provenance["occurrenceId"]=instanceId;edge.provenance["index1"]=i;edge.provenance["index2"]=j;target.solidEdges.push_back(edge);}
        if(!cap.face.IsNull()){
            const auto transformed=moved.ModifiedShape(cap.face);if(transformed.IsNull() || transformed.ShapeType()!=TopAbs_FACE)reject("pattern_cap_unavailable","Instance cap provenance is unavailable");cap.face=TopoDS::Face(transformed);cap.bodyId=target.bodyId;cap.producerFeatureId=id;cap.faceId=QUuid::createUuidV5(QUuid(instanceId),QByteArray("alloy-pattern-cap/1")).toString(QUuid::WithoutBraces);
            auto origin=cap.frame["origin"].toArray();origin[0]=origin[0].toDouble()+translation.X();origin[1]=origin[1].toDouble()+translation.Y();origin[2]=origin[2].toDouble()+translation.Z();cap.frame["origin"]=origin;
            for(auto& edge:cap.boundaryEdges){
                const auto transformed=moved.ModifiedShape(edge.edge);if(transformed.IsNull() || transformed.ShapeType()!=TopAbs_EDGE)reject("pattern_boundary_unavailable","Instance boundary provenance is unavailable");edge.edge=TopoDS::Edge(transformed);
                edge.edgeId=QUuid::createUuidV5(QUuid(instanceId),QByteArray("alloy-pattern-boundary/1/")+edge.edgeId.toUtf8()).toString(QUuid::WithoutBraces);edge.provenance["occurrenceId"]=instanceId;
                for(const auto& key:{"startMm","endMm","centerMm"})if(edge.provenance[key].isArray()){auto point=edge.provenance[key].toArray();if(point.size()==3){point[0]=point[0].toDouble()+translation.X();point[1]=point[1].toDouble()+translation.Y();point[2]=point[2].toDouble()+translation.Z();edge.provenance[key]=point;}}
            }
            target.supportFaces.push_back(cap);
        }
    }
    if(tools.empty())reject("pattern_no_instances","Pattern needs at least one additional instance");
    target.shape=GeometryKernel::booleanOperation("join",target.shape,tools,&target.supportFaces,&target.solidEdges);auto summary=GeometryKernel::summarize(target.shape);
    if(!summary["valid"].toBool() || !summary["closed"].toBool() || summary["topology"].toObject()["solids"]!=1 || summary["volumeMm3"].toDouble()<=volumeBefore+1e-6)reject("pattern_invalid_result","Every instance must add material to one connected target solid");
    summary["operation"]="rectangularPattern";summary["seedFeatureId"]=seed["id"];summary["lastModifierFeatureId"]=id;summary["instances"]=instances;summary["instanceCount"]=q1*q2;summary["seedCenterMm"]=QJsonArray{seedCenter.X(),seedCenter.Y(),seedCenter.Z()};summary["patternAxes"]=resolvedAxes;summary["computeType"]="Identical";summary["distribution"]="Extent";summary["supportFaces"]=PlanarFaces::descriptors(target.supportFaces);summary["solidEdges"]=SolidFillet::descriptors(target.solidEdges);target.summary=summary;
}
