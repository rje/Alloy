// SPDX-License-Identifier: MIT
#include "FeaturePattern.h"
#include "SolidFillet.h"
#include "SolidChamfer.h"
#include "PlanarFaces.h"
#include "SolidHole.h"
#include <BRepAdaptor_Curve.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <gp_Ax2.hxx>
#include <GeomAbs_CurveType.hxx>
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

void FeaturePattern::mirrorCuts(EvaluatedBody& target,const QJsonObject& feature,const EvaluatedDocument& prior) {
    const auto parameters=feature["parameters"].toObject();
    auto fail=[&](const QString& code,const QString& message){throw GeometryKernel::EvaluationError({
        {"code",code},{"message",message},{"featureId",feature["id"]},{"targetBodyId",target.bodyId}});};
    const auto plane=parameters["mirrorPlane"].toObject();
    const auto axis=plane["plane"].toString();
    if(plane["kind"]!="origin" || (axis!="XZ" && axis!="YZ"))
        fail("mirror_plane_unsupported","Feature Mirror requires origin XZ or YZ");
    gp_Trsf reflection;reflection.SetMirror(gp_Ax2(gp_Pnt(0,0,0),axis=="YZ" ? gp_Dir(1,0,0) : gp_Dir(0,1,0)));
    const auto originalVolume=GeometryKernel::summarize(target.shape)["volumeMm3"].toDouble();
    QJsonArray instances;
    for(const auto& value:parameters["sourceFeatureIds"].toArray()) {
        const auto sourceId=value.toString();
        if(!prior.cutFeatureTools.contains(sourceId) || prior.cutFeatureTools[sourceId].empty())
            fail("mirror_source_unavailable","The earlier Cut feature has no retained source-stage tool");
        std::vector<TopoDS_Shape> reflected;
        std::vector<EvaluatedSolidEdge> reflectedEdges;
        const auto& sourceTools=prior.cutFeatureTools[sourceId];
        const auto sourceEdges=prior.cutFeatureEdges.value(sourceId);
        for(const auto& sourceTool:sourceTools) {
            BRepBuilderAPI_Transform transform(sourceTool,reflection,true);
            if(!transform.IsDone() || transform.Shape().IsNull())fail("mirror_transform_failed","Cut tool reflection failed");
            reflected.push_back(transform.Shape());
            for(auto edge:sourceEdges) {
                const auto transformed=transform.ModifiedShape(edge.edge);
                if(transformed.IsNull() || transformed.ShapeType()!=TopAbs_EDGE)continue;
                edge.edge=TopoDS::Edge(transformed);
                edge.edgeId=QUuid::createUuidV5(QUuid(feature["id"].toString()),
                    QByteArray("alloy-feature-mirror-cut/1/")+sourceId.toUtf8()+'/'+edge.edgeId.toUtf8()).toString(QUuid::WithoutBraces);
                for(const auto* key:{"startMm","endMm","centerMm"})if(edge.provenance[key].isArray()){
                    auto coords=edge.provenance[key].toArray();
                    if(coords.size()==3){gp_Pnt p(coords[0].toDouble(),coords[1].toDouble(),coords[2].toDouble());p.Transform(reflection);
                        edge.provenance[key]=QJsonArray{p.X(),p.Y(),p.Z()};}
                }
                edge.provenance["sourceFeatureId"]=sourceId;
                edge.provenance["mirrorFeatureId"]=feature["id"];
                reflectedEdges.push_back(std::move(edge));
            }
        }
        const auto before=GeometryKernel::summarize(target.shape)["volumeMm3"].toDouble();
        target.shape=GeometryKernel::booleanOperation("cut",target.shape,reflected,
            &target.supportFaces,&target.solidEdges,&reflectedEdges,feature["id"].toString());
        // The exact Boolean history also names exposed straight boundaries on
        // retained support faces. Publish those same current edges to solid
        // edge selection so later edge finishes can refer to the mirrored cut.
        for(const auto& face:target.supportFaces)for(const auto& edge:face.boundaryEdges) {
            if(edge.provenance["generatorFeatureId"]!=feature["id"] || edge.edge.IsNull())continue;
            const auto existing=std::find_if(target.solidEdges.begin(),target.solidEdges.end(),
                [&](const auto& item){return !item.edge.IsNull() && item.edge.IsSame(edge.edge);});
            if(existing==target.solidEdges.end())
                target.solidEdges.push_back({edge.edgeId,edge.edge,edge.provenance});
        }
        const auto after=GeometryKernel::summarize(target.shape)["volumeMm3"].toDouble();
        if(before-after<=std::max(1e-6,before*1e-12))
            fail("mirror_no_effect","Mirrored Cut removes no target material");
        instances.append(QJsonObject{{"sourceFeatureId",sourceId},{"removedVolumeMm3",before-after}});
    }
    auto summary=GeometryKernel::summarize(target.shape);
    summary["operation"]="featureMirror";summary["lastModifierFeatureId"]=feature["id"];
    summary["sourceFeatureIds"]=parameters["sourceFeatureIds"];
    summary["mirrorPlane"]=plane;summary["instances"]=instances;
    summary["removedVolumeMm3"]=originalVolume-summary["volumeMm3"].toDouble();
    summary["supportFaces"]=PlanarFaces::descriptors(target.supportFaces);
    summary["solidEdges"]=SolidFillet::descriptors(target.solidEdges);
    target.summary=summary;
}

void FeaturePattern::mirrorSnapGroup(EvaluatedBody& target,const QJsonObject& feature,
    const QJsonArray& features,const EvaluatedDocument& prior) {
    const auto parameters=feature["parameters"].toObject();
    auto fail=[&](const QString& code,const QString& message,const QString& sourceId=QString{}){
        throw GeometryKernel::EvaluationError({{"code",code},{"message",message},
            {"featureId",feature["id"]},{"targetBodyId",target.bodyId},{"sourceFeatureId",sourceId}});};
    const auto selected=parameters["sourceFeatureIds"].toArray();
    const auto plane=parameters["mirrorPlane"].toObject();
    const auto axis=plane["plane"].toString();
    if(selected.size()!=5 || plane["kind"]!="origin" || (axis!="XZ" && axis!="YZ"))
        fail("mirror_group_unsupported","Snap Mirror requires five ordered features and origin XZ or YZ");
    QMap<QString,QJsonObject> sources;
    for(const auto& value:features)if(selected.contains(value.toObject()["id"]))sources[value.toObject()["id"].toString()]=value.toObject();
    const auto joinId=selected.first().toString();
    if(!prior.joinFeatureTools.contains(joinId) || prior.joinFeatureTools[joinId].size()!=1)
        fail("mirror_source_unavailable","Snap Join needs one retained source-stage prism",joinId);
    gp_Trsf reflection;reflection.SetMirror(gp_Ax2(gp_Pnt(0,0,0),axis=="YZ" ? gp_Dir(1,0,0) : gp_Dir(0,1,0)));
    auto reflectEndpoints=[&](QJsonObject& provenance){
        for(const auto* key:{"startMm","endMm"}){
            const auto xyz=provenance[key].toArray();
            if(xyz.size()!=3)continue;
            gp_Pnt point(xyz[0].toDouble(),xyz[1].toDouble(),xyz[2].toDouble());
            point.Transform(reflection);
            provenance[key]=QJsonArray{point.X(),point.Y(),point.Z()};
        }
    };
    BRepBuilderAPI_Transform moved(prior.joinFeatureTools[joinId].front(),reflection,true);
    if(!moved.IsDone() || moved.Shape().IsNull())fail("mirror_transform_failed","Snap Join reflection failed",joinId);
    auto nextFaces=target.supportFaces;
    const auto caps=prior.joinFeatureCaps.value(joinId);
    if(caps.size()!=1 || caps.front().face.IsNull())
        fail("mirror_source_cap_unavailable","Snap Join has no unique end-cap reference",joinId);
    auto cap=caps.front();
    const auto transformedCap=moved.ModifiedShape(cap.face);
    if(transformedCap.IsNull() || transformedCap.ShapeType()!=TopAbs_FACE)
        fail("mirror_source_cap_unavailable","Snap Join end cap cannot be reflected",joinId);
    cap.face=TopoDS::Face(transformedCap);cap.bodyId=target.bodyId;
    cap.faceId=QUuid::createUuidV5(QUuid(feature["id"].toString()),
        QByteArray("alloy-feature-mirror-snap-cap/1/")+joinId.toUtf8()).toString(QUuid::WithoutBraces);
    cap.producerFeatureId=feature["id"].toString();cap.provenance["sourceFeatureId"]=joinId;
    auto transformedFrame=cap.frame;
    for(const auto* key:{"origin","uAxis","vAxis","normal"}){
        const auto values=transformedFrame[key].toArray();
        if(values.size()!=3)fail("mirror_source_cap_unavailable","Snap Join cap frame is incomplete",joinId);
        if(QString(key)=="origin"){
            gp_Pnt point(values[0].toDouble(),values[1].toDouble(),values[2].toDouble());point.Transform(reflection);
            transformedFrame[key]=QJsonArray{point.X(),point.Y(),point.Z()};
        }else{
            gp_Vec axis(values[0].toDouble(),values[1].toDouble(),values[2].toDouble());axis.Transform(reflection);
            if(QString(key)=="vAxis")axis.Reverse();
            transformedFrame[key]=QJsonArray{axis.X(),axis.Y(),axis.Z()};
        }
    }
    cap.frame=transformedFrame;
    for(auto& edge:cap.boundaryEdges){
        const auto mapped=moved.ModifiedShape(edge.edge);
        if(mapped.IsNull() || mapped.ShapeType()!=TopAbs_EDGE)
            fail("mirror_source_cap_unavailable","Snap Join cap boundary cannot be reflected",joinId);
        edge.edge=TopoDS::Edge(mapped);
        edge.edgeId=QUuid::createUuidV5(QUuid(feature["id"].toString()),
            QByteArray("alloy-feature-mirror-snap-cap-edge/1/")+edge.edgeId.toUtf8()).toString(QUuid::WithoutBraces);
        reflectEndpoints(edge.provenance);
        edge.provenance["sourceFeatureId"]=joinId;edge.provenance["mirrorFeatureId"]=feature["id"];
    }
    nextFaces.push_back(std::move(cap));
    std::vector<EvaluatedSolidEdge> nextEdges=target.solidEdges;
    for(auto edge:prior.joinFeatureEdges.value(joinId)){
        if(edge.edge.IsNull())continue;
        const auto mapped=moved.ModifiedShape(edge.edge);
        if(mapped.IsNull() || mapped.ShapeType()!=TopAbs_EDGE)continue;
        edge.edge=TopoDS::Edge(mapped);
        edge.edgeId=QUuid::createUuidV5(QUuid(feature["id"].toString()),
            QByteArray("alloy-feature-mirror-snap-edge/1/")+edge.edgeId.toUtf8()).toString(QUuid::WithoutBraces);
        reflectEndpoints(edge.provenance);
        edge.provenance["sourceFeatureId"]=joinId;edge.provenance["mirrorFeatureId"]=feature["id"];
        nextEdges.push_back(std::move(edge));
    }
    const auto before=GeometryKernel::summarize(target.shape)["volumeMm3"].toDouble();
    target.shape=GeometryKernel::booleanOperation("join",target.shape,{moved.Shape()},
        &nextFaces,&nextEdges);
    target.supportFaces=std::move(nextFaces);
    target.solidEdges=std::move(nextEdges);
    const auto joined=GeometryKernel::summarize(target.shape)["volumeMm3"].toDouble();
    if(joined<=before+std::max(1e-6,before*1e-12))fail("mirror_no_effect","Reflected snap adds no target material",joinId);
    auto mirroredEdge=[&](const EvaluatedSolidEdge& source,const QString& sourceId){
        if(source.edge.IsNull())fail("mirror_edge_unavailable","Source finish edge is unavailable",sourceId);
        BRepAdaptor_Curve curve(source.edge);
        if(curve.GetType()!=GeomAbs_Line)fail("mirror_edge_unsupported","Snap Mirror currently requires straight finish edges",sourceId);
        auto first=curve.Value(curve.FirstParameter()),last=curve.Value(curve.LastParameter());
        first.Transform(reflection);last.Transform(reflection);
        QString found;int matches=0;
        for(const auto& candidate:target.solidEdges){
            if(candidate.edge.IsNull())continue;
            BRepAdaptor_Curve current(candidate.edge);
            if(current.GetType()!=GeomAbs_Line)continue;
            const auto a=current.Value(current.FirstParameter()),b=current.Value(current.LastParameter());
            if(std::abs(a.Distance(b)-first.Distance(last))>1e-6)continue;
            if((a.Distance(first)<1e-6 && b.Distance(last)<1e-6)
                || (a.Distance(last)<1e-6 && b.Distance(first)<1e-6)){found=candidate.edgeId;++matches;}
        }
        if(matches!=1)fail("mirror_edge_ambiguous","A reflected source finish edge has no unique current descendant",sourceId);
        return found;
    };
    for(int index=1;index<5;++index){
        const auto sourceId=selected[index].toString();const auto source=sources.value(sourceId);
        const auto inputs=source["parameters"].toObject();
        const auto cached=prior.finishFeatureInputs.value(sourceId);
        if(cached.size()!=inputs["edgeIds"].toArray().size() || cached.empty())
            fail("mirror_edge_unavailable","Source finish has no exact retained edge set",sourceId);
        QJsonArray mapped;
        for(const auto& sourceEdge:cached)mapped.append(mirroredEdge(sourceEdge,sourceId));
        const auto replayId=QUuid::createUuidV5(QUuid(feature["id"].toString()),
            QByteArray("alloy-feature-mirror-snap-finish/1/")+sourceId.toUtf8()).toString(QUuid::WithoutBraces);
        if(index<4)SolidChamfer::evaluate(target,{{"targetBodyId",target.bodyId},
            {"edgeIds",mapped},{"distanceMm",inputs["distanceMm"]}},replayId);
        else SolidFillet::evaluate(target,{{"targetBodyId",target.bodyId},{"edgeIds",mapped},
            {"radiusMm",inputs["radiusMm"]},{"tangentChain",true}},replayId);
    }
    auto summary=GeometryKernel::summarize(target.shape);
    summary["operation"]="featureMirror";summary["lastModifierFeatureId"]=feature["id"];
    summary["sourceFeatureIds"]=selected;summary["mirrorPlane"]=parameters["mirrorPlane"];
    summary["addedVolumeMm3"]=summary["volumeMm3"].toDouble()-before;
    summary["supportFaces"]=PlanarFaces::descriptors(target.supportFaces);
    summary["solidEdges"]=SolidFillet::descriptors(target.solidEdges);
    target.summary=summary;
}
