// SPDX-License-Identifier: MIT
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
}

int main(int argc,char** argv){
    QCoreApplication app(argc,argv);
    try {
        auto sketch=SketchGeometry::newFeature("13fa543c-e237-4e2e-947c-320071321821","XY");
        sketch=SketchGeometry::addLine(sketch,{{"u1Mm",0},{"v1Mm",0},{"u2Mm",40},{"v2Mm",0}});
        const auto first=sketch.value("parameters").toObject().value("entities").toArray().last().toObject();
        const auto geometry=SketchGeometry::pointerTangentArc(sketch.value("parameters").toObject(),first.value("id").toString(),70,-20);
        check(!geometry.isEmpty() && geometry.value("sweepAngleDeg").toDouble()<0,"clockwise concave tangent preview");
        const auto invalid=SketchGeometry::addPointerTangentArc(sketch,first.value("id").toString(),70,0);
        check(!invalid.ok && invalid.feature==sketch,"collinear release refuses without mutation");
        const auto created=SketchGeometry::addPointerTangentArc(sketch,first.value("id").toString(),70,-20);
        if(!created.ok)throw std::runtime_error(created.error.toStdString());
        sketch=created.feature;const auto parameters=sketch.value("parameters").toObject();
        const auto entities=parameters.value("entities").toArray();
        const auto arc=entities.last().toObject();
        check(entities.size()==2 && arc.value("type")=="arc" && arc.value("startPointId")==first.value("endPointId"),
            "arc shares exact incoming endpoint");
        int tangents=0;for(const auto& value:parameters.value("constraints").toArray()){
            const auto relation=value.toObject();if(relation.value("type")!="arcTangent")continue;
            check(relation.value("entityIds").toArray()==QJsonArray{arc.value("id"),first.value("id")}
                && relation.value("pointIds").toArray()==QJsonArray{first.value("endPointId")},
                "retained tangent provenance");++tangents;
        }check(tangents==1,"one incoming tangent relation");
        const auto before=sketch;
        auto continued=SketchGeometry::addPointerEntity(sketch,"line",{{"u1Mm",70},{"v1Mm",-20},
            {"u2Mm",100},{"v2Mm",-10},
            {"startSnapTarget",QJsonObject{{"kind","point"},{"pointId",arc.value("endPointId")}}}});
        if(!continued.ok)throw std::runtime_error(continued.error.toStdString());
        sketch=continued.feature;const auto outgoing=sketch.value("parameters").toObject().value("entities").toArray().last().toObject();
        check(outgoing.value("startPointId")==arc.value("endPointId"),"outgoing line shares exact arc endpoint identity");
        bool outgoingTangent=false;for(const auto& value:sketch.value("parameters").toObject().value("constraints").toArray()){
            const auto relation=value.toObject();if(relation.value("type")=="coincident" &&
                relation.value("pointIds").toArray()==QJsonArray{outgoing.value("startPointId"),arc.value("endPointId")})
                throw std::runtime_error("shared endpoint must not add a self-coincidence");
            if(relation.value("type")=="arcTangent" && relation.value("entityIds").toArray().contains(outgoing.value("id")))outgoingTangent=true;
        }
        check(!outgoingTangent,"straight continuation has no invented outgoing tangent");
        check(before!=sketch,"continuation changed draft");
        std::cout<<"Line tangent arc contract passed\n";return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
