// SPDX-License-Identifier: MIT
#pragma once
#include "multi_face_fixtures.h"
#include "RoundedLoopOffset.h"
namespace GeneratedLoopRepairFixtures {
inline QJsonObject replaceRow(QJsonObject authored,const QJsonObject& feature) {auto rows=authored.value("features").toArray();for(int i=0;i<rows.size();++i)if(rows.at(i).toObject().value("id")==feature.value("id"))rows[i]=feature;authored["features"]=rows;return authored;}
inline QJsonObject sourcePointEdit(const QJsonObject& authored) {
    auto source=authored.value("features").toArray().first().toObject();
    auto bytes=QJsonDocument(source.value("parameters").toObject()).toJson(QJsonDocument::Compact);
    for(const auto& point:source.value("parameters").toObject().value("points").toArray())bytes.replace(point.toObject().value("id").toString().toUtf8(),CadDocument::newId().toUtf8());
    const auto parameters=QJsonDocument::fromJson(bytes).object();source["parameters"]=parameters;
    return {{"candidate",replaceRow(authored,source)},{"arguments",QJsonObject{{"featureId",source.value("id")},{"parameters",parameters}}}};
}
inline QJsonObject create(const QString& folder) {
 using namespace MultiFaceFixtures;
 auto ids=MultiFaceFixtures::create(folder).value("failure").toObject();CadDocument doc;check(doc.open(folder+"/support-failure.alloy").isEmpty(),"open source");
 QJsonObject notch;
 for(const auto& f:doc.authored().value("features").toArray())if(f.toObject().value("name")=="Notch tool")notch=f.toObject();
 auto p=notch.value("parameters").toObject();p["heightMm"]=2;notch["parameters"]=p;commit(doc,replaceRow(doc.authored(),notch));
 QJsonObject split;for(const auto& f:doc.authored().value("features").toArray())if(f.toObject().value("name")=="Split tool")split=f.toObject();p=split.value("parameters").toObject();p["xMm"]=49;split["parameters"]=p;commit(doc,replaceRow(doc.authored(),split));
 auto old=SketchGeometry::findFeature(doc.authored(),ids.value("sketchId").toString());const auto support=GeometryKernel::resolveSketchSupport(doc.evaluationBefore(ids.value("sketchId").toString()),ids.value("support").toObject());
 auto generated=SketchGeometry::newSupportedFeature(doc.authored().value("documentId").toString(),ids.value("support").toObject(),"Cached support loop");generated["id"]=old.value("id");p=generated.value("parameters").toObject();p["loopOffsets"]=QJsonArray{QJsonObject{{"id",CadDocument::newId()},{"kind","planarInsetV1"},{"outerLoopId",RoundedLoopOffset::insetLoopId(support)},{"distanceMm",-1},{"chain",true}}};generated["parameters"]=p;commit(doc,SketchGeometry::replaceFeature(doc.authored(),generated));
 const auto replacement=box(doc,"Replacement support",65,0,0,40,25,5);commit(doc,CadDocument::reorderCandidate(doc.authored(),replacement.value("id").toString(),ids.value("combineFeatureId").toString()));
 for(const auto& body:doc.bodies())if(body.bodyId==replacement.value("bodyId"))for(const auto& face:body.supportFaces)if(face.role=="box.z1")ids["replacementSupport"]=ref(face);
 check(doc.save(folder+"/generated-loop-repair.alloy").isEmpty(),"save generated fixture");return ids;
}
}
