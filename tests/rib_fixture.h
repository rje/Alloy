// SPDX-License-Identifier: MIT
#pragma once
#include "CadDocument.h"
#include "SketchGeometry.h"
#include <cmath>
#include <stdexcept>
namespace RibFixture {
inline void check(bool ok,const QString& message){if(!ok)throw std::runtime_error(message.toStdString());}
inline void commit(CadDocument& d,const QJsonObject& a){const auto error=d.commit(a,"Fixture");check(error.isEmpty(),error);}
inline QJsonObject make(CadDocument& d){
 auto a=d.authored();QJsonArray features;QStringList bodies;QString distal;
 for(int i=0;i<3;++i){const auto id=CadDocument::newId(),body=CadDocument::newId();bodies<<body;if(i==2)distal=id;features.append(QJsonObject{{"id",id},{"bodyId",body},{"ownerId",d.activeOwnerId()},{"type","box"},{"name",i==0?"U channel":"Wall"},{"visible",true},{"parameters",QJsonObject{{"widthMm",40.},{"depthMm",i==0?30.:2.},{"heightMm",i==0?2.:21.},{"xMm",0.},{"yMm",i==2?28.:0.},{"zMm",i==0?0.:1.}}}});}
 a["features"]=features;commit(d,a);commit(d,CadDocument::booleanCandidate(d.authored(),{{"targetBodyId",bodies[0]},{"toolBodyIds",QJsonArray{bodies[1],bodies[2]}},{"operation","join"},{"keepTools",false}}));
 a=d.authored();features=a["features"].toArray();const auto planeId=CadDocument::newId();features.append(QJsonObject{{"id",planeId},{"ownerId",d.activeOwnerId()},{"type","constructionPlane"},{"name","Rib plane"},{"visible",true},{"parameters",QJsonObject{{"source",QJsonObject{{"plane","XY"}}},{"offsetMm",4.}}}});a["features"]=features;commit(d,a);
 auto sketch=SketchGeometry::newConstructionPlaneFeature(d.activeOwnerId(),planeId,"Rib Line");sketch=SketchGeometry::addLine(sketch,{{"u1Mm",2.},{"v1Mm",1.},{"u2Mm",38.},{"v2Mm",1.}});commit(d,SketchGeometry::replaceFeature(d.authored(),sketch));
 const auto line=sketch["parameters"].toObject()["entities"].toArray().first().toObject().value("id");
 return {{"sketchId",sketch["id"]},{"sourceEntityId",line},{"targetBodyId",bodies[0]},{"thicknessMm",2.},{"farFeatureId",distal}};
}
}
