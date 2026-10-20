// SPDX-License-Identifier: MIT
#include "Interference.h"
#include <BRepAlgoAPI_Common.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepGProp.hxx>
#include <BRep_Tool.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <GProp_GProps.hxx>
#include <Precision.hxx>
#include <Standard_Failure.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <cmath>
namespace {
QJsonObject failure(const QString& code,const QString& message){return {{"ok",false},{"completion","failed"},{"error",QJsonObject{{"code",code},{"message",message}}}};}
const EvaluatedBody* find(const std::vector<EvaluatedBody>& bodies,const QJsonObject& ref){for(const auto& b:bodies)if(b.bodyId==ref.value("bodyId") && b.occurrenceId==ref.value("occurrenceId").toString())return &b;return nullptr;}
QJsonArray point(const gp_Pnt& p){return {p.X(),p.Y(),p.Z()};}
}
QJsonObject Interference::resolve(const std::vector<EvaluatedBody>& bodies,const QJsonObject& ref){
    if(ref.value("bodyId").toString().isEmpty())return failure("interference_body_required","Select a committed solid body using bodyId");
    const EvaluatedBody* found=nullptr;int count=0;
    for(const auto& b:bodies)if(b.bodyId==ref.value("bodyId") && (!ref.contains("occurrenceId") || b.occurrenceId==ref.value("occurrenceId"))){found=&b;++count;}
    if(count>1)return failure("interference_ambiguous","Specify occurrenceId to inspect exactly one placed body");
    if(!found || !found->visible || found->shape.IsNull())return failure("interference_unavailable","Selected body or occurrence is hidden or unavailable; choose a current visible solid");
    try {
        int solids=0;for(TopExp_Explorer e(found->shape,TopAbs_SOLID);e.More();e.Next())++solids;
        if(solids!=1 || found->summary.value("type")=="surface")return failure("interference_solid_required","Select one closed solid body; surfaces and meshes are unsupported");
        for(TopExp_Explorer e(found->shape,TopAbs_SHELL);e.More();e.Next())if(!BRep_Tool::IsClosed(e.Current()))return failure("interference_solid_required","Selected solid has an open shell; repair it before inspection");
        if(!BRepCheck_Analyzer(found->shape).IsValid())return failure("interference_invalid_geometry","Selected solid is invalid; repair its source before inspection");
    }catch(const Standard_Failure&){return failure("interference_invalid_geometry","Selected BRep could not be validated; repair its source");}
    QJsonObject identity{{"bodyId",found->bodyId},{"bodyName",found->name},{"componentId",found->ownerId},{"featureId",found->featureId}};
    if(!found->occurrenceId.isEmpty())identity["occurrenceId"]=found->occurrenceId;
    return identity;
}
Interference::Result Interference::evaluate(const std::vector<EvaluatedBody>& bodies,const QJsonArray& refs,const QString& units){
    Result result;
    if(refs.size()!=2){result.json=failure("interference_selection_count","Choose exactly two committed closed solid bodies");return result;}
    const auto a=resolve(bodies,refs[0].toObject()),b=resolve(bodies,refs[1].toObject());
    if(a.contains("error") || b.contains("error")){result.json=a.contains("error")?a:b;return result;}
    if(a.value("bodyId")==b.value("bodyId") && a.value("occurrenceId")==b.value("occurrenceId")){result.json=failure("interference_duplicate","Choose two distinct bodies or occurrences");return result;}
    const auto* first=find(bodies,a);const auto* second=find(bodies,b);
    try {
        BRepAlgoAPI_Common common(first->shape,second->shape);common.Build();
        if(!common.IsDone() || common.HasErrors())throw Standard_Failure("Common failed");
        GProp_GProps props;BRepGProp::VolumeProperties(common.Shape(),props);const double volume=std::abs(props.Mass());
        if(!std::isfinite(volume))throw Standard_Failure("Nonfinite volume");
        // Explicit canonical threshold, independent of display precision.
        constexpr double volumeToleranceMm3=1e-9;
        QJsonObject canonical{{"overlapVolumeMm3",volume>volumeToleranceMm3?volume:0.0},{"clearanceMm",0.0}};
        QString kind="overlap";double distance=0;
        if(volume>volumeToleranceMm3)result.feedback=common.Shape();
        else {
            BRepExtrema_DistShapeShape extrema(first->shape,second->shape);extrema.Perform();
            if(!extrema.IsDone() || extrema.NbSolution()<1)throw Standard_Failure("Distance failed");
            distance=extrema.Value();if(!std::isfinite(distance) || distance<0)throw Standard_Failure("Invalid distance");
            kind=distance<=Precision::Confusion()?"touching":"clearance";
            if(kind=="touching")distance=0;
            const auto p=extrema.PointOnShape1(1),q=extrema.PointOnShape2(1);
            canonical["clearanceMm"]=distance;canonical["closestPointsMm"]=QJsonArray{point(p),point(q)};
            if(distance>0)result.feedback=BRepBuilderAPI_MakeEdge(p,q).Shape();
        }
        const auto unit=units=="in"?QString("in"):QString("mm");const double scale=unit=="in"?25.4:1.;
        result.json={{"kind",kind},{"first",a},{"second",b},{"canonical",canonical},
            {"display",QJsonObject{{"volume",canonical.value("overlapVolumeMm3").toDouble()/(scale*scale*scale)},{"length",distance/scale},{"volumeUnit",unit+"³"},{"lengthUnit",unit}}},
            {"tolerance",QJsonObject{{"volumeMm3",volumeToleranceMm3},{"lengthMm",Precision::Confusion()}}},{"authoredChange",false}};
        result.sources={first->shape,second->shape};
    }catch(const Standard_Failure&){result.json=failure("interference_evaluation_failed","Exact solid inspection failed; repair or simplify the selected bodies and retry");}
    return result;
}
