// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <cmath>
#include <iostream>
#include <stdexcept>

static void require(bool ok,const QString& why){if(!ok)throw std::runtime_error(why.toStdString());}
int main(int argc,char** argv){
    QCoreApplication app(argc,argv);
    try{
        CadDocument document;
        auto draft=SketchGeometry::newFeature(document.authored().value("documentId").toString(),"XY");
        draft=SketchGeometry::addRectangle(draft,{{"u1Mm",0},{"v1Mm",0},{"u2Mm",40},{"v2Mm",25}});
        const auto entities=draft.value("parameters").toObject().value("entities").toArray();
        const auto bottom=entities[0].toObject(),right=entities[1].toObject();
        auto solved=SketchGeometry::addConstraint(draft,{{"type","fixed"},{"pointIds",QJsonArray{bottom.value("startPointId")}}});
        require(solved.ok,"anchored rectangle");draft=solved.feature;
        solved=SketchGeometry::addDimension(draft,{{"kind","horizontal"},{"entityId",bottom.value("id")},{"valueMm",40}});
        require(solved.ok,"driving width");draft=solved.feature;
        solved=SketchGeometry::addDimension(draft,{{"kind","vertical"},{"entityId",right.value("id")},{"valueMm",25}});
        require(solved.ok,"driving height");draft=solved.feature;
        const QJsonObject matching{{"kind","horizontal"},{"entityId",bottom.value("id")},{"valueMm",40}};
        const auto redundant=SketchGeometry::addDimension(draft,matching);
        require(!redundant.ok && redundant.feature==draft,"duplicate driver refuses atomically");
        const auto offer=SketchGeometry::drivenAlternative(draft,matching,redundant);
        require(offer && offer->ok,"matching redundant driver offers measurement");
        const auto dimensions=offer->feature.value("parameters").toObject().value("dimensions").toArray();
        require(dimensions.size()==3 && !dimensions.last().toObject().value("driving").toBool(),"only new dimension is driven");
        const QJsonObject conflicting{{"kind","horizontal"},{"entityId",bottom.value("id")},{"valueMm",41}};
        const auto rejected=SketchGeometry::addDimension(draft,conflicting);
        require(!rejected.ok && !SketchGeometry::drivenAlternative(draft,conflicting,rejected),"conflicting value has no offer");
        const auto width=dimensions.first().toObject();const auto id=width.value("id").toString();
        auto converted=SketchGeometry::setDimensionDriving(draft,id,false);
        require(converted.ok,converted.error);
        auto convertedRow=converted.feature.value("parameters").toObject().value("dimensions").toArray().first().toObject();
        auto expected=width;expected["driving"]=false;expected.remove("valueMm");
        require(convertedRow==expected,"conversion preserves identity, name and references");
        require(converted.feature.value("parameters").toObject().value("points")==draft.value("parameters").toObject().value("points"),"conversion preserves geometry");
        auto expressionDraft=SketchGeometry::updateDimension(draft,{{"dimensionId",id},{"expression","20 + 20"},{"expressionUnits","mm"}});
        require(expressionDraft.ok,expressionDraft.error);
        const auto unbound=SketchGeometry::setDimensionDriving(expressionDraft.feature,id,false);
        require(unbound.ok && !unbound.feature.value("parameters").toObject().value("dimensions").toArray().first().toObject().contains("expression"),"own expression removed on conversion");
        const auto heightId=dimensions[1].toObject().value("id");
        const auto widthName=width.value("name").toString();
        auto dependent=SketchGeometry::updateDimension(draft,{{"dimensionId",heightId},{"expression",widthName+" / 2"},{"expressionUnits","mm"}});
        require(dependent.ok,dependent.error);
        const auto refusedDependency=SketchGeometry::setDimensionDriving(dependent.feature,id,false);
        require(!refusedDependency.ok && refusedDependency.feature==dependent.feature && refusedDependency.error.contains("Another dimension expression"),"dependent-expression conversion refuses with guidance");
        const auto restored=SketchGeometry::setDimensionDriving(converted.feature,id,true);
        require(restored.ok,restored.error);require(restored.feature==draft,"reciprocal conversion preserves exact intent");
        const auto redundantBack=SketchGeometry::setDimensionDriving(offer->feature,dimensions.last().toObject().value("id").toString(),true);
        require(!redundantBack.ok && redundantBack.feature==offer->feature,"redundant reciprocal driver refuses atomically");
        auto circle=SketchGeometry::addCircle(SketchGeometry::newFeature(document.authored().value("documentId").toString(),"XY"),{{"centerUMm",10},{"centerVMm",10},{"diameterMm",10}});
        auto diameter=SketchGeometry::addDimension(circle,{{"kind","diameter"},{"entityId",circle.value("parameters").toObject().value("entities").toArray().first().toObject().value("id")},{"valueMm",10}});
        require(diameter.ok,"circle driver");const auto circleId=diameter.feature.value("parameters").toObject().value("dimensions").toArray().first().toObject().value("id").toString();
        auto drivenCircle=SketchGeometry::setDimensionDriving(diameter.feature,circleId,false);require(drivenCircle.ok,drivenCircle.error);
        require(SketchGeometry::setDimensionDriving(drivenCircle.feature,circleId,true).ok,"circle reciprocal driver");
        std::cout<<"sketch driven offer: redundant matching accepted, conflict refused\n";
        return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
