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
        const auto face=BRepBuilderAPI_MakeFace(gp_Pln(gp::XOY()),0,20,0,30).Face();
        const auto open=GeometryKernel::summarize(face);
        check(open["volumeMm3"].isNull() && !open["closed"].toBool(),"Open surface cannot claim solid volume");
        std::cout<<"PASS: real valid solids, analytic volumes, unique topology, open-surface boundary\n";
        return 0;
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
