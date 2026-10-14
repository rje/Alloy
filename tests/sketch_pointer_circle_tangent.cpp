// SPDX-License-Identifier: MIT
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
QJsonObject accept(const SketchGeometry::SolveResult& result){if(!result.ok)throw std::runtime_error(result.error.toStdString());return result.feature;}
QJsonObject byId(const QJsonArray& rows,const QJsonValue& id){for(const auto& value:rows)if(value.toObject().value("id")==id)return value.toObject();return {};}
bool relation(const QJsonObject& sketch,const QString& kind,const QJsonArray& ids){
    for(const auto& value:sketch.value("parameters").toObject().value("constraints").toArray()){
        const auto row=value.toObject();if(row.value("type")==kind && row.value("entityIds").toArray()==ids)return true;
    }
    return false;
}
}

int main(int argc,char** argv){
    QCoreApplication app(argc,argv);
    try {
        auto sketch=SketchGeometry::newFeature("13fa543c-e237-4e2e-947c-320071321821","XY");
        sketch=SketchGeometry::addCircle(sketch,{{"centerUMm",0},{"centerVMm",0},{"diameterMm",20}});
        sketch=SketchGeometry::addCircle(sketch,{{"centerUMm",100},{"centerVMm",0},{"diameterMm",20}});
        const auto circles=sketch.value("parameters").toObject().value("entities").toArray();
        const QString left=circles[0].toObject().value("id").toString(),right=circles[1].toObject().value("id").toString();
        const auto firstContact=SketchGeometry::pointerCircleContact(sketch.value("parameters").toObject(),left,0,10.2);
        check(!firstContact.isEmpty() && std::abs(firstContact.value("vMm").toDouble()-10)<1e-9,"exact starting circle contact");
        const auto preview=SketchGeometry::pointerCircleFrom(sketch.value("parameters").toObject(),left,
            firstContact.value("uMm").toDouble(),firstContact.value("vMm").toDouble(),40,12);
        check(!preview.isEmpty() && std::abs(preview.value("vMm").toDouble()-10)<1e-9,"preview tangent projects endpoint");
        const auto mixedTarget=SketchGeometry::addPointerEntity(sketch,"line",{{"u1Mm",0},{"v1Mm",10},
            {"u2Mm",100},{"v2Mm",0},{"inferCircleTangent",true},
            {"startSnapTarget",QJsonObject{{"kind","circle"},{"entityId",left}}},
            {"endSnapTarget",QJsonObject{{"kind","point"},{"pointId",circles[1].toObject().value("centerPointId")}}}});
        check(!mixedTarget.ok && mixedTarget.feature==sketch,"Shift tangent refuses an acquired ending point atomically");
        sketch=accept(SketchGeometry::addPointerEntity(sketch,"line",{{"u1Mm",0},{"v1Mm",10.2},
            {"u2Mm",40},{"v2Mm",12},{"inferCircleTangent",true},
            {"startSnapTarget",QJsonObject{{"kind","circle"},{"entityId",left}}}}));
        auto parameters=sketch.value("parameters").toObject();auto firstLine=parameters.value("entities").toArray().last().toObject();
        check(relation(sketch,"tangent",{left,firstLine.value("id")}),"starting circle retains signed tangent");
        bool contact=false;for(const auto& value:parameters.value("constraints").toArray()){
            const auto row=value.toObject();if(row.value("type")=="pointOnCurve" &&
                row.value("pointIds").toArray()==QJsonArray{firstLine.value("startPointId")} &&
                row.value("entityIds").toArray()==QJsonArray{left})contact=true;
        }check(contact,"starting endpoint retained on circle");
        const auto end=byId(parameters.value("points").toArray(),firstLine.value("endPointId"));
        const auto candidates=SketchGeometry::pointerCircleTangents(parameters,right,end.value("uMm").toDouble(),end.value("vMm").toDouble());
        check(candidates.size()==2,"two target-circle tangent branches");
        auto chosen=candidates[0].toObject();if(std::hypot(candidates[1].toObject().value("uMm").toDouble()-100,
            candidates[1].toObject().value("vMm").toDouble()-10)<
            std::hypot(chosen.value("uMm").toDouble()-100,chosen.value("vMm").toDouble()-10))chosen=candidates[1].toObject();
        const QJsonObject startTarget{{"kind","point"},{"pointId",firstLine.value("endPointId")}};
        const QJsonObject endTarget{{"kind","circle"},{"entityId",right},{"branch",chosen.value("branch")}};
        const QJsonObject secondArgs{{"u1Mm",end.value("uMm")},{"v1Mm",end.value("vMm")},
            {"u2Mm",100},{"v2Mm",10},{"startSnapTarget",startTarget},{"endSnapTarget",endTarget}};
        sketch=accept(SketchGeometry::addPointerEntity(sketch,"line",secondArgs));
        parameters=sketch.value("parameters").toObject();const auto secondLine=parameters.value("entities").toArray().last().toObject();
        check(relation(sketch,"tangent",{right,secondLine.value("id")}),"ordinary target click retains signed tangent");
        contact=false;for(const auto& value:parameters.value("constraints").toArray()){
            const auto row=value.toObject();if(row.value("type")=="pointOnCurve" &&
                row.value("pointIds").toArray()==QJsonArray{secondLine.value("endPointId")} &&
                row.value("entityIds").toArray()==QJsonArray{right})contact=true;
        }check(contact,"ending endpoint retained on circle");
        auto missingContact=sketch;
        auto missingParameters=missingContact.value("parameters").toObject();
        auto missingRelations=missingParameters.value("constraints").toArray();
        for(int i=0;i<missingRelations.size();++i){
            const auto row=missingRelations[i].toObject();
            if(row.value("type")=="pointOnCurve" && row.value("entityIds").toArray()==QJsonArray{right}
                && row.value("pointIds").toArray()==QJsonArray{secondLine.value("endPointId")}){
                missingRelations.removeAt(i);break;
            }
        }
        missingParameters["constraints"]=missingRelations;missingContact["parameters"]=missingParameters;
        check(!SketchGeometry::solveDraft(missingContact).ok,"retained tangent refuses missing contact relation");
        sketch=accept(SketchGeometry::addDimension(sketch,{{"kind","radius"},{"entityId",right},{"valueMm",11}}));
        check(relation(sketch,"tangent",{right,secondLine.value("id")}),"radius edit retains source tangent relation");
        std::cout<<"Pointer circle tangency contract passed\n";return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
