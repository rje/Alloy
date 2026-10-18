// SPDX-License-Identifier: MIT
#pragma once
#include "multi_face_fixtures.h"
#include "RoundedLoopOffset.h"
namespace FailedEditRepairFixtures {
inline QJsonObject create(const QString& folder) {
    auto metadata=MultiFaceFixtures::create(folder);const auto ids=metadata.value("failure").toObject();
    CadDocument doc;MultiFaceFixtures::check(doc.open(folder+"/support-failure.alloy").isEmpty(),"support fixture open");
    // Keep the initial Combine's top cap untouched, so the accepted fixture can
    // own a fully named loop. The same split/delete tools still lose that cap.
    auto authored=doc.authored();auto features=authored.value("features").toArray();
    for(int i=0;i<features.size();++i) {
        auto feature=features.at(i).toObject();
        if(feature.value("type")=="box" && feature.value("name")=="Notch tool") {
            auto p=feature.value("parameters").toObject();p["heightMm"]=2;feature["parameters"]=p;features[i]=feature;
        }
    }
    authored["features"]=features;MultiFaceFixtures::commit(doc,authored);
    const auto support=GeometryKernel::resolveSketchSupport(doc.evaluation(),ids.value("support").toObject());
    auto sketch=SketchGeometry::newSupportedFeature(doc.activeOwnerId(),ids.value("support").toObject(),"Cached support loop",{});
    sketch["id"]=ids.value("sketchId");auto parameters=sketch.value("parameters").toObject();
    parameters["loopOffsets"]=QJsonArray{QJsonObject{{"id",CadDocument::newId()},{"kind","planarInsetV1"},
        {"outerLoopId",RoundedLoopOffset::insetLoopId(support)},{"distanceMm",-1},{"chain",true}}};sketch["parameters"]=parameters;
    MultiFaceFixtures::commit(doc,SketchGeometry::replaceFeature(doc.authored(),sketch));
    MultiFaceFixtures::check(doc.save(folder+"/support-failure.alloy").isEmpty(),"loop fixture save");
    QFile file(folder+"/fixtures.json");MultiFaceFixtures::check(file.open(QIODevice::WriteOnly),"loop fixture metadata");
    file.write(QJsonDocument(metadata).toJson(QJsonDocument::Indented));return metadata;
}
}
