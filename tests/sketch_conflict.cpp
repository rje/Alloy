// SPDX-License-Identifier: MIT
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QUuid>
#include <cmath>
#include <iostream>
#include <stdexcept>
static void require(bool ok,const QString& message){if(!ok)throw std::runtime_error(message.toStdString());}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    auto draft=SketchGeometry::newFeature(QUuid::createUuid().toString(QUuid::WithoutBraces),"XY");
    draft=SketchGeometry::addLine(draft,{{"u1Mm",0},{"v1Mm",0},{"u2Mm",40},{"v2Mm",0}});
    const auto line=draft.value("parameters").toObject().value("entities").toArray().last().toObject();
    int phase=0;auto accept=[&](const SketchGeometry::SolveResult& r){++phase;require(r.ok,QString("phase %1: ").arg(phase)+r.error);draft=r.feature;};
    accept(SketchGeometry::addConstraint(draft,{{"type","fixed"},{"pointIds",QJsonArray{line.value("startPointId")}}}));
    accept(SketchGeometry::addDimension(draft,{{"kind","linear"},{"pointIds",QJsonArray{line.value("startPointId"),line.value("endPointId")}},{"valueMm",40},{"driving",true}}));
    accept(SketchGeometry::addConstraint(draft,{{"type","horizontal"},{"entityIds",QJsonArray{line.value("id")}}}));
    const auto baseline=draft;const auto existing=draft.value("parameters").toObject().value("constraints").toArray().last().toObject();
    const auto rejected=SketchGeometry::addConstraint(draft,{{"type","vertical"},{"entityIds",QJsonArray{line.value("id")}}});
    require(!rejected.ok && rejected.feature==baseline,"failed candidate mutated draft");
    const auto d=rejected.diagnostics;
    require(d.value("code")=="constraint_conflict","missing certified conflict");
    require(d.value("conflictingRelations").toArray()==QJsonArray{existing},"wrong existing relation");
    require(d.value("attemptedRelation").toObject().value("type")=="vertical","wrong attempted kind");
    require(d.value("conflictingEntityIds").toArray()==QJsonArray{line.value("id")},"wrong line");
    require(d.value("supportingDimensions").toArray().size()==1,"missing positive length evidence");
    auto p=draft.value("parameters").toObject();auto cs=p.value("constraints").toArray();cs.removeLast();p["constraints"]=cs;draft["parameters"]=p;
    accept(SketchGeometry::addConstraint(draft,{{"type","vertical"},{"entityIds",QJsonArray{line.value("id")}}}));
    const auto points=draft.value("parameters").toObject().value("points").toArray();
    QJsonObject end;for(const auto& row:points)if(row.toObject().value("id")==line.value("endPointId"))end=row.toObject();
    require(std::abs(end.value("uMm").toDouble())<1e-6 && std::abs(end.value("vMm").toDouble()-40)<1e-6,"Vertical repair did not retain positive 40mm length");
    // Ordinary inverse orientation switch, with unrelated geometry unchanged.
    p=draft.value("parameters").toObject();cs=p.value("constraints").toArray();cs.removeLast();p["constraints"]=cs;draft["parameters"]=p;
    draft=SketchGeometry::addLine(draft,{{"u1Mm",70},{"v1Mm",20},{"u2Mm",85},{"v2Mm",25}});
    const auto unrelated=draft.value("parameters").toObject().value("points").toArray();
    accept(SketchGeometry::addConstraint(draft,{{"type","horizontal"},{"entityIds",QJsonArray{line.value("id")}}}));
    const auto switched=draft.value("parameters").toObject().value("points").toArray();
    require(switched[2]==unrelated[2] && switched[3]==unrelated[3],"orientation retry moved unrelated geometry");
    require(std::abs(switched[1].toObject().value("uMm").toDouble()-40)<1e-6,"inverse Horizontal switch failed");
    // A locked endpoint cannot be repaired by changing only the solver seed.
    p=baseline.value("parameters").toObject();p["dimensions"]=QJsonArray{};cs=p.value("constraints").toArray();cs.removeLast();
    cs.append(QJsonObject{{"id",QUuid::createUuid().toString(QUuid::WithoutBraces)},{"type","fixed"},{"pointIds",QJsonArray{line.value("endPointId")}},{"uMm",40},{"vMm",0}});
    p["constraints"]=cs;auto locked=baseline;locked["parameters"]=p;
    const auto impossible=SketchGeometry::addConstraint(locked,{{"type","vertical"},{"entityIds",QJsonArray{line.value("id")}}});
    require(!impossible.ok && impossible.feature==locked && impossible.diagnostics.value("code")!="constraint_conflict","seed bypassed fixed endpoint or invented certificate");
    // Without a positive driving length this certificate must not be claimed.
    p=baseline.value("parameters").toObject();p["dimensions"]=QJsonArray{};auto free=baseline;free["parameters"]=p;
    require(SketchGeometry::addConstraint(free,{{"type","vertical"},{"entityIds",QJsonArray{line.value("id")}}}).diagnostics.value("code")!="constraint_conflict","uncertified collapsed-line claim");
    std::cout<<"PASS: certified direct orientation conflict, immutable failure and vertical repair\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
