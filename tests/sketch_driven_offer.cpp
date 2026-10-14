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
        std::cout<<"sketch driven offer: redundant matching accepted, conflict refused\n";
        return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
