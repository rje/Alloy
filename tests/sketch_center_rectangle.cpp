#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void near(double actual,double expected,const char* message){check(std::abs(actual-expected)<1e-6,message);}
QJsonObject point(const QJsonObject& sketch,const QJsonValue& id){
    for(const auto& value:sketch["parameters"].toObject()["points"].toArray())if(value.toObject()["id"]==id)return value.toObject();
    return {};
}
}

int main(int argc,char** argv) {
    QCoreApplication app(argc,argv);
    try {
        CadDocument document;
        const auto owner=document.authored()["documentId"].toString();
        const auto empty=SketchGeometry::newFeature(owner,"XY");
        auto rejected=SketchGeometry::addCenterRectangle(empty,{{"centerUMm",12.5},{"centerVMm",12.5},{"widthMm",0},{"heightMm",12}});
        check(!rejected.ok && rejected.feature==empty,"invalid dimensions leave draft unchanged");
        auto result=SketchGeometry::addCenterRectangle(empty,{{"centerUMm",12.5},{"centerVMm",12.5},{"widthMm",12},{"heightMm",12}});
        check(result.ok,result.error.toUtf8().constData());
        auto sketch=result.feature;auto parameters=sketch["parameters"].toObject();
        const auto entities=parameters["entities"].toArray(),points=parameters["points"].toArray(),dimensions=parameters["dimensions"].toArray();
        check(entities.size()==6 && points.size()==5 && dimensions.size()==2,"center rectangle retains four edges, two guides, five points and two dimensions");
        check(entities[4].toObject()["construction"].toBool() && entities[5].toObject()["construction"].toBool(),"diagonals are construction geometry");
        check(result.diagnostics["dof"].toInt(-1)==0,"center rectangle is constrained by its own center and dimensions");
        near(point(sketch,points[4].toObject()["id"])["uMm"].toDouble(),12.5,"authored center x");
        const auto firstCenterId=points[4].toObject()["id"];
        const auto firstEntityId=entities[0].toObject()["id"];
        const auto widthId=dimensions[0].toObject()["id"];
        result=SketchGeometry::updateDimension(sketch,{{"dimensionId",widthId},{"valueMm",16}});
        check(result.ok,result.error.toUtf8().constData());sketch=result.feature;
        near(point(sketch,firstCenterId)["uMm"].toDouble(),12.5,"width edit retains center x");
        near(point(sketch,firstCenterId)["vMm"].toDouble(),12.5,"width edit retains center y");
        check(sketch["parameters"].toObject()["entities"].toArray()[0].toObject()["id"]==firstEntityId,"width edit retains edge identity");
        auto geometry=SketchGeometry::evaluateFeature(sketch);
        check(geometry.profiles.size()==1,"construction guides do not split the single profile");
        near(geometry.profiles[0].summary["areaMm2"].toDouble(),192,"resized profile area");
        check(document.commit(SketchGeometry::replaceFeature(document.authored(),sketch),"Center rectangle").isEmpty(),"commit centered sketch");
        const auto committed=document.authored();
        document.history().undo();check(document.sketches().empty(),"undo removes centered sketch");
        document.history().redo();check(document.authored()==committed,"redo retains authored rectangle");
        QTemporaryDir directory;check(directory.isValid(),"temporary save directory");
        const auto path=directory.path()+"/center-rectangle.alloy";
        check(document.save(path).isEmpty(),"save centered sketch");
        CadDocument reopened;check(reopened.open(path).isEmpty() && reopened.authored()==committed,"reopen retains center, guides and dimensions");
        auto origin=SketchGeometry::newFeature(owner,"XY");
        auto snapped=SketchGeometry::addCenterRectangle(origin,{{"centerUMm",0.02},{"centerVMm",0.02},{"widthMm",12},{"heightMm",12},
            {"centerSnapTarget",QJsonObject{{"kind","origin"}}}});
        check(snapped.ok,snapped.error.toUtf8().constData());
        check(snapped.feature["parameters"].toObject()["originPointId"].isString(),"origin snap retains its target");
        check(snapped.diagnostics["dof"].toInt(-1)==0,"snapped center remains constrained");
        auto offset=SketchGeometry::newConstructionPlaneFeature(owner,CadDocument::newId());
        auto onPlane=SketchGeometry::addCenterRectangle(offset,{{"centerUMm",12.5},{"centerVMm",12.5},{"widthMm",12},{"heightMm",12}});
        check(onPlane.ok,onPlane.error.toUtf8().constData());
        const QJsonObject frame{{"origin",QJsonArray{0,0,6.5}},{"uAxis",QJsonArray{1,0,0}},
            {"vAxis",QJsonArray{0,1,0}},{"normal",QJsonArray{0,0,1}}};
        const auto elevated=SketchGeometry::evaluateFeature(onPlane.feature,frame);
        check(elevated.profiles.size()==1,"center rectangle produces one region on offset plane");
        near(elevated.profiles[0].summary["areaMm2"].toDouble(),144,"12 by 12 offset-plane profile area");
        near(elevated.summary["boundsMm"].toObject()["min"].toArray()[2].toDouble(),6.5,"offset-plane world elevation");
        std::cout<<"PASS: center rectangle relations, resize, profile, snap, undo and reopen\n";
    } catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
