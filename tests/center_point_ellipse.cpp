#include "CadDocument.h"
#include "SketchEllipse.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
#include <numbers>
#include <stdexcept>

namespace {
void check(bool good,const char* message){if(!good)throw std::runtime_error(message);}
void near(double actual,double expected,const char* message){check(std::abs(actual-expected)<1e-5,message);}
}
int main(int argc,char** argv){
    QCoreApplication app(argc,argv);
    try{
        CadDocument document;
        const auto empty=SketchGeometry::newFeature(document.activeOwnerId(),"XY");
        auto result=SketchGeometry::addCenterPointEllipse(empty,{{"centerUMm",0},{"centerVMm",0},
            {"firstUMm",130},{"firstVMm",0},{"contourUMm",78},{"contourVMm",40},
            {"centerSnapTarget",QJsonObject{{"kind","origin"}}}});
        if(!result.ok)throw std::runtime_error(result.error.toStdString());
        auto sketch=result.feature;auto parameters=sketch["parameters"].toObject();
        const auto entities=parameters["entities"].toArray();const auto ellipse=entities.last().toObject();
        check(ellipse["type"]=="ellipse" && result.diagnostics["dof"]==3,"origin-snapped rough ellipse has three DOF");
        auto axes=SketchEllipse::axes(parameters,ellipse);
        near(axes.firstRadius,130,"rough first semiaxis");near(axes.secondRadius,50,"oblique contour establishes second semiaxis");
        const auto firstId=ellipse["firstAxisId"],secondId=ellipse["secondAxisId"];
        result=SketchGeometry::addConstraint(sketch,{{"type","horizontal"},{"entityIds",QJsonArray{firstId}}});
        if(!result.ok)throw std::runtime_error(result.error.toStdString());sketch=result.feature;
        check(result.diagnostics["dof"]==2,"horizontal first axis removes only orientation DOF");
        result=SketchGeometry::addDimension(sketch,{{"kind","linear"},{"entityId",firstId},{"valueMm",275}});
        if(!result.ok)throw std::runtime_error(result.error.toStdString());sketch=result.feature;
        check(result.diagnostics["dof"]==1,"first full-axis D275 removes one DOF");
        const auto firstDimensionId=sketch["parameters"].toObject()["dimensions"].toArray().last().toObject()["id"].toString();
        result=SketchGeometry::addDimension(sketch,{{"kind","linear"},{"entityId",secondId},{"valueMm",100}});
        if(!result.ok)throw std::runtime_error(result.error.toStdString());sketch=result.feature;
        check(result.diagnostics["dof"]==0,"second full-axis D100 finishes ellipse definition");
        const auto geometry=SketchGeometry::evaluateFeature(sketch);
        check(geometry.profiles.size()==1,"ellipse forms one real selectable profile");
        near(geometry.profiles.front().summary["areaMm2"].toDouble(),6875*std::numbers::pi,"ellipse analytic area");
        axes=SketchEllipse::axes(sketch["parameters"].toObject(),ellipse);
        near(axes.firstRadius,137.5,"first source semiaxis");near(axes.secondRadius,50,"second source semiaxis");
        const auto before=sketch;
        result=SketchGeometry::updateDimension(sketch,{{"dimensionId",firstDimensionId},{"valueMm",100}});
        if(!result.ok)throw std::runtime_error(result.error.toStdString());sketch=result.feature;
        near(SketchGeometry::evaluateFeature(sketch).profiles.front().summary["areaMm2"].toDouble(),2500*std::numbers::pi,
            "circle equality evaluates without replacing authored ellipse");
        result=SketchGeometry::updateDimension(sketch,{{"dimensionId",firstDimensionId},{"valueMm",80}});
        if(!result.ok)throw std::runtime_error(result.error.toStdString());sketch=result.feature;
        near(SketchGeometry::evaluateFeature(sketch).profiles.front().summary["areaMm2"].toDouble(),2000*std::numbers::pi,
            "crossed axes canonicalize only evaluated conic");
        check(sketch["parameters"].toObject()["entities"].toArray().last().toObject()==ellipse,
            "authored ellipse and axis IDs survive equality and crossing edits");
        result=SketchGeometry::updateDimension(sketch,{{"dimensionId",firstDimensionId},{"valueMm",275}});
        if(!result.ok)throw std::runtime_error(result.error.toStdString());sketch=result.feature;
        check(sketch==before,"reverted axis length restores exact authored source sketch");
        check(document.commit(SketchGeometry::replaceFeature(document.authored(),sketch),"Ellipse").isEmpty(),"commit ellipse");
        const auto authored=document.authored();document.history().undo();check(document.sketches().empty(),"undo ellipse");
        document.history().redo();check(document.authored()==authored,"redo exact ellipse");
        QTemporaryDir directory;check(directory.isValid(),"temporary file");const auto path=directory.path()+"/ellipse.alloy";
        check(document.save(path).isEmpty(),"save ellipse");CadDocument reopened;
        check(reopened.open(path).isEmpty() && reopened.authored()==authored,"reopen exact ellipse");
        const auto small=SketchGeometry::newFeature(document.activeOwnerId(),"XY");
        result=SketchGeometry::addCenterPointEllipse(small,{{"centerUMm",0},{"centerVMm",0},
            {"firstUMm",10},{"firstVMm",0},{"contourUMm",6},{"contourVMm",4}});
        if(!result.ok)throw std::runtime_error(result.error.toStdString());
        const auto smallEllipse=result.feature["parameters"].toObject()["entities"].toArray().last().toObject();
        axes=SketchEllipse::axes(result.feature["parameters"].toObject(),smallEllipse);
        near(axes.secondRadius,5,"oblique contour uses true conic equation");
        const auto withOpenLine=SketchGeometry::addLine(result.feature,
            {{"u1Mm",30},{"v1Mm",30},{"u2Mm",40},{"v2Mm",45}});
        check(SketchGeometry::evaluateFeature(withOpenLine).profiles.size()==1,
            "open neighboring sketch geometry does not erase ellipse profile");
        const auto invalid=SketchGeometry::addCenterPointEllipse(small,{{"centerUMm",0},{"centerVMm",0},
            {"firstUMm",10},{"firstVMm",0},{"contourUMm",10},{"contourVMm",0}});
        check(!invalid.ok && invalid.feature==small,"underdetermined contour refuses atomically");
        std::cout<<"Center point ellipse core passed\n";return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
