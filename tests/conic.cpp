// SPDX-License-Identifier: MIT
#include "SketchConic.h"
#include "CadDocument.h"
#include <QCoreApplication>
#include <QJsonArray>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool value,const char* message) { if(!value)throw std::runtime_error(message); }
QJsonObject accepted(const SketchGeometry::SolveResult& result) {
    if(!result.ok)throw std::runtime_error(result.error.toStdString());
    return result.feature;
}
QJsonObject entity(const QJsonObject& feature) { return feature["parameters"].toObject()["entities"].toArray()[0].toObject(); }
}
int main(int argc,char** argv) {
    QCoreApplication app(argc,argv);
    try {
        CadDocument document;
        const auto empty=SketchGeometry::newFeature(document.activeOwnerId(),"XY");
        const QJsonObject inputs{{"start",QJsonObject{{"uMm",-20},{"vMm",5}}},
            {"end",QJsonObject{{"uMm",70},{"vMm",25}}},
            {"vertex",QJsonObject{{"uMm",15},{"vMm",90}}},{"rho",0.541}};
        auto result=SketchConic::add(empty,inputs);
        auto draft=accepted(result);
        check(result.diagnostics["dof"].toInt()==7,"Three free points plus editable Rho");
        const auto id=entity(draft)["id"].toString();
        const gp_Pnt start(-20,5,0),end(70,25,0),vertex(15,90,0),midpoint(25,15,0);
        for(double rho : {0.5,0.541,0.683}) {
            draft=accepted(SketchConic::setRho(draft,id,rho));
            const auto curve=SketchConic::evaluate(draft["parameters"].toObject(),entity(draft));
            check(curve->Degree()==2,"Real quadratic conic");
            check(curve->Value(0).Distance(start)<1e-8 && curve->Value(1).Distance(end)<1e-8,"Conic endpoints");
            check(curve->Value(0.5).Distance(midpoint.Translated(gp_Vec(midpoint,vertex)*rho))<1e-8,"Rho shoulder ratio");
            gp_Pnt p;gp_Vec tangent;
            curve->D1(0,p,tangent);check(tangent.IsParallel(gp_Vec(start,vertex),1e-8)&&tangent.Dot(gp_Vec(start,vertex))>0,"Start tangent");
            curve->D1(1,p,tangent);check(tangent.IsParallel(gp_Vec(vertex,end),1e-8)&&tangent.Dot(gp_Vec(vertex,end))>0,"End tangent");
        }
        auto linkedInputs=inputs;
        linkedInputs["start"]=QJsonObject{{"uMm",-20},{"vMm",5},{"pointId",entity(draft)["startPointId"]}};
        const auto linked=accepted(SketchConic::add(draft,linkedInputs));
        const auto linkedParameters=linked["parameters"].toObject();
        check(linkedParameters["points"].toArray().size()==5 &&
              linkedParameters["entities"].toArray()[1].toObject()["startPointId"]==entity(draft)["startPointId"],
              "Explicit point snap preserves shared identity");
        const auto invalid=SketchConic::setRho(draft,id,1);
        check(!invalid.ok && invalid.feature==draft,"Invalid Rho preserves authoring");
        auto collinear=inputs;collinear["vertex"]=QJsonObject{{"uMm",25},{"vMm",15}};
        const auto rejected=SketchConic::add(empty,collinear);
        check(!rejected.ok && rejected.feature==empty,"Collinear creation refuses atomically");
        check(SketchGeometry::evaluateFeature(draft).summary["valid"].toBool(),"Actual OCCT edge evaluates");
        check(document.commit(SketchGeometry::replaceFeature(document.authored(),draft),"Conic").isEmpty(),"Commit conic");
        const auto before=document.authored();
        auto changed=accepted(SketchConic::setRho(draft,id,0.5));
        check(document.commit(SketchGeometry::replaceFeature(before,changed),"Rho").isEmpty(),"Commit Rho edit");
        const auto after=document.authored();document.history().undo();check(document.authored()==before,"Exact undo");
        document.history().redo();check(document.authored()==after,"Exact redo");
        QTemporaryDir directory;const auto path=directory.filePath("conic.alloy");
        check(document.save(path).isEmpty(),"Save");CadDocument reopened;
        check(reopened.open(path).isEmpty()&&reopened.authored()==after,"Exact reopen");
        std::cout<<"Conic retained geometry, asymmetric shoulder/tangents and history pass\n";
        return 0;
    } catch(const std::exception& error) { std::cerr<<error.what()<<'\n';return 1; }
}
