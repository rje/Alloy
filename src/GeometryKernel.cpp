#include "GeometryKernel.h"
#include "SketchGeometry.h"
#include <BRepBndLib.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepGProp.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRep_Tool.hxx>
#include <Bnd_Box.hxx>
#include <GProp_GProps.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <gp_Vec.hxx>
#include <QJsonArray>
#include <QMap>
#include <Standard_Failure.hxx>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace {
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
TopoDS_Shape GeometryKernel::extrusion(const EvaluatedProfile& profile, double distanceMm) {
    if (!std::isfinite(distanceMm) || std::abs(distanceMm) < 0.001 || std::abs(distanceMm) > 100000)
        throw std::invalid_argument("Extrusion distance must be signed 0.001..100000 mm; zero is unsupported");
    if (profile.face.IsNull() || !BRepCheck_Analyzer(profile.face).IsValid())
        throw std::invalid_argument("Extrusion requires a current valid closed profile face");
    const auto normal = SketchGeometry::frame(profile.plane)["normal"].toArray();
    BRepPrimAPI_MakePrism prism(profile.face,gp_Vec(normal[0].toDouble()*distanceMm,
        normal[1].toDouble()*distanceMm,normal[2].toDouble()*distanceMm),true,true);
    if (!prism.IsDone()) throw std::runtime_error("OCCT could not build the profile extrusion");
    const auto shape = prism.Shape();
    if (shape.IsNull() || !BRepCheck_Analyzer(shape).IsValid() || count(shape,TopAbs_SOLID) != 1)
        throw std::runtime_error("Extrusion did not produce one valid solid");
    const auto summary = summarize(shape);
    const double expected = profile.summary["areaMm2"].toDouble()*std::abs(distanceMm);
    if (!summary["closed"].toBool() || !summary["volumeMm3"].isDouble() || !std::isfinite(expected)
        || expected <= 0 || std::abs(summary["volumeMm3"].toDouble()-expected) > std::max(1e-6,expected*1e-9))
        throw std::runtime_error("Extrusion volume does not match profile area times distance");
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
            if (type == "sketch") {
                priorSketches[id] = evaluated.sketches.size();
                evaluated.sketches.push_back(SketchGeometry::evaluateFeature(feature));
            } else {
                TopoDS_Shape shape;
                QJsonObject summary;
                if (type == "extrude") {
                    const auto sketchId = parameters["sketchId"].toString(), profileId = parameters["profileId"].toString();
                    context["sketchId"] = sketchId; context["profileId"] = profileId;
                    if (!priorSketches.contains(sketchId)) {
                        context["code"] = "source_unavailable"; context["message"] = "Extrusion source sketch must exist earlier in feature history";
                        throw EvaluationError(context);
                    }
                    const auto& source = evaluated.sketches[priorSketches.value(sketchId)];
                    const auto found = std::find_if(source.profiles.begin(),source.profiles.end(),[&](const auto& profile){return profile.profileId == profileId;});
                    if (found == source.profiles.end()) {
                        context["code"] = "profile_unavailable"; context["message"] = "The referenced profile no longer exists; repair the sketch or select a current closed profile";
                        context["profileDiagnostics"] = source.summary["profileDiagnostics"];
                        throw EvaluationError(context);
                    }
                    const double distance = parameters["distanceMm"].toDouble();
                    shape = extrusion(*found,distance); summary = summarize(shape);
                    summary["operation"] = "extrude"; summary["distanceMm"] = distance;
                    summary["sourceSketchId"] = sketchId; summary["sourceProfileId"] = profileId;
                    summary["sourceEntityIds"] = found->summary["sourceEntityIds"];
                    summary["analyticVolumeMm3"] = found->summary["areaMm2"].toDouble()*std::abs(distance);
                } else {shape = primitive(type,parameters);summary = summarize(shape);}
                evaluated.bodies.push_back({id,feature["bodyId"].toString(),feature["name"].toString(),feature["visible"].toBool(),shape,summary});
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
        {"minimumDimensionMm",0.001},{"maximumDimensionMm",100000},
        {"referenceRoleToleranceMm",1e-6},{"analyticRelativeTolerance",1e-9},
        {"analyticAbsoluteTolerance",1e-6},{"automaticHealing",false}};
}
