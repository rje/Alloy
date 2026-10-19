// SPDX-License-Identifier: MIT
#include "SketchPlaneRepair.h"
#include "SketchGeometry.h"
#include "RoundedLoopOffset.h"
#include <QJsonDocument>
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

bool SketchPlaneRepair::singleInset(const QJsonObject& feature) {
    auto plain=feature;auto p=plain.value("parameters").toObject();const auto rows=p.value("loopOffsets").toArray();
    if(rows.size()!=1 || rows.first().toObject().value("kind")!="planarInsetV1" || !RoundedLoopOffset::validate(p).isEmpty())return false;
    p.remove("loopOffsets");plain["parameters"]=p;return ordinary(plain);
}
SketchPlaneRepair::Candidate SketchPlaneRepair::recreateInset(const QJsonObject& failed,const EvaluatedDocument& prefix,
    const QString& sketchId,const QJsonObject& replacement,const QJsonObject& acceptedFrame,const QSet<QString>& oldGeneratedIds) {
    const auto original=SketchGeometry::findFeature(failed,sketchId);
    if(!singleInset(original))fail("unsupported_sketch_repair","Recreation requires exactly one planar inset owner without linked geometry");
    int owners=0;for(const auto& f:failed.value("features").toArray())owners+=f.toObject().value("parameters").toObject().value("loopOffsets").toArray().size();
    if(owners!=1)fail("unsupported_sketch_repair","First recreation scope requires one loop Offset owner in the document");
    if(replacement.size()!=2 || !replacement.contains("support") || !replacement.value("distanceMm").isDouble())
        fail("offset_confirmation_required","Select an explicit face boundary and confirm its signed Offset distance");
    const auto distance=replacement.value("distanceMm").toDouble();if(!std::isfinite(distance) || distance>=-1e-7)
        fail("invalid_offset_distance","Planar inset recreation requires a finite negative signed distance");
    const auto parameters=original.value("parameters").toObject();const auto oldOwner=parameters.value("loopOffsets").toArray().first().toObject();
    const auto support=replacement.value("support").toObject();
    if(support==parameters.value("support").toObject())fail("distinct_support_required","Select a distinct replacement face; same-face boundary repair is separate");
    // Generated profile and edge references contain owner-derived UUIDs. Reject
    // consumers of this sketch's generated outputs before removing its owner.
    const auto acceptedProfile=QUuid::createUuidV5(QUuid(oldOwner.value("id").toString()),"alloy-inset-interior/1").toString(QUuid::WithoutBraces);
    for(const auto& f:failed.value("features").toArray()) {
        const auto row=f.toObject();if(position(failed,row.value("id").toString())<=position(failed,sketchId))continue;
        const auto bytes=QJsonDocument(row).toJson(QJsonDocument::Compact);
        bool consumes=bytes.contains(acceptedProfile.toUtf8()) || bytes.contains(sketchId.toUtf8());
        for(const auto& id:oldGeneratedIds)if(!id.isEmpty() && bytes.contains(id.toUtf8()))consumes=true;
        if(consumes)
            fail("generated_profile_consumer","A downstream feature consumes this generated sketch. Repair that consumer separately before recreating the Offset");
    }
    auto plain=original;auto p=parameters;p.remove("loopOffsets");plain["parameters"]=p;
    const auto ordinaryFailed=SketchGeometry::replaceFeature(failed,plain);
    auto result=candidate(ordinaryFailed,prefix,sketchId,{{"support",support}},acceptedFrame);
    const auto face=GeometryKernel::resolveSketchSupport(prefix,support);
    const auto loop=RoundedLoopOffset::insetLoopId(face);
    if(loop==oldOwner.value("outerLoopId").toString())fail("distinct_boundary_required","The replacement must have a distinct exact boundary");
    auto changed=SketchGeometry::findFeature(result.authored,sketchId);p=changed.value("parameters").toObject();
    // Recreate intent, retaining no old owner identity or implicit binding.
    p["loopOffsets"]=QJsonArray{QJsonObject{{"kind","planarInsetV1"},{"id",QUuid::createUuid().toString(QUuid::WithoutBraces)},{"outerLoopId",loop},{"distanceMm",distance},{"chain",true}}};
    changed["parameters"]=p;result.authored=SketchGeometry::replaceFeature(result.authored,changed);result.reference=replacement;return result;
}
