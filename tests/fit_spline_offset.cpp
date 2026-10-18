// SPDX-License-Identifier: MIT
#include "SketchSplineOffset.h"
#include "SketchSpline.h"
#include "CadDocument.h"
#include <QCoreApplication>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <BRepCheck_Analyzer.hxx>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool condition,const QString& message){if(!condition)throw std::runtime_error(message.toStdString());}
QJsonObject accept(const SketchGeometry::SolveResult& result){check(result.ok,result.error);return result.feature;}
QJsonObject find(const QJsonObject& draft,const QString& type){for(const auto& value:draft["parameters"].toObject()["entities"].toArray())if(value.toObject()["type"]==type)return value.toObject();return {};}
QJsonObject point(const QJsonObject& draft,const QString& id){for(const auto& value:draft["parameters"].toObject()["points"].toArray())if(value.toObject()["id"]==id)return value.toObject();return {};}
double pointDistance(const QJsonObject& a,const gp_Pnt& b){return std::hypot(a["uMm"].toDouble()-b.X(),a["vMm"].toDouble()-b.Y());}
void inspect(const QJsonObject& draft,const QString& offsetId,const QString& expectedProfileId={}){
    QJsonObject offset;for(const auto& value:draft["parameters"].toObject()["entities"].toArray())if(value.toObject()["id"]==offsetId)offset=value.toObject();
    check(!offset.isEmpty(),"retained offset identity");
    const auto evaluated=SketchSplineOffset::evaluate(draft["parameters"].toObject(),offset);
    check(pointDistance(point(draft,offset["startPointId"].toString()),evaluated.start)<1e-6 &&
          pointDistance(point(draft,offset["endPointId"].toString()),evaluated.end)<1e-6,"derived endpoint cache follows exact evaluator");
    const auto sketch=SketchGeometry::evaluateFeature(draft);
    check(sketch.profiles.size()==1,"exact offset and two explicit Lines bound one region: "+
        QString::fromUtf8(QJsonDocument(sketch.summary["profileDiagnostics"].toObject()).toJson(QJsonDocument::Compact)));
    check(BRepCheck_Analyzer(sketch.profiles[0].face).IsValid(),"valid OCCT strip face");
    check(sketch.profiles[0].summary["areaMm2"].toDouble()>400,"usable strip area");
    if(!expectedProfileId.isEmpty())check(sketch.profiles[0].profileId==expectedProfileId,"profile identity survives supported edits");
}
}

int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    CadDocument document;
    auto draft=SketchGeometry::newFeature(document.authored()["documentId"].toString(),"XY");
    draft=accept(SketchSpline::add(draft,{{"points",QJsonArray{
        QJsonObject{{"uMm",0},{"vMm",-20}},QJsonObject{{"uMm",8},{"vMm",20}},
        QJsonObject{{"uMm",-6},{"vMm",60}},QJsonObject{{"uMm",4},{"vMm",100}}}}}));
    const auto source=find(draft,"fitSpline");
    draft=accept(SketchSplineOffset::add(draft,{{"sourceEntityId",source["id"]},{"signedDistanceMm",-5}}));
    const auto offset=find(draft,"fitSplineOffset");const auto sourceIds=source["pointIds"].toArray();
    const auto evaluated=SketchSplineOffset::evaluate(draft["parameters"].toObject(),offset);
    const auto basis=SketchSpline::evaluate(draft["parameters"].toObject(),source).curve;
    for(int sample=0;sample<=128;++sample){
        const double t=basis->FirstParameter()+(basis->LastParameter()-basis->FirstParameter())*sample/128.0;
        gp_Pnt onSource;gp_Vec tangent;basis->D1(t,onSource,tangent);
        const auto shifted=evaluated.curve->Value(t);const gp_Vec delta(onSource,shifted);
        const double speed=std::hypot(tangent.X(),tangent.Y());
        const double rightNormal=(delta.X()*tangent.Y()-delta.Y()*tangent.X())/speed;
        check(std::abs(delta.Dot(tangent)/speed)<1e-6 && std::abs(rightNormal+5)<1e-6,
            "sampled offset remains five millimeters on the signed normal to the original spline");
    }
    auto siblingDraft=accept(SketchSplineOffset::add(draft,{{"sourceEntityId",source["id"]},{"signedDistanceMm",-7}}));
    QString siblingId;
    for(const auto& value:siblingDraft["parameters"].toObject()["entities"].toArray()){
        const auto entity=value.toObject();
        if(entity["type"]=="fitSplineOffset" && entity["id"]!=offset["id"])siblingId=entity["id"].toString();
    }
    check(!siblingId.isEmpty(),"second offset retains its own identity");
    // Result Delete is whole-owner removal; its authored source and sibling
    // remain exact, including their retained points and tangent records.
    auto expected=siblingDraft["parameters"].toObject();QJsonArray keptEntities,keptPoints;
    for(const auto& row:expected.value("entities").toArray())if(row.toObject().value("id")!=offset.value("id"))keptEntities.append(row);
    for(const auto& row:expected.value("points").toArray())if(row.toObject().value("id")!=offset.value("startPointId") && row.toObject().value("id")!=offset.value("endPointId"))keptPoints.append(row);
    expected["entities"]=keptEntities;expected["points"]=keptPoints;expected["constraints"]=QJsonArray{};
    const auto deleted=accept(SketchGeometry::deleteEntities(siblingDraft,{offset.value("id")}));
    check(deleted.value("parameters").toObject()==expected,"result Delete changes only its entity and two endpoints");
    SketchGeometry::evaluateFeature(deleted);
    for(const auto& ids:QList<QJsonArray>{{source.value("id")},{source.value("id"),offset.value("id")},{offset.value("id"),source.value("id")},{offset.value("id"),"stale"}}){
        const auto refused=SketchGeometry::deleteEntities(siblingDraft,ids);
        check(!refused.ok && refused.feature==siblingDraft,"source/mixed/stale Delete refuses atomically");
    }

    siblingDraft=accept(SketchSplineOffset::update(siblingDraft,{{"offsetEntityId",siblingId},{"signedDistanceMm",-6}}));
    QJsonObject firstSibling,secondSibling;
    for(const auto& value:siblingDraft["parameters"].toObject()["entities"].toArray()){
        const auto entity=value.toObject();
        if(entity["id"]==offset["id"])firstSibling=entity;
        if(entity["id"]==siblingId)secondSibling=entity;
    }
    check(firstSibling["signedDistanceMm"]==-5 && secondSibling["signedDistanceMm"]==-6,
        "explicit offset UUID edit does not silently change a sibling");
    const auto conversion=SketchSplineOffset::breakLink(draft,offset.value("id").toString());
    check(conversion.ok,"Break Link: "+conversion.error);
    auto independent=conversion.feature;const auto control=find(independent,"controlSpline");
    check(control.value("id")==offset.value("id")&&control.value("startPointId")==offset.value("startPointId")&&control.value("endPointId")==offset.value("endPointId"),"Break Link retains entity and endpoint IDs");
    check(find(independent,"fitSpline")==source&&find(independent,"fitSplineOffset").isEmpty(),"Break Link preserves source and removes association");
    check(point(independent,control.value("startPointId").toString())==point(draft,offset.value("startPointId").toString())&&point(independent,control.value("endPointId").toString())==point(draft,offset.value("endPointId").toString()),"Break Link preserves exact endpoint coordinates");
    const auto independentCurve=SketchSpline::evaluate(independent.value("parameters").toObject(),control).curve;
    double conversionError=0;for(int i=0;i<=1024;++i)conversionError=std::max(conversionError,independentCurve->Value(double(i)/1024).Distance(evaluated.curve->Value(double(i)/1024)));
    check(conversionError<=1e-3,"Break Link measured shape preservation");
    std::cout<<"Break Link poles "<<control.value("pointIds").toArray().size()<<" max sampled deviation mm "<<conversionError<<"\n";
    const auto sourceMoved=accept(SketchGeometry::movePoint(independent,{{"pointId",sourceIds[1]},{"uMm",9},{"vMm",20}}));
    const auto afterSource=SketchSpline::evaluate(sourceMoved.value("parameters").toObject(),control).curve;
    for(int i=0;i<=32;++i)check(independentCurve->Value(double(i)/32).Distance(afterSource->Value(double(i)/32))<1e-10,"source edit no longer drives converted result");
    const auto poleId=control.value("pointIds").toArray()[1].toString();const auto pole=point(sourceMoved,poleId);
    const auto poleMoved=accept(SketchGeometry::movePoint(sourceMoved,{{"pointId",poleId},{"uMm",pole.value("uMm").toDouble()+2},{"vMm",pole.value("vMm")}}));
    check(SketchSpline::evaluate(poleMoved.value("parameters").toObject(),control).curve->Pole(2).Distance(independentCurve->Pole(2))>1,"converted control pole is independently editable");
    for(const auto& invalidId:QStringList{source.value("id").toString(),"missing"}){
        const auto refusal=SketchSplineOffset::breakLink(draft,invalidId);check(!refusal.ok&&refusal.feature==draft,"wrong/stale Break Link refuses atomically");
    }
    // Explicit knot storage must remain normalized, clamped and count-valid.
    for(const auto& mutation:QList<QJsonObject>{QJsonObject{{"knots",QJsonArray{0,0,1}}},QJsonObject{{"multiplicities",QJsonArray{6,2.5,6}}},QJsonObject{{"knots",QJsonArray{0,2}}}}){
        auto invalid=control;for(auto i=mutation.begin();i!=mutation.end();++i)invalid[i.key()]=i.value();
        check(!SketchSpline::validate(independent.value("parameters").toObject(),invalid).isEmpty(),"malformed explicit knot data refused");
    }
    CadDocument convertedDocument;auto storedIndependent=independent;storedIndependent["ownerId"]=convertedDocument.authored().value("documentId");check(convertedDocument.commit(SketchGeometry::replaceFeature(convertedDocument.authored(),storedIndependent),"Break Link").isEmpty(),"commit independent curve");
    const auto convertedAuthored=convertedDocument.authored();convertedDocument.history().undo();check(convertedDocument.authored()!=convertedAuthored,"one-step conversion Undo");convertedDocument.history().redo();check(convertedDocument.authored()==convertedAuthored,"one-step conversion Redo");
    QTemporaryDir conversionTemp;const auto convertedPath=conversionTemp.filePath("independent.alloy");check(convertedDocument.save(convertedPath).isEmpty(),"save independent curve");CadDocument convertedReopen;check(convertedReopen.open(convertedPath).isEmpty()&&convertedReopen.authored()==convertedAuthored,"reopen exact authored poles and knots");
    const auto firstSource=point(draft,sourceIds.first().toString()),lastSource=point(draft,sourceIds.last().toString());
    draft=accept(SketchGeometry::addPointerEntity(draft,"line",{{"u1Mm",firstSource["uMm"]},{"v1Mm",firstSource["vMm"]},
        {"u2Mm",evaluated.start.X()},{"v2Mm",evaluated.start.Y()},
        {"startSnapTarget",QJsonObject{{"kind","point"},{"pointId",sourceIds.first()}}},
        {"endSnapTarget",QJsonObject{{"kind","point"},{"pointId",offset["startPointId"]}}}}));
    const auto oneDependent=SketchGeometry::deleteEntities(draft,{offset.value("id")});
    check(!oneDependent.ok && oneDependent.feature==draft,"one surviving closure Line refuses result Delete atomically");
    draft=accept(SketchGeometry::addPointerEntity(draft,"line",{{"u1Mm",lastSource["uMm"]},{"v1Mm",lastSource["vMm"]},
        {"u2Mm",evaluated.end.X()},{"v2Mm",evaluated.end.Y()},
        {"startSnapTarget",QJsonObject{{"kind","point"},{"pointId",sourceIds.last()}}},
        {"endSnapTarget",QJsonObject{{"kind","point"},{"pointId",offset["endPointId"]}}}}));
    const auto connectedConversion=SketchSplineOffset::breakLink(draft,offset.value("id").toString());
    check(connectedConversion.ok,"legal endpoint connector Lines survive Break Link: "+connectedConversion.error);
    check(SketchGeometry::evaluateFeature(connectedConversion.feature).profiles.size()==1,"converted curve keeps closed connector profile");
    auto dependent=draft;auto dependentParameters=dependent.value("parameters").toObject();dependentParameters["unsupportedConsumer"]=QJsonObject{{"entityId",offset.value("id")}};dependent["parameters"]=dependentParameters;
    const auto refusedConversion=SketchSplineOffset::breakLink(dependent,offset.value("id").toString());check(!refusedConversion.ok&&refusedConversion.feature==dependent&&refusedConversion.error.contains("dependent"),"semantic dependency refuses conversion atomically");
    draft=accept(SketchGeometry::solveDraft(draft));
    inspect(draft,offset["id"].toString());
    const auto connector=draft["parameters"].toObject()["entities"].toArray()[2].toObject();
    const auto unsupported=SketchGeometry::addConstraint(draft,{{"type","horizontal"},
        {"entityIds",QJsonArray{connector["id"]}}});
    check(!unsupported.ok && unsupported.feature==draft,"connector relation cannot turn a derived endpoint into a solver unknown");
    const auto profileId=SketchGeometry::evaluateFeature(draft).profiles[0].profileId;
    const auto dependentDelete=SketchGeometry::deleteEntities(draft,{offset.value("id")});
    check(!dependentDelete.ok && dependentDelete.feature==draft && dependentDelete.error.contains("endpoint"),"live closure Line prevents result Delete atomically");
    QJsonArray connectorIds;for(const auto& row:draft.value("parameters").toObject().value("entities").toArray())if(row.toObject().value("type")=="line")connectorIds.append(row.toObject().value("id"));
    const auto withoutConnectors=accept(SketchGeometry::deleteEntities(draft,connectorIds));
    const auto withoutOffset=accept(SketchGeometry::deleteEntities(withoutConnectors,{offset.value("id")}));
    check(withoutOffset.value("parameters").toObject().value("entities").toArray()==QJsonArray{source},"explicit closure removal permits result Delete without source cascade");

    const auto before=draft;
    const auto bad=SketchSplineOffset::update(draft,{{"offsetEntityId",offset["id"]},{"signedDistanceMm",10000}});
    check(!bad.ok && bad.feature==before,"invalid offset edit leaves authored draft exact");
    draft=accept(SketchSplineOffset::update(draft,{{"offsetEntityId",offset["id"]},{"signedDistanceMm",-4}}));
    inspect(draft,offset["id"].toString(),profileId);
    const auto changed=SketchSpline::edit(draft,"tangent",{{"entityId",source["id"]},{"pointId",sourceIds[1]},
        {"duMm",-5},{"dvMm",42}});
    check(changed.ok,"source tangent edit remains valid: "+changed.error);
    draft=changed.feature;inspect(draft,offset["id"].toString(),profileId);
    auto stock=SketchGeometry::addRectangle(SketchGeometry::newFeature(document.authored()["documentId"].toString(),"XY"),
        {{"u1Mm",-15},{"v1Mm",-30},{"u2Mm",8},{"v2Mm",110}});
    check(document.commit(SketchGeometry::replaceFeature(document.authored(),stock),"Stock sketch").isEmpty(),"commit stock sketch");
    const auto stockProfile=document.sketches().back().profiles[0].profileId;
    check(document.commit(CadDocument::extrusionCandidate(document.authored(),{{"sketchId",stock["id"]},
        {"profileId",stockProfile},{"distanceMm",8}}),"Stock").isEmpty(),"commit stock body");
    const auto stockBody=document.bodies()[0].bodyId;
    check(document.commit(SketchGeometry::replaceFeature(document.authored(),draft),"Closed offset strip").isEmpty(),"commit strip");
    check(document.commit(CadDocument::extrusionCandidate(document.authored(),{{"sketchId",draft["id"]},
        {"profileId",profileId},{"distanceMm",8},{"operation","intersect"},
        {"targetBodyIds",QJsonArray{stockBody}}}),"Strip Intersect").isEmpty(),
        "Intersect exact strip with retained stock: "+document.lastEvaluationError()["message"].toString());
    check(document.bodies().size()==1 && document.bodies()[0].bodyId==stockBody &&
          document.bodies()[0].summary["valid"].toBool() && document.bodies()[0].summary["closed"].toBool(),
          "Intersect produces one closed retained target");
    const auto fitPoint=point(draft,sourceIds[2].toString());
    const auto moved=SketchGeometry::movePoint(draft,{{"pointId",sourceIds[2]},
        {"uMm",fitPoint["uMm"].toDouble()+0.2},{"vMm",fitPoint["vMm"].toDouble()}});
    check(moved.ok,"source fit point move with retained offset: "+moved.error);
    inspect(moved.feature,offset["id"].toString(),profileId);
    check(document.commit(SketchGeometry::replaceFeature(document.authored(),moved.feature),"Move source fit point").isEmpty(),
        "downstream Intersect rebuilds after source move: "+document.lastEvaluationError()["message"].toString());
    check(document.bodies().size()==1 && document.bodies()[0].bodyId==stockBody,"edited strip retains Intersect body");
    const auto authored=document.authored();document.history().undo();check(document.authored()!=authored,"undo strip");
    document.history().redo();check(document.authored()==authored,"redo exact strip authoring");
    QTemporaryDir temp;const auto path=temp.filePath("offset.alloy");check(document.save(path).isEmpty(),"save strip");
    CadDocument reopened;check(reopened.open(path).isEmpty() && reopened.authored()==authored,"reopen exact strip authoring");
    std::cout<<"fit spline exact offset strip, stable profile, atomic edit and history passed\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
