// SPDX-License-Identifier: MIT
#pragma once
#include "multi_face_fixtures.h"
namespace SketchPlaneRepairFixtures {
inline QJsonObject create(const QString& folder) {
    using namespace MultiFaceFixtures;
    const auto meta=MultiFaceFixtures::create(folder);auto ids=meta.value("failure").toObject();CadDocument doc;check(doc.open(folder+"/support-failure.alloy").isEmpty(),"ordinary open");
    const auto reference=box(doc,"Replacement support",0,40,0,50,25,8);
    QString anchor;for(const auto& row:doc.authored().value("features").toArray()){const auto f=row.toObject();if(f.value("id")==ids.value("combineFeatureId"))break;anchor=f.value("id").toString();}
    commit(doc,CadDocument::reorderCandidate(doc.authored(),reference.value("id").toString(),anchor));
    auto a=doc.authored();auto rows=a.value("features").toArray();for(int i=0;i<rows.size();++i){auto f=rows.at(i).toObject();if(f.value("id")==reference.value("id")){f["visible"]=true;rows[i]=f;}}a["features"]=rows;commit(doc,a);
    const auto root=doc.authored().value("documentId");const auto planeId=CadDocument::newId();
    QJsonObject plane{{"id",planeId},{"ownerId",root},{"type","constructionPlane"},{"name","Repair plane"},{"visible",true},{"parameters",QJsonObject{{"source",QJsonObject{{"plane","XY"}}},{"offsetMm",8}}}};
    a=doc.authored();rows=a.value("features").toArray();rows.append(plane);a["features"]=rows;commit(doc,a);commit(doc,CadDocument::reorderCandidate(doc.authored(),planeId,reference.value("id").toString()));
    const auto cylinderId=CadDocument::newId(),cylinderBody=CadDocument::newId();
    QJsonObject cylinder{{"id",cylinderId},{"bodyId",cylinderBody},{"ownerId",root},{"type","cylinder"},{"name","Curved candidate"},{"visible",true},{"parameters",QJsonObject{{"xMm",70},{"yMm",0},{"zMm",0},{"radiusMm",5},{"heightMm",8}}}};
    a=doc.authored();rows=a.value("features").toArray();rows.append(cylinder);a["features"]=rows;commit(doc,a);commit(doc,CadDocument::reorderCandidate(doc.authored(),cylinderId,planeId));
    const auto sketch=SketchGeometry::findFeature(doc.authored(),ids.value("sketchId").toString());
    commit(doc,CadDocument::extrusionCandidate(doc.authored(),{{"sketchId",sketch.value("id")},{"profileId",doc.sketches().back().profiles.front().profileId},{"distanceMm",2}}));
    ids["downstreamFeatureId"]=doc.authored().value("features").toArray().last().toObject().value("id");
    const auto later=box(doc,"Later candidate",70,40,0,10,10,4);
    for(const auto& body:doc.bodies())for(const auto& face:body.supportFaces){if(body.bodyId==reference.value("bodyId").toString() && face.role=="box.z1")ids["replacementSupport"]=ref(face);if(body.bodyId==later.value("bodyId").toString() && face.role=="box.z1")ids["laterSupport"]=ref(face);if(body.bodyId==cylinderBody)ids["curvedSupport"]=ref(face);}
    ids["constructionPlaneId"]=planeId;ids["referenceFeatureId"]=reference.value("id");
    check(doc.save(folder+"/ordinary-plane-repair.alloy").isEmpty(),"ordinary repair save");QFile file(folder+"/repair-fixtures.json");check(file.open(QIODevice::WriteOnly),"repair metadata");file.write(QJsonDocument(ids).toJson(QJsonDocument::Indented));return ids;
}
}
