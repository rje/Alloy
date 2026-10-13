// SPDX-License-Identifier: MIT
#include "SketchGeometry.h"
#include "SketchText.h"
#include <BRepCheck_Analyzer.hxx>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QUuid>
#include <cmath>
#include <iostream>
#include <stdexcept>
namespace {
void check(bool value,const char* why){if(!value)throw std::runtime_error(why);}
}
int main(int argc,char** argv){QGuiApplication app(argc,argv);try{
    auto sketch=SketchGeometry::newFeature(QUuid::createUuid().toString(QUuid::WithoutBraces),"XY","Raised initials");
    auto p=sketch["parameters"].toObject();auto text=SketchText::create({{"text","DSB"},{"heightMm",14},{"uMm",2},{"vMm",4},{"angleDeg",0}});
    p["texts"]=QJsonArray{text};sketch["parameters"]=p;
    auto evaluated=SketchGeometry::evaluateFeature(sketch);
    check(evaluated.profiles.size()==3,"DSB needs three separate glyph profiles");
    check(evaluated.profiles[0].summary["innerLoopCount"].toInt()==1,"D hole missing");
    check(evaluated.profiles[1].summary["innerLoopCount"].toInt()==0,"S has unexpected hole");
    check(evaluated.profiles[2].summary["innerLoopCount"].toInt()==2,"B holes missing");
    for(const auto& profile:evaluated.profiles){
        check(BRepCheck_Analyzer(profile.face).IsValid(),"Glyph face invalid");
        const auto solid=GeometryKernel::extrusion(profile,1.5);
        const auto summary=GeometryKernel::summarize(solid);
        check(summary["valid"].toBool() && summary["closed"].toBool(),"Glyph extrusion invalid");
        check(std::abs(summary["volumeMm3"].toDouble()-profile.summary["areaMm2"].toDouble()*1.5)<1e-3,"Glyph volume mismatch");
    }
    const auto ids=QJsonArray{evaluated.profiles[0].profileId,evaluated.profiles[1].profileId,evaluated.profiles[2].profileId};
    text["text"]="DS8";text["heightMm"]=16;text["angleDeg"]=180;text["uMm"]=20;text["vMm"]=-3;
    p["texts"]=QJsonArray{text};sketch["parameters"]=p;
    const auto reopened=QJsonDocument::fromJson(QJsonDocument(sketch).toJson()).object();
    evaluated=SketchGeometry::evaluateFeature(reopened);
    check(evaluated.profiles.size()==3 && evaluated.profiles[2].summary["innerLoopCount"].toInt()==2,"DS8 edit lost its two-hole third glyph");
    for(int i=0;i<3;++i)check(evaluated.profiles[i].profileId==ids[i].toString(),"Text slot ID changed on supported edit");
    auto invalid=text;invalid["fontSha256"]="bad";QSet<QString> owned;
    check(!SketchText::validate(invalid,owned).isEmpty(),"Missing or changed font identity was accepted");
    std::cout<<"PASS: pinned font, exact cubic glyph faces, holes, extrusion, supported edit and authored reopen\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
