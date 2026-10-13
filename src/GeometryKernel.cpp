#include "PlanarFaces.h"
#include "SolidFillet.h"
#include "SolidShell.h"
#include <BRepBuilderAPI_Transform.hxx>
#include <gp_Ax1.hxx>
#include <gp_Trsf.hxx>
#include "GeometryKernel.h"
#include "SolidFillet.h"
#include "SketchGeometry.h"
#include "StepExchange.h"
#include <BRepBndLib.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <TopoDS_Vertex.hxx>
#include <BRepAlgoAPI_BooleanOperation.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepGProp.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRep_Tool.hxx>
#include <Bnd_Box.hxx>
#include <GProp_GProps.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_ListOfShape.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <gp_Vec.hxx>
#include <gp_Circ.hxx>
#include <QJsonArray>
#include <QCryptographicHash>
#include <QMap>
#include <QSet>
#include <QUuid>
#include <Standard_Failure.hxx>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace {
[[noreturn]] void reject(const QString& code,const QString& message) {throw GeometryKernel::EvaluationError({{"code",code},{"message",message}});}
int count(const TopoDS_Shape& shape, TopAbs_ShapeEnum kind) {
    TopTools_IndexedMapOfShape map; TopExp::MapShapes(shape,kind,map); return map.Extent();
}
double length(const QJsonObject& parameters, const QString& key, bool placement=false) {
    const auto value=parameters.value(key);
    if (!value.isDouble() || !std::isfinite(value.toDouble()) || std::abs(value.toDouble())>100000
        || (!placement && value.toDouble()<0.001)) throw std::invalid_argument(("Unsupported length: "+key).toStdString());
    return value.toDouble();
}
}

GeometryKernel::EvaluationError::EvaluationError(const QJsonObject& diagnostics)
    : std::runtime_error(diagnostics["message"].toString().toStdString()), diagnostics_(diagnostics) {}

TopoDS_Shape GeometryKernel::primitive(const QString& type, const QJsonObject& p) {
    const gp_Pnt origin(length(p,"xMm",true),length(p,"yMm",true),length(p,"zMm",true));
    TopoDS_Shape shape;
    if (type=="box") shape=BRepPrimAPI_MakeBox(origin,length(p,"widthMm"),length(p,"depthMm"),length(p,"heightMm")).Shape();
    else if (type=="cylinder") shape=BRepPrimAPI_MakeCylinder(gp_Ax2(origin,gp::DZ()),length(p,"radiusMm"),length(p,"heightMm")).Shape();
    else throw std::invalid_argument("Unsupported primitive type");
    if (shape.IsNull() || !BRepCheck_Analyzer(shape).IsValid() || count(shape,TopAbs_SOLID)!=1)
        throw std::runtime_error("Kernel did not produce a valid solid");
    return shape;
}
EvaluatedSupportFace GeometryKernel::resolveSketchSupport(const EvaluatedDocument& evaluation,const QJsonObject& support) {
    auto reject=[&](const QString& code,const QString& message){auto diagnostic=support;diagnostic["code"]=code;diagnostic["message"]=message;throw EvaluationError(diagnostic);};
    if(support.size()!=3)reject("invalid_support","Support requires producerFeatureId, bodyId and faceId");
    for(const auto& field:{"producerFeatureId","bodyId","faceId"})if(!support[field].isString() || QUuid(support[field].toString()).isNull())reject("invalid_support","Support requires stable UUID references");
    const auto body=std::find_if(evaluation.bodies.begin(),evaluation.bodies.end(),[&](const auto& value){return value.bodyId==support["bodyId"].toString();});
    if(body==evaluation.bodies.end())reject("support_unavailable","The exact support producer/body is unavailable or consumed");
    const EvaluatedSupportFace* found=nullptr;
    for(const auto& face:body->supportFaces)if(face.faceId==support["faceId"].toString() && face.producerFeatureId==support["producerFeatureId"].toString()){if(found)reject("ambiguous_support","More than one face matches the support reference");found=&face;}
    if(!found)reject("support_unavailable","The exact named planar support has no unique surviving face");

    int matches=0;for(TopExp_Explorer face(body->shape,TopAbs_FACE);face.More();face.Next())if(face.Current().IsSame(found->face))++matches;
    if(matches!=1 || found->face.IsNull() || !BRepCheck_Analyzer(found->face).IsValid()
        || BRepAdaptor_Surface(found->face).GetType()!=GeomAbs_Plane)reject("support_unavailable","The retained support is not one real planar producer face");
    return *found;
}

QJsonObject GeometryKernel::resolvePlaneSource(const EvaluatedDocument& evaluation,const QJsonObject& source) {
    if(source.size()!=1)throw EvaluationError({{"code","invalid_plane_source"},{"message","Plane requires exactly one source"}});
    if(source.contains("plane"))return SketchGeometry::frame(source["plane"].toString());
    if(source.contains("planeId")) {
        for(const auto& plane:evaluation.constructionPlanes)if(plane.featureId==source["planeId"].toString())return plane.frame;
    } else if(source.contains("sketchId")) {
        for(const auto& sketch:evaluation.sketches)if(sketch.featureId==source["sketchId"].toString())return sketch.resolvedFrame;
    } else if(source.contains("support"))return resolveSketchSupport(evaluation,source["support"].toObject()).frame;
    throw EvaluationError({{"code","plane_source_unavailable"},{"message","The retained plane source is unavailable"},{"source",source}});
}

QJsonObject GeometryKernel::offsetPlaneFrame(const QJsonObject& frame,double offsetMm) {
    auto result=frame;auto origin=frame["origin"].toArray(),normal=frame["normal"].toArray();
    if(origin.size()!=3 || normal.size()!=3 || !std::isfinite(offsetMm))throw EvaluationError({{"code","invalid_plane_frame"},{"message","Plane source frame is invalid"}});
    for(int axis=0;axis<3;++axis)origin[axis]=origin[axis].toDouble()+offsetMm*normal[axis].toDouble();
    result["origin"]=origin;return result;
}

QJsonArray GeometryKernel::supportEdgeDescriptors(const EvaluatedSupportFace& support) {
    QJsonArray result;
    for(const auto& edge:support.boundaryEdges) {
        auto descriptor=edge.provenance;descriptor["edgeId"]=edge.edgeId;descriptor["type"]=edge.type;
        descriptor["supported"]=edge.type=="line";result.append(descriptor);
    }
    return result;
}
QJsonObject GeometryKernel::projectionContext(const EvaluatedDocument& evaluation,const QJsonObject& feature) {
    const auto parameters=feature["parameters"].toObject();QJsonArray lines,unsupported,points;
    if(parameters["plane"]!="face")return {{"lines",lines},{"unsupportedEdges",unsupported},{"points",points}};
    const auto cap=resolveSketchSupport(evaluation,parameters["support"].toObject());
    for(const auto& edge:cap.boundaryEdges)if(edge.type!="line")unsupported.append(edge.edgeId);
    const auto origin=cap.frame["origin"].toArray(),u=cap.frame["uAxis"].toArray(),v=cap.frame["vAxis"].toArray();
    auto local=[&](const QJsonArray& point,const QJsonArray& axis){double result=0;for(int i=0;i<3;++i)result+=(point[i].toDouble()-origin[i].toDouble())*axis[i].toDouble();return result;};
    QMap<QString,QJsonObject> centers;
    for(const auto& edge:cap.boundaryEdges) {
        if(edge.type=="line")continue;
        BRepAdaptor_Curve curve(edge.edge);if(curve.GetType()!=GeomAbs_Circle)continue;
        const auto circle=curve.Circle();const auto center=circle.Location();
        const auto lineage=edge.provenance;const auto key=lineage["sourceSketchId"].toString()+":"+lineage["sourceEntityId"].toString();
        auto source=parameters["support"].toObject();source["sourceSketchId"]=lineage["sourceSketchId"];source["sourceEntityId"]=lineage["sourceEntityId"];source["role"]="curveCenter";
        const QJsonArray world{center.X(),center.Y(),center.Z()};
        const double cu=local(world,u),cv=local(world,v);
        double normalDistance=0;const auto normal=cap.frame["normal"].toArray();for(int i=0;i<3;++i)normalDistance+=(world[i].toDouble()-origin[i].toDouble())*normal[i].toDouble();
        if(std::abs(normalDistance)>1e-6)reject("ambiguous_projection","Circular cap center is outside the resolved support plane");
        if(centers.contains(key)) {
            const auto prior=centers[key];
            if(std::hypot(prior["uMm"].toDouble()-cu,prior["vMm"].toDouble()-cv)>1e-6 || std::abs(prior["radiusMm"].toDouble()-circle.Radius())>1e-6)
                reject("ambiguous_projection","Circular descendants disagree on their retained source center");
            continue;
        }
        const auto id=SketchGeometry::curveCenterPointId(feature["id"].toString(),source);
        centers[key]={{"id",id},{"source",source},{"uMm",cu},{"vMm",cv},{"radiusMm",circle.Radius()},{"retained",false}};
    }
    for(const auto& value:parameters["pointReferences"].toArray()) {
        const auto reference=value.toObject();bool found=false;
        for(auto it=centers.begin();it!=centers.end();++it)if(it.value()["id"]==reference["id"] && it.value()["source"]==reference["source"]){auto point=it.value();point["retained"]=true;it.value()=point;found=true;break;}
        if(!found)throw EvaluationError({{"code","projection_unavailable"},{"pointReferenceId",reference["id"]},{"source",reference["source"]},{"message","Retained circular support center is no longer uniquely available"}});
    }
    for(auto point:centers){point.remove("radiusMm");points.append(point);}
    for(const auto& item:parameters["projections"].toArray()) {
        const auto projection=item.toObject(),source=projection["source"].toObject();
        for(const auto& key:{"producerFeatureId","bodyId","faceId"})if(source[key]!=parameters["support"].toObject()[key])
            reject("projection_unavailable","Projected edge must belong to the retained sketch support");
        const auto found=std::find_if(cap.boundaryEdges.begin(),cap.boundaryEdges.end(),[&](const auto& edge){return edge.edgeId==source["edgeId"].toString();});
        if(found==cap.boundaryEdges.end())throw EvaluationError({{"code","projection_unavailable"},{"projectionId",projection["id"]},{"source",source},{"message","Retained support boundary edge is no longer available"}});
        if(found->type!="line")reject("unsupported_projection","This projection slice supports straight cap edges only");
        const auto id=projection["id"].toString();const auto a=found->provenance["startMm"].toArray(),b=found->provenance["endMm"].toArray();
        lines.append(QJsonObject{{"id",id},{"startPointId",SketchGeometry::projectionPointId(id,false)},{"endPointId",SketchGeometry::projectionPointId(id,true)},
            {"u1Mm",local(a,u)},{"v1Mm",local(a,v)},{"u2Mm",local(b,u)},{"v2Mm",local(b,v)},{"source",source}});
    }
    return {{"lines",lines},{"unsupportedEdges",unsupported},{"points",points}};
}

EvaluatedProfile GeometryKernel::unionProfiles(const std::vector<EvaluatedProfile>& profiles) {
    if(profiles.empty())throw std::invalid_argument("Select at least one current profile");
    if(profiles.size()==1)return profiles.front();
    const auto& first=profiles.front();TopoDS_Shape shape=first.face;
    QStringList profileIds,entityIds;double expectedArea=0;
    auto lineage=first.boundaryEdges;
    QSet<QString> distinct;
    for(const auto& profile:profiles){
        if(profile.sketchId!=first.sketchId || profile.plane!=first.plane || profile.face.IsNull()
            || !BRepCheck_Analyzer(profile.face).IsValid() || distinct.contains(profile.profileId))
            throw std::invalid_argument("Profile union requires distinct valid regions from one sketch");
        distinct.insert(profile.profileId);profileIds.append(profile.profileId);expectedArea+=profile.summary["areaMm2"].toDouble();
        for(const auto& id:profile.summary["sourceEntityIds"].toArray())entityIds.append(id.toString());
    }
    // All regions are already derived on the same plane. Fuse before making a
    // prism so shared partitions cannot survive as separate feature bodies.
    for(size_t i=1;i<profiles.size();++i){BRepAlgoAPI_Fuse fused(shape,profiles[i].face);
        if(!fused.IsDone() || fused.HasErrors())throw std::runtime_error("OCCT could not unite the selected planar regions");
        fused.SimplifyResult(true,true);shape=fused.Shape();
        lineage.insert(lineage.end(),profiles[i].boundaryEdges.begin(),profiles[i].boundaryEdges.end());
        std::vector<EvaluatedBoundaryEdge> next;
        TopTools_IndexedMapOfShape present;TopExp::MapShapes(shape,TopAbs_EDGE,present);
        for(const auto& source:lineage) {
            TopTools_IndexedMapOfShape descendants;
            if(present.Contains(source.edge))descendants.Add(source.edge);
            for(const auto& changed:fused.Modified(source.edge))if(changed.ShapeType()==TopAbs_EDGE && present.Contains(changed))descendants.Add(changed);
            for(int j=1;j<=descendants.Extent();++j){auto mapped=source;mapped.edge=TopoDS::Edge(descendants(j));
                if(source.type=="line") {
                    TopoDS_Vertex a,b,c,d;TopExp::Vertices(source.edge,a,b,true);TopExp::Vertices(mapped.edge,c,d,true);
                    const auto pa=BRep_Tool::Pnt(a),pb=BRep_Tool::Pnt(b),pc=BRep_Tool::Pnt(c),pd=BRep_Tool::Pnt(d);
                    if(pa.Distance(pc)<=1e-6 && pb.Distance(pd)<=1e-6){}
                    else if(pa.Distance(pd)<=1e-6 && pb.Distance(pc)<=1e-6)std::swap(mapped.startContact,mapped.endContact);
                    else reject("ambiguous_projection","A straight source edge was split or merged; endpoint lineage requires explicit repair");
                }
                const bool duplicate=std::any_of(next.begin(),next.end(),[&](const auto& prior){return prior.edge.IsSame(mapped.edge) && prior.sourceEntityId==mapped.sourceEntityId && prior.sourceSketchId==mapped.sourceSketchId;});
                if(!duplicate)next.push_back(mapped);}
        }
        lineage=std::move(next);
    }
    if(shape.IsNull() || !BRepCheck_Analyzer(shape).IsValid() || count(shape,TopAbs_FACE)!=1)
        throw std::invalid_argument("Selected regions must form one connected planar face");
    TopExp_Explorer face(shape,TopAbs_FACE);const auto united=TopoDS::Face(face.Current());
    GProp_GProps area;BRepGProp::SurfaceProperties(united,area);
    if(!std::isfinite(area.Mass()) || std::abs(area.Mass()-expectedArea)>std::max(1e-6,expectedArea*1e-9))
        throw std::runtime_error("Selected region union area does not match its disjoint planar regions");
    profileIds.sort();entityIds.removeDuplicates();entityIds.sort();QJsonArray refs,sources;
    for(const auto& id:profileIds)refs.append(id);for(const auto& id:entityIds)sources.append(id);
    const auto id=QUuid::createUuidV5(QUuid(first.sketchId),QByteArray("alloy-profile-union/1\n")+profileIds.join('\n').toUtf8()).toString(QUuid::WithoutBraces);
    QJsonObject summary{{"profileId",id},{"profileIds",refs},{"sketchId",first.sketchId},{"plane",first.plane},
        {"frame",first.summary["frame"]},{"valid",true},{"areaUnit","mm2"},{"areaMm2",area.Mass()},{"sourceEntityIds",sources},{"type","profile_union"}};
    return {id,first.sketchId,first.plane,united,summary,lineage};
}

TopoDS_Shape GeometryKernel::extrusion(const EvaluatedProfile& profile, double distanceMm, EvaluatedSupportFace* endCap, std::vector<EvaluatedSolidEdge>* edges, const QString& generatorFeatureId, std::vector<EvaluatedSupportFace>* planarFaces) {
    if (!std::isfinite(distanceMm) || std::abs(distanceMm) < 0.001 || std::abs(distanceMm) > 100000)
        throw std::invalid_argument("Extrusion distance must be signed 0.001..100000 mm; zero is unsupported");
    if (profile.face.IsNull() || !BRepCheck_Analyzer(profile.face).IsValid())
        throw std::invalid_argument("Extrusion requires a current valid closed profile face");
    const auto sourceFrame=profile.summary["frame"].toObject();
    const auto normal = sourceFrame["normal"].toArray();
    if(normal.size()!=3)throw std::invalid_argument("Extrusion profile has no resolved frame");
    BRepPrimAPI_MakePrism prism(profile.face,gp_Vec(normal[0].toDouble()*distanceMm,
        normal[1].toDouble()*distanceMm,normal[2].toDouble()*distanceMm),true,true);
    if (!prism.IsDone()) throw std::runtime_error("OCCT could not build the profile extrusion");
    const auto shape = prism.Shape();
    if(edges) *edges=SolidFillet::prismEdges(prism,profile,generatorFeatureId);
    if(planarFaces)*planarFaces=PlanarFaces::prismSides(prism,profile,generatorFeatureId,distanceMm);
    if(endCap && distanceMm>0) {
        const auto cap=prism.LastShape();
        if(cap.IsNull() || cap.ShapeType()!=TopAbs_FACE || BRepAdaptor_Surface(TopoDS::Face(cap)).GetType()!=GeomAbs_Plane)
            throw std::runtime_error("Prism has no unique planar generated end cap");
        int matches=0;for(TopExp_Explorer face(shape,TopAbs_FACE);face.More();face.Next())if(face.Current().IsSame(cap))++matches;
        if(matches!=1)throw std::runtime_error("Generated end cap is not unique on the prism result");
        auto transported=sourceFrame;auto origin=sourceFrame["origin"].toArray();
        for(int i=0;i<3;++i)origin[i]=origin[i].toDouble()+normal[i].toDouble()*distanceMm;
        transported["origin"]=origin;
        const gp_Pnt expected(origin[0].toDouble(),origin[1].toDouble(),origin[2].toDouble());
        const gp_Dir direction(normal[0].toDouble(),normal[1].toDouble(),normal[2].toDouble());
        const auto realPlane=BRepAdaptor_Surface(TopoDS::Face(cap)).Plane();
        if(realPlane.Distance(expected)>1e-6 || std::abs(realPlane.Axis().Direction().Dot(direction))<1-1e-9)
            throw std::runtime_error("Generated end cap does not match the transported source frame");
        endCap->role="extrude.endCap";endCap->face=TopoDS::Face(cap);endCap->frame=transported;
        TopTools_IndexedMapOfShape capEdges;TopExp::MapShapes(cap,TopAbs_EDGE,capEdges);
        QSet<QString> identities;
        for(const auto& boundary:profile.boundaryEdges) {
            const auto transportedEdge=prism.LastShape(boundary.edge);
            if(transportedEdge.IsNull() || transportedEdge.ShapeType()!=TopAbs_EDGE || !capEdges.Contains(transportedEdge))
                reject("projection_lineage_unavailable","A source boundary has no unique actual prism cap edge");
            for(const auto& other:profile.boundaryEdges)if(other.sourceEntityId!=boundary.sourceEntityId && other.edge.IsSame(boundary.edge))
                reject("ambiguous_projection","A cap edge combines more than one authored source");
            QStringList contacts{boundary.startContact,boundary.endContact};contacts.sort();
            const auto signature=boundary.sourceEntityId+"\n"+contacts.join("\n");
            const auto edgeId=QUuid::createUuidV5(QUuid(boundary.sourceSketchId),QByteArray("alloy-cap-edge/1\n")+signature.toUtf8()).toString(QUuid::WithoutBraces);
            if(boundary.type=="line" && identities.contains(edgeId))reject("ambiguous_projection","One authored boundary maps to multiple cap edges");identities.insert(edgeId);
            QJsonObject provenance{{"sourceSketchId",boundary.sourceSketchId},{"sourceEntityId",boundary.sourceEntityId},
                {"startContact",contacts[0]},{"endContact",contacts[1]}};
            TopoDS_Vertex start,end;TopExp::Vertices(TopoDS::Edge(transportedEdge),start,end,true);
            auto a=BRep_Tool::Pnt(start),b=BRep_Tool::Pnt(end);
            if(boundary.type=="line") {
                TopoDS_Vertex originalStart,originalEnd;TopExp::Vertices(boundary.edge,originalStart,originalEnd,true);
                const auto shift=gp_Vec(normal[0].toDouble()*distanceMm,normal[1].toDouble()*distanceMm,normal[2].toDouble()*distanceMm);
                const auto expectedStart=BRep_Tool::Pnt(originalStart).Translated(shift),expectedEnd=BRep_Tool::Pnt(originalEnd).Translated(shift);
                if(a.Distance(expectedStart)<=1e-6 && b.Distance(expectedEnd)<=1e-6){}
                else if(b.Distance(expectedStart)<=1e-6 && a.Distance(expectedEnd)<=1e-6)std::swap(a,b);
                else reject("ambiguous_projection","Prism endpoint lineage does not match the source boundary");
            }
            if(boundary.startContact>boundary.endContact)std::swap(a,b);
            provenance["startMm"]=QJsonArray{a.X(),a.Y(),a.Z()};provenance["endMm"]=QJsonArray{b.X(),b.Y(),b.Z()};
            endCap->boundaryEdges.push_back({edgeId,boundary.type,TopoDS::Edge(transportedEdge),provenance});
        }

    }
    if (shape.IsNull() || !BRepCheck_Analyzer(shape).IsValid() || count(shape,TopAbs_SOLID) != 1)
        throw std::runtime_error("Extrusion did not produce one valid solid");
    const auto summary = summarize(shape);
    const double expected = profile.summary["areaMm2"].toDouble()*std::abs(distanceMm);
    if (!summary["closed"].toBool() || !summary["volumeMm3"].isDouble() || !std::isfinite(expected)
        || expected <= 0 || std::abs(summary["volumeMm3"].toDouble()-expected) > std::max(1e-6,expected*1e-9))
        throw std::runtime_error("Extrusion volume does not match profile area times distance");
    return shape;
}
gp_Trsf GeometryKernel::moveTransform(const QJsonObject& parameters) {
    gp_Trsf result;const gp_Vec pivot(parameters["pivotXMm"].toDouble(),parameters["pivotYMm"].toDouble(),parameters["pivotZMm"].toDouble());result.SetTranslation(pivot);
    const char* angles[]={"xDeg","yDeg","zDeg"};const gp_Dir axes[]={gp_Dir(1,0,0),gp_Dir(0,1,0),gp_Dir(0,0,1)};
    for(const auto& item:parameters["steps"].toArray()){const auto step=item.toObject();gp_Trsf rotation;
        for(int i=0;i<3;++i){gp_Trsf r;r.SetRotation(gp_Ax1(gp_Pnt(0,0,0),axes[i]),step[angles[i]].toDouble()*std::acos(-1.0)/180);rotation=r*rotation;}
        gp_Trsf translation;translation.SetTranslation(gp_Vec(step["xMm"].toDouble(),step["yMm"].toDouble(),step["zMm"].toDouble()));result=result*translation*rotation;
    }
    gp_Trsf undoPivot;undoPivot.SetTranslation(-pivot);return result*undoPivot;
}
TopoDS_Shape GeometryKernel::booleanOperation(const QString& operation, const TopoDS_Shape& target,
    const std::vector<TopoDS_Shape>& tools, std::vector<EvaluatedSupportFace>* retainedFaces, std::vector<EvaluatedSolidEdge>* retainedEdges) {
    auto reject = [](const QString& code, const QString& message) {
        throw EvaluationError({{"code",code},{"message",message}});
    };
    BOPAlgo_Operation kind;
    if (operation == "join") kind = BOPAlgo_FUSE;
    else if (operation == "cut") kind = BOPAlgo_CUT;
    else if (operation == "intersect") kind = BOPAlgo_COMMON;
    else throw std::invalid_argument("Unsupported Boolean operation");
    if (target.IsNull() || tools.empty()) throw std::invalid_argument("Boolean operation needs a target and at least one tool");
    auto validInput = [](const TopoDS_Shape& shape) {
        if (shape.IsNull() || count(shape,TopAbs_SOLID) != 1) return false;
        const auto summary = summarize(shape);
        return summary["valid"].toBool() && summary["closed"].toBool() && summary["volumeMm3"].isDouble();
    };
    if (!validInput(target)) reject("boolean_input_invalid","Boolean target must be one valid closed solid");
    TopTools_ListOfShape objects, operands;objects.Append(target);
    for (const auto& tool : tools) {
        if (!validInput(tool)) reject("boolean_input_invalid","Every Boolean tool must be one valid closed solid");
        operands.Append(tool);
    }
    if (tools.size() > 1) {
        // Common with several separate tool arguments can preserve their
        // partition boundaries as adjacent solids. Unite the tool group first
        // so Intersect means overlap with their union, with no split artifacts.
        TopTools_ListOfShape groupObjects,groupTools;groupObjects.Append(tools.front());
        for (size_t i = 1; i < tools.size(); ++i) groupTools.Append(tools[i]);
        BRepAlgoAPI_BooleanOperation group;
        group.SetArguments(groupObjects);group.SetTools(groupTools);group.SetOperation(BOPAlgo_FUSE);
        group.SetNonDestructive(true);group.SetRunParallel(false);group.SetFuzzyValue(0.0);group.Build();
        if (!group.IsDone() || group.HasErrors()) reject("boolean_kernel_failed","OCCT could not unite the Boolean tool group");
        group.SimplifyResult(true,true,1e-12);
        if (group.Shape().IsNull() || !BRepCheck_Analyzer(group.Shape()).IsValid())
            reject("boolean_input_invalid","Boolean tool group did not produce valid topology");
        operands.Clear();operands.Append(group.Shape());
    }
    BRepAlgoAPI_BooleanOperation builder;
    builder.SetArguments(objects);builder.SetTools(operands);builder.SetOperation(kind);
    // Candidate and undo snapshots share input shapes. Never let the algorithm
    // modify their topology/tolerances while evaluating a disposable candidate.
    builder.SetNonDestructive(true);builder.SetRunParallel(false);builder.SetFuzzyValue(0.0);
    builder.Build();
    if (!builder.IsDone() || builder.HasErrors()) reject("boolean_kernel_failed","OCCT could not complete the Boolean operation");
    builder.SimplifyResult(true,true,1e-12);
    const auto shape = builder.Shape();
    if (shape.IsNull() || count(shape,TopAbs_SOLID) == 0)
        reject("boolean_empty_result","Boolean result is empty or has no solid volume; the target was preserved");
    if (count(shape,TopAbs_SOLID) != 1)
        reject("boolean_multiple_solids","This Combine slice requires one connected solid; disconnected or split results are unsupported");
    // Refuse stray faces/edges rather than silently dropping lower-dimensional
    // pieces from an otherwise positive-volume result.
    TopExp_Explorer solids(shape,TopAbs_SOLID);
    const auto solid = solids.Current();
    for (const auto kind : {TopAbs_FACE,TopAbs_EDGE,TopAbs_VERTEX})
        if (count(shape,kind) != count(solid,kind))
            reject("boolean_mixed_result","Boolean result contains geometry outside its single solid");
    const auto summary = summarize(shape);
    if (!summary["valid"].toBool() || !summary["closed"].toBool() || !summary["volumeMm3"].isDouble())
        reject("boolean_result_invalid","Boolean operation did not produce a valid closed positive-volume solid");
    if(retainedFaces)*retainedFaces=PlanarFaces::transfer(builder,*retainedFaces,shape);
    if(retainedEdges)*retainedEdges=SolidFillet::transferEdges(builder,*retainedEdges,shape);
    return shape;
}
QJsonObject GeometryKernel::summarize(const TopoDS_Shape& shape) {
    if (shape.IsNull()) throw std::invalid_argument("Cannot summarize a null shape");
    const bool valid=BRepCheck_Analyzer(shape).IsValid();
    TopTools_IndexedMapOfShape shells; TopExp::MapShapes(shape,TopAbs_SHELL,shells);
    bool closed=shells.Extent()>0;
    for(int i=1;i<=shells.Extent();++i) closed=closed && BRep_Tool::IsClosed(shells(i));
    const int solids=count(shape,TopAbs_SOLID);
    Bnd_Box bounds; BRepBndLib::AddOptimal(shape,bounds,false,false);
    double x0,y0,z0,x1,y1,z1; bounds.Get(x0,y0,z0,x1,y1,z1);
    GProp_GProps area; BRepGProp::SurfaceProperties(shape,area);
    QJsonValue volume(QJsonValue::Null);
    if (valid && closed && solids>0) {
        GProp_GProps properties; BRepGProp::VolumeProperties(shape,properties,true);
        if (std::isfinite(properties.Mass()) && properties.Mass()>0) volume=properties.Mass();
    }
    return {{"kernel","OCCT"},{"version",ALLOY_OCCT_VERSION},{"valid",valid},{"closed",closed},
        {"type",solids>0 ? "solid" : "surface"},{"lengthUnit","mm"},{"areaUnit","mm2"},{"volumeUnit","mm3"},
        {"volumeMm3",volume},{"areaMm2",area.Mass()},
        {"topology",QJsonObject{{"solids",solids},{"shells",shells.Extent()},
            {"faces",count(shape,TopAbs_FACE)},{"edges",count(shape,TopAbs_EDGE)},{"vertices",count(shape,TopAbs_VERTEX)}}},
        {"boundsMm",QJsonObject{{"min",QJsonArray{x0,y0,z0}},{"max",QJsonArray{x1,y1,z1}},
            {"size",QJsonArray{x1-x0,y1-y0,z1-z0}}}}};
}
std::vector<EvaluatedBody> GeometryKernel::evaluate(const QJsonObject& authored) {
    return evaluateDocument(authored).bodies;
}
EvaluatedDocument GeometryKernel::evaluateDocument(const QJsonObject& authored) {
    EvaluatedDocument evaluated;
    QMap<QString,size_t> priorSketches;
    for (const auto& entry : authored["features"].toArray()) {
        const auto feature = entry.toObject();
        const auto id = feature["id"].toString(), type = feature["type"].toString();
        const auto parameters = feature["parameters"].toObject();
        QJsonObject context{{"featureId",id},{"featureType",type}};
        try {
            if(type=="constructionPlane") {
                const auto source=parameters["source"].toObject();
                evaluated.constructionPlanes.push_back({id,feature["name"].toString(),feature["visible"].toBool(),
                    offsetPlaneFrame(resolvePlaneSource(evaluated,source),parameters["offsetMm"].toDouble())});
            } else if (type == "sketch") {
                priorSketches[id] = evaluated.sketches.size();
                QJsonObject resolvedFrame;
                if(parameters["plane"]=="face") {
                    try {resolvedFrame=resolveSketchSupport(evaluated,parameters["support"].toObject()).frame;}
                    catch(const EvaluationError& error){auto diagnostic=context;const auto details=error.diagnostics();for(auto it=details.begin();it!=details.end();++it)diagnostic[it.key()]=it.value();throw EvaluationError(diagnostic);}
                }
                if(parameters["plane"]=="construction") {
                    for(const auto& plane:evaluated.constructionPlanes)if(plane.featureId==parameters["planeId"].toString())resolvedFrame=plane.frame;
                    if(resolvedFrame.isEmpty())throw EvaluationError({{"code","plane_unavailable"},{"message","Sketch construction plane is unavailable"},{"featureId",id}});
                }
                evaluated.sketches.push_back(SketchGeometry::evaluateFeature(feature,resolvedFrame,projectionContext(evaluated,feature)));
            } else if (type == "stepImport") {
                const auto failStep=[&](const QString& code,const QString& message) {
                    auto details=context;details["code"]=code;details["message"]=message;
                    throw EvaluationError(details);
                };
                const auto source=parameters["source"].toObject();
                const auto encoded=source["bytesBase64"].toString().toLatin1();
                if (encoded.size()>44739244)
                    failStep("step_source_size","Embedded STEP source exceeds 32 MiB");
                const auto decoded=QByteArray::fromBase64Encoding(encoded,QByteArray::AbortOnBase64DecodingErrors);
                if (!decoded || decoded.decoded.isEmpty() || decoded.decoded.size()>StepExchange::maximumSourceBytes
                    || decoded.decoded.size()!=source["byteCount"].toInteger()
                    || decoded.decoded.toBase64()!=encoded)
                    failStep("step_source_corrupt","Embedded STEP source encoding or byte count is invalid");
                const auto digest=QCryptographicHash::hash(decoded.decoded,QCryptographicHash::Sha256).toHex();
                if (QString::fromLatin1(digest)!=source["sha256"].toString())
                    failStep("step_source_hash","Embedded STEP source hash does not match its bytes");
                StepExchange::ReadResult imported;
                try {imported=StepExchange::read(decoded.decoded);}
                catch(const StepExchange::Error& error){
                    const auto details=error.diagnostics();
                    for(auto it=details.begin();it!=details.end();++it)context[it.key()]=it.value();
                    throw EvaluationError(context);
                }
                auto summary=summarize(imported.shape);
                if (!summary["valid"].toBool() || !summary["closed"].toBool()
                    || summary["topology"].toObject()["solids"].toInt()!=1
                    || !summary["volumeMm3"].isDouble()
                    || !std::isfinite(summary["volumeMm3"].toDouble())
                    || summary["volumeMm3"].toDouble()<=0)
                    failStep("step_invalid_solid","Imported STEP must evaluate to one valid closed solid with positive volume");
                summary["sourceFormat"]="step";summary["sourcePolicy"]="embedded";
                summary["sourceSha256"]=source["sha256"];
                summary["sourceOriginalName"]=source["originalName"];
                summary["sourceLengthUnits"]=imported.diagnostics["fileLengthUnits"];
                summary["translator"]="OCCT STEPControl";
                evaluated.bodies.push_back({id,feature["bodyId"].toString(),feature["name"].toString(),
                    feature["visible"].toBool(),imported.shape,summary,{}});
            } else if(type=="fillet") {
                const auto target=std::find_if(evaluated.bodies.begin(),evaluated.bodies.end(),[&](const auto& body){return body.bodyId==parameters["targetBodyId"].toString();});
                if(target==evaluated.bodies.end())throw EvaluationError({{"code","body_unavailable"},{"featureId",id},{"message","Fillet target must be active earlier in history"}});
                SolidFillet::evaluate(*target,parameters,id);
            } else if(type=="shell") {
                const auto target=std::find_if(evaluated.bodies.begin(),evaluated.bodies.end(),[&](const auto& body){return body.bodyId==parameters["targetBodyId"].toString();});
                if(target==evaluated.bodies.end())throw EvaluationError({{"code","body_unavailable"},{"featureId",id},{"message","Shell target must be active earlier in history"}});
                SolidShell::evaluate(*target,parameters,id);
            } else if (type == "move") {
                const auto bodyId=parameters["targetBodyId"].toString();
                auto body=std::find_if(evaluated.bodies.begin(),evaluated.bodies.end(),[&](const auto& item){return item.bodyId==bodyId;});
                if(body==evaluated.bodies.end()){context["code"]="body_unavailable";context["message"]="Move body must be active earlier in history";throw EvaluationError(context);}
                const auto transform=moveTransform(parameters);
                BRepBuilderAPI_Transform builder(body->shape,transform,true);if(!builder.IsDone())throw std::runtime_error("Move transform failed");
                body->shape=builder.Shape();body->summary=summarize(body->shape);body->summary["lastModifierFeatureId"]=id;body->summary["operation"]="move";
                // Transported cap lineage is outside this bounded solid-body slice.
                body->supportFaces.clear();body->solidEdges.clear();
            } else if (type == "boolean") {
                const auto targetId = parameters["targetBodyId"].toString();
                const auto toolIds = parameters["toolBodyIds"].toArray();
                context["targetBodyId"] = targetId;context["toolBodyIds"] = toolIds;context["operation"] = parameters["operation"];
                auto findBody = [&](const QString& bodyId) {
                    return std::find_if(evaluated.bodies.begin(),evaluated.bodies.end(),[&](const auto& body){return body.bodyId == bodyId;});
                };
                auto target = findBody(targetId);
                if (target == evaluated.bodies.end()) {
                    context["code"] = "body_unavailable";context["message"] = "Boolean target must be an active body earlier in feature history";
                    throw EvaluationError(context);
                }
                std::vector<TopoDS_Shape> tools;
                for (const auto& toolId : toolIds) {
                    const auto tool = findBody(toolId.toString());
                    if (tool == evaluated.bodies.end()) {
                        context["code"] = "body_unavailable";context["bodyId"] = toolId;context["message"] = "Boolean tool must be an active body earlier in feature history";
                        throw EvaluationError(context);
                    }
                    tools.push_back(tool->shape);
                    if(parameters["operation"]=="join") {
                        for(auto face:tool->supportFaces){face.bodyId=targetId;target->supportFaces.push_back(face);}
                        target->solidEdges.insert(target->solidEdges.end(),tool->solidEdges.begin(),tool->solidEdges.end());
                    }
                }
                TopoDS_Shape shape;
                try {shape = booleanOperation(parameters["operation"].toString(),target->shape,tools,&target->supportFaces,&target->solidEdges);}
                catch (const EvaluationError& error) {
                    const auto details = error.diagnostics();
                    for (auto it = details.begin(); it != details.end(); ++it) context[it.key()] = it.value();
                    throw EvaluationError(context);
                }
                auto summary = summarize(shape);
                summary["operation"] = parameters["operation"];summary["lastModifierFeatureId"] = id;
                summary["producerFeatureId"] = target->featureId;summary["targetBodyId"] = targetId;
                summary["toolBodyIds"] = toolIds;summary["keepTools"] = parameters["keepTools"];
                summary["supportFaces"]=PlanarFaces::descriptors(target->supportFaces);
                summary["solidEdges"]=SolidFillet::descriptors(target->solidEdges);
                target->shape = shape;target->summary = summary;
                if (!parameters["keepTools"].toBool())
                    std::erase_if(evaluated.bodies,[&](const auto& body){return toolIds.contains(body.bodyId);});
            } else {
                TopoDS_Shape shape;
                QJsonObject summary;std::vector<EvaluatedSupportFace> supportFaces;std::vector<EvaluatedSolidEdge> solidEdges;
                if (type == "extrude") {
                    const auto sketchId = parameters["sketchId"].toString();
                    const bool plural=parameters.contains("profileIds");
                    const QJsonArray profileIds=plural ? parameters["profileIds"].toArray() : QJsonArray{parameters["profileId"]};
                    context["sketchId"] = sketchId;
                    context[plural ? "profileIds" : "profileId"] = plural ? QJsonValue(profileIds) : parameters["profileId"];
                    if (!priorSketches.contains(sketchId)) {
                        context["code"] = "source_unavailable"; context["message"] = "Extrusion source sketch must exist earlier in feature history";
                        throw EvaluationError(context);
                    }
                    const auto& source = evaluated.sketches[priorSketches.value(sketchId)];
                    std::vector<EvaluatedProfile> selected;
                    for(const auto& requested:profileIds){const auto found=std::find_if(source.profiles.begin(),source.profiles.end(),[&](const auto& profile){return profile.profileId==requested.toString();});
                        if(found==source.profiles.end()) {context["code"]="profile_unavailable";context["message"]="The referenced profile no longer exists; repair the sketch or select a current closed profile";
                            context["missingProfileId"]=requested;context["profileDiagnostics"]=source.summary["profileDiagnostics"];throw EvaluationError(context);}
                        selected.push_back(*found);}
                    if(parameters["operation"]=="join" && feature.contains("bodyResults")) {
                        std::vector<EvaluatedBody> participants;
                        const auto targets=parameters["targetBodyIds"].toArray();
                        for(const auto& targetId:targets) {
                            const auto found=std::find_if(evaluated.bodies.begin(),evaluated.bodies.end(),[&](const auto& body){return body.bodyId==targetId.toString();});
                            if(found==evaluated.bodies.end())reject("body_unavailable","Join requires active earlier participant bodies");
                            participants.push_back(*found);
                        }
                        auto joined=joinResults(feature,participants,selected);
                        std::erase_if(evaluated.bodies,[&](const auto& body){return targets.contains(body.bodyId);});
                        evaluated.bodies.insert(evaluated.bodies.end(),joined.bodies.begin(),joined.bodies.end());
                        evaluated.joinBodyResults[id]=joined.ownership;
                        evaluated.featureOrder.push_back(id);continue;
                    }
                    if(parameters["operation"]=="cut" || parameters["operation"]=="join") {
                        const bool joining=parameters["operation"]=="join";
                        const auto targets=parameters["targetBodyIds"].toArray();context["targetBodyIds"]=targets;context["operation"]=parameters["operation"];
                        if(targets.size()!=1)reject("unsupported_extrude_targets","Join/Cut requires one retained target body");
                        auto target=std::find_if(evaluated.bodies.begin(),evaluated.bodies.end(),[&](const auto& body){return body.bodyId==targets[0].toString();});
                        if(target==evaluated.bodies.end()) {context["code"]="body_unavailable";context["message"]="The exact extrusion target must be an active body earlier in feature history";throw EvaluationError(context);}
                        const double distance=parameters["distanceMm"].toDouble();std::vector<TopoDS_Shape> tools;std::vector<EvaluatedSupportFace> joinCaps;double area=0;QStringList sourceEntities;
                        for(const auto& profile:selected) {EvaluatedSupportFace cap;tools.push_back(extrusion(profile,distance,joining ? &cap : nullptr));if(!cap.face.IsNull())joinCaps.push_back(cap);area+=profile.summary["areaMm2"].toDouble();
                            for(const auto& entity:profile.summary["sourceEntityIds"].toArray())sourceEntities.append(entity.toString());}
                        const double previousVolume=target->summary["volumeMm3"].toDouble();
                        TopoDS_Shape cutShape;
                        try {cutShape=booleanOperation(joining ? "join" : "cut",target->shape,tools,&target->supportFaces,&target->solidEdges);}
                        catch(const EvaluationError& error){const auto details=error.diagnostics();for(auto it=details.begin();it!=details.end();++it)context[it.key()]=it.value();throw EvaluationError(context);}
                        auto cutSummary=summarize(cutShape);
                        const double removed=joining ? cutSummary["volumeMm3"].toDouble()-previousVolume : previousVolume-cutSummary["volumeMm3"].toDouble();
                        if(removed<=std::max(1e-6,previousVolume*1e-12)){context["code"]=joining ? "join_no_addition" : "cut_no_intersection";context["message"]=joining ? "Selected profile prisms do not add positive volume to the retained target" : "Selected profile prisms do not remove positive volume from the retained target";throw EvaluationError(context);}
                        cutSummary["operation"]=parameters["operation"];cutSummary["lastModifierFeatureId"]=id;cutSummary["producerFeatureId"]=target->featureId;
                        cutSummary["targetBodyIds"]=targets;cutSummary["sourceSketchId"]=sketchId;cutSummary["distanceMm"]=distance;
                        if(plural)cutSummary["sourceProfileIds"]=profileIds;else cutSummary["sourceProfileId"]=parameters["profileId"];
                        sourceEntities.removeDuplicates();sourceEntities.sort();QJsonArray refs;for(const auto& entity:sourceEntities)refs.append(entity);
                        cutSummary["sourceEntityIds"]=refs;cutSummary["sourceAreaMm2"]=area;cutSummary[joining ? "addedVolumeMm3" : "removedVolumeMm3"]=removed;
                        target->shape=cutShape;
                        if(joining && joinCaps.size()==1) {
                            auto cap=joinCaps.front();
                            // Retain only the independently named positive cap when the
                            // complete prism cap survives as a face of the joined solid.
                            for(TopExp_Explorer faces(cutShape,TopAbs_FACE);faces.More();faces.Next())if(faces.Current().IsSame(cap.face)) {
                                cap.producerFeatureId=id;cap.bodyId=target->bodyId;
                                cap.faceId=QUuid::createUuidV5(QUuid(id),QByteArray("alloy-face/extrude-end-cap/1")).toString(QUuid::WithoutBraces);
                                target->supportFaces.push_back(cap);
                                cutSummary["supportFaces"]=QJsonArray{QJsonObject{{"faceId",cap.faceId},{"bodyId",cap.bodyId},{"producerFeatureId",id},{"role",cap.role},{"planar",true},{"frame",cap.frame},{"edges",supportEdgeDescriptors(cap)}}};break;
                            }
                        }
                        cutSummary["supportFaces"]=PlanarFaces::descriptors(target->supportFaces);
                        cutSummary["solidEdges"]=SolidFillet::descriptors(target->solidEdges);
                        target->summary=cutSummary;
                        evaluated.featureOrder.push_back(id);continue;
                    }
                    const auto united=unionProfiles(selected);const double distance=parameters["distanceMm"].toDouble();
                    EvaluatedSupportFace cap;
                    shape=extrusion(united,distance,&cap,&solidEdges,id,&supportFaces);summary=summarize(shape);
                    summary["solidEdges"]=SolidFillet::descriptors(solidEdges);
                    if(!cap.face.IsNull()) {
                        cap.producerFeatureId=id;cap.bodyId=feature["bodyId"].toString();
                        cap.faceId=QUuid::createUuidV5(QUuid(id),QByteArray("alloy-face/extrude-end-cap/1")).toString(QUuid::WithoutBraces);
                        supportFaces.insert(supportFaces.begin(),cap);
                        summary["supportFaces"]=QJsonArray{QJsonObject{{"faceId",cap.faceId},{"bodyId",cap.bodyId},{"producerFeatureId",id},{"role",cap.role},{"planar",true},{"frame",cap.frame},{"edges",supportEdgeDescriptors(cap)}}};
                    }
                    for(auto& face:supportFaces)face.bodyId=feature["bodyId"].toString();
                    summary["supportFaces"]=PlanarFaces::descriptors(supportFaces);
                    summary["operation"]="extrude";summary["distanceMm"]=distance;summary["sourceSketchId"]=sketchId;
                    if(plural)summary["sourceProfileIds"]=profileIds;else summary["sourceProfileId"]=parameters["profileId"];
                    summary["sourceEntityIds"]=united.summary["sourceEntityIds"];summary["sourceAreaMm2"]=united.summary["areaMm2"];
                    summary["analyticVolumeMm3"]=united.summary["areaMm2"].toDouble()*std::abs(distance);
                } else {shape = primitive(type,parameters);summary = summarize(shape);}
                evaluated.bodies.push_back({id,feature["bodyId"].toString(),feature["name"].toString(),feature["visible"].toBool(),shape,summary,supportFaces,solidEdges});
            }
            evaluated.featureOrder.push_back(id);
        } catch (const EvaluationError&) {throw;}
          catch (const Standard_Failure& error) {
            context["code"] = "kernel_failure"; context["message"] = "OCCT feature evaluation failed: "+QString::fromUtf8(error.GetMessageString());
            throw EvaluationError(context);
        } catch (const std::exception& error) {
            context["code"] = "evaluation_failed"; context["message"] = QString::fromUtf8(error.what());
            throw EvaluationError(context);
        }
    }
    return evaluated;
}
QJsonObject GeometryKernel::policy() {
    return {{"kernel","OCCT"},{"version",ALLOY_OCCT_VERSION},{"canonicalLengthUnit","mm"},
        {"step",QJsonObject{{"importSource","embedded"},{"maximumSourceBytes",static_cast<qint64>(StepExchange::maximumSourceBytes)},
            {"importSolids",1},{"exportSolids",1},{"exportLengthUnits",QJsonArray{"mm","in"}},
            {"translator","OCCT STEPControl"}}},
        {"minimumDimensionMm",0.001},{"maximumDimensionMm",100000},
        {"referenceRoleToleranceMm",1e-6},{"analyticRelativeTolerance",1e-9},
        {"analyticAbsoluteTolerance",1e-6},{"automaticHealing",false},
        {"boolean",QJsonObject{{"targetIdentity","preserved"},{"inputs","explicit_earlier_active_body_ids"},
            {"hiddenInputsAllowed",true},{"resultSolids",1},{"emptyOrDisconnectedResult","reject_candidate"},
            {"toolGroup","union"},{"additionalToleranceMm",0.0},{"nonDestructive",true}}}};
}
