// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
#include <numbers>
#include <stdexcept>

static void check(bool ok,const QString& reason){if(!ok)throw std::runtime_error(reason.toStdString());}
static QJsonObject lookup(const QJsonObject& sketch,const char* field,const QString& id){
    for(const auto& value:sketch["parameters"].toObject()[field].toArray())if(value.toObject()["id"]==id)return value.toObject();return {};
}
static void near(double a,double b,const char* reason){check(std::abs(a-b)<1e-5,reason);}

int main(int argc,char** argv){
    QCoreApplication app(argc,argv);
    try{
        CadDocument document;auto sketch=SketchGeometry::newFeature(document.authored()["documentId"].toString(),"XY");
        auto parameters=sketch["parameters"].toObject();QJsonArray points,entities;
        auto addPoint=[&](double x,double y){const auto id=CadDocument::newId();points.append(QJsonObject{{"id",id},{"uMm",x},{"vMm",y}});return id;};
        auto addLine=[&](const QString& a,const QString& b,bool construction){const auto id=CadDocument::newId();entities.append(QJsonObject{{"id",id},{"type","line"},{"startPointId",a},{"endPointId",b},{"construction",construction}});return id;};
        const auto ll=addPoint(-14,10),lr=addPoint(12,10),ur=addPoint(12,36),ul=addPoint(-14,36);
        const auto lc=addPoint(-14+std::sqrt(56.),23),rc=addPoint(12-std::sqrt(56.),23);
        const auto a=addPoint(-25,-3),b=addPoint(25,-3);
        const auto bottom=addLine(ll,lr,false),right=addLine(lr,ur,true),top=addLine(ur,ul,false),left=addLine(ul,ll,true),axis=addLine(a,b,true);
        const double toDegrees=180/std::numbers::pi,sweep=2*std::atan2(13.,std::sqrt(56.))*toDegrees;
        const auto leftArc=CadDocument::newId(),rightArc=CadDocument::newId();
        entities.append(QJsonObject{{"id",leftArc},{"type","arc"},{"startPointId",ul},{"endPointId",ll},{"centerPointId",lc},{"radiusMm",15},{"startAngleDeg",std::atan2(13.,-std::sqrt(56.))*toDegrees},{"sweepAngleDeg",sweep}});
        entities.append(QJsonObject{{"id",rightArc},{"type","arc"},{"startPointId",lr},{"endPointId",ur},{"centerPointId",rc},{"radiusMm",15},{"startAngleDeg",std::atan2(-13.,std::sqrt(56.))*toDegrees},{"sweepAngleDeg",sweep}});
        parameters["points"]=points;parameters["entities"]=entities;sketch["parameters"]=parameters;
        auto initial=SketchGeometry::solveDraft(sketch);check(initial.ok,"initial: "+initial.error+" "+QString::fromUtf8(QJsonDocument(initial.diagnostics).toJson(QJsonDocument::Compact)));sketch=initial.feature;
        initial=SketchGeometry::addConstraint(sketch,{{"type","horizontal"},{"entityIds",QJsonArray{top}}});
        check(initial.ok,"source horizontal: "+initial.error);sketch=initial.feature;
        const auto sources=QJsonArray{bottom,right,top,left,leftArc,rightArc};
        auto invalid=SketchGeometry::mirrorEntities(sketch,{{"sourceEntityIds",sources},{"axisEntityId",CadDocument::newId()}});
        check(!invalid.ok && invalid.feature==sketch,"missing axis is atomic");
        auto mirrored=SketchGeometry::mirrorEntities(sketch,{{"sourceEntityIds",sources},{"axisEntityId",axis}});
        check(mirrored.ok,"mirror: "+mirrored.error+" "+QString::fromUtf8(QJsonDocument(mirrored.diagnostics).toJson(QJsonDocument::Compact)));sketch=mirrored.feature;parameters=sketch["parameters"].toObject();
        const auto group=parameters["mirrorGroups"].toArray().first().toObject();
        check(group["entityPairs"].toArray().size()==6 && group["pointPairs"].toArray().size()==6,"six curves and six deduplicated point identities");
        check(parameters["entities"].toArray().size()==13 && parameters["points"].toArray().size()==14,"source and result geometry retained");
        check(parameters["constraints"].toArray().size()==1 && parameters["constraints"].toArray().first().toObject()["entityIds"].toArray()==QJsonArray{top},"source relation retained without copying constraints to results");
        check(mirrored.diagnostics["dof"]==initial.diagnostics["dof"],"reflection does not add independent DOF");
        for(int i=0;i<4;++i){const auto pair=group["entityPairs"].toArray()[i].toObject();
            const auto source=lookup(sketch,"entities",pair["sourceEntityId"].toString()),result=lookup(sketch,"entities",pair["resultEntityId"].toString());
            check(source["construction"]==result["construction"],"construction flag retained per reflected line");}
        const auto resultArc=lookup(sketch,"entities",group["entityPairs"].toArray()[4].toObject()["resultEntityId"].toString());
        near(resultArc["radiusMm"].toDouble(),15,"arc radius reflected");near(resultArc["sweepAngleDeg"].toDouble(),-sweep,"arc sweep reversed");
        const auto resultPointId=group["pointPairs"].toArray().first().toObject()["resultPointId"].toString();
        near(lookup(sketch,"points",resultPointId)["vMm"].toDouble(),-16,"point reflected across rough axis");
        const auto beforeResultEditSource=lookup(sketch,"points",ll),beforeResultEditAxis=lookup(sketch,"points",a);
        auto resultMoved=SketchGeometry::movePoint(sketch,{{"pointId",resultPointId},{"uMm",-13.5},{"vMm",-16.5}});
        check(resultMoved.ok,"result point move: "+resultMoved.error);sketch=resultMoved.feature;
        const auto movedResult=lookup(sketch,"points",resultPointId),movedSource=lookup(sketch,"points",ll);
        check(std::hypot(movedResult["uMm"].toDouble()+14,movedResult["vMm"].toDouble()+16)>0.1,"result-point gesture moves solved pair");
        const auto movedAxisA=lookup(sketch,"points",a),movedAxisB=lookup(sketch,"points",b);
        const double ax=movedAxisA["uMm"].toDouble(),ay=movedAxisA["vMm"].toDouble();
        const double dx=movedAxisB["uMm"].toDouble()-ax,dy=movedAxisB["vMm"].toDouble()-ay;
        const double t=((movedSource["uMm"].toDouble()-ax)*dx+(movedSource["vMm"].toDouble()-ay)*dy)/(dx*dx+dy*dy);
        near(movedResult["uMm"].toDouble(),2*(ax+t*dx)-movedSource["uMm"].toDouble(),"result edit remains symmetric x");
        near(movedResult["vMm"].toDouble(),2*(ay+t*dy)-movedSource["vMm"].toDouble(),"result edit remains symmetric y");
        check(movedSource!=beforeResultEditSource || movedAxisA!=beforeResultEditAxis,"result edit updates the associated source or free axis");
        auto moved=SketchGeometry::movePoint(sketch,{{"pointId",a},{"uMm",-25},{"vMm",-5}});
        check(moved.ok,moved.error);sketch=moved.feature;
        check(lookup(sketch,"points",resultPointId)["vMm"].toDouble()<-16.1,"axis edit propagates result");
        QString lowerRightResult;
        for(const auto& value:group["pointPairs"].toArray())if(value.toObject()["sourcePointId"]==lr)lowerRightResult=value.toObject()["resultPointId"].toString();
        check(!lowerRightResult.isEmpty(),"source/result endpoint map exists");
        const auto beforeSourceMove=lookup(sketch,"points",lowerRightResult);
        moved=SketchGeometry::movePoint(sketch,{{"pointId",lr},{"uMm",13},{"vMm",11}});
        check(moved.ok,"source endpoint: "+moved.error);sketch=moved.feature;
        check(std::hypot(lookup(sketch,"points",lowerRightResult)["uMm"].toDouble()-beforeSourceMove["uMm"].toDouble(),
            lookup(sketch,"points",lowerRightResult)["vMm"].toDouble()-beforeSourceMove["vMm"].toDouble())>0.1,"source endpoint edit propagates");
        auto sized=SketchGeometry::addDimension(sketch,{{"kind","radius"},{"entityId",leftArc},{"valueMm",17}});
        check(sized.ok,"source radius: "+sized.error);sketch=sized.feature;
        near(lookup(sketch,"entities",leftArc)["radiusMm"].toDouble(),17,"source radius edit");
        near(lookup(sketch,"entities",resultArc["id"].toString())["radiusMm"].toDouble(),17,"result radius follows source");
        QString leftCenterResult;
        for(const auto& value:group["pointPairs"].toArray())if(value.toObject()["sourcePointId"]==lc)leftCenterResult=value.toObject()["resultPointId"].toString();
        check(!leftCenterResult.isEmpty(),"arc center mapping retained");
        const auto beforeCenterMove=lookup(sketch,"points",leftCenterResult);
        const auto sourceCenter=lookup(sketch,"points",lc);
        moved=SketchGeometry::movePoint(sketch,{{"pointId",lc},{"uMm",sourceCenter["uMm"].toDouble()+0.5},{"vMm",sourceCenter["vMm"].toDouble()+0.5}});
        check(moved.ok,"source center: "+moved.error);sketch=moved.feature;
        check(std::hypot(lookup(sketch,"points",leftCenterResult)["uMm"].toDouble()-beforeCenterMove["uMm"].toDouble(),
            lookup(sketch,"points",leftCenterResult)["vMm"].toDouble()-beforeCenterMove["vMm"].toDouble())>0.1,"source center edit propagates");
        check(document.commit(SketchGeometry::replaceFeature(document.authored(),sketch),"Sketch Mirror").isEmpty(),"commit");
        const auto authored=document.authored();document.history().undo();check(document.sketches().empty(),"undo");
        document.history().redo();check(document.authored()==authored,"redo exact authored mapping");
        QTemporaryDir dir;check(dir.isValid(),"temporary directory");const auto path=dir.path()+"/mirror.alloy";
        check(document.save(path).isEmpty(),"save");CadDocument reopened;
        check(reopened.open(path).isEmpty() && reopened.authored()==authored,"reopen exact authored mapping");
        std::cout<<"sketch mirror: retained symmetry and history passed\n";return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
