// SPDX-License-Identifier: MIT
#include "FeaturePattern.h"
#include "SolidFillet.h"
#include "PlanarFaces.h"
#include <BRepAdaptor_Curve.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <TopoDS.hxx>
#include <QUuid>
#include <algorithm>
#include <cmath>

void FeaturePattern::evaluate(EvaluatedBody& target,const QJsonObject& feature,const QJsonObject& seed,const EvaluatedDocument& prior) {
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
    GProp_GProps properties;BRepGProp::SurfaceProperties(profile->face,properties);auto seedCenter=properties.CentreOfMass();const auto normal=profile->summary["frame"].toObject()["normal"].toArray();seedCenter.Translate(gp_Vec(normal[0].toDouble(),normal[1].toDouble(),normal[2].toDouble())*(source["distanceMm"].toDouble()/2));
    QJsonArray resolvedAxes;const auto authoredAxes=p["axes"].toArray();for(size_t i=0;i<directions.size();++i){auto axis=authoredAxes[int(i)].toObject();axis["direction"]=QJsonArray{directions[i].X(),directions[i].Y(),directions[i].Z()};resolvedAxes.append(axis);}
    std::vector<TopoDS_Shape> tools;QJsonArray instances;const auto id=feature["id"].toString();
    const auto volumeBefore=target.summary["volumeMm3"].toDouble();
    for(int j=0;j<q2;++j)for(int i=0;i<q1;++i){
        gp_Vec translation=directions[0]*(extents[0]*i/(q1-1));if(directions.size()==2)translation+=directions[1]*(extents[1]*j/(q2-1));
        const auto instanceId=QUuid::createUuidV5(QUuid(id),QByteArray("alloy-pattern-instance/1/")+QByteArray::number(i)+"/"+QByteArray::number(j)).toString(QUuid::WithoutBraces);
        instances.append(QJsonObject{{"instanceId",instanceId},{"index1",i},{"index2",j},{"translationMm",QJsonArray{translation.X(),translation.Y(),translation.Z()}},{"centerMm",QJsonArray{seedCenter.X()+translation.X(),seedCenter.Y()+translation.Y(),seedCenter.Z()+translation.Z()}},{"seed",i==0 && j==0}});
        if(i==0 && j==0)continue;
        EvaluatedSupportFace cap;std::vector<EvaluatedSolidEdge> edges;
        const auto prism=GeometryKernel::extrusion(*profile,source["distanceMm"].toDouble(),&cap,&edges,instanceId);
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
