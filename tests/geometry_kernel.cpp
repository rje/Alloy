#include "GeometryKernel.h"
#include <BRepBuilderAPI_MakeFace.hxx>
#include <gp_Pln.hxx>
#include <TopoDS_Face.hxx>
#include <QCoreApplication>
#include <QJsonArray>
#include <cmath>
#include <iostream>
#include <stdexcept>
void check(bool value,const char* reason) { if(!value) throw std::runtime_error(reason); }
int main(int argc,char** argv) {
    QCoreApplication app(argc,argv);
    try {
        QJsonObject p{{"widthMm",20},{"depthMm",30},{"heightMm",10},{"xMm",0},{"yMm",0},{"zMm",0}};
        const auto summary=GeometryKernel::summarize(GeometryKernel::primitive("box",p));
        check(summary["valid"].toBool() && summary["closed"].toBool(),"Box must be closed and valid");
        check(std::abs(summary["volumeMm3"].toDouble()-6000)<1e-6,"Analytic box volume");
        check(summary["topology"].toObject()["edges"].toInt()==12,"Unique box topology");
        p["radiusMm"]=5;
        const auto cylinder=GeometryKernel::summarize(GeometryKernel::primitive("cylinder",p));
        check(std::abs(cylinder["volumeMm3"].toDouble()-std::acos(-1)*250)<1e-6,"Analytic cylinder volume");
        QJsonObject coneParameters{{"baseDiameterMm",20},{"topDiameterMm",6},{"heightMm",24},{"xMm",3},{"yMm",-2},{"zMm",5}};
        const auto cone=GeometryKernel::summarize(GeometryKernel::primitive("cone",coneParameters));
        check(cone["valid"].toBool() && cone["closed"].toBool(),"Frustum is a closed valid solid");
        check(std::abs(cone["volumeMm3"].toDouble()-1112*std::acos(-1))<1e-6,"Analytic frustum volume");
        const auto size=cone["boundsMm"].toObject()["size"].toArray();
        check(std::abs(size[0].toDouble()-20)<1e-6 && std::abs(size[2].toDouble()-24)<1e-6,"Frustum bounds");
        coneParameters["topDiameterMm"]=0;
        check(std::abs(GeometryKernel::summarize(GeometryKernel::primitive("cone",coneParameters))["volumeMm3"].toDouble()-800*std::acos(-1))<1e-6,"Pointed cone volume");
        coneParameters["topDiameterMm"]=20;
        check(std::abs(GeometryKernel::summarize(GeometryKernel::primitive("cone",coneParameters))["volumeMm3"].toDouble()-2400*std::acos(-1))<1e-6,"Equal diameters retain cylindrical limit");
        QJsonObject torusParameters{{"majorRadiusMm",14},{"minorRadiusMm",4},{"xMm",40},{"yMm",0},{"zMm",8}};
        const auto torus=GeometryKernel::summarize(GeometryKernel::primitive("torus",torusParameters));
        check(torus["valid"].toBool() && torus["closed"].toBool(),"Torus is a closed valid solid");
        check(std::abs(torus["volumeMm3"].toDouble()-448*std::pow(std::acos(-1),2))<1e-6,"Analytic torus volume");
        const auto torusSize=torus["boundsMm"].toObject()["size"].toArray();
        check(std::abs(torusSize[0].toDouble()-36)<1e-6 && std::abs(torusSize[2].toDouble()-8)<1e-6,"Torus bounds");
        for(const auto& invalid:QJsonArray{QJsonObject{{"majorRadiusMm",4},{"minorRadiusMm",4}},QJsonObject{{"majorRadiusMm",3},{"minorRadiusMm",4}}}) {
            auto parameters=torusParameters;
            const auto values=invalid.toObject();for(auto it=values.begin();it!=values.end();++it)parameters[it.key()]=it.value();
            bool refused=false;try{GeometryKernel::primitive("torus",parameters);}catch(const std::exception&){refused=true;}
            check(refused,"Horn/spindle torus must be refused");
        }
        const auto face=BRepBuilderAPI_MakeFace(gp_Pln(gp::XOY()),0,20,0,30).Face();
        const auto open=GeometryKernel::summarize(face);
        check(open["volumeMm3"].isNull() && !open["closed"].toBool(),"Open surface cannot claim solid volume");
        std::cout<<"PASS: real valid solids, analytic volumes, unique topology, open-surface boundary\n";
        return 0;
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
