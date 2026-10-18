// SPDX-License-Identifier: MIT
#include "SketchGeometry.h"
#include "GeometryKernel.h"
#include <QCoreApplication>
#include <QJsonDocument>
#include <QUuid>
#include <cmath>
#include <iostream>
#include <stdexcept>
namespace {
void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
QString id(){return QUuid::createUuid().toString(QUuid::WithoutBraces);}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    auto sketch=SketchGeometry::newConstructionPlaneFeature(id(),id(),"Three linked port clearances");
    auto parameters=sketch["parameters"].toObject();QJsonArray projections,lines;std::array<QJsonArray,3> groups;
    const std::array<std::array<double,4>,3> bounds{{{-28,-7.5,7.6,21.1},{-7,9,7.6,23.6},{11,27,7.6,23.6}}};
    for(int g=0;g<3;++g){const auto b=bounds[g];const std::array<std::array<double,4>,4> sides{{{b[0],b[2],b[1],b[2]},
        {b[1],b[2],b[1],b[3]},{b[1],b[3],b[0],b[3]},{b[0],b[3],b[0],b[2]}}};
        const auto body=id();for(const auto& side:sides){const auto edge=id(),projection=id();
            const QJsonObject source{{"bodyId",body},{"edgeId",edge}};
            projections.append(QJsonObject{{"id",projection},{"source",source},{"profile",true}});groups[g].append(projection);
            lines.append(QJsonObject{{"id",projection},{"source",source},{"u1Mm",side[0]},{"v1Mm",side[1]},
                {"u2Mm",side[2]},{"v2Mm",side[3]}});
        }}
    parameters["projections"]=projections;sketch["parameters"]=parameters;
    const QJsonObject context{{"lines",lines},{"circles",QJsonArray{}},{"points",QJsonArray{}}};
    for(const auto& group:groups){const auto result=SketchGeometry::addClosedRectOffset(sketch,group,1,context);
        if(!result.ok)std::cerr<<result.error.toStdString()<<'\n';check(result.ok,"Offset group failed");sketch=result.feature;}
    const auto p=sketch["parameters"].toObject();check(p["closedRectOffsets"].toArray().size()==3,"Three retained groups");
    for(const auto& row:p["projections"].toArray())check(row.toObject()["profile"]==false,"Original purple Project must not duplicate cut profiles");
    // Whole-owner Delete preserves each projection/support identity, restores
    // prior profile flags, and leaves independent owners byte-equivalent.
    const auto owner=p.value("closedRectOffsets").toArray()[0].toObject();
    const auto target=owner.value("entityIds").toArray().at(0);
    auto deleted=SketchGeometry::deleteEntities(sketch,{target},{},context);
    check(deleted.ok,qPrintable(deleted.error));auto expected=p;
    auto retainedGroups=p.value("closedRectOffsets").toArray();retainedGroups.removeAt(0);expected["closedRectOffsets"]=retainedGroups;
    for(const auto& field:{"entities","points"}) {
        QJsonArray kept;const auto ids=owner.value(QString(field)=="entities"?"entityIds":"pointIds").toArray();
        for(const auto& row:p.value(field).toArray())if(!ids.contains(row.toObject().value("id")))kept.append(row);
        expected[field]=kept;
    }
    auto restored=p.value("projections").toArray();for(int i=0;i<4;++i){auto row=restored[i].toObject();row["profile"]=true;restored[i]=row;}expected["projections"]=restored;
    expected["constraints"]=QJsonArray{};
    check(deleted.feature.value("parameters").toObject()==expected,"Delete changed independent owner/source IDs or retained result members");
    const auto refuse=[&](const QJsonObject& fixture,const QJsonArray& ids){const auto r=SketchGeometry::deleteEntities(fixture,ids,{},context);check(!r.ok && r.feature==fixture,"Unsafe owner Delete must refuse atomically");};
    refuse(sketch,{groups[0][0]});refuse(sketch,{target,groups[0][0]});refuse(sketch,{target,"stale"});
    auto legacy=sketch;auto legacyP=p;auto legacyGroups=p.value("closedRectOffsets").toArray();auto legacyOwner=owner;legacyOwner.remove("sourceProfiles");legacyGroups[0]=legacyOwner;legacyP["closedRectOffsets"]=legacyGroups;legacy["parameters"]=legacyP;
    check(SketchGeometry::solveDraft(legacy,context).ok,"Legacy owner must remain editable");refuse(legacy,{target});
    auto malformed=sketch;auto malformedP=p;auto malformedGroups=p.value("closedRectOffsets").toArray();auto malformedOwner=owner;
    auto malformedCorners=owner.value("pointIds").toArray();malformedCorners[1]=malformedCorners[0];malformedOwner["pointIds"]=malformedCorners;malformedGroups[0]=malformedOwner;malformedP["closedRectOffsets"]=malformedGroups;malformed["parameters"]=malformedP;refuse(malformed,{target});
    auto missingSource=sketch;auto missingP=p;auto missingProjections=p.value("projections").toArray();missingProjections.removeAt(0);missingP["projections"]=missingProjections;missingSource["parameters"]=missingP;refuse(missingSource,{target});
    auto source=SketchGeometry::newConstructionPlaneFeature(id(),id(),"Prior mixed profile flags");auto sourceP=source.value("parameters").toObject();auto mixed=projections;
    for(int i=0;i<mixed.size();++i){auto row=mixed[i].toObject();row["profile"]=i%2==0;mixed[i]=row;}sourceP["projections"]=mixed;source["parameters"]=sourceP;
    auto mixedOffset=SketchGeometry::addClosedRectOffset(source,groups[0],1,context);check(mixedOffset.ok,"Mixed prior profiles must offset");
    const auto mixedTarget=mixedOffset.feature.value("parameters").toObject().value("closedRectOffsets").toArray()[0].toObject().value("entityIds").toArray().at(0);
    sourceP["constraints"]=QJsonArray{};sourceP["dimensions"]=QJsonArray{};
    const auto mixedDeleted=SketchGeometry::deleteEntities(mixedOffset.feature,{mixedTarget},{},context);
    check(mixedDeleted.ok && mixedDeleted.feature.value("parameters").toObject()==sourceP,"Prior false profile flags must restore exactly");
    const auto corner=owner.value("pointIds").toArray().at(0);QJsonObject cornerPoint;for(const auto& row:p.value("points").toArray())if(row.toObject().value("id")==corner)cornerPoint=row.toObject();
    auto dependent=SketchGeometry::addLine(sketch,{{"u1Mm",cornerPoint.value("uMm")},{"v1Mm",cornerPoint.value("vMm")},{"u2Mm",-45},{"v2Mm",-15},{"startPointId",corner}});
    check(SketchGeometry::solveDraft(dependent,context).ok,"Shared corner fixture solves");refuse(dependent,{target});
    const auto measured=SketchGeometry::addDimension(sketch,{{"kind","linear"},{"entityId",target},{"driving",false}},context);
    check(measured.ok,"Result dimension fixture solves");refuse(measured.feature,{target});
    auto result=SketchGeometry::evaluateFeature(sketch,SketchGeometry::frame("XZ"),context);
    check(!result.profiles.empty(),"Overlapping offset contours must form bounded regions");
    auto united=GeometryKernel::unionProfiles(result.profiles);
    check(std::abs(united.summary["areaMm2"].toDouble()-973.5)<1e-5,"Union of touching/overlapping port clearances has exact area");
    QJsonArray groupIds;for(const auto& row:p["closedRectOffsets"].toArray())groupIds.append(row.toObject()["id"]);
    const auto cells=GeometryKernel::closedRectOffsetUnionCells(result,sketch,groupIds);
    check(cells.size()==result.profiles.size(),"Explicit group union selects the original four cells");
    auto update=sketch;auto updateParameters=update["parameters"].toObject();auto updateGroups=updateParameters["closedRectOffsets"].toArray();
    auto firstGroup=updateGroups[0].toObject();firstGroup["distanceMm"]=1.1;updateGroups[0]=firstGroup;
    updateParameters["closedRectOffsets"]=updateGroups;update["parameters"]=updateParameters;
    auto updated=SketchGeometry::solveDraft(update,context);check(updated.ok,"Offset distance edit resolves");
    const auto editedRegions=SketchGeometry::evaluateFeature(updated.feature,SketchGeometry::frame("XZ"),context);
    const auto editedCells=GeometryKernel::closedRectOffsetUnionCells(editedRegions,updated.feature,groupIds);
    const auto editedUnion=GeometryKernel::unionProfiles(editedCells);
    check(std::abs(editedUnion.summary["areaMm2"].toDouble()-979.43)<1e-5,
        "Explicit group union follows the changed contact topology without double counting");
    QSet<QString> baselineIds,editedIds;for(const auto& profile:result.profiles)baselineIds.insert(profile.profileId);
    for(const auto& profile:editedRegions.profiles)editedIds.insert(profile.profileId);
    check(baselineIds!=editedIds,"Changing contact topology must change ordinary cell profile identities");
    const auto firstId=p["closedRectOffsets"].toArray()[0].toObject()["pointIds"].toArray()[0].toString();
    auto shifted=context;auto shiftedLines=lines;
    for(int i=0;i<4;++i){auto edge=shiftedLines[i].toObject();edge["u1Mm"]=edge["u1Mm"].toDouble()+.25;
        edge["u2Mm"]=edge["u2Mm"].toDouble()+.25;shiftedLines[i]=edge;}
    shifted["lines"]=shiftedLines;const auto followed=SketchGeometry::solveDraft(sketch,shifted);
    check(followed.ok,"Complete upstream connector move must retain offset");
    auto expanded=SketchGeometry::expandedProjectionFeature(followed.feature,shifted)["parameters"].toObject();
    QJsonObject movedCorner;for(const auto& value:expanded["points"].toArray())if(value.toObject()["id"]==firstId)movedCorner=value.toObject();
    check(std::abs(movedCorner["uMm"].toDouble()+28.75)<1e-6,"Derived offset corner follows current linked source");
    auto edited=context;auto changed=lines;auto line=changed[4].toObject();line["u1Mm"]=line["u1Mm"].toDouble()+.25;line["u2Mm"]=line["u2Mm"].toDouble()+.25;changed[4]=line;
    // A single altered edge breaks the complete rectangle and must refuse atomically.
    edited["lines"]=changed;check(!SketchGeometry::solveDraft(sketch,edited).ok,"Partial source edit must refuse");
    auto collapsed=SketchGeometry::newConstructionPlaneFeature(id(),id(),"Collapsed");auto cp=collapsed["parameters"].toObject();cp["projections"]=projections;collapsed["parameters"]=cp;
    check(!SketchGeometry::addClosedRectOffset(collapsed,groups[0],-12,context).ok,"Inward collapse must refuse");
    check(!firstId.isEmpty(),"Generated stable point ID");
    std::cout<<"Three linked closed offsets, overlapping union and atomic refusals passed\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
