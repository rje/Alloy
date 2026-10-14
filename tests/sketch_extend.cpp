// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
#include <stdexcept>

static void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    CadDocument document;
    auto sketch=SketchGeometry::newFeature(document.authored().value("documentId").toString(),"XY","Open corner");
    for(const auto& segment:QVector<QJsonArray>{
        QJsonArray{0,0,20,0},QJsonArray{20,0,20,8},QJsonArray{20,10,0,10},QJsonArray{0,10,0,0}})
        sketch=SketchGeometry::addLine(sketch,{{"u1Mm",segment[0]},{"v1Mm",segment[1]},
            {"u2Mm",segment[2]},{"v2Mm",segment[3]}});
    const auto original=sketch;
    const auto source=sketch.value("parameters").toObject().value("entities").toArray()[1].toObject();
    const auto target=sketch.value("parameters").toObject().value("entities").toArray()[2].toObject();
    const auto sourceId=source.value("id").toString(),movingId=source.value("endPointId").toString();
    const auto missing=SketchGeometry::extendLine(sketch,{{"entityId",sourceId},{"uMm",20},{"vMm",.5}});
    require(!missing.ok && missing.feature==original,"No forward hit leaves the entire draft unchanged");
    const auto fartherOverlap=SketchGeometry::addLine(sketch,{{"u1Mm",20},{"v1Mm",12},{"u2Mm",20},{"v2Mm",14}});
    require(SketchGeometry::extendLine(fartherOverlap,{{"entityId",sourceId},{"uMm",20},{"vMm",7.5}}).ok,
        "A farther collinear segment does not hide the nearer unique corner");
    const auto result=SketchGeometry::extendLine(sketch,{{"entityId",sourceId},{"uMm",20},{"vMm",7.5}});
    require(result.ok,qPrintable(result.error));sketch=result.feature;
    const auto segment=result.diagnostics.value("extendedSegment").toObject();
    require(segment.value("movingPointId")==movingId && segment.value("targetEntityId")==target.value("id")
        && segment.value("targetPointId")==target.value("startPointId") &&
        std::abs(segment.value("v2Mm").toDouble()-10)<1e-7,"Exact retained endpoint meets the top corner");
    const auto after=sketch.value("parameters").toObject();
    require(after.value("entities").toArray()[1].toObject().value("id")==sourceId &&
        after.value("entities").toArray()[1].toObject().value("endPointId")==movingId,
        "Extend retains source line and endpoint identities");
    const auto relation=after.value("constraints").toArray().last().toObject();
    require(relation.value("type")=="coincident" && relation.value("pointIds").toArray()==QJsonArray{movingId,target.value("startPointId")},
        "Extension retains exact target endpoint identity");
    auto authored=SketchGeometry::replaceFeature(document.authored(),sketch);
    require(document.commit(authored,"Extend corner").isEmpty(),"Document accepts the extended sketch");
    const auto profiles=document.sketches().front().profiles;
    require(profiles.size()==1 && std::abs(profiles.front().summary.value("areaMm2").toDouble()-200)<1e-6,
        "Extended lines form one exact 20 by 10 profile");
    document.history().undo();require(document.authored().value("features").toArray().isEmpty(),"Undo removes the accepted sketch");
    document.history().redo();require(document.authored()==authored,"Redo restores exact authored identities");
    QTemporaryDir dir;const auto file=dir.filePath("extended.alloy");
    require(document.save(file).isEmpty(),"Save succeeds");CadDocument reopened;
    require(reopened.open(file).isEmpty() && reopened.authored()==authored,"Reopen preserves identities and relation");
    std::cout<<"Straight-line Extend core passed\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
