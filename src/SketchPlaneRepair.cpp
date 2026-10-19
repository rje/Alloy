// SPDX-License-Identifier: MIT
#include "SketchPlaneRepair.h"
#include "SketchGeometry.h"
#include <BRepAdaptor_Surface.hxx>
#include <QUuid>
#include <cmath>
namespace {
[[noreturn]] void fail(const QString& code,const QString& message){throw GeometryKernel::EvaluationError({{"code",code},{"message",message}});}
int position(const QJsonObject& authored,const QString& id) {const auto rows=authored.value("features").toArray();for(int i=0;i<rows.size();++i)if(rows.at(i).toObject().value("id")==id)return i;return -1;}
void earlier(const QJsonObject& authored,const QString& source,const QString& sketch) {
    const auto i=position(authored,source);if(i<0)fail("support_unavailable","The replacement producer does not exist");
    if(i>=position(authored,sketch))fail("later_support","The replacement must precede the sketch in history");
}
}
bool SketchPlaneRepair::ordinary(const QJsonObject& feature) {
    if(feature.value("type")!="sketch" || feature.value("parameters").toObject().value("plane")!="face")return false;
    const auto p=feature.value("parameters").toObject();
    for(const auto& key:{"projections","circleProjections","pointProjections","pointReferences","faceProjectionGroups","loopOffsets","openChainOffsets","closedRectOffsets","mirrorGroups","chamferGroups"})
        if(!p.value(key).toArray().isEmpty())return false;
    return true;
}
SketchPlaneRepair::Candidate SketchPlaneRepair::candidate(const QJsonObject& failed,const EvaluatedDocument& prefix,
    const QString& sketchId,const QJsonObject& replacement,const QJsonObject& acceptedFrame) {
    auto sketch=SketchGeometry::findFeature(failed,sketchId);
    if(!ordinary(sketch))fail("unsupported_sketch_repair","Plane repair requires an ordinary face sketch without linked or generated geometry");
    if(sketch.value("ownerId")!=failed.value("documentId"))fail("unsupported_sketch_origin_context","Plane repair currently requires the root modeling context");
    if(replacement.size()!=1)fail("invalid_replacement","Choose exactly one planar face, origin plane or construction plane");
    auto p=sketch.value("parameters").toObject();QJsonObject frame;
    if(replacement.contains("support")) {
        const auto support=replacement.value("support").toObject();earlier(failed,support.value("producerFeatureId").toString(),sketchId);
        int matches=0;
        for(const auto& body:prefix.bodies)if(body.bodyId==support.value("bodyId").toString()) {
            if(!body.occurrenceId.isEmpty() || body.ownerId!=failed.value("documentId").toString())fail("unsupported_sketch_origin_context","Choose a face in the root modeling context");
            for(const auto& face:body.supportFaces)
            if(face.faceId==support.value("faceId").toString() && face.producerFeatureId==support.value("producerFeatureId").toString()) {
                ++matches;if(!face.face.IsNull() && BRepAdaptor_Surface(face.face).GetType()!=GeomAbs_Plane)fail("nonplanar_support","Choose a planar face; curved faces cannot support this sketch");
            }
        }
        if(matches>1)fail("ambiguous_support","The replacement face is not unique");
        GeometryKernel::resolveSketchSupport(prefix,support);
        p["plane"]="face";p["support"]=support;p.remove("planeId");sketch["parameters"]=p;frame=GeometryKernel::sketchFrame(prefix,sketch);
    } else if(replacement.contains("planeId")) {
        const auto id=replacement.value("planeId").toString();earlier(failed,id,sketchId);
        for(const auto& plane:prefix.constructionPlanes)if(plane.featureId==id){if(plane.ownerId!=failed.value("documentId").toString())fail("unsupported_sketch_origin_context","Choose a plane in the root modeling context");frame=plane.frame;}
        if(frame.isEmpty())fail("plane_unavailable","The construction plane is unavailable before this sketch");
        p["plane"]="construction";p["planeId"]=id;p.remove("support");p.remove("originConvention");
    } else if(replacement.contains("plane")) {
        const auto plane=replacement.value("plane").toString();
        if(plane!="XY" && plane!="XZ" && plane!="YZ")fail("invalid_replacement","Choose an existing origin plane");
        frame=SketchGeometry::frame(plane);p["plane"]=plane;p.remove("planeId");p.remove("support");p.remove("originConvention");
    } else fail("invalid_replacement","Choose an explicit planar reference");
    for(const auto& key:{"uAxis","vAxis","normal"}) {
        const auto a=frame.value(key).toArray(),b=acceptedFrame.value(key).toArray();
        if(a.size()!=3 || b.size()!=3)fail("unsupported_plane_orientation","Replacement frame is unavailable");
        for(int i=0;i<3;++i)if(std::abs(a.at(i).toDouble()-b.at(i).toDouble())>1e-9)
            fail("unsupported_plane_orientation","Choose a parallel plane with matching sketch axes; side, inclined and reversed frames are not supported yet");
    }
    QSet<QString> ids;if(const auto error=SketchGeometry::validateParameters(p,ids);!error.isEmpty())fail("invalid_replacement",error);
    sketch["parameters"]=p;
    return {SketchGeometry::replaceFeature(failed,sketch),replacement,frame};
}
