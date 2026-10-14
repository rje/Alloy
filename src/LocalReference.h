// SPDX-License-Identifier: MIT
#pragma once
#include "GeometryKernel.h"
#include "PlanarFaces.h"
#include "SolidFillet.h"
#include <TopLoc_Location.hxx>
#include <TopoDS.hxx>
#include <gp_Ax1.hxx>
#include <gp_Trsf.hxx>
#include <gp.hxx>
#include <QUuid>
#include <cmath>
#include <stdexcept>

namespace LocalReference {
inline QJsonObject identityPlacement(){return {{"axis","Z"},{"angleDeg",0},{"translationMm",QJsonArray{0,0,0}}};}
inline QString outerOccurrenceId(const QString& featureId){return QUuid::createUuidV5(QUuid(featureId),"local-reference-outer").toString(QUuid::WithoutBraces);}
inline gp_Trsf transform(const QJsonObject& p){
    const auto axis=p.value("axis").toString();const auto t=p.value("translationMm").toArray();
    if(p.size()!=3 || (axis!="X" && axis!="Y" && axis!="Z") || !p.value("angleDeg").isDouble() || t.size()!=3)
        throw std::invalid_argument("Reference placement needs axis, angle and XYZ translation");
    const double angle=p.value("angleDeg").toDouble();
    if(!std::isfinite(angle) || std::abs(angle)>360000)throw std::invalid_argument("Reference angle is outside the supported range");
    for(const auto& v:t)if(!v.isDouble() || !std::isfinite(v.toDouble()) || std::abs(v.toDouble())>100000)
        throw std::invalid_argument("Reference translation must be finite within 100000 mm");
    const gp_Dir direction=axis=="X" ? gp::DX() : axis=="Y" ? gp::DY() : gp::DZ();
    gp_Trsf rotation;rotation.SetRotation(gp_Ax1(gp_Pnt(0,0,0),direction),angle*std::acos(-1.0)/180.0);
    gp_Trsf translation;translation.SetTranslation(gp_Vec(t[0].toDouble(),t[1].toDouble(),t[2].toDouble()));
    return translation*rotation;
}
inline QJsonArray coordinates(const QJsonArray& values,const gp_Trsf& trsf,bool vector=false){
    if(values.size()!=3)return values;
    const double x=values[0].toDouble(),y=values[1].toDouble(),z=values[2].toDouble();
    if(vector){const auto d=gp_Vec(x,y,z).Transformed(trsf);return {d.X(),d.Y(),d.Z()};}
    const auto p=gp_Pnt(x,y,z).Transformed(trsf);return {p.X(),p.Y(),p.Z()};
}
inline EvaluatedBody placed(EvaluatedBody body,const gp_Trsf& trsf){
    const TopLoc_Location location(trsf);
    body.shape=body.shape.Moved(location);
    for(auto& face:body.supportFaces){
        face.face=TopoDS::Face(face.face.Moved(location));
        for(const auto& key:{"origin","uAxis","vAxis","normal"})if(face.frame.contains(key))
            face.frame[key]=coordinates(face.frame[key].toArray(),trsf,QString(key)!="origin");
        for(auto& edge:face.boundaryEdges){edge.edge=TopoDS::Edge(edge.edge.Moved(location));
            for(const auto& key:{"startMm","endMm"})if(edge.provenance.contains(key))edge.provenance[key]=coordinates(edge.provenance[key].toArray(),trsf);}
    }
    for(auto& edge:body.solidEdges){edge.edge=TopoDS::Edge(edge.edge.Moved(location));
        for(const auto& key:{"startMm","endMm"})if(edge.provenance.contains(key))edge.provenance[key]=coordinates(edge.provenance[key].toArray(),trsf);}
    const auto original=body.summary;
    body.summary=GeometryKernel::summarize(body.shape);
    for(const auto& key:{"sourceFormat","sourcePolicy","sourceSha256","sourceOriginalName","sourceLengthUnits","translator"})
        if(original.contains(key))body.summary[key]=original[key];
    body.summary["supportFaces"]=PlanarFaces::descriptors(body.supportFaces);
    body.summary["solidEdges"]=SolidFillet::descriptors(body.solidEdges);
    return body;
}
}
