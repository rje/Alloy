#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
#include <numbers>
#include <stdexcept>

namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void near(double actual,double expected,const char* message){check(std::abs(actual-expected)<1e-6,message);}
QJsonObject entity(const QJsonObject& sketch,const QString& id){
    for(const auto& value:sketch["parameters"].toObject()["entities"].toArray())if(value.toObject()["id"]==id)return value.toObject();
    return {};
}
QJsonObject point(const QJsonObject& sketch,const QString& id){
    for(const auto& value:sketch["parameters"].toObject()["points"].toArray())if(value.toObject()["id"]==id)return value.toObject();
    return {};
}
}

int main(int argc,char** argv){
    QCoreApplication app(argc,argv);
    try {
        CadDocument document;const auto owner=document.authored()["documentId"].toString();
        auto sketch=SketchGeometry::addCircle(SketchGeometry::newFeature(owner,"XY"),
            {{"centerUMm",7.9},{"centerVMm",7.9},{"diameterMm",6.3}});
        const auto source=sketch["parameters"].toObject()["entities"].toArray()[0].toObject();
        auto sized=SketchGeometry::addDimension(sketch,{{"kind","diameter"},{"entityId",source["id"]},{"valueMm",6.3}});
        check(sized.ok,sized.error.toUtf8().constData());sketch=sized.feature;
        const auto diameterId=sketch.value("parameters").toObject().value("dimensions").toArray()[0].toObject().value("id");
        auto invalid=SketchGeometry::addCircleOffset(sketch,{{"sourceEntityId",source["id"]},{"distanceMm",-3.15}});
        check(!invalid.ok && invalid.feature==sketch,"impossible inward radius leaves sketch unchanged");
        auto result=SketchGeometry::addCircleOffset(sketch,{{"sourceEntityId",source["id"]},{"distanceMm",-1}});
        check(result.ok,result.error.toUtf8().constData());sketch=result.feature;
        auto parameters=sketch["parameters"].toObject();const auto circles=parameters["entities"].toArray(),constraints=parameters["constraints"].toArray();
        check(circles.size()==2 && constraints.size()==2,"offset creates one circle, coincident center and radial relation");
        const auto target=circles[1].toObject(),relation=constraints[1].toObject();
        check(relation["type"]=="circleOffset" && relation["entityIds"].toArray()==QJsonArray{source["id"],target["id"]}
            && relation["distanceMm"].toDouble()==-1,"signed offset retains exact source/result identities");
        near(target["radiusMm"].toDouble(),2.15,"one millimeter inward yields diameter 4.3");
        const auto profile=SketchGeometry::evaluateFeature(sketch);
        check(profile.profiles.size()==2,"concentric circles produce annulus and disk regions");
        bool annulus=false;for(const auto& region:profile.profiles)
            if(std::abs(region.summary["areaMm2"].toDouble()-std::numbers::pi*(3.15*3.15-2.15*2.15))<1e-6)annulus=true;
        check(annulus,"annular region reflects real concentric circles");
        result=SketchGeometry::updateDimension(sketch,{{"dimensionId",diameterId},{"valueMm",8.3}});
        check(result.ok,result.error.toUtf8().constData());sketch=result.feature;
        near(entity(sketch,source["id"].toString())["radiusMm"].toDouble(),4.15,"source diameter edit changes source");
        near(entity(sketch,target["id"].toString())["radiusMm"].toDouble(),3.15,"source diameter edit changes retained result");
        result=SketchGeometry::updateCircleOffset(sketch,{{"constraintId",relation["id"]},{"distanceMm",-2}});
        check(result.ok,result.error.toUtf8().constData());sketch=result.feature;
        near(entity(sketch,source["id"].toString())["radiusMm"].toDouble(),4.15,"offset edit preserves source radius");
        near(entity(sketch,target["id"].toString())["radiusMm"].toDouble(),2.15,"offset edit changes result");
        invalid=SketchGeometry::updateCircleOffset(sketch,{{"constraintId",relation["id"]},{"distanceMm",-5}});
        check(!invalid.ok && invalid.feature==sketch,"invalid offset edit leaves authored circle pair unchanged");
        const auto sourceCenterId=source["centerPointId"].toString(),targetCenterId=target["centerPointId"].toString();
        result=SketchGeometry::movePoint(sketch,{{"pointId",sourceCenterId},{"uMm",9},{"vMm",10}});
        check(result.ok,result.error.toUtf8().constData());sketch=result.feature;
        near(point(sketch,sourceCenterId)["uMm"].toDouble(),9,"source center reaches requested x");
        near(point(sketch,sourceCenterId)["vMm"].toDouble(),10,"source center reaches requested y");
        near(point(sketch,targetCenterId)["uMm"].toDouble(),9,"target center follows source center x");
        near(point(sketch,targetCenterId)["vMm"].toDouble(),10,"target center follows source center y");
        // Result geometry Delete is an explicit retained-owner operation. Keep
        // source annotations and an unrelated fixed circle byte-for-byte.
        auto fixture=SketchGeometry::addCircle(sketch,{{"centerUMm",30},{"centerVMm",0},{"diameterMm",8}});
        const auto unrelated=fixture.value("parameters").toObject().value("entities").toArray().last().toObject();
        auto fixed=SketchGeometry::toggleFixed(fixture,{{"entityIds",QJsonArray{unrelated.value("id")}}});
        check(fixed.ok,"unrelated fixed circle");fixture=fixed.feature;
        fixed=SketchGeometry::addConstraint(fixture,{{"type","fixed"},{"pointIds",QJsonArray{sourceCenterId}}});
        check(fixed.ok,"source center relation");fixture=fixed.feature;
        const auto baseline=fixture.value("parameters").toObject();
        for(const auto& ids: {QJsonArray{source.value("id")},QJsonArray{source.value("id"),target.value("id")},QJsonArray{target.value("id"),"stale"}}) {
            const auto refused=SketchGeometry::deleteEntities(fixture,ids);
            check(!refused.ok && refused.feature==fixture,"source/mixed/stale Delete must refuse atomically");
        }
        auto deleted=SketchGeometry::deleteEntities(fixture,{target.value("id")});
        check(deleted.ok,deleted.error.toUtf8().constData());
        auto expected=baseline;
        QJsonArray keptEntities,keptPoints,keptRelations;
        for(const auto& row:baseline.value("entities").toArray())if(row.toObject().value("id")!=target.value("id"))keptEntities.append(row);
        for(const auto& row:baseline.value("points").toArray())if(row.toObject().value("id")!=targetCenterId)keptPoints.append(row);
        for(const auto& row:baseline.value("constraints").toArray()) {
            const auto r=row.toObject();
            if(r.value("id")!=relation.value("id") && !(r.value("type")=="coincident" && r.value("pointIds").toArray().contains(targetCenterId)))keptRelations.append(row);
        }
        expected["entities"]=keptEntities;expected["points"]=keptPoints;expected["constraints"]=keptRelations;
        check(deleted.feature.value("parameters").toObject()==expected,"result Delete changed unrelated authored data");
        check(SketchGeometry::evaluateFeature(deleted.feature).profiles.size()==2,"surviving circles evaluate");
        // A survivor sharing the generated center preserves that point, while
        // the exact owner-created center relation must still be removed.
        auto shared=SketchGeometry::addCircle(fixture,{{"centerUMm",9},{"centerVMm",10},{"diameterMm",2}});
        auto sharedParams=shared.value("parameters").toObject();auto es=sharedParams.value("entities").toArray();
        auto circle=es.last().toObject();circle["centerPointId"]=targetCenterId;es[es.size()-1]=circle;sharedParams["entities"]=es;shared["parameters"]=sharedParams;
        deleted=SketchGeometry::deleteEntities(shared,{target.value("id")});check(deleted.ok,"shared center deletion");
        check(!point(deleted.feature,targetCenterId).isEmpty(),"shared center retired");
        for(const auto& row:deleted.feature.value("parameters").toObject().value("constraints").toArray())
            check(!row.toObject().value("pointIds").toArray().contains(targetCenterId),"owner center coincidence survived shared point");
        check(document.commit(SketchGeometry::replaceFeature(document.authored(),sketch),"Offset circle").isEmpty(),"offset sketch commits");
        const auto committed=document.authored();document.history().undo();check(document.sketches().empty(),"undo removes offset sketch");
        document.history().redo();check(document.authored()==committed,"redo retains source/result/relation IDs");
        QTemporaryDir directory;check(directory.isValid(),"temporary save directory");
        const auto path=directory.path()+"/circle-offset.alloy";check(document.save(path).isEmpty(),"save offset");
        CadDocument reopened;check(reopened.open(path).isEmpty() && reopened.authored()==committed,"reopen retains exact authored offset");
        std::cout<<"PASS: retained signed circle offset, annulus, source/offset edits, centers, invalid input, undo and reopen\n";
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
