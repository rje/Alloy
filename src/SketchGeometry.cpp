#include "SketchGeometry.h"
#include "LengthExpression.h"
#include "SketchSolver.h"
#include "SketchSpline.h"
#include "SketchConic.h"
#include "SketchText.h"
#include "RoundedLoopOffset.h"
#include "SketchMirror.h"
#include "SketchArcSlot.h"
#include "SketchSplineOffset.h"
#include "SketchOpenChainOffset.h"
#include "SketchEllipse.h"
#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRep_Builder.hxx>
#include <Bnd_Box.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Edge.hxx>
#include <gp_Circ.hxx>
#include <gp_Elips.hxx>
#include <gp_Ax2.hxx>
#include <numbers>
#include <QJsonArray>
#include <QMap>
#include <QPointF>
#include <QUuid>
#include <cmath>
#include <algorithm>
#include <array>
#include <tuple>
#include <stdexcept>

namespace {
constexpr double minimumLength = 0.001;
constexpr double maximumLength = 100000;
constexpr double degrees = std::numbers::pi / 180.0;
constexpr double endpointTolerance = 1e-6;
QString newId() { return QUuid::createUuid().toString(QUuid::WithoutBraces); }
bool validId(const QString& id) {
    return !QUuid(id).isNull() && QUuid(id).toString(QUuid::WithoutBraces) == id;
}
[[noreturn]] void reject(const QString& message) { throw std::invalid_argument(message.toStdString()); }
double coordinate(const QJsonObject& object, const QString& key) {
    const auto value = object.value(key);
    if (!value.isDouble() || !std::isfinite(value.toDouble()) || std::abs(value.toDouble()) > maximumLength)
        reject("Sketch coordinate out of supported range: " + key);
    return value.toDouble();
}
QSet<QString> closedRectOffsetPointIds(const QJsonObject& parameters) {
    QSet<QString> result;
    for(const auto& value:parameters["closedRectOffsets"].toArray())
        for(const auto& id:value.toObject()["pointIds"].toArray())result.insert(id.toString());
    return result;
}
std::array<double,4> linkedRectangleBounds(const QJsonObject& parameters,const QJsonArray& sourceIds) {
    if(sourceIds.size()!=4)reject("Closed Offset needs four linked rectangle edges");
    QMap<QString,QPointF> points;for(const auto& value:parameters["points"].toArray()) {
        const auto p=value.toObject();points[p["id"].toString()]={coordinate(p,"uMm"),coordinate(p,"vMm")};
    }
    QMap<QString,QJsonObject> lines;for(const auto& value:parameters["entities"].toArray()) {
        const auto e=value.toObject();if(e["type"]=="line")lines[e["id"].toString()]=e;
    }
    QSet<QString> distinct;double x0=maximumLength,x1=-maximumLength,y0=maximumLength,y1=-maximumLength;
    std::array<std::pair<QPointF,QPointF>,4> segments;
    for(int i=0;i<4;++i) {
        const auto id=sourceIds[i].toString();
        if(!validId(id) || distinct.contains(id) || !lines.contains(id))reject("Closed Offset source edge is unavailable or repeated");
        distinct.insert(id);const auto line=lines[id];
        if(!points.contains(line["startPointId"].toString()) || !points.contains(line["endPointId"].toString()))
            reject("Closed Offset source endpoint is unavailable");
        const auto a=points[line["startPointId"].toString()],b=points[line["endPointId"].toString()];segments[i]={a,b};
        x0=std::min({x0,a.x(),b.x()});x1=std::max({x1,a.x(),b.x()});
        y0=std::min({y0,a.y(),b.y()});y1=std::max({y1,a.y(),b.y()});
    }
    if(x1-x0<minimumLength || y1-y0<minimumLength)reject("Closed Offset rectangle is degenerate");
    QSet<int> sides;
    const auto near=[](double a,double b){return std::abs(a-b)<1e-6;};
    for(const auto& [a,b]:segments) {
        int side=-1;
        if(near(a.y(),y0) && near(b.y(),y0) && near(std::abs(a.x()-b.x()),x1-x0))side=0;
        else if(near(a.x(),x1) && near(b.x(),x1) && near(std::abs(a.y()-b.y()),y1-y0))side=1;
        else if(near(a.y(),y1) && near(b.y(),y1) && near(std::abs(a.x()-b.x()),x1-x0))side=2;
        else if(near(a.x(),x0) && near(b.x(),x0) && near(std::abs(a.y()-b.y()),y1-y0))side=3;
        if(side<0 || sides.contains(side))reject("Closed Offset requires one complete edge on each rectangle side");
        sides.insert(side);
    }
    return {x0,x1,y0,y1};
}
QJsonObject deriveClosedRectOffsets(const QJsonObject& parameters) {
    if(!parameters.contains("closedRectOffsets"))return parameters;
    auto result=parameters;auto points=result["points"].toArray();
    for(const auto& value:parameters["closedRectOffsets"].toArray()) {
        const auto group=value.toObject();const auto bounds=linkedRectangleBounds(parameters,group["sourceEntityIds"].toArray());
        const double d=coordinate(group,"distanceMm");
        if(std::abs(d)<minimumLength)reject("Closed Offset distance must be nonzero");
        const double x0=bounds[0]-d,x1=bounds[1]+d,y0=bounds[2]-d,y1=bounds[3]+d;
        if(x1-x0<minimumLength || y1-y0<minimumLength)reject("Closed Offset collapses the rectangle");
        const std::array<QPointF,4> corners{QPointF{x0,y0},QPointF{x1,y0},QPointF{x1,y1},QPointF{x0,y1}};
        const auto ids=group["pointIds"].toArray();
        if(ids.size()!=4)reject("Closed Offset lost generated corners");
        for(int i=0;i<4;++i) {
            bool found=false;for(int j=0;j<points.size();++j)if(points[j].toObject()["id"]==ids[i]) {
                auto point=points[j].toObject();point["uMm"]=corners[i].x();point["vMm"]=corners[i].y();points[j]=point;found=true;break;
            }
            if(!found)reject("Closed Offset generated corner is unavailable");
        }
    }
    result["points"]=points;return result;
}
void validateDraft(const QJsonObject& draft, const QSet<QString>& derivedPointIds = {}) {
    if (draft["type"].toString() != "sketch" || draft.contains("bodyId")
        || !validId(draft["id"].toString()) || !validId(draft["ownerId"].toString())
        || !draft["name"].isString() || draft["name"].toString().trimmed().isEmpty()
        || !draft["visible"].isBool() || !draft["parameters"].isObject())
        reject("Invalid authored sketch");
    QSet<QString> ids{draft["id"].toString(), draft["ownerId"].toString()};
    const auto error = SketchGeometry::validateParameters(draft["parameters"].toObject(), ids,derivedPointIds);
    if (!error.isEmpty()) reject(error);
}
void validateRoundedIntent(const QJsonObject& parameters) {
    if(!parameters.contains("roundedRectangleIntent"))return;
    const auto intent=parameters["roundedRectangleIntent"].toObject();
    const auto ids=intent["sideIds"].toArray(),directions=intent["sideDirections"].toArray();
    if(ids.size()!=4 || directions.size()!=4)reject("Rounded rectangle has incomplete side intent");
    QMap<QString,QJsonObject> points,entities,dimensions;
    for(const auto& value:parameters["points"].toArray()){const auto p=value.toObject();points[p["id"].toString()]=p;}
    for(const auto& value:parameters["entities"].toArray()){const auto e=value.toObject();entities[e["id"].toString()]=e;}
    for(const auto& value:parameters["dimensions"].toArray()){const auto d=value.toObject();dimensions[d["id"].toString()]=d;}
    const auto width=dimensions[intent["widthId"].toString()],height=dimensions[intent["heightId"].toString()];
    if(width["kind"]!="horizontal" || height["kind"]!="vertical" || !width["driving"].toBool() || !height["driving"].toBool())
        reject("Rounded rectangle lost driving overall dimensions");
    const double w=width["valueMm"].toDouble(),h=height["valueMm"].toDouble(),tol=1e-5;
    if(w<=minimumLength || h<=minimumLength || intent["originPointId"]!=parameters["originPointId"])
        reject("Rounded rectangle lost origin or positive overall size");
    const auto at=[&](const QString& id){const auto p=points[id];return QPointF(p["uMm"].toDouble(),p["vMm"].toDouble());};
    if(std::hypot(at(intent["originPointId"].toString()).x(),at(intent["originPointId"].toString()).y())>tol)
        reject("Rounded rectangle origin moved");
    int horizontal=0,vertical=0;QSet<QString> distinct;
    for(int i=0;i<4;++i){const auto id=ids[i].toString();const auto e=entities[id];if(e["type"]!="line" || distinct.contains(id))reject("Rounded rectangle side identity changed");distinct.insert(id);
        const auto a=at(e["startPointId"].toString()),b=at(e["endPointId"].toString());
        const double dx=b.x()-a.x(),dy=b.y()-a.y();const int sign=directions[i].toInt();
        if(sign!=1 && sign!=-1)reject("Rounded rectangle side direction is invalid");
        if(std::abs(dy)<tol && std::abs(dx)>minimumLength){++horizontal;if(dx*sign<=minimumLength ||
            (std::abs(a.y())>tol && std::abs(a.y()+h)>tol) || std::min(a.x(),b.x()) < -tol || std::max(a.x(),b.x())>w+tol)reject("Rounded rectangle horizontal side folded or left its support");}
        else if(std::abs(dx)<tol && std::abs(dy)>minimumLength){++vertical;if(dy*sign<=minimumLength ||
            (std::abs(a.x())>tol && std::abs(a.x()-w)>tol) || std::min(a.y(),b.y()) < -h-tol || std::max(a.y(),b.y())>tol)reject("Rounded rectangle vertical side folded or left its support");}
        else reject("Rounded rectangle side is no longer axis aligned");
    }
    if(horizontal!=2 || vertical!=2)reject("Rounded rectangle lost opposing supports");
    for(const auto& dimension:{width,height}){
        const auto refs=dimension["pointIds"].toArray();if(refs.size()!=2)reject("Rounded rectangle overall witnesses are missing");
        const auto a=at(refs[0].toString()),b=at(refs[1].toString());
        if(std::abs(dimension["kind"]=="horizontal" ? std::abs(a.x()-b.x())-w : std::abs(a.y()-b.y())-h)>tol)
            reject("Rounded rectangle overall dimension no longer spans its supports");
    }
    // Only arcs belonging to the retained rounded boundary participate in this
    // invariant. Other curves may be authored in the same sketch later.
    QJsonArray cornerArcIds=intent["cornerArcIds"].toArray();
    if(!intent.contains("cornerArcIds")){
        // Accept the first candidate's five-field saved intent without rewriting
        // it: its two arc-tangent references to retained side IDs identify each
        // corner arc, independently of any subsequently authored loose curves.
        QMap<QString,QSet<QString>> sideContacts;
        for(const auto& value:parameters["constraints"].toArray()){
            const auto relation=value.toObject();const auto refs=relation["entityIds"].toArray();
            if(relation["type"]=="arcTangent" && refs.size()==2 && ids.contains(refs[1]))
                sideContacts[refs[0].toString()].insert(refs[1].toString());
        }
        for(auto it=sideContacts.begin();it!=sideContacts.end();++it)if(it.value().size()==2)cornerArcIds.append(it.key());
    }
    if(cornerArcIds.isEmpty() || cornerArcIds.size()>4)reject("Rounded rectangle has an unsupported corner count");
    QSet<QString> distinctArcs;QSet<int> corners;
    for(const auto& value:cornerArcIds){const auto id=value.toString();const auto e=entities[id];
        if(e["type"]!="arc" || distinctArcs.contains(id))reject("Rounded rectangle corner arc identity changed");distinctArcs.insert(id);
        const double r=e["radiusMm"].toDouble();const auto c=at(e["centerPointId"].toString());
        if(std::abs(std::abs(e["sweepAngleDeg"].toDouble())-90)>1e-3 || r<=minimumLength)
            reject("Rounded rectangle arc changed its quarter-circle branch");
        const bool left=std::abs(c.x()-r)<tol,right=std::abs(c.x()-(w-r))<tol;
        const bool top=std::abs(c.y()+r)<tol,bottom=std::abs(c.y()-(-h+r))<tol;
        if((left==right) || (top==bottom))reject("Rounded rectangle arc left an outer corner");
        const int corner=(right ? 1 : 0)+(bottom ? 2 : 0);if(corners.contains(corner))reject("Rounded rectangle duplicated a corner arc");corners.insert(corner);
        const auto a=at(e["startPointId"].toString()),b=at(e["endPointId"].toString());
        const auto onVertical=[&](const QPointF& p){return std::abs(p.x()-(right ? w : 0))<tol;};
        const auto onHorizontal=[&](const QPointF& p){return std::abs(p.y()-(bottom ? -h : 0))<tol;};
        if(!(onVertical(a)&&onHorizontal(b)) && !(onHorizontal(a)&&onVertical(b)))
            reject("Rounded rectangle arc contacts left their adjacent supports");
    }
}
void validateSlotGroups(const QJsonObject& parameters) {
    if(!parameters.contains("slotGroups"))return;
    QMap<QString,QJsonObject> points,entities;
    for(const auto& value:parameters["points"].toArray()){const auto p=value.toObject();points[p["id"].toString()]=p;}
    for(const auto& value:parameters["entities"].toArray()){const auto e=value.toObject();entities[e["id"].toString()]=e;}
    auto at=[&](const QJsonValue& id){const auto p=points[id.toString()];return QPointF(p["uMm"].toDouble(),p["vMm"].toDouble());};
    constexpr double tol=1e-5;
    for(const auto& value:parameters["slotGroups"].toArray()){
        const auto group=value.toObject(),guide=entities[group["centerlineId"].toString()];
        const auto sides=group["sideIds"].toArray(),arcs=group["arcIds"].toArray();
        const auto top=entities[sides[0].toString()],bottom=entities[sides[1].toString()];
        const auto right=entities[arcs[0].toString()],left=entities[arcs[1].toString()];
        if(guide["type"]!="line" || !guide["construction"].toBool() || top["type"]!="line" || bottom["type"]!="line"
            || right["type"]!="arc" || left["type"]!="arc" || right["centerPointId"]!=guide["endPointId"]
            || left["centerPointId"]!=guide["startPointId"] || right["startPointId"]!=top["endPointId"]
            || right["endPointId"]!=bottom["startPointId"] || left["startPointId"]!=bottom["endPointId"]
            || left["endPointId"]!=top["startPointId"] || top["construction"].toBool() || bottom["construction"].toBool())
            reject("Center Point Slot boundary identity changed");
        const auto a=at(guide["startPointId"]),b=at(guide["endPointId"]),c=at(group["centerPointId"]);
        const auto d=b-a;const double length=std::hypot(d.x(),d.y());
        if(length<minimumLength || std::hypot(c.x()-(a.x()+b.x())/2,c.y()-(a.y()+b.y())/2)>tol)
            reject("Center Point Slot lost its midpoint or length");
        const double r=right["radiusMm"].toDouble();
        if(r<minimumLength || std::abs(r-left["radiusMm"].toDouble())>tol ||
            std::abs(right["sweepAngleDeg"].toDouble()+180)>1e-3 || std::abs(left["sweepAngleDeg"].toDouble()+180)>1e-3)
            reject("Center Point Slot lost equal semicircular ends");
        const auto cross=[&](const QPointF& p,const QPointF& origin){return (d.x()*(p.y()-origin.y())-d.y()*(p.x()-origin.x()))/length;};
        const auto dot=[&](const QPointF& p,const QPointF& origin){return (d.x()*(p.x()-origin.x())+d.y()*(p.y()-origin.y()))/length;};
        for(const auto& id:{top["startPointId"],top["endPointId"]})
            if(std::abs(cross(at(id),id==top["startPointId"] ? a : b)-r)>tol ||
                std::abs(dot(at(id),id==top["startPointId"] ? a : b))>tol)reject("Slot upper side left its tangent contacts");
        for(const auto& id:{bottom["startPointId"],bottom["endPointId"]})
            if(std::abs(cross(at(id),id==bottom["startPointId"] ? b : a)+r)>tol ||
                std::abs(dot(at(id),id==bottom["startPointId"] ? b : a))>tol)reject("Slot lower side left its tangent contacts");
    }
}
void validateArcSlotGroups(const QJsonObject& parameters) {
    if(!parameters.contains("arcSlotGroups"))return;
    QMap<QString,QJsonObject> entities;
    for(const auto& value:parameters["entities"].toArray()){
        const auto e=value.toObject();entities[e["id"].toString()]=e;
    }
    for(const auto& value:parameters["arcSlotGroups"].toArray()){
        const auto group=value.toObject();const auto caps=group["capIds"].toArray();
        if(caps.size()!=2)reject("Arc slot lost its two caps");
        const auto path=entities[group["pathArcId"].toString()],outer=entities[group["outerArcId"].toString()];
        const auto inner=entities[group["innerArcId"].toString()],lastCap=entities[caps[0].toString()],firstCap=entities[caps[1].toString()];
        if(path["type"]!="arc" || !path["construction"].toBool() || outer["type"]!="arc" || inner["type"]!="arc" ||
            lastCap["type"]!="arc" || firstCap["type"]!="arc" || outer["construction"].toBool() ||
            inner["construction"].toBool() || lastCap["construction"].toBool() || firstCap["construction"].toBool() ||
            outer["centerPointId"]!=path["centerPointId"] || inner["centerPointId"]!=path["centerPointId"] ||
            lastCap["centerPointId"]!=path["endPointId"] || firstCap["centerPointId"]!=path["startPointId"] ||
            outer["endPointId"]!=lastCap["startPointId"] || lastCap["endPointId"]!=inner["startPointId"] ||
            inner["endPointId"]!=firstCap["startPointId"] || firstCap["endPointId"]!=outer["startPointId"])
            reject("Arc slot lost its analytic boundary identities");
    }
    const auto expected=SketchArcSlot::derive(parameters);
    QMap<QString,QJsonObject> expectedPoints,expectedEntities;
    for(const auto& value:expected["points"].toArray()){
        const auto p=value.toObject();expectedPoints[p["id"].toString()]=p;
    }
    for(const auto& value:expected["entities"].toArray()){
        const auto e=value.toObject();expectedEntities[e["id"].toString()]=e;
    }
    const auto derivedPoints=SketchArcSlot::derivedPointIds(parameters),derivedArcs=SketchArcSlot::derivedArcIds(parameters);
    for(const auto& value:parameters["points"].toArray()){
        const auto p=value.toObject();if(!derivedPoints.contains(p["id"].toString()))continue;
        const auto e=expectedPoints[p["id"].toString()];
        if(std::hypot(p["uMm"].toDouble()-e["uMm"].toDouble(),p["vMm"].toDouble()-e["vMm"].toDouble())>1e-5)
            reject("Arc slot contact left its tangent radial position");
    }
    for(const auto& value:parameters["entities"].toArray()){
        const auto arc=value.toObject();if(!derivedArcs.contains(arc["id"].toString()))continue;
        const auto e=expectedEntities[arc["id"].toString()];
        if(std::abs(arc["radiusMm"].toDouble()-e["radiusMm"].toDouble())>1e-5 ||
            std::abs(std::remainder(arc["startAngleDeg"].toDouble()-e["startAngleDeg"].toDouble(),360.0))>1e-5 ||
            std::abs(arc["sweepAngleDeg"].toDouble()-e["sweepAngleDeg"].toDouble())>1e-5)
            reject("Arc slot analytic boundary left its retained path");
    }
}
void validateArguments(const QJsonObject& arguments, bool line) {
    const QSet<QString> fields = line ? QSet<QString>{"u1Mm", "v1Mm", "u2Mm", "v2Mm", "startPointId", "endPointId"}
                                     : QSet<QString>{"u1Mm", "v1Mm", "u2Mm", "v2Mm"};
    for (auto it = arguments.begin(); it != arguments.end(); ++it)
        if (!fields.contains(it.key())) reject("Unknown sketch argument: " + it.key());
    for (const auto& key : {"u1Mm", "v1Mm", "u2Mm", "v2Mm"}) coordinate(arguments, key);
    for (const auto& key : {"startPointId", "endPointId"})
        if (arguments.contains(key) && (!arguments[key].isString() || !validId(arguments[key].toString())))
            reject("Expected a stable sketch point UUID");
}
QString addPoint(QJsonArray& points, double u, double v, const QString& requestedId = {}) {
    for (const auto& entry : points) {
        const auto point = entry.toObject();
        if (!requestedId.isEmpty() && point["id"].toString() != requestedId) continue;
        if (std::hypot(point["uMm"].toDouble()-u, point["vMm"].toDouble()-v) < 1e-9) return point["id"].toString();
        if (!requestedId.isEmpty()) reject("Point ID coordinates do not match the authored endpoint");
    }
    if (!requestedId.isEmpty()) reject("Sketch point does not exist");
    const auto id = newId();
    points.append(QJsonObject{{"id", id}, {"uMm", u}, {"vMm", v}});
    return id;
}
gp_Pnt worldPoint(const QJsonObject& frame, double u, double v) {
    const auto origin=frame["origin"].toArray(),uAxis=frame["uAxis"].toArray(),vAxis=frame["vAxis"].toArray();
    return {origin[0].toDouble()+u*uAxis[0].toDouble()+v*vAxis[0].toDouble(),
        origin[1].toDouble()+u*uAxis[1].toDouble()+v*vAxis[1].toDouble(),
        origin[2].toDouble()+u*uAxis[2].toDouble()+v*vAxis[2].toDouble()};
}
void checkPointIds(const QJsonObject& arguments) {
    for(const auto& key : {"centerPointId","startPointId","endPointId"})
        if(arguments.contains(key) && (!arguments[key].isString() || !validId(arguments[key].toString())))
            reject("Expected a stable sketch point UUID");
}
void checkFields(const QJsonObject& arguments, const QSet<QString>& allowed) {
    for (auto it = arguments.begin(); it != arguments.end(); ++it)
        if (!allowed.contains(it.key())) reject("Unknown sketch argument: " + it.key());
}
void retainSupportPoint(QJsonObject& parameters,const QJsonObject& context,const QString& id) {
    for(const auto& value:parameters.value("pointProjections").toArray())
        if(value.toObject()["id"]==id)return; // Already retained by Project, with its own edge provenance.
    for(const auto& value:context["points"].toArray()) {
        const auto available=value.toObject();if(available["id"]!=id)continue;
        auto references=parameters["pointReferences"].toArray();
        for(const auto& reference:references)if(reference.toObject()["id"]==id){if(reference.toObject()["source"]!=available["source"])reject("Support point provenance changed");return;}
        references.append(QJsonObject{{"id",id},{"source",available["source"]}});parameters["pointReferences"]=references;return;
    }
}
SketchGeometry::SolveResult invalidDraft(const QJsonObject& draft, const QString& error) {
    return {false, draft, {{"solver", "PlaneGCS"}, {"status", "invalid"}}, error};
}
}

namespace {
QJsonObject centerlinePair(const QJsonObject& parameters,const QJsonArray& ids) {
    if(ids.size()!=2 || ids[0]==ids[1])reject("Diameter requires a centerline and a distinct parallel profile line");
    QJsonObject first,second;
    for(const auto& value:parameters["entities"].toArray()){const auto line=value.toObject();if(line["id"]==ids[0])first=line;if(line["id"]==ids[1])second=line;}
    if(first["type"]!="line" || second["type"]!="line")reject("Centerline diameter supports straight lines only");
    if(first["centerline"].toBool()==second["centerline"].toBool())reject("Diameter requires exactly one Centerline");
    if(second["centerline"].toBool())std::swap(first,second);
    if(first["construction"].toBool() || second["construction"].toBool())reject("Centerline diameter requires boundary-capable lines");
    QMap<QString,QPointF> points;for(const auto& value:parameters["points"].toArray()){const auto point=value.toObject();points[point["id"].toString()]={point["uMm"].toDouble(),point["vMm"].toDouble()};}
    const auto a=points[first["startPointId"].toString()],b=points[first["endPointId"].toString()],c=points[second["startPointId"].toString()],d=points[second["endPointId"].toString()];
    const auto axis=b-a,edge=d-c;const auto length=std::hypot(axis.x(),axis.y()),edgeLength=std::hypot(edge.x(),edge.y());
    if(length<minimumLength || edgeLength<minimumLength)reject("Centerline diameter line is degenerate");
    if(std::abs(axis.x()*edge.y()-axis.y()*edge.x())/(length*edgeLength)>1e-9)reject("Centerline diameter requires parallel lines; retain their orientation with sketch constraints");
    const auto offset=(axis.x()*(c.y()-a.y())-axis.y()*(c.x()-a.x()))/length;
    if(std::abs(offset)<1e-9)reject("Centerline diameter requires a nonzero offset; use Coincident for zero");
    return {{"entityIds",QJsonArray{first["id"],second["id"]}},{"signedOffsetMm",offset}};
}
QJsonObject parallelLinePair(const QJsonObject& parameters,const QJsonArray& ids) {
    if(ids.size()!=2 || ids[0]==ids[1])reject("Distance requires two distinct straight lines");
    QJsonObject first,second;
    for(const auto& value:parameters["entities"].toArray()){const auto line=value.toObject();if(line["id"]==ids[0])first=line;if(line["id"]==ids[1])second=line;}
    if(first["type"]!="line" || second["type"]!="line")reject("Distance requires two straight lines");
    QMap<QString,QPointF> points;for(const auto& value:parameters["points"].toArray()){const auto point=value.toObject();points[point["id"].toString()]={point["uMm"].toDouble(),point["vMm"].toDouble()};}
    const auto a=points[first["startPointId"].toString()],b=points[first["endPointId"].toString()],c=points[second["startPointId"].toString()],d=points[second["endPointId"].toString()];
    const auto axis=b-a,edge=d-c;const auto length=std::hypot(axis.x(),axis.y()),edgeLength=std::hypot(edge.x(),edge.y());
    if(length<minimumLength || edgeLength<minimumLength)reject("Distance line is degenerate");
    if(std::abs(axis.x()*edge.y()-axis.y()*edge.x())/(length*edgeLength)>1e-9)reject("Distance requires parallel lines; retain their orientation with sketch constraints");
    const auto offset=(axis.x()*(c.y()-a.y())-axis.y()*(c.x()-a.x()))/length;
    if(std::abs(offset)<minimumLength)reject("Use Coincident for overlapping lines");
    return {{"entityIds",ids},{"signedOffsetMm",offset}};
}
}
namespace {
bool localLengthDimension(const QJsonObject& dimension) {
    const auto kind=dimension.value("kind").toString();
    return dimension.value("driving").toBool() && kind!="angle" && kind!="arcSweep" && kind!="overallSlotAngle";
}
double dimensionUnitScale(const QString& units) {
    if(units=="mm")return 1;
    if(units=="in")return 25.4;
    reject("Sketch expression entry units must be mm or in");
}
QJsonObject allocateDimensionNames(QJsonObject parameters){
    if(!parameters.contains("nextDimensionNameIndex"))return parameters;
    auto dimensions=parameters.value("dimensions").toArray();
    int next=parameters.value("nextDimensionNameIndex").toInt();
    if(next<1 || next>1000000)reject("Sketch dimension name counter is invalid");
    for(int i=0;i<dimensions.size();++i){
        auto row=dimensions[i].toObject();
        if(!localLengthDimension(row) || row.contains("name"))continue;
        row["name"]="d"+QString::number(next++);dimensions[i]=row;
    }
    parameters["nextDimensionNameIndex"]=next;parameters["dimensions"]=dimensions;
    return parameters;
}
QJsonObject bindLocalExpression(QJsonObject dimension,const QJsonArray& dimensions,
    const QString& expression,const QString& units,const QSet<QString>& userNames){
    const double scale=dimensionUnitScale(units);
    if(!localLengthDimension(dimension) || dimension.contains("parameterId"))
        reject("Only an unbound driving length accepts a local expression");
    QJsonArray refs;
    LengthExpression parser(expression,[&](const QString& name,int)->LengthQuantity{
        QString match;
        for(const auto& value:dimensions){const auto row=value.toObject();
            if(row.value("name")!=name || !localLengthDimension(row))continue;
            if(!match.isEmpty())reject("Duplicate local dimension name");
            match=row.value("id").toString();
        }
        if(userNames.contains(name) && !match.isEmpty())reject("Ambiguous local and user length name: "+name);
        if(match.isEmpty())reject("Local driving dimension name is unavailable: "+name);
        refs.append(match);return {1,true};
    },scale);
    parser.parse();
    dimension["expression"]=expression;dimension["expressionUnits"]=units;
    dimension["referenceIds"]=refs;dimension["valueMm"]=1;
    return dimension;
}
}
QJsonObject SketchGeometry::resolvedDimensionFeature(const QJsonObject& feature) {
    auto result=feature,parameters=allocateDimensionNames(feature.value("parameters").toObject());
    auto dimensions=parameters.value("dimensions").toArray();
    QMap<QString,QJsonObject> byId;QSet<QString> names;
    for(const auto& value:dimensions){
        const auto row=value.toObject();
        const auto id=row.value("id").toString(),name=row.value("name").toString();
        if(byId.contains(id))reject("Duplicate sketch dimension ID");
        byId[id]=row;
        if(name.isEmpty())continue;
        if(!localLengthDimension(row) || names.contains(name))reject("Duplicate or invalid local dimension name");
        names.insert(name);
    }
    QMap<QString,double> resolved;QSet<QString> visiting;
    std::function<double(const QString&)> evaluate=[&](const QString& id)->double {
        if(resolved.contains(id))return resolved[id];
        if(!byId.contains(id))reject("Sketch expression references a missing dimension ID");
        if(visiting.contains(id))reject("Cyclic sketch dimension expression");
        if(visiting.size()>=64)reject("Sketch dimension dependency chain is too deep");
        visiting.insert(id);const auto row=byId[id];double amount=row.value("valueMm").toDouble();
        if(row.contains("expression")) {
            if(!localLengthDimension(row) || row.contains("parameterId"))
                reject("Only an unbound driving length can use a sketch expression");
            const auto units=row.value("expressionUnits").toString();
            const double scale=dimensionUnitScale(units);
            const auto refs=row.value("referenceIds").toArray();
            LengthExpression parser(row.value("expression").toString(),[&](const QString& name,int index)->LengthQuantity {
                if(index>=refs.size() || !refs[index].isString())reject("Sketch expression reference count differs from tokens");
                const auto ref=refs[index].toString();
                if(!byId.contains(ref) || !localLengthDimension(byId[ref]) || byId[ref].value("name")!=name)
                    reject("Sketch expression reference name or ID is unavailable");
                return {evaluate(ref),true};
            },scale);
            const auto quantity=parser.parse();
            if(static_cast<int>(parser.identifiers().size())!=refs.size())
                reject("Sketch expression reference count differs from tokens");
            amount=quantity.length ? quantity.value : quantity.value*scale;
        }
        if(row.contains("expression") && (!std::isfinite(amount) || amount<minimumLength || amount>maximumLength))
            reject("Sketch length result must be 0.001..100000 mm");
        visiting.remove(id);resolved[id]=amount;return amount;
    };
    for(int i=0;i<dimensions.size();++i){
        auto row=dimensions[i].toObject();
        if(row.contains("expression")){row["valueMm"]=evaluate(row.value("id").toString());dimensions[i]=row;}
    }
    parameters["dimensions"]=dimensions;result["parameters"]=parameters;
    return result;
}
double SketchGeometry::centerlineDiameterValue(const QJsonObject& draft,const QJsonArray& ids,const QJsonObject& context) {
    return 2*std::abs(centerlinePair(expandedProjectionFeature(draft,context)["parameters"].toObject(),ids)["signedOffsetMm"].toDouble());
}
QJsonObject SketchGeometry::newFeature(const QString& ownerId, const QString& plane, const QString& name) {
    frame(plane);
    QJsonObject feature{{"id", newId()}, {"ownerId", ownerId}, {"type", "sketch"},
        {"name", name}, {"visible", true},
        {"parameters", QJsonObject{{"plane", plane}, {"points", QJsonArray{}}, {"entities", QJsonArray{}},
            {"nextDimensionNameIndex",1}}}};
    validateDraft(feature);
    return feature;
}

QJsonObject SketchGeometry::newSupportedFeature(const QString& ownerId, const QJsonObject& support, const QString& name, const QJsonArray& supportEdges) {
    auto feature=newFeature(ownerId,"XY",name);auto parameters=feature["parameters"].toObject();
    parameters["plane"]="face";parameters["support"]=support;
    QJsonArray projections;
    for(const auto& value:supportEdges){const auto edge=value.toObject();if(!edge["supported"].toBool())continue;
        auto source=support;source["edgeId"]=edge["edgeId"];
        const auto id=QUuid::createUuidV5(QUuid(feature["id"].toString()),QByteArray("alloy-projection/1\n")+edge["edgeId"].toString().toUtf8()).toString(QUuid::WithoutBraces);
        projections.append(QJsonObject{{"id",id},{"source",source}});
    }
    if(!projections.isEmpty())parameters["projections"]=projections;
    feature["parameters"]=parameters;validateDraft(feature);return feature;
}

QJsonObject SketchGeometry::newConstructionPlaneFeature(const QString& ownerId,const QString& planeId,const QString& name) {
    auto feature=newFeature(ownerId,"XY",name);auto parameters=feature["parameters"].toObject();
    parameters["plane"]="construction";parameters["planeId"]=planeId;feature["parameters"]=parameters;
    validateDraft(feature);return feature;
}

QString SketchGeometry::curveCenterPointId(const QString& sketchId,const QJsonObject& source) {
    QStringList signature;for(const auto& key:{"producerFeatureId","bodyId","faceId","sourceSketchId","sourceEntityId","role"})signature.append(source[key].toString());
    // Keep legacy center UUIDs unchanged when no feature occurrence exists.
    if(source.contains("occurrenceId"))signature.append(source["occurrenceId"].toString());
    return QUuid::createUuidV5(QUuid(sketchId),QByteArray("alloy-support-point/1\n")+signature.join('\n').toUtf8()).toString(QUuid::WithoutBraces);
}
QString SketchGeometry::projectionPointId(const QString& id,bool end) {
    return QUuid::createUuidV5(QUuid(id),end ? QByteArray("end") : QByteArray("start")).toString(QUuid::WithoutBraces);
}
QJsonObject SketchGeometry::linkedProjectionCandidate(const EvaluatedDocument& evaluation,
    const QJsonObject& sketch,const QJsonObject& source) {
    if((sketch["parameters"].toObject()["plane"]!="face" && sketch["parameters"].toObject()["plane"]!="construction") || source.size()!=2
        || !validId(source["bodyId"].toString()) || !validId(source["edgeId"].toString()))
        reject("Linked Project requires a supported sketch and stable body/edge IDs");
    auto candidate=sketch,parameters=candidate["parameters"].toObject();
    QJsonObject retainedSource=source;
    for(const auto& body:evaluation.bodies)if(body.bodyId==source["bodyId"].toString() && !body.occurrenceId.isEmpty())
        retainedSource["occurrenceId"]=body.occurrenceId;
    for(const auto& key:{"projections","pointProjections","circleProjections"}) {
        auto entries=parameters.value(key).toArray();
        for(int i=0;i<entries.size();++i) {
            auto entry=entries[i].toObject(),previous=entry["source"].toObject();
            if(previous["bodyId"]!=source["bodyId"] || previous["edgeId"]!=source["edgeId"])continue;
            if(QString(key)=="projections" && !entry["profile"].toBool()) {
                entry["profile"]=true;entries[i]=entry;parameters[key]=entries;candidate["parameters"]=parameters;
            }
            return candidate;
        }
    }
    const auto id=QUuid::createUuidV5(QUuid(sketch["id"].toString()),
        QByteArray("alloy-linked-projection/1\n")+source["bodyId"].toString().toUtf8()+'\n'+source["edgeId"].toString().toUtf8())
        .toString(QUuid::WithoutBraces);
    auto circles=parameters["circleProjections"].toArray();circles.append(QJsonObject{{"id",id},{"source",retainedSource},{"profile",true}});
    parameters["circleProjections"]=circles;candidate["parameters"]=parameters;
    try {GeometryKernel::projectionContext(evaluation,candidate);return candidate;}
    catch(const GeometryKernel::EvaluationError& error) {
        if(error.diagnostics()["code"]!="unsupported_projection")throw;
    }
    circles.removeLast();if(circles.isEmpty())parameters.remove("circleProjections");else parameters["circleProjections"]=circles;
    auto lines=parameters["projections"].toArray();lines.append(QJsonObject{{"id",id},{"source",retainedSource},{"profile",true}});
    parameters["projections"]=lines;candidate["parameters"]=parameters;
    try {GeometryKernel::projectionContext(evaluation,candidate);return candidate;}
    catch(const GeometryKernel::EvaluationError& error) {
        if(error.diagnostics()["code"]!="projection_dimension_changed")throw;
    }
    lines.removeLast();parameters["projections"]=lines;
    auto pointProjections=parameters["pointProjections"].toArray();
    pointProjections.append(QJsonObject{{"id",id},{"source",retainedSource}});
    parameters["pointProjections"]=pointProjections;candidate["parameters"]=parameters;
    GeometryKernel::projectionContext(evaluation,candidate);
    return candidate;
}
SketchGeometry::SolveResult SketchGeometry::addClosedRectOffset(const QJsonObject& draft,const QJsonArray& sourceEntityIds,
    double signedDistanceMm,const QJsonObject& projectionContext) {
    try {
        validateDraft(draft);
        if(!std::isfinite(signedDistanceMm) || std::abs(signedDistanceMm)<minimumLength)
            reject("Closed Offset requires a nonzero finite distance");
        auto p=draft["parameters"].toObject();auto projections=p["projections"].toArray();
        if(sourceEntityIds.size()!=4 || (p["plane"]!="face" && p["plane"]!="construction"))
            reject("Select four linked projected rectangle edges");
        QSet<QString> requested,already;
        for(const auto& value:p["closedRectOffsets"].toArray())for(const auto& id:value.toObject()["sourceEntityIds"].toArray())already.insert(id.toString());
        for(const auto& value:sourceEntityIds) {
            const auto id=value.toString();bool found=false;
            for(int i=0;i<projections.size();++i)if(projections[i].toObject()["id"]==id) {
                found=true;auto projection=projections[i].toObject();projection["profile"]=false;projections[i]=projection;break;
            }
            if(!found || requested.contains(id) || already.contains(id))reject("Closed Offset needs four unused linked projected edges");
            requested.insert(id);
        }
        p["projections"]=projections;
        auto expanded=expandedProjectionFeature(QJsonObject{{"parameters",p}},projectionContext)["parameters"].toObject();
        const auto bounds=linkedRectangleBounds(expanded,sourceEntityIds);
        const std::array<QPointF,4> corners{QPointF{bounds[0]-signedDistanceMm,bounds[2]-signedDistanceMm},
            QPointF{bounds[1]+signedDistanceMm,bounds[2]-signedDistanceMm},
            QPointF{bounds[1]+signedDistanceMm,bounds[3]+signedDistanceMm},
            QPointF{bounds[0]-signedDistanceMm,bounds[3]+signedDistanceMm}};
        if(corners[1].x()-corners[0].x()<minimumLength || corners[2].y()-corners[1].y()<minimumLength)
            reject("Closed Offset collapses the rectangle");
        auto points=p["points"].toArray(),entities=p["entities"].toArray(),groups=p["closedRectOffsets"].toArray();
        QJsonArray pointIds,entityIds;
        for(int i=0;i<4;++i) {
            const auto id=newId();pointIds.append(id);points.append(QJsonObject{{"id",id},{"uMm",corners[i].x()},{"vMm",corners[i].y()}});
        }
        for(int i=0;i<4;++i) {
            const auto id=newId();entityIds.append(id);
            entities.append(QJsonObject{{"id",id},{"type","line"},{"startPointId",pointIds[i]},
                {"endPointId",pointIds[(i+1)%4]}});
        }
        groups.append(QJsonObject{{"id",newId()},{"sourceEntityIds",sourceEntityIds},
            {"distanceMm",signedDistanceMm},{"pointIds",pointIds},{"entityIds",entityIds}});
        p["points"]=points;p["entities"]=entities;p["closedRectOffsets"]=groups;
        auto candidate=draft;candidate["parameters"]=p;
        return solveDraft(candidate,projectionContext);
    } catch(const std::exception& error) {return {false,draft,{},QString::fromUtf8(error.what())};}
}
QJsonObject SketchGeometry::expandedProjectionFeature(const QJsonObject& feature,const QJsonObject& context) {
    auto expanded=feature;auto parameters=feature["parameters"].toObject();
    const auto projections=parameters["projections"].toArray();
    auto points=parameters["points"].toArray(),entities=parameters["entities"].toArray(),dimensions=parameters["dimensions"].toArray();
    for(const auto& value:projections) {
        const auto projection=value.toObject();QJsonObject line;
        for(const auto& resolved:context["lines"].toArray())if(resolved.toObject()["id"]==projection["id"]){if(!line.isEmpty())reject("Ambiguous resolved projection");line=resolved.toObject();}
        if(line.isEmpty() || line["source"]!=projection["source"])reject("Retained projection requires current resolved support geometry");
        const auto id=projection["id"].toString();const auto start=projectionPointId(id,false),end=projectionPointId(id,true);
        points.append(QJsonObject{{"id",start},{"uMm",coordinate(line,"u1Mm")},{"vMm",coordinate(line,"v1Mm")}});
        points.append(QJsonObject{{"id",end},{"uMm",coordinate(line,"u2Mm")},{"vMm",coordinate(line,"v2Mm")}});
        entities.append(QJsonObject{{"id",id},{"type","line"},{"startPointId",start},{"endPointId",end},
            {"construction",!projection["profile"].toBool()}});
    }
    for(const auto& value:parameters["circleProjections"].toArray()) {
        const auto projection=value.toObject();QJsonObject circle;
        for(const auto& resolved:context["circles"].toArray())if(resolved.toObject()["id"]==projection["id"]){if(!circle.isEmpty())reject("Ambiguous resolved circle projection");circle=resolved.toObject();}
        if(circle.isEmpty() || circle["source"]!=projection["source"])reject("Retained circle requires current resolved support geometry");
        const auto id=projection["id"].toString(),center=projectionPointId(id,false);const double radius=coordinate(circle,"radiusMm");
        points.append(QJsonObject{{"id",center},{"uMm",coordinate(circle,"uMm")},{"vMm",coordinate(circle,"vMm")}});
        entities.append(QJsonObject{{"id",id},{"type","circle"},{"centerPointId",center},{"radiusMm",radius}});
        const auto diameterId=QUuid::createUuidV5(QUuid(id),QByteArray("alloy-projected-circle-diameter/1")).toString(QUuid::WithoutBraces);
        dimensions.append(QJsonObject{{"id",diameterId},{"kind","diameter"},{"entityId",id},{"driving",true},{"valueMm",2*radius}});
    }
    for(const auto& value:parameters["pointReferences"].toArray()) {
        const auto reference=value.toObject();QJsonObject point;
        for(const auto& available:context["points"].toArray())if(available.toObject()["id"]==reference["id"] && available.toObject()["source"]==reference["source"]){if(!point.isEmpty())reject("Ambiguous resolved support point");point=available.toObject();}
        if(point.isEmpty())reject("Retained support point requires current resolved source geometry");
        points.append(QJsonObject{{"id",reference["id"]},{"uMm",coordinate(point,"uMm")},{"vMm",coordinate(point,"vMm")}});
    }
    for(const auto& value:parameters["pointProjections"].toArray()) {
        const auto projection=value.toObject();QJsonObject point;
        for(const auto& available:context["points"].toArray())if(available.toObject()["id"]==projection["id"] && available.toObject()["source"]==projection["source"]){if(!point.isEmpty())reject("Ambiguous resolved projected point");point=available.toObject();}
        if(point.isEmpty())reject("Retained point projection requires current resolved source edge");
        points.append(QJsonObject{{"id",projection["id"]},{"uMm",coordinate(point,"uMm")},{"vMm",coordinate(point,"vMm")}});
    }
    parameters.remove("pointReferences");parameters.remove("pointProjections");parameters.remove("projections");parameters.remove("circleProjections");parameters["points"]=points;parameters["entities"]=entities;parameters["dimensions"]=dimensions;expanded["parameters"]=parameters;return expanded;
}

QJsonObject SketchGeometry::findFeature(const QJsonObject& authored, const QString& sketchId) {
    for (const auto& entry : authored["features"].toArray()) {
        const auto feature = entry.toObject();
        if (feature["id"].toString() != sketchId) continue;
        if (feature["type"].toString() != "sketch") reject("Feature is not a sketch");
        validateDraft(feature);
        return feature;
    }
    reject("Sketch does not exist");
}

QJsonObject SketchGeometry::replaceFeature(const QJsonObject& authored, const QJsonObject& draft) {
    validateDraft(draft);
    bool ownerExists=draft["ownerId"]==authored["documentId"];
    for(const auto& value:authored["components"].toArray())ownerExists=ownerExists || value.toObject()["id"]==draft["ownerId"];
    if (!ownerExists) reject("Sketch component does not exist");
    auto candidate = authored;
    auto features = candidate["features"].toArray();
    bool found = false;
    for (int i = 0; i < features.size(); ++i) {
        const auto feature = features[i].toObject();
        if (feature["id"] != draft["id"]) continue;
        if (feature["ownerId"] != draft["ownerId"]) reject("Sketch edits cannot change owner");
        if (found || feature["type"].toString() != "sketch") reject("Invalid sketch feature identity");
        features[i] = draft;
        found = true;
    }
    if (!found) features.append(draft);
    candidate["features"] = features;
    return candidate;
}

QJsonObject SketchGeometry::addLine(const QJsonObject& draft, const QJsonObject& arguments) {
    validateDraft(draft);
    validateArguments(arguments, true);
    const double u1 = coordinate(arguments, "u1Mm"), v1 = coordinate(arguments, "v1Mm");
    const double u2 = coordinate(arguments, "u2Mm"), v2 = coordinate(arguments, "v2Mm");
    const auto length = std::hypot(u2 - u1, v2 - v1);
    if (length < minimumLength || length > maximumLength) reject("Sketch line length out of supported range (0.001..100000 mm)");
    auto changed = draft;
    auto parameters = changed["parameters"].toObject();
    auto points = parameters["points"].toArray();
    auto entities = parameters["entities"].toArray();
    const auto start = addPoint(points, u1, v1, arguments["startPointId"].toString());
    const auto end = addPoint(points, u2, v2, arguments["endPointId"].toString());
    entities.append(QJsonObject{{"id", newId()}, {"type", "line"}, {"startPointId", start}, {"endPointId", end}});
    parameters["points"] = points;
    parameters["entities"] = entities;
    changed["parameters"] = parameters;
    return changed;
}

SketchGeometry::SolveResult SketchGeometry::addStandalonePoint(const QJsonObject& draft,
    const QJsonObject& arguments,const QJsonObject& projectionContext) {
    try {
        validateDraft(draft);checkFields(arguments,{"uMm","vMm","snapTarget"});
        double u=coordinate(arguments,"uMm"),v=coordinate(arguments,"vMm");
        auto parameters=draft["parameters"].toObject();auto points=parameters["points"].toArray();
        auto entities=parameters["entities"].toArray(),constraints=parameters["constraints"].toArray();
        QString target;
        if(arguments.contains("snapTarget")) {
            if(!arguments["snapTarget"].isObject())reject("Point snap target must be an object");
            const auto snap=arguments["snapTarget"].toObject();
            if(snap["kind"]=="origin" && snap.size()==1) {
                target=parameters["originPointId"].toString();u=0;v=0;
                if(target.isEmpty()) {
                    target=newId();parameters["originPointId"]=target;
                    points.append(QJsonObject{{"id",target},{"uMm",0},{"vMm",0}});
                    constraints.append(QJsonObject{{"id",newId()},{"type","fixed"},{"pointIds",QJsonArray{target}},{"uMm",0},{"vMm",0}});
                }
            } else if(snap["kind"]=="point" && snap.size()==2 && validId(snap["pointId"].toString())) {
                target=snap["pointId"].toString();retainSupportPoint(parameters,projectionContext,target);
                bool found=false;auto lookup=draft;lookup["parameters"]=parameters;
                for(const auto& value:expandedProjectionFeature(lookup,projectionContext)["parameters"].toObject()["points"].toArray()) {
                    const auto point=value.toObject();if(point["id"]!=target)continue;
                    u=point["uMm"].toDouble();v=point["vMm"].toDouble();found=true;break;
                }
                if(!found)reject("Point snap target is unavailable");
            } else reject("Point accepts an origin or retained point snap");
        }
        const auto id=newId();points.append(QJsonObject{{"id",id},{"uMm",u},{"vMm",v}});
        entities.append(QJsonObject{{"id",newId()},{"type","point"},{"pointId",id}});
        if(!target.isEmpty())constraints.append(QJsonObject{{"id",newId()},{"type","coincident"},{"pointIds",QJsonArray{id,target}}});
        parameters["points"]=points;parameters["entities"]=entities;parameters["constraints"]=constraints;
        auto changed=draft;changed["parameters"]=parameters;return solveDraft(changed,projectionContext);
    } catch(const std::exception& e){return {false,draft,{},QString::fromUtf8(e.what())};}
}

QJsonObject SketchGeometry::addCircle(const QJsonObject& draft, const QJsonObject& arguments) {
    validateDraft(draft);
    checkFields(arguments, {"centerUMm", "centerVMm", "diameterMm", "centerPointId"});
    checkPointIds(arguments);
    const double u = coordinate(arguments, "centerUMm"), v = coordinate(arguments, "centerVMm");
    const double radius = coordinate(arguments, "diameterMm") / 2;
    if (radius < minimumLength || radius > maximumLength) reject("Circle radius out of supported range (0.001..100000 mm)");
    auto changed = draft; auto parameters = changed["parameters"].toObject();
    auto points = parameters["points"].toArray(), entities = parameters["entities"].toArray();
    const auto center = addPoint(points, u, v, arguments["centerPointId"].toString());
    entities.append(QJsonObject{{"id",newId()},{"type","circle"},{"centerPointId",center},{"radiusMm",radius}});
    parameters["points"] = points; parameters["entities"] = entities; changed["parameters"] = parameters;
    validateDraft(changed); return changed;
}

QJsonObject SketchGeometry::addArc(const QJsonObject& draft, const QJsonObject& arguments) {
    validateDraft(draft);
    checkPointIds(arguments);
    const bool threePoint = arguments.contains("throughUMm") || arguments.contains("throughVMm");
    double u, v, radius, start, sweep;
    if (threePoint) {
        checkFields(arguments, {"startUMm","startVMm","endUMm","endVMm","throughUMm","throughVMm","startPointId","endPointId","centerPointId"});
        const double ax = coordinate(arguments,"startUMm"), ay = coordinate(arguments,"startVMm");
        const double bx = coordinate(arguments,"endUMm"), by = coordinate(arguments,"endVMm");
        const double cx = coordinate(arguments,"throughUMm"), cy = coordinate(arguments,"throughVMm");
        const double x = bx-ax, y = by-ay, q = cx-ax, t = cy-ay;
        const double determinant = 2*(x*t-y*q);
        const double scale = std::max({std::hypot(x,y),std::hypot(q,t),std::hypot(cx-bx,cy-by)});
        if (scale < minimumLength || std::abs(determinant) <= 1e-9*scale*scale) reject("Three-point arc requires distinct noncollinear points");
        u = ax + ((x*x+y*y)*t-(q*q+t*t)*y)/determinant;
        v = ay + (x*(q*q+t*t)-q*(x*x+y*y))/determinant;
        radius = std::hypot(ax-u,ay-v); start = std::atan2(ay-v,ax-u)/degrees;
        auto positive = [](double angle) { angle = std::fmod(angle,360.0); return angle < 0 ? angle+360.0 : angle; };
        const double end = positive(std::atan2(by-v,bx-u)/degrees-start);
        const double through = positive(std::atan2(cy-v,cx-u)/degrees-start);
        sweep = through < end ? end : end-360;
    } else {
        checkFields(arguments, {"centerUMm","centerVMm","radiusMm","startAngleDeg","sweepAngleDeg","centerPointId","startPointId","endPointId"});
        u = coordinate(arguments,"centerUMm"); v = coordinate(arguments,"centerVMm");
        radius = coordinate(arguments,"radiusMm"); start = coordinate(arguments,"startAngleDeg"); sweep = coordinate(arguments,"sweepAngleDeg");
    }
    if (!std::isfinite(radius) || radius < minimumLength || radius > maximumLength || !std::isfinite(sweep)
        || std::abs(sweep) < 1e-6 || std::abs(sweep) >= 360 || radius*std::abs(sweep)*degrees < minimumLength)
        reject("Arc requires a usable radius and nonzero sweep smaller than 360 degrees");
    start = std::remainder(start,360.0);
    auto changed = draft; auto parameters = changed["parameters"].toObject();
    auto points = parameters["points"].toArray(), entities = parameters["entities"].toArray();
    const auto center = addPoint(points,u,v,arguments["centerPointId"].toString());
    const auto first = addPoint(points,u+radius*std::cos(start*degrees),v+radius*std::sin(start*degrees),arguments["startPointId"].toString());
    const auto last = addPoint(points,u+radius*std::cos((start+sweep)*degrees),v+radius*std::sin((start+sweep)*degrees),arguments["endPointId"].toString());
    entities.append(QJsonObject{{"id",newId()},{"type","arc"},{"centerPointId",center},{"startPointId",first},{"endPointId",last},
        {"radiusMm",radius},{"startAngleDeg",start},{"sweepAngleDeg",sweep}});
    parameters["points"] = points; parameters["entities"] = entities; changed["parameters"] = parameters;
    validateDraft(changed); return changed;
}

TopoDS_Edge SketchGeometry::analyticEdge(const QJsonObject& parameters, const QJsonObject& entity, const QJsonObject& resolvedFrame) {
    QMap<QString,gp_Pnt> points;
    const auto sketchFrame=evaluationFrame(parameters,resolvedFrame);
    for (const auto& value : parameters["points"].toArray()) { const auto p=value.toObject();
        points[p["id"].toString()] = worldPoint(sketchFrame,p["uMm"].toDouble(),p["vMm"].toDouble()); }
    if(entity["type"]=="fitSpline"||entity["type"]=="controlSpline")return BRepBuilderAPI_MakeEdge(SketchSpline::evaluate(parameters,entity,evaluationFrame(parameters,resolvedFrame)).curve).Edge();
    if(entity["type"]=="fitSplineOffset")return BRepBuilderAPI_MakeEdge(SketchSplineOffset::evaluate(parameters,entity,evaluationFrame(parameters,resolvedFrame)).curve).Edge();
    if(entity["type"]=="conic")return BRepBuilderAPI_MakeEdge(SketchConic::evaluate(parameters,entity,evaluationFrame(parameters,resolvedFrame))).Edge();
    if (entity["type"] == "line") return BRepBuilderAPI_MakeEdge(points[entity["startPointId"].toString()],points[entity["endPointId"].toString()]).Edge();
    if(entity["type"]=="ellipse"){
        const auto resolved=SketchEllipse::axes(parameters,entity);
        const auto normal=sketchFrame["normal"].toArray(),uAxis=sketchFrame["uAxis"].toArray(),vAxis=sketchFrame["vAxis"].toArray();
        const bool firstMajor=resolved.firstRadius>=resolved.secondRadius;
        const double directionU=firstMajor ? resolved.firstU : resolved.secondU;
        const double directionV=firstMajor ? resolved.firstV : resolved.secondV;
        const gp_Dir majorDirection(uAxis[0].toDouble()*directionU+vAxis[0].toDouble()*directionV,
            uAxis[1].toDouble()*directionU+vAxis[1].toDouble()*directionV,
            uAxis[2].toDouble()*directionU+vAxis[2].toDouble()*directionV);
        const gp_Ax2 ellipseAxes(points[entity["centerPointId"].toString()],
            gp_Dir(normal[0].toDouble(),normal[1].toDouble(),normal[2].toDouble()),majorDirection);
        const double major=std::max(resolved.firstRadius,resolved.secondRadius),minor=std::min(resolved.firstRadius,resolved.secondRadius);
        if(std::abs(major-minor)<=1e-9*major)return BRepBuilderAPI_MakeEdge(gp_Circ(ellipseAxes,major)).Edge();
        return BRepBuilderAPI_MakeEdge(gp_Elips(ellipseAxes,major,minor)).Edge();
    }
    const auto f=sketchFrame; const auto n=f["normal"].toArray(), u=f["uAxis"].toArray();
    const gp_Ax2 axes(points[entity["centerPointId"].toString()],gp_Dir(n[0].toDouble(),n[1].toDouble(),n[2].toDouble()),gp_Dir(u[0].toDouble(),u[1].toDouble(),u[2].toDouble()));
    const gp_Circ circle(axes,entity["radiusMm"].toDouble());
    if (entity["type"] == "circle") return BRepBuilderAPI_MakeEdge(circle).Edge();
    const double start=entity["startAngleDeg"].toDouble()*degrees, sweep=entity["sweepAngleDeg"].toDouble()*degrees;
    auto edge=BRepBuilderAPI_MakeEdge(circle,std::min(start,start+sweep),std::max(start,start+sweep)).Edge();
    if (sweep < 0) edge.Reverse(); return edge;
}

QJsonObject SketchGeometry::addRectangle(const QJsonObject& draft, const QJsonObject& arguments) {
    validateArguments(arguments, false);
    const double u1 = coordinate(arguments, "u1Mm"), v1 = coordinate(arguments, "v1Mm");
    const double u2 = coordinate(arguments, "u2Mm"), v2 = coordinate(arguments, "v2Mm");
    if (std::abs(u2 - u1) < minimumLength || std::abs(v2 - v1) < minimumLength)
        reject("Sketch rectangle sides must be at least 0.001 mm");
    auto changed = addLine(draft, {{"u1Mm", u1}, {"v1Mm", v1}, {"u2Mm", u2}, {"v2Mm", v1}});
    changed = addLine(changed, {{"u1Mm", u2}, {"v1Mm", v1}, {"u2Mm", u2}, {"v2Mm", v2}});
    changed = addLine(changed, {{"u1Mm", u2}, {"v1Mm", v2}, {"u2Mm", u1}, {"v2Mm", v2}});
    changed = addLine(changed, {{"u1Mm", u1}, {"v1Mm", v2}, {"u2Mm", u1}, {"v2Mm", v1}});
    auto parameters = changed["parameters"].toObject();
    const auto entities = parameters["entities"].toArray();
    auto constraints = parameters["constraints"].toArray();
    for (int i = 0; i < 4; ++i) {
        const auto edge=entities[entities.size()-4+i].toObject();const auto type=i%2==0 ? QString("horizontal") : QString("vertical");
        // Adjacent rectangles reuse exact point IDs. Their shared side already
        // has the same inferred relation; adding it twice makes PlaneGCS reject
        // an otherwise useful overlapping-profile sketch as redundant.
        bool inferred=false;
        for(const auto& relation:constraints)if(relation.toObject()["type"]==type) {
            const auto existingId=relation.toObject()["entityIds"].toArray()[0].toString();
            for(const auto& value:entities)if(value.toObject()["id"]==existingId) {
                const auto other=value.toObject();
                inferred|=(other["startPointId"]==edge["startPointId"] && other["endPointId"]==edge["endPointId"])
                    || (other["startPointId"]==edge["endPointId"] && other["endPointId"]==edge["startPointId"]);
            }
        }
        if(!inferred)constraints.append(QJsonObject{{"id",newId()},{"type",type},{"entityIds",QJsonArray{edge["id"]}}});
    }
    parameters["constraints"] = constraints;
    changed["parameters"] = parameters;
    return changed;
}

SketchGeometry::SolveResult SketchGeometry::addCenterRectangle(const QJsonObject& draft, const QJsonObject& arguments,
    const QJsonObject& projectionContext) {
    try {
        validateDraft(draft);
        checkFields(arguments,{"centerUMm","centerVMm","widthMm","heightMm","centerSnapTarget","drivingSize"});
        if(arguments.contains("drivingSize") && !arguments["drivingSize"].isBool())reject("Center rectangle drivingSize must be boolean");
        const bool drivingSize=arguments.value("drivingSize").toBool(true);
        double u=coordinate(arguments,"centerUMm"),v=coordinate(arguments,"centerVMm");
        const double width=coordinate(arguments,"widthMm"),height=coordinate(arguments,"heightMm");
        if(width<minimumLength || height<minimumLength || width>maximumLength || height>maximumLength)
            reject("Center rectangle width and height must be within 0.001..100000 mm");
        auto parameters=draft["parameters"].toObject();auto points=parameters["points"].toArray();
        auto entities=parameters["entities"].toArray(),constraints=parameters["constraints"].toArray();
        auto dimensions=parameters["dimensions"].toArray();
        QString coincidentPoint,midpointLine;
        if(arguments.contains("centerSnapTarget")) {
            if(!arguments["centerSnapTarget"].isObject())reject("Center snap target must be an object");
            const auto target=arguments["centerSnapTarget"].toObject();
            if(target["kind"]=="origin") {
                if(target.size()!=1)reject("Origin snap target only accepts kind");
                coincidentPoint=parameters["originPointId"].toString();
                if(coincidentPoint.isEmpty()) {
                    coincidentPoint=newId();parameters["originPointId"]=coincidentPoint;
                    points.append(QJsonObject{{"id",coincidentPoint},{"uMm",0},{"vMm",0}});
                    constraints.append(QJsonObject{{"id",newId()},{"type","fixed"},{"pointIds",QJsonArray{coincidentPoint}},{"uMm",0},{"vMm",0}});
                }
                u=0;v=0;
            } else if(target["kind"]=="point") {
                if(target.size()!=2 || !target["pointId"].isString() || !validId(target["pointId"].toString()))reject("Center snap requires a stable pointId");
                retainSupportPoint(parameters,projectionContext,target["pointId"].toString());
                for(const auto& value:expandedProjectionFeature(QJsonObject{{"parameters",parameters}},projectionContext)["parameters"].toObject()["points"].toArray()) {
                    const auto point=value.toObject();if(point["id"]!=target["pointId"])continue;
                    coincidentPoint=point["id"].toString();u=point["uMm"].toDouble();v=point["vMm"].toDouble();break;
                }
                if(coincidentPoint.isEmpty())reject("Center snap point no longer exists");
            } else if(target["kind"]=="midpoint") {
                if(target.size()!=2 || !target["entityId"].isString() || !validId(target["entityId"].toString()))reject("Midpoint snap requires a line UUID");
                for(const auto& entry:entities) {
                    const auto line=entry.toObject();if(line["id"]!=target["entityId"])continue;
                    if(line["type"]!="line")reject("Midpoint target must be a line");
                    QJsonObject first,last;
                    for(const auto& value:points) {const auto point=value.toObject();
                        if(point["id"]==line["startPointId"])first=point;
                        if(point["id"]==line["endPointId"])last=point;}
                    midpointLine=line["id"].toString();u=(first["uMm"].toDouble()+last["uMm"].toDouble())/2;
                    v=(first["vMm"].toDouble()+last["vMm"].toDouble())/2;break;
                }
                if(midpointLine.isEmpty())reject("Center midpoint target no longer exists");
            } else reject("Unsupported center snap target");
        }
        auto point=[&](double x,double y){const auto id=newId();points.append(QJsonObject{{"id",id},{"uMm",x},{"vMm",y}});return id;};
        const auto a=point(u-width/2,v-height/2),b=point(u+width/2,v-height/2);
        const auto c=point(u+width/2,v+height/2),d=point(u-width/2,v+height/2),center=point(u,v);
        auto line=[&](const QString& first,const QString& last,bool construction=false){const auto id=newId();
            QJsonObject entity{{"id",id},{"type","line"},{"startPointId",first},{"endPointId",last}};
            if(construction)entity["construction"]=true;entities.append(entity);return id;};
        const auto bottom=line(a,b),right=line(b,c);line(c,d);line(d,a);
        const auto diagonalAC=line(a,c,true),diagonalBD=line(b,d,true);
        constraints.append(QJsonObject{{"id",newId()},{"type","horizontal"},{"entityIds",QJsonArray{bottom}}});
        constraints.append(QJsonObject{{"id",newId()},{"type","vertical"},{"entityIds",QJsonArray{right}}});
        for(const auto& diagonal:{diagonalAC,diagonalBD})
            constraints.append(QJsonObject{{"id",newId()},{"type","midpoint"},{"pointIds",QJsonArray{center}},{"entityIds",QJsonArray{diagonal}}});
        if(!coincidentPoint.isEmpty())constraints.append(QJsonObject{{"id",newId()},{"type","coincident"},{"pointIds",QJsonArray{center,coincidentPoint}}});
        else if(!midpointLine.isEmpty())constraints.append(QJsonObject{{"id",newId()},{"type","midpoint"},{"pointIds",QJsonArray{center}},{"entityIds",QJsonArray{midpointLine}}});
        else constraints.append(QJsonObject{{"id",newId()},{"type","fixed"},{"pointIds",QJsonArray{center}},{"uMm",u},{"vMm",v}});
        if(drivingSize) {
            dimensions.append(QJsonObject{{"id",newId()},{"kind","horizontal"},{"pointIds",QJsonArray{a,b}},{"driving",true},{"valueMm",width}});
            dimensions.append(QJsonObject{{"id",newId()},{"kind","vertical"},{"pointIds",QJsonArray{b,c}},{"driving",true},{"valueMm",height}});
        }
        parameters["points"]=points;parameters["entities"]=entities;parameters["constraints"]=constraints;parameters["dimensions"]=dimensions;
        auto candidate=draft;candidate["parameters"]=parameters;
        const auto result=solveDraft(candidate,projectionContext);
        return result.ok ? result : SolveResult{false,draft,result.diagnostics,result.error};
    } catch(const std::exception& error) {return invalidDraft(draft,QString::fromUtf8(error.what()));}
}

SketchGeometry::SolveResult SketchGeometry::addCenterPointSlot(const QJsonObject& draft,const QJsonObject& arguments,
    const QJsonObject& projectionContext) {
    try {
        validateDraft(draft);
        checkFields(arguments,{"centerUMm","centerVMm","endUMm","endVMm","widthMm","centerSnapTarget"});
        double u=coordinate(arguments,"centerUMm"),v=coordinate(arguments,"centerVMm");
        const double endU=coordinate(arguments,"endUMm"),endV=coordinate(arguments,"endVMm");
        const double width=coordinate(arguments,"widthMm");
        const double radius=width/2;
        auto parameters=draft["parameters"].toObject();auto points=parameters["points"].toArray();
        auto entities=parameters["entities"].toArray(),constraints=parameters["constraints"].toArray();
        QString targetId;
        if(arguments.contains("centerSnapTarget")) {
            if(!arguments["centerSnapTarget"].isObject())reject("Slot center snap target must be an object");
            const auto target=arguments["centerSnapTarget"].toObject();
            if(target["kind"]=="origin" && target.size()==1) {
                targetId=parameters["originPointId"].toString();
                if(targetId.isEmpty()) {
                    targetId=newId();parameters["originPointId"]=targetId;
                    points.append(QJsonObject{{"id",targetId},{"uMm",0},{"vMm",0}});
                    constraints.append(QJsonObject{{"id",newId()},{"type","fixed"},{"pointIds",QJsonArray{targetId}},{"uMm",0},{"vMm",0}});
                }
                u=0;v=0;
            } else if(target["kind"]=="point" && target.size()==2 && validId(target["pointId"].toString())) {
                targetId=target["pointId"].toString();retainSupportPoint(parameters,projectionContext,targetId);
                bool found=false;
                for(const auto& value:expandedProjectionFeature(QJsonObject{{"parameters",parameters}},projectionContext)["parameters"].toObject()["points"].toArray()) {
                    const auto p=value.toObject();if(p["id"]!=targetId)continue;u=p["uMm"].toDouble();v=p["vMm"].toDouble();found=true;break;
                }
                if(!found)reject("Slot center snap point is unavailable");
            } else reject("Slot center accepts an origin or retained point snap");
        }
        const double half=std::hypot(endU-u,endV-v);
        if(half<minimumLength || radius<minimumLength || half*2+width>maximumLength)
            reject("Center Point Slot needs a nonzero half-length and width within the supported range");
        const double dx=(endU-u)/half,dy=(endV-v)/half;
        const double nx=-dy,ny=dx, angle=std::atan2(ny,nx)/degrees;
        auto point=[&](double x,double y){const auto id=newId();points.append(QJsonObject{{"id",id},{"uMm",x},{"vMm",y}});return id;};
        const auto center=point(u,v),leftCenter=point(u-half*dx,v-half*dy),rightCenter=point(u+half*dx,v+half*dy);
        const auto leftTop=point(u-half*dx+radius*nx,v-half*dy+radius*ny);
        const auto rightTop=point(u+half*dx+radius*nx,v+half*dy+radius*ny);
        const auto rightBottom=point(u+half*dx-radius*nx,v+half*dy-radius*ny);
        const auto leftBottom=point(u-half*dx-radius*nx,v-half*dy-radius*ny);
        auto line=[&](const QString& first,const QString& last,bool construction){const auto id=newId();
            QJsonObject e{{"id",id},{"type","line"},{"startPointId",first},{"endPointId",last}};
            if(construction)e["construction"]=true;entities.append(e);return id;};
        const auto top=line(leftTop,rightTop,false),bottom=line(rightBottom,leftBottom,false);
        const auto guide=line(leftCenter,rightCenter,true);
        auto arc=[&](const QString& arcCenter,const QString& first,const QString& last,double start){const auto id=newId();
            entities.append(QJsonObject{{"id",id},{"type","arc"},{"centerPointId",arcCenter},{"startPointId",first},
                {"endPointId",last},{"radiusMm",radius},{"startAngleDeg",std::remainder(start,360.0)},{"sweepAngleDeg",-180.0}});return id;};
        const auto rightArc=arc(rightCenter,rightTop,rightBottom,angle);
        const auto leftArc=arc(leftCenter,leftBottom,leftTop,angle-180);
        constraints.append(QJsonObject{{"id",newId()},{"type","midpoint"},{"pointIds",QJsonArray{center}},{"entityIds",QJsonArray{guide}}});
        if(!targetId.isEmpty())constraints.append(QJsonObject{{"id",newId()},{"type","coincident"},{"pointIds",QJsonArray{center,targetId}}});
        constraints.append(QJsonObject{{"id",newId()},{"type","equal"},{"entityIds",QJsonArray{rightArc,leftArc}}});
        for(const auto& contact:std::array<std::tuple<QString,QString,QString>,4>{{
                {rightArc,top,rightTop},{rightArc,bottom,rightBottom},{leftArc,bottom,leftBottom},{leftArc,top,leftTop}}})
            constraints.append(QJsonObject{{"id",newId()},{"type","arcTangent"},
                {"entityIds",QJsonArray{std::get<0>(contact),std::get<1>(contact)}},
                {"pointIds",QJsonArray{std::get<2>(contact)}},{"side",-1}});
        auto groups=parameters["slotGroups"].toArray();
        groups.append(QJsonObject{{"id",newId()},{"centerPointId",center},{"centerlineId",guide},
            {"sideIds",QJsonArray{top,bottom}},{"arcIds",QJsonArray{rightArc,leftArc}}});
        parameters["slotGroups"]=groups;
        parameters["points"]=points;parameters["entities"]=entities;parameters["constraints"]=constraints;
        auto candidate=draft;candidate["parameters"]=parameters;
        auto solved=solveDraft(candidate,projectionContext);
        return solved.ok ? solved : SolveResult{false,draft,solved.diagnostics,solved.error};
    }catch(const std::exception& error){return invalidDraft(draft,QString::fromUtf8(error.what()));}
}

SketchGeometry::SolveResult SketchGeometry::addCenterToCenterSlot(const QJsonObject& draft,
    const QJsonObject& arguments,const QJsonObject& projectionContext) {
    try {
        validateDraft(draft);
        checkFields(arguments,{"firstUMm","firstVMm","secondUMm","secondVMm","widthMm",
            "firstSnapTarget","secondSnapTarget"});
        double firstU=coordinate(arguments,"firstUMm"),firstV=coordinate(arguments,"firstVMm");
        double secondU=coordinate(arguments,"secondUMm"),secondV=coordinate(arguments,"secondVMm");
        const double width=coordinate(arguments,"widthMm");
        auto prepared=draft,parameters=draft["parameters"].toObject();
        auto points=parameters["points"].toArray(),constraints=parameters["constraints"].toArray();
        auto resolveSnap=[&](const char* key,double& u,double& v)->QString {
            if(!arguments.contains(key))return {};
            if(!arguments[key].isObject())reject("Slot cap-center snap target must be an object");
            const auto target=arguments[key].toObject();
            QString id;
            if(target["kind"]=="origin" && target.size()==1) {
                id=parameters["originPointId"].toString();
                if(id.isEmpty()) {
                    id=newId();parameters["originPointId"]=id;
                    points.append(QJsonObject{{"id",id},{"uMm",0},{"vMm",0}});
                    constraints.append(QJsonObject{{"id",newId()},{"type","fixed"},
                        {"pointIds",QJsonArray{id}},{"uMm",0},{"vMm",0}});
                }
                u=0;v=0;
                return id;
            }
            if(target["kind"]!="point" || target.size()!=2 || !validId(target["pointId"].toString()))
                reject("Slot cap center accepts an origin or retained point snap");
            id=target["pointId"].toString();
            parameters["points"]=points;parameters["constraints"]=constraints;
            retainSupportPoint(parameters,projectionContext,id);
            bool found=false;
            for(const auto& value:expandedProjectionFeature(QJsonObject{{"parameters",parameters}},projectionContext)
                    ["parameters"].toObject()["points"].toArray()) {
                const auto point=value.toObject();if(point["id"]!=id)continue;
                u=coordinate(point,"uMm");v=coordinate(point,"vMm");found=true;break;
            }
            if(!found)reject("Slot cap-center snap point is unavailable");
            return id;
        };
        const auto firstId=resolveSnap("firstSnapTarget",firstU,firstV);
        const auto secondId=resolveSnap("secondSnapTarget",secondU,secondV);
        if(!firstId.isEmpty() && firstId==secondId)reject("Slot cap centers need distinct snap points");
        const double distance=std::hypot(secondU-firstU,secondV-firstV);
        if(distance<2*minimumLength || width<2*minimumLength || distance+width>maximumLength)
            reject("Center to Center Slot needs distinct cap centers and a positive width within range");
        parameters["points"]=points;parameters["constraints"]=constraints;prepared["parameters"]=parameters;
        const auto base=addCenterPointSlot(prepared,{{"centerUMm",(firstU+secondU)/2},
            {"centerVMm",(firstV+secondV)/2},{"endUMm",secondU},{"endVMm",secondV},
            {"widthMm",width}},projectionContext);
        if(!base.ok)return invalidDraft(draft,base.error);
        auto candidate=base.feature;parameters=candidate["parameters"].toObject();
        const auto group=parameters["slotGroups"].toArray().last().toObject();
        QJsonObject guide;
        for(const auto& value:parameters["entities"].toArray())
            if(value.toObject()["id"]==group["centerlineId"]){guide=value.toObject();break;}
        if(guide.isEmpty())reject("Slot guide is unavailable");
        constraints=parameters["constraints"].toArray();
        if(!firstId.isEmpty())constraints.append(QJsonObject{{"id",newId()},{"type","coincident"},
            {"pointIds",QJsonArray{guide["startPointId"],firstId}}});
        if(!secondId.isEmpty())constraints.append(QJsonObject{{"id",newId()},{"type","coincident"},
            {"pointIds",QJsonArray{guide["endPointId"],secondId}}});
        parameters["constraints"]=constraints;candidate["parameters"]=parameters;
        const auto solved=solveDraft(candidate,projectionContext);
        return solved.ok ? solved : SolveResult{false,draft,solved.diagnostics,solved.error};
    }catch(const std::exception& error){return invalidDraft(draft,QString::fromUtf8(error.what()));}
}

SketchGeometry::SolveResult SketchGeometry::addOverallSlot(const QJsonObject& draft,const QJsonObject& arguments,
    const QJsonObject& projectionContext) {
    try {
        validateDraft(draft);
        checkFields(arguments,{"firstUMm","firstVMm","lastUMm","lastVMm","widthMm"});
        const double x=coordinate(arguments,"firstUMm"),y=coordinate(arguments,"firstVMm");
        const double lastX=coordinate(arguments,"lastUMm"),lastY=coordinate(arguments,"lastVMm");
        const double width=coordinate(arguments,"widthMm"),length=std::hypot(lastX-x,lastY-y);
        if(width<minimumLength || length-width<minimumLength || length>maximumLength)
            reject("Overall Slot needs a positive width smaller than its total end-to-end length");
        const double dx=(lastX-x)/length,dy=(lastY-y)/length;
        const double middleX=(x+lastX)/2,middleY=(y+lastY)/2,half=(length-width)/2;
        const auto base=addCenterPointSlot(draft,{{"centerUMm",middleX},{"centerVMm",middleY},
            {"endUMm",middleX+half*dx},{"endVMm",middleY+half*dy},{"widthMm",width}},projectionContext);
        if(!base.ok)return base;
        auto feature=base.feature,parameters=feature["parameters"].toObject();
        auto groups=parameters["slotGroups"].toArray();auto group=groups.last().toObject();group["mode"]="overall";groups[groups.size()-1]=group;
        parameters["slotGroups"]=groups;
        QJsonObject guide;for(const auto& value:parameters["entities"].toArray())if(value.toObject()["id"]==group["centerlineId"]){guide=value.toObject();break;}
        const QJsonArray endpoints{guide["startPointId"],guide["endPointId"]};
        const auto arc=group["arcIds"].toArray().first();
        auto dimensions=parameters["dimensions"].toArray();
        dimensions.append(QJsonObject{{"id",newId()},{"kind","overallSlotLength"},{"pointIds",endpoints},
            {"entityId",arc},{"driving",true},{"valueMm",length}});
        dimensions.append(QJsonObject{{"id",newId()},{"kind","diameter"},{"entityId",arc},
            {"driving",true},{"valueMm",width}});
        dimensions.append(QJsonObject{{"id",newId()},{"kind","overallSlotAngle"},{"pointIds",endpoints},
            {"driving",true},{"valueDeg",std::atan2(dy,dx)/degrees},
            {"annotation",QJsonObject{{"uMm",x+width*dx/2+10*dy},{"vMm",y+width*dy/2-10*dx}}}});
        parameters["dimensions"]=dimensions;feature["parameters"]=parameters;
        const auto solved=solveDraft(feature,projectionContext);
        return solved.ok ? solved : SolveResult{false,draft,solved.diagnostics,solved.error};
    }catch(const std::exception& error){return invalidDraft(draft,QString::fromUtf8(error.what()));}
}

SketchGeometry::SolveResult SketchGeometry::addCenterPointArcSlot(const QJsonObject& draft,const QJsonObject& arguments,
    const QJsonObject& projectionContext) {
    try {
        validateDraft(draft);
        checkFields(arguments,{"centerUMm","centerVMm","firstUMm","firstVMm","lastUMm","lastVMm","widthMm","centerSnapTarget"});
        double u=coordinate(arguments,"centerUMm"),v=coordinate(arguments,"centerVMm");
        const double ax=coordinate(arguments,"firstUMm"),ay=coordinate(arguments,"firstVMm");
        const double bx=coordinate(arguments,"lastUMm"),by=coordinate(arguments,"lastVMm");
        const double endRadius=coordinate(arguments,"widthMm")/2;
        auto parameters=draft["parameters"].toObject();auto points=parameters["points"].toArray();
        auto entities=parameters["entities"].toArray(),constraints=parameters["constraints"].toArray();
        QString targetId;
        if(arguments.contains("centerSnapTarget")) {
            if(!arguments["centerSnapTarget"].isObject())reject("Arc slot center snap target must be an object");
            const auto target=arguments["centerSnapTarget"].toObject();
            if(target["kind"]=="origin" && target.size()==1) {
                targetId=parameters["originPointId"].toString();
                if(targetId.isEmpty()){
                    targetId=newId();parameters["originPointId"]=targetId;
                    points.append(QJsonObject{{"id",targetId},{"uMm",0},{"vMm",0}});
                    constraints.append(QJsonObject{{"id",newId()},{"type","fixed"},{"pointIds",QJsonArray{targetId}},{"uMm",0},{"vMm",0}});
                }
                u=0;v=0;
            } else if(target["kind"]=="point" && target.size()==2 && validId(target["pointId"].toString())) {
                targetId=target["pointId"].toString();retainSupportPoint(parameters,projectionContext,targetId);
                bool found=false;
                for(const auto& value:expandedProjectionFeature(QJsonObject{{"parameters",parameters}},projectionContext)["parameters"].toObject()["points"].toArray()){
                    const auto p=value.toObject();if(p["id"]!=targetId)continue;u=p["uMm"].toDouble();v=p["vMm"].toDouble();found=true;break;
                }
                if(!found)reject("Arc slot center snap point is unavailable");
            } else reject("Arc slot center accepts an origin or retained point snap");
        }
        const double radius=std::hypot(ax-u,ay-v),lastDistance=std::hypot(bx-u,by-v);
        if(radius<minimumLength || lastDistance<minimumLength || endRadius<minimumLength ||
            radius<=endRadius+minimumLength || radius+endRadius>maximumLength)
            reject("Arc slot needs a nonzero path radius and a smaller positive end radius");
        const double firstAngle=std::atan2(ay-v,ax-u),lastAngle=std::atan2(by-v,bx-u);
        const double sweep=std::remainder(lastAngle-firstAngle,2*std::numbers::pi);
        if(std::abs(sweep)<1e-5 || std::abs(sweep)>=std::numbers::pi-1e-5)
            reject("Arc slot needs distinct path endpoints and an open sweep below 180 degrees");
        auto point=[&](double x,double y){const auto id=newId();points.append(QJsonObject{{"id",id},{"uMm",x},{"vMm",y}});return id;};
        const auto center=point(u,v),first=point(ax,ay),last=point(u+radius*std::cos(lastAngle),v+radius*std::sin(lastAngle));
        const auto outerFirst=point(0,0),outerLast=point(0,0),innerLast=point(0,0),innerFirst=point(0,0);
        auto arc=[&](const QString& arcCenter,const QString& start,const QString& end,double r,double startAngle,double arcSweep,bool construction){
            const auto id=newId();QJsonObject e{{"id",id},{"type","arc"},{"centerPointId",arcCenter},
                {"startPointId",start},{"endPointId",end},{"radiusMm",r},
                {"startAngleDeg",startAngle/degrees},{"sweepAngleDeg",arcSweep/degrees}};
            if(construction)e["construction"]=true;entities.append(e);return id;
        };
        const auto path=arc(center,first,last,radius,firstAngle,sweep,true);
        const auto outer=arc(center,outerFirst,outerLast,radius+endRadius,firstAngle,sweep,false);
        const auto capLast=arc(last,outerLast,innerLast,endRadius,lastAngle,(sweep<0 ? -1 : 1)*std::numbers::pi,false);
        const auto inner=arc(center,innerLast,innerFirst,radius-endRadius,lastAngle,-sweep,false);
        const auto capFirst=arc(first,innerFirst,outerFirst,endRadius,firstAngle+std::numbers::pi,(sweep<0 ? -1 : 1)*std::numbers::pi,false);
        if(!targetId.isEmpty())constraints.append(QJsonObject{{"id",newId()},{"type","coincident"},{"pointIds",QJsonArray{center,targetId}}});
        auto groups=parameters["arcSlotGroups"].toArray();groups.append(QJsonObject{{"id",newId()},
            {"pathArcId",path},{"outerArcId",outer},{"innerArcId",inner},
            {"capIds",QJsonArray{capLast,capFirst}},{"sweepSign",sweep<0 ? -1 : 1}});
        parameters["arcSlotGroups"]=groups;parameters["points"]=points;parameters["entities"]=entities;
        parameters["constraints"]=constraints;parameters=SketchArcSlot::derive(parameters);
        auto candidate=draft;candidate["parameters"]=parameters;
        const auto solved=solveDraft(candidate,projectionContext);
        return solved.ok ? solved : SolveResult{false,draft,solved.diagnostics,solved.error};
    }catch(const std::exception& error){return invalidDraft(draft,QString::fromUtf8(error.what()));}
}

SketchGeometry::SolveResult SketchGeometry::addCenterPointEllipse(const QJsonObject& draft,const QJsonObject& arguments,
    const QJsonObject& projectionContext) {
    try {
        validateDraft(draft);
        checkFields(arguments,{"centerUMm","centerVMm","firstUMm","firstVMm","contourUMm","contourVMm","secondFullMm","secondSide","centerSnapTarget"});
        double cu=coordinate(arguments,"centerUMm"),cv=coordinate(arguments,"centerVMm");
        const double au=coordinate(arguments,"firstUMm"),av=coordinate(arguments,"firstVMm");
        const bool contour=arguments.contains("contourUMm") || arguments.contains("contourVMm");
        const bool typed=arguments.contains("secondFullMm") || arguments.contains("secondSide");
        if(contour==typed || (contour && (!arguments.contains("contourUMm") || !arguments.contains("contourVMm"))) ||
            (typed && (!arguments.contains("secondFullMm") || !arguments.contains("secondSide"))))
            reject("Ellipse needs either one contour point or one full second-axis length and side");
        auto parameters=draft["parameters"].toObject();auto points=parameters["points"].toArray();
        auto entities=parameters["entities"].toArray(),constraints=parameters["constraints"].toArray();
        QString targetId,midpointLine;
        if(arguments.contains("centerSnapTarget")){
            if(!arguments["centerSnapTarget"].isObject())reject("Ellipse center snap must be an object");
            const auto target=arguments["centerSnapTarget"].toObject();
            if(target["kind"]=="origin" && target.size()==1){
                targetId=parameters["originPointId"].toString();
                if(targetId.isEmpty()){
                    targetId=newId();parameters["originPointId"]=targetId;
                    points.append(QJsonObject{{"id",targetId},{"uMm",0},{"vMm",0}});
                    constraints.append(QJsonObject{{"id",newId()},{"type","fixed"},{"pointIds",QJsonArray{targetId}},{"uMm",0},{"vMm",0}});
                }
                cu=0;cv=0;
            }else if(target["kind"]=="point" && target.size()==2 && validId(target["pointId"].toString())){
                targetId=target["pointId"].toString();retainSupportPoint(parameters,projectionContext,targetId);
                bool found=false;
                for(const auto& value:expandedProjectionFeature(QJsonObject{{"parameters",parameters}},projectionContext)["parameters"].toObject()["points"].toArray()){
                    const auto p=value.toObject();if(p["id"]!=targetId)continue;cu=p["uMm"].toDouble();cv=p["vMm"].toDouble();found=true;break;
                }
                if(!found)reject("Ellipse center snap point is unavailable");
            }else if(target["kind"]=="midpoint" && target.size()==2 && validId(target["entityId"].toString())) {
                const auto expanded=expandedProjectionFeature(QJsonObject{{"parameters",parameters}},projectionContext)["parameters"].toObject();
                for(const auto& value:expanded["entities"].toArray()) {
                    const auto line=value.toObject();if(line["id"]!=target["entityId"])continue;
                    if(line["type"]!="line")reject("Ellipse midpoint target must be a line");
                    QJsonObject first,last;
                    for(const auto& item:expanded["points"].toArray()) {
                        const auto p=item.toObject();if(p["id"]==line["startPointId"])first=p;
                        if(p["id"]==line["endPointId"])last=p;
                    }
                    if(first.isEmpty() || last.isEmpty())reject("Ellipse midpoint endpoints are unavailable");
                    cu=(first["uMm"].toDouble()+last["uMm"].toDouble())/2;
                    cv=(first["vMm"].toDouble()+last["vMm"].toDouble())/2;midpointLine=line["id"].toString();break;
                }
                if(midpointLine.isEmpty())reject("Ellipse midpoint target is unavailable");
            }else reject("Ellipse center accepts an origin, retained point or line midpoint snap");
        }
        double signedB=0;
        if(contour) signedB=SketchEllipse::secondRadiusFromContour(cu,cv,au,av,
            coordinate(arguments,"contourUMm"),coordinate(arguments,"contourVMm"));
        else {
            const double full=coordinate(arguments,"secondFullMm"),side=coordinate(arguments,"secondSide");
            if(full<0.002 || full>200000 || (side!=1 && side!=-1))reject("Ellipse second full length and side are invalid");
            signedB=side*full/2;
        }
        const double dx=au-cu,dy=av-cv,a=std::hypot(dx,dy),nx=-dy/a,ny=dx/a;
        auto point=[&](double u,double v){const auto id=newId();points.append(QJsonObject{{"id",id},{"uMm",u},{"vMm",v}});return id;};
        const auto center=point(cu,cv),firstNegative=point(2*cu-au,2*cv-av),firstPositive=point(au,av);
        const auto secondNegative=point(cu-signedB*nx,cv-signedB*ny),secondPositive=point(cu+signedB*nx,cv+signedB*ny);
        const auto firstAxis=newId(),secondAxis=newId(),ellipseId=newId();
        entities.append(QJsonObject{{"id",firstAxis},{"type","line"},{"startPointId",firstNegative},{"endPointId",firstPositive},{"construction",true}});
        entities.append(QJsonObject{{"id",secondAxis},{"type","line"},{"startPointId",secondNegative},{"endPointId",secondPositive},{"construction",true}});
        entities.append(QJsonObject{{"id",ellipseId},{"type","ellipse"},{"centerPointId",center},{"firstAxisId",firstAxis},{"secondAxisId",secondAxis}});
        constraints.append(QJsonObject{{"id",newId()},{"type","midpoint"},{"pointIds",QJsonArray{center}},{"entityIds",QJsonArray{firstAxis}}});
        constraints.append(QJsonObject{{"id",newId()},{"type","midpoint"},{"pointIds",QJsonArray{center}},{"entityIds",QJsonArray{secondAxis}}});
        constraints.append(QJsonObject{{"id",newId()},{"type","perpendicular"},{"entityIds",QJsonArray{firstAxis,secondAxis}}});
        if(!targetId.isEmpty())constraints.append(QJsonObject{{"id",newId()},{"type","coincident"},{"pointIds",QJsonArray{center,targetId}}});
        if(!midpointLine.isEmpty())constraints.append(QJsonObject{{"id",newId()},{"type","midpoint"},{"pointIds",QJsonArray{center}},{"entityIds",QJsonArray{midpointLine}}});
        parameters["points"]=points;parameters["entities"]=entities;parameters["constraints"]=constraints;
        auto candidate=draft;candidate["parameters"]=parameters;
        const auto result=solveDraft(candidate,projectionContext);
        return result.ok ? result : SolveResult{false,draft,result.diagnostics,result.error};
    }catch(const std::exception& error){return invalidDraft(draft,QString::fromUtf8(error.what()));}
}

QJsonObject SketchGeometry::pointerLineAxis(double u1,double v1,double u2,double v2,bool endAcquired) {
    const double dx=u2-u1,dy=v2-v1,length=std::hypot(dx,dy);
    // Alloy's bounded pointer policy: within one degree and one millimeter of
    // an axis, for a line long enough that pointer pixels have useful direction.
    if(!std::isfinite(length) || length<2.0)return {};
    const double tolerance=std::min(1.0,length*std::tan(std::numbers::pi/180.0));
    const bool horizontal=std::abs(dx)>std::abs(dy);
    const double deviation=horizontal ? std::abs(dy) : std::abs(dx);
    // The first click is never moved. A snapped final target also cannot be
    // moved to an inferred axis; admit that case only when already aligned.
    if(deviation>tolerance || (endAcquired && deviation>1e-6))return {};
    if(horizontal) {
        if(!endAcquired)v2=v1;
    } else {
        if(!endAcquired)u2=u1;
    }
    return {{"type",horizontal ? "horizontal" : "vertical"},
        {"u1Mm",u1},{"v1Mm",v1},{"u2Mm",u2},{"v2Mm",v2}};
}

namespace {
QJsonObject pointerCircle(const QJsonObject& parameters,const QString& circleId) {
    QJsonObject circle,center;
    for(const auto& value:parameters.value("entities").toArray()) {
        const auto row=value.toObject();if(row.value("id")==circleId && row.value("type")=="circle")circle=row;
    }
    if(circle.isEmpty())return {};
    for(const auto& value:parameters.value("points").toArray()) {
        const auto row=value.toObject();if(row.value("id")==circle.value("centerPointId"))center=row;
    }
    if(center.isEmpty() || circle.value("radiusMm").toDouble()<0.001)return {};
    return {{"uMm",center.value("uMm")},{"vMm",center.value("vMm")},{"radiusMm",circle.value("radiusMm")}};
}
}

QJsonObject SketchGeometry::pointerCircleContact(const QJsonObject& parameters,const QString& circleId,double u,double v) {
    const auto circle=pointerCircle(parameters,circleId);if(circle.isEmpty())return {};
    const double cx=circle.value("uMm").toDouble(),cy=circle.value("vMm").toDouble();
    const double dx=u-cx,dy=v-cy,distance=std::hypot(dx,dy);
    if(!std::isfinite(distance) || distance<1e-9)return {};
    const double radius=circle.value("radiusMm").toDouble();
    return {{"uMm",cx+radius*dx/distance},{"vMm",cy+radius*dy/distance}};
}

QJsonObject SketchGeometry::pointerCircleFrom(const QJsonObject& parameters,const QString& circleId,
    double contactU,double contactV,double cursorU,double cursorV) {
    const auto circle=pointerCircle(parameters,circleId);if(circle.isEmpty())return {};
    const double rx=contactU-circle.value("uMm").toDouble(),ry=contactV-circle.value("vMm").toDouble();
    const double radius=circle.value("radiusMm").toDouble();
    if(std::abs(std::hypot(rx,ry)-radius)>1e-5)return {};
    const double tx=-ry/radius,ty=rx/radius;
    const double travel=(cursorU-contactU)*tx+(cursorV-contactV)*ty;
    if(!std::isfinite(travel) || std::abs(travel)<0.001)return {{"status","ambiguous"}};
    return {{"uMm",contactU+travel*tx},{"vMm",contactV+travel*ty}};
}

QJsonArray SketchGeometry::pointerCircleTangents(const QJsonObject& parameters,const QString& circleId,double startU,double startV) {
    const auto circle=pointerCircle(parameters,circleId);if(circle.isEmpty())return {};
    const double cx=circle.value("uMm").toDouble(),cy=circle.value("vMm").toDouble();
    const double dx=startU-cx,dy=startV-cy,d2=dx*dx+dy*dy,r=circle.value("radiusMm").toDouble();
    if(!std::isfinite(d2) || d2<=r*r+1e-6)return {};
    const double along=r*r/d2,across=r*std::sqrt(d2-r*r)/d2;
    return {QJsonObject{{"branch",-1},{"uMm",cx+along*dx+across*dy},{"vMm",cy+along*dy-across*dx}},
        QJsonObject{{"branch",1},{"uMm",cx+along*dx-across*dy},{"vMm",cy+along*dy+across*dx}}};
}

QJsonObject SketchGeometry::pointerTangentArc(const QJsonObject& parameters,const QString& lineId,double endU,double endV) {
    if(!std::isfinite(endU) || !std::isfinite(endV) || std::abs(endU)>maximumLength || std::abs(endV)>maximumLength)return {};
    QJsonObject line,start,previous;
    for(const auto& value:parameters.value("entities").toArray()){
        const auto row=value.toObject();if(row.value("id")==lineId && row.value("type")=="line")line=row;
    }
    if(line.isEmpty())return {};
    for(const auto& value:parameters.value("points").toArray()){
        const auto row=value.toObject();if(row.value("id")==line.value("startPointId"))previous=row;
        if(row.value("id")==line.value("endPointId"))start=row;
    }
    if(start.isEmpty() || previous.isEmpty())return {};
    const double px=start.value("uMm").toDouble(),py=start.value("vMm").toDouble();
    const double dx=px-previous.value("uMm").toDouble(),dy=py-previous.value("vMm").toDouble();
    const double lineLength=std::hypot(dx,dy),ex=endU-px,ey=endV-py;
    if(lineLength<minimumLength || std::hypot(ex,ey)<minimumLength)return {};
    const double tx=dx/lineLength,ty=dy/lineLength,nx=-ty,ny=tx;
    const double normalTravel=nx*ex+ny*ey;
    if(std::abs(normalTravel)<1e-6*std::max(1.0,std::hypot(ex,ey)))return {};
    const double signedRadius=(ex*ex+ey*ey)/(2*normalTravel),radius=std::abs(signedRadius);
    if(!std::isfinite(radius) || radius<minimumLength || radius>maximumLength)return {};
    const double cx=px+signedRadius*nx,cy=py+signedRadius*ny;
    const double startAngle=std::atan2(py-cy,px-cx)/degrees;
    const double endAngle=std::atan2(endV-cy,endU-cx)/degrees;
    auto positive=[](double angle){angle=std::fmod(angle,360.0);return angle<0 ? angle+360.0 : angle;};
    const double sweep=signedRadius>0 ? positive(endAngle-startAngle) : -positive(startAngle-endAngle);
    if(std::abs(sweep)<1e-6 || std::abs(sweep)>=360 || radius*std::abs(sweep)*degrees<minimumLength)return {};
    return {{"centerUMm",cx},{"centerVMm",cy},{"radiusMm",radius},
        {"startAngleDeg",std::remainder(startAngle,360.0)},{"sweepAngleDeg",sweep},
        {"startPointId",line.value("endPointId")},{"endUMm",endU},{"endVMm",endV},
        {"side",signedRadius>0 ? 1 : -1}};
}

SketchGeometry::SolveResult SketchGeometry::addPointerTangentArc(const QJsonObject& draft,const QString& lineId,
    double endU,double endV,const QJsonObject& projectionContext) {
    try {
        validateDraft(draft);
        const auto parameters=draft.value("parameters").toObject();
        const auto entities=parameters.value("entities").toArray();
        if(entities.isEmpty() || entities.last().toObject().value("id")!=lineId || entities.last().toObject().value("type")!="line")
            reject("Tangent Arc drag needs the most recently authored Line");
        const auto geometry=pointerTangentArc(parameters,lineId,endU,endV);
        if(geometry.isEmpty())reject("Tangent Arc drag needs a nondegenerate finite circular sweep");
        auto candidate=addArc(draft,{{"centerUMm",geometry.value("centerUMm")},{"centerVMm",geometry.value("centerVMm")},
            {"radiusMm",geometry.value("radiusMm")},{"startAngleDeg",geometry.value("startAngleDeg")},
            {"sweepAngleDeg",geometry.value("sweepAngleDeg")},{"startPointId",geometry.value("startPointId")}});
        auto updated=candidate.value("parameters").toObject();auto relations=updated.value("constraints").toArray();
        const auto arc=updated.value("entities").toArray().last().toObject();
        relations.append(QJsonObject{{"id",newId()},{"type","arcTangent"},
            {"entityIds",QJsonArray{arc.value("id"),lineId}},
            {"pointIds",QJsonArray{geometry.value("startPointId")}},{"side",geometry.value("side")}});
        updated["constraints"]=relations;candidate["parameters"]=updated;
        const auto solved=solveDraft(candidate,projectionContext);
        if(!solved.ok)return {false,draft,solved.diagnostics,solved.error};
        const auto after=solved.feature.value("parameters").toObject();
        const auto beforePoints=parameters.value("points").toArray(),afterPoints=after.value("points").toArray();
        QMap<QString,QJsonObject> resolved;for(const auto& value:afterPoints){const auto point=value.toObject();resolved[point.value("id").toString()]=point;}
        for(const auto& value:beforePoints){const auto point=value.toObject(),newPoint=resolved.value(point.value("id").toString());
            if(newPoint.isEmpty() || std::hypot(newPoint.value("uMm").toDouble()-point.value("uMm").toDouble(),
                newPoint.value("vMm").toDouble()-point.value("vMm").toDouble())>1e-6)
                reject("Tangent Arc would move existing sketch geometry");}
        return solved;
    }catch(const std::exception& error){return invalidDraft(draft,QString::fromUtf8(error.what()));}
}

SketchGeometry::SolveResult SketchGeometry::addPointerEntity(const QJsonObject& draft, const QString& tool, const QJsonObject& arguments, const QJsonObject& projectionContext) {
    try {
        validateDraft(draft);
        if (tool!="line" && tool!="rectangle") reject("Pointer authoring supports Line and Rectangle");
        checkFields(arguments,{"u1Mm","v1Mm","u2Mm","v2Mm","startSnapTarget","endSnapTarget","inferAxis","inferCircleTangent"});
        if(arguments.contains("inferAxis") && (tool!="line" || !arguments["inferAxis"].isBool()))
            reject("Only pointer-created Line may request axis inference");
        if(arguments.contains("inferCircleTangent") && (tool!="line" || !arguments["inferCircleTangent"].isBool()))
            reject("Only pointer-created Line may request circle tangency");
        double u1=coordinate(arguments,"u1Mm"),v1=coordinate(arguments,"v1Mm"),u2=coordinate(arguments,"u2Mm"),v2=coordinate(arguments,"v2Mm");
        auto parameters=draft["parameters"].toObject();auto points=parameters["points"].toArray();
        auto entities=parameters["entities"].toArray();auto constraints=parameters["constraints"].toArray();
        auto freshPoint=[&](double u,double v) {const auto id=newId();points.append(QJsonObject{{"id",id},{"uMm",u},{"vMm",v}});return id;};
        QString startMidpointLine,endMidpointLine,startCircle,endCircle;
        int endCircleBranch=0;
        auto target=[&](const QString& key,double& u,double& v,QString& midpointLine) -> QString {
            if (!arguments.contains(key)) return {};
            if (!arguments[key].isObject()) reject("Snap target must be an object");
            const auto selected=arguments[key].toObject();const auto kind=selected["kind"].toString();QString id;
            if (kind=="origin") {
                if (selected.size()!=1) reject("Origin snap target only accepts kind");
                id=parameters["originPointId"].toString();
                if (id.isEmpty()) {
                    id=freshPoint(0,0);parameters["originPointId"]=id;
                    constraints.append(QJsonObject{{"id",newId()},{"type","fixed"},{"pointIds",QJsonArray{id}},{"uMm",0},{"vMm",0}});
                }
            } else if (kind=="point") {
                if (selected.size()!=2 || !selected["pointId"].isString() || !validId(selected["pointId"].toString())) reject("Endpoint snap requires a stable pointId");
                id=selected["pointId"].toString();retainSupportPoint(parameters,projectionContext,id);
            } else if (kind=="midpoint" && tool=="line") {
                if(selected.size()!=2 || !selected["entityId"].isString() || !validId(selected["entityId"].toString()))reject("Midpoint snap requires a stable line ID");
                for(const auto& value:entities){const auto line=value.toObject();if(line["id"]!=selected["entityId"] || line["type"]!="line")continue;
                    QJsonObject first,last;for(const auto& value:points){const auto point=value.toObject();if(point["id"]==line["startPointId"])first=point;if(point["id"]==line["endPointId"])last=point;}
                    if(first.isEmpty() || last.isEmpty())break;
                    u=(first["uMm"].toDouble()+last["uMm"].toDouble())/2;v=(first["vMm"].toDouble()+last["vMm"].toDouble())/2;
                    midpointLine=line["id"].toString();return {};
                }
                reject("Midpoint snap line no longer exists");
            } else if(kind=="circle" && tool=="line") {
                if(!selected["entityId"].isString() || !validId(selected["entityId"].toString()))reject("Circle contact needs a retained circle ID");
                const auto circleId=selected["entityId"].toString();
                if(key=="startSnapTarget") {
                    if(selected.size()!=2)reject("Starting circle contact only accepts its circle ID");
                    const auto contact=pointerCircleContact(parameters,circleId,u,v);
                    if(contact.isEmpty())reject("Starting circle contact is unavailable");
                    u=contact["uMm"].toDouble();v=contact["vMm"].toDouble();startCircle=circleId;
                } else {
                    if(selected.size()!=3 || !selected["branch"].isDouble() ||
                        (selected["branch"].toInt()!=-1 && selected["branch"].toInt()!=1))
                        reject("Ending circle contact needs an explicit tangent branch");
                    endCircle=circleId;endCircleBranch=selected["branch"].toInt();
                }
                return {};
            } else reject("Unsupported snap target kind");
            for (const auto& entry:points) {const auto p=entry.toObject();if(p["id"]==id){u=p["uMm"].toDouble();v=p["vMm"].toDouble();return id;}}
            for(const auto& entry:expandedProjectionFeature(QJsonObject{{"parameters",parameters}},projectionContext)["parameters"].toObject()["points"].toArray()) {
                const auto p=entry.toObject();if(p["id"]==id){u=p["uMm"].toDouble();v=p["vMm"].toDouble();return id;}
            }
            reject("Snap target point does not exist");
        };
        const auto startTarget=target("startSnapTarget",u1,v1,startMidpointLine),endTarget=target("endSnapTarget",u2,v2,endMidpointLine);
        if(!startCircle.isEmpty() && !endCircle.isEmpty())reject("One pointer Line cannot infer contacts on two circles");
        if(arguments.value("inferCircleTangent").toBool()) {
            if(startCircle.isEmpty() || !endCircle.isEmpty() || !endTarget.isEmpty() || !endMidpointLine.isEmpty())
                reject("Shift tangency requires a starting circle and free endpoint");
            const auto projected=pointerCircleFrom(parameters,startCircle,u1,v1,u2,v2);
            if(projected.isEmpty() || projected.value("status")=="ambiguous")reject("Starting circle tangent direction is ambiguous");
            u2=projected["uMm"].toDouble();v2=projected["vMm"].toDouble();
        }
        if(!endCircle.isEmpty()) {
            const auto candidates=pointerCircleTangents(parameters,endCircle,u1,v1);
            if(candidates.size()!=2)reject("Ending circle has no tangent contact from this Line start");
            const auto contact=candidates[endCircleBranch==1 ? 1 : 0].toObject();
            u2=contact["uMm"].toDouble();v2=contact["vMm"].toDouble();
        }
        const auto inference=tool=="line" && startCircle.isEmpty() && endCircle.isEmpty() && arguments.value("inferAxis").toBool()
            ? pointerLineAxis(u1,v1,u2,v2,!endTarget.isEmpty() || !endMidpointLine.isEmpty()) : QJsonObject{};
        if(!inference.isEmpty()) {
            u1=inference["u1Mm"].toDouble();v1=inference["v1Mm"].toDouble();
            u2=inference["u2Mm"].toDouble();v2=inference["v2Mm"].toDouble();
        }
        if (tool=="line" && (std::hypot(u2-u1,v2-v1)<minimumLength || std::hypot(u2-u1,v2-v1)>maximumLength)) reject("Snapped line length out of supported range");
        if (tool=="rectangle" && (std::abs(u2-u1)<minimumLength || std::abs(v2-v1)<minimumLength)) reject("Snapped rectangle sides must be at least 0.001 mm");
        auto derivedOffsetPoints=SketchSplineOffset::derivedPointIds(parameters);
        derivedOffsetPoints.unite(SketchOpenChainOffset::derivedPointIds(parameters));
        const bool fromLatestArc=tool=="line" && !entities.isEmpty() && entities.last().toObject().value("type")=="arc"
            && !startTarget.isEmpty() && entities.last().toObject().value("endPointId")==startTarget;
        const auto first=tool=="line" && (derivedOffsetPoints.contains(startTarget) || fromLatestArc)
            ? startTarget : freshPoint(u1,v1);
        QString last;
        auto edge=[&](const QString& a,const QString& b) {const auto id=newId();entities.append(QJsonObject{{"id",id},{"type","line"},{"startPointId",a},{"endPointId",b}});return id;};
        QString lineId;
        if (tool=="line") {last=derivedOffsetPoints.contains(endTarget) ? endTarget : freshPoint(u2,v2);lineId=edge(first,last);}
        else {
            const auto second=freshPoint(u2,v1);last=freshPoint(u2,v2);const auto fourth=freshPoint(u1,v2);
            const QString corners[]{first,second,last,fourth};
            for (int i=0;i<4;++i) {
                const auto id=edge(corners[i],corners[(i+1)%4]);
                constraints.append(QJsonObject{{"id",newId()},{"type",i%2==0 ? "horizontal" : "vertical"},{"entityIds",QJsonArray{id}}});
            }
        }
        auto coincide=[&](const QString& endpoint,const QString& selected) {
            if (!selected.isEmpty())constraints.append(QJsonObject{{"id",newId()},{"type","coincident"},{"pointIds",QJsonArray{endpoint,selected}}});
        };
        if(first!=startTarget)coincide(first,startTarget);
        if(last!=endTarget)coincide(last,endTarget);
        if(!startMidpointLine.isEmpty())constraints.append(QJsonObject{{"id",newId()},{"type","midpoint"},{"pointIds",QJsonArray{first}},{"entityIds",QJsonArray{startMidpointLine}}});
        if(!endMidpointLine.isEmpty())constraints.append(QJsonObject{{"id",newId()},{"type","midpoint"},{"pointIds",QJsonArray{last}},{"entityIds",QJsonArray{endMidpointLine}}});
        auto circleRelation=[&](const QString& circleId,const QString& contactPoint,bool tangent) {
            if(circleId.isEmpty())return;
            constraints.append(QJsonObject{{"id",newId()},{"type","pointOnCurve"},
                {"pointIds",QJsonArray{contactPoint}},{"entityIds",QJsonArray{circleId}}});
            if(!tangent)return;
            const auto circle=pointerCircle(parameters,circleId);
            const double dx=u2-u1,dy=v2-v1,cross=dx*(circle["vMm"].toDouble()-v1)-dy*(circle["uMm"].toDouble()-u1);
            if(std::abs(cross)<1e-9)reject("Circle tangent side is ambiguous");
            constraints.append(QJsonObject{{"id",newId()},{"type","tangent"},
                {"entityIds",QJsonArray{circleId,lineId}},{"pointIds",QJsonArray{contactPoint}},
                {"side",cross>0 ? 1 : -1}});
        };
        circleRelation(startCircle,first,arguments.value("inferCircleTangent").toBool());
        circleRelation(endCircle,last,true);
        if(!inference.isEmpty())constraints.append(QJsonObject{{"id",newId()},{"type",inference["type"]},
            {"entityIds",QJsonArray{lineId}}});
        parameters["points"]=points;parameters["entities"]=entities;parameters["constraints"]=constraints;
        auto candidate=draft;candidate["parameters"]=parameters;
        const auto result=solveDraft(candidate,projectionContext);
        if(result.ok && !inference.isEmpty())for(const auto& oldValue:draft["parameters"].toObject()["points"].toArray()) {
            const auto old=oldValue.toObject();bool same=false;
            for(const auto& newValue:result.feature["parameters"].toObject()["points"].toArray()) {
                const auto now=newValue.toObject();if(now["id"]!=old["id"])continue;
                same=std::hypot(now["uMm"].toDouble()-old["uMm"].toDouble(),
                    now["vMm"].toDouble()-old["vMm"].toDouble())<1e-5;break;
            }
            if(!same)return {false,draft,result.diagnostics,"Inferred axis would move existing sketch geometry"};
        }
        return result.ok ? result : SolveResult{false,draft,result.diagnostics,result.error};
    } catch (const std::exception& error) {return invalidDraft(draft,QString::fromUtf8(error.what()));}
}

QString SketchGeometry::validateParameters(const QJsonObject& parameters, QSet<QString>& authoredIds, const QSet<QString>& derivedPointIds) {
    const auto plane = parameters["plane"].toString();
    if (plane != "XY" && plane != "XZ" && plane != "YZ" && plane != "face" && plane != "construction") return "Unsupported sketch plane";
    if(plane=="face") {
        if(!parameters["support"].isObject())return "Face sketch requires authored support references";
        const auto support=parameters["support"].toObject();
        if(support.size()!=3)return "Unexpected sketch support fields";
        for(const auto& key:{"producerFeatureId","bodyId","faceId"})if(!support[key].isString() || !validId(support[key].toString()))return "Sketch support requires stable producer, body and face UUIDs";
    } else if(parameters.contains("support"))return "Non-face sketch cannot carry face support";
    if(parameters.contains("originConvention") &&
        (plane!="face" || parameters["originConvention"]!="projectedOwnerV1"))
        return "Unsupported face-sketch origin convention";
    if(plane=="construction") {if(!validId(parameters["planeId"].toString()))return "Construction sketch requires a stable plane ID";}
    else if(parameters.contains("planeId"))return "Only construction sketches can reference a plane feature";
    if(parameters.contains("projections") || parameters.contains("pointReferences") || parameters.contains("pointProjections") || parameters.contains("circleProjections")) {
        if((plane!="face" && plane!="construction") || (plane!="face" && parameters.contains("pointReferences")) || (parameters.contains("projections") && !parameters["projections"].isArray()) || (parameters.contains("pointReferences") && !parameters["pointReferences"].isArray()) || (parameters.contains("pointProjections") && !parameters["pointProjections"].isArray()) || (parameters.contains("circleProjections") && !parameters["circleProjections"].isArray()))return "Projections require a retained face or construction plane and valid arrays";
        QJsonArray lines,circles,resolvedPoints;QSet<QString> sources,referenceIds=derivedPointIds;
        for(const auto& value:parameters["projections"].toArray()) {
            if(!value.isObject())return "Projection must be an object";const auto projection=value.toObject();
            if((projection.size()!=2 && !(projection.size()==3 && projection["profile"].isBool()))
                || !validId(projection["id"].toString()) || !projection["source"].isObject())return "Invalid retained projection";
            const auto source=projection["source"].toObject();if(source.size()!=4 && source.size()!=2 && source.size()!=3)return "Invalid projection source fields";
            for(const auto& key:{"bodyId","edgeId"})if(!source[key].isString() || !validId(source[key].toString()))return "Projection source requires stable UUIDs";
            if(source.size()==3 && (!source["occurrenceId"].isString() || !validId(source["occurrenceId"].toString())))return "Projection occurrence requires a stable UUID";
            if(source.size()==4) {
                if(plane!="face")return "Legacy support projection requires its face sketch";
                for(const auto& key:{"producerFeatureId","faceId"})if(!source[key].isString() || !validId(source[key].toString()))return "Projection source requires stable UUIDs";
                for(const auto& key:{"producerFeatureId","bodyId","faceId"})if(source[key]!=parameters["support"].toObject()[key])return "Projection source must belong to sketch support";
            }
            const auto key=source["bodyId"].toString()+":"+source["edgeId"].toString();
            if(sources.contains(key))return "Duplicate projected source edge";sources.insert(key);
            lines.append(QJsonObject{{"id",projection["id"]},{"source",source},{"u1Mm",0},{"v1Mm",0},{"u2Mm",1},{"v2Mm",0}});
        }
        for(const auto& value:parameters["pointProjections"].toArray()) {
            if(!value.isObject())return "Point projection must be an object";const auto projection=value.toObject(),source=projection["source"].toObject();
            if(projection.size()!=2 || !validId(projection["id"].toString()) || (source.size()!=2 && source.size()!=3)
                || !validId(source["bodyId"].toString()) || !validId(source["edgeId"].toString()))return "Invalid retained point projection";
            if(source.size()==3 && !validId(source["occurrenceId"].toString()))return "Invalid point projection occurrence";
            const auto key=source["bodyId"].toString()+":"+source["edgeId"].toString();
            if(sources.contains(key))return "Duplicate projected source edge";sources.insert(key);
            referenceIds.insert(projection["id"].toString());
            resolvedPoints.append(QJsonObject{{"id",projection["id"]},{"source",source},{"uMm",0},{"vMm",0}});
        }
        for(const auto& value:parameters["circleProjections"].toArray()) {
            if(!value.isObject())return "Circle projection must be an object";
            const auto projection=value.toObject(),source=projection["source"].toObject();
            if(projection.size()!=3 || !validId(projection["id"].toString()) || projection["profile"]!=true
                || source.size()!=3 || !validId(source["bodyId"].toString()) || !validId(source["edgeId"].toString())
                || !validId(source["occurrenceId"].toString()))return "Invalid retained circular projection";
            const auto key=source["bodyId"].toString()+":"+source["edgeId"].toString();
            if(sources.contains(key))return "Duplicate projected source edge";sources.insert(key);
            circles.append(QJsonObject{{"id",projection["id"]},{"source",source},{"uMm",0},{"vMm",0},{"radiusMm",1}});
            referenceIds.insert(projectionPointId(projection["id"].toString(),false));
        }
        for(const auto& value:parameters["pointReferences"].toArray()) {
            if(!value.isObject())return "Support point reference must be an object";const auto reference=value.toObject();
            if(reference.size()!=2 || !validId(reference["id"].toString()) || !reference["source"].isObject())return "Invalid support point reference";
            const auto source=reference["source"].toObject();if((source.size()!=6 && source.size()!=7) || source["role"]!="curveCenter"
                || (source.size()==7 && (!source["occurrenceId"].isString() || !validId(source["occurrenceId"].toString()))))return "Unsupported support point role";
            for(const auto& key:{"producerFeatureId","bodyId","faceId","sourceSketchId","sourceEntityId"})if(!source[key].isString() || !validId(source[key].toString()))return "Support point source requires stable UUIDs";
            for(const auto& key:{"producerFeatureId","bodyId","faceId"})if(source[key]!=parameters["support"].toObject()[key])return "Point reference must belong to sketch support";
            const auto centerKey=source["sourceSketchId"].toString()+":"+source["sourceEntityId"].toString()
                +(source.contains("occurrenceId") ? ":"+source["occurrenceId"].toString() : QString{});
            if(sources.contains(centerKey))return "Duplicate support center reference";
            sources.insert(centerKey);referenceIds.insert(reference["id"].toString());
            resolvedPoints.append(QJsonObject{{"id",reference["id"]},{"source",source},{"uMm",0},{"vMm",0}});
        }
        if(parameters.contains("texts")){
            auto textOnly=parameters;textOnly.remove("projections");textOnly.remove("pointReferences");textOnly.remove("pointProjections");textOnly.remove("circleProjections");
            return validateParameters(textOnly,authoredIds);
        }
        try {return validateParameters(expandedProjectionFeature({{"parameters",parameters}},{{"lines",lines},{"circles",circles},{"points",resolvedPoints}})["parameters"].toObject(),authoredIds,referenceIds);}
        catch(const std::exception& error){return QString::fromUtf8(error.what());}
    }
    const QSet<QString> allowed{"plane", "planeId", "originConvention", "points", "entities", "constraints", "dimensions", "nextDimensionNameIndex", "originPointId", "support", "texts", "loopOffsets", "openChainOffsets", "closedRectOffsets", "roundedRectangleIntent", "mirrorGroups", "slotGroups", "arcSlotGroups"};
    for (auto it = parameters.begin(); it != parameters.end(); ++it)
        if (!allowed.contains(it.key())) return "Unknown sketch parameter: " + it.key();
    if (!parameters["points"].isArray() || !parameters["entities"].isArray()) return "Sketch requires points and entities";
    if(parameters.contains("loopOffsets")){
        const auto error=RoundedLoopOffset::validate(parameters);
        if(!error.isEmpty())return error;
        const auto id=parameters["loopOffsets"].toArray().first().toObject()["id"].toString();
        if(authoredIds.contains(id))return "Duplicate loop offset UUID";
        authoredIds.insert(id);return {};
    }
    if(parameters.contains("texts")){
        if(!parameters["texts"].isArray() || parameters["texts"].toArray().size()!=1 || !parameters["entities"].toArray().isEmpty())return "Sketch Text requires exactly one text object and no analytic entities";
        auto ids=authoredIds;const auto error=SketchText::validate(parameters["texts"].toArray().first().toObject(),ids);
        if(!error.isEmpty())return error;
        authoredIds=ids;
    }
    for (const auto& field : {"constraints", "dimensions"})
        if (parameters.contains(field) && !parameters[field].isArray()) return QString(field) + " must be an array";
    if(parameters.contains("nextDimensionNameIndex") &&
        (!parameters["nextDimensionNameIndex"].isDouble() ||
         parameters["nextDimensionNameIndex"].toInt()<1 ||
         parameters["nextDimensionNameIndex"].toInt()>1000000))
        return "Sketch dimension name counter is invalid";
    if(parameters.contains("roundedRectangleIntent")){
        const auto marker=parameters["roundedRectangleIntent"].toObject();
        if(!parameters["roundedRectangleIntent"].isObject() || (marker.size()!=5 && marker.size()!=6) ||
            !marker["sideIds"].isArray() || marker["sideIds"].toArray().size()!=4 ||
            !marker["sideDirections"].isArray() || marker["sideDirections"].toArray().size()!=4 ||
            !validId(marker["widthId"].toString()) || !validId(marker["heightId"].toString()) ||
            !validId(marker["originPointId"].toString()))return "Invalid rounded rectangle driving intent";
        if(marker.size()==6 && (!marker["cornerArcIds"].isArray() || marker["cornerArcIds"].toArray().isEmpty() || marker["cornerArcIds"].toArray().size()>4))
            return "Invalid rounded rectangle corner references";
        for(const auto& id:marker["sideIds"].toArray())if(!validId(id.toString()))return "Invalid rounded rectangle side reference";
        for(const auto& id:marker["cornerArcIds"].toArray())if(!validId(id.toString()))return "Invalid rounded rectangle corner reference";
        for(const auto& sign:marker["sideDirections"].toArray())if(!sign.isDouble() || std::abs(sign.toInt())!=1)return "Invalid rounded rectangle side direction";
    }
    QMap<QString, QJsonObject> points;
    auto claimId = [&](const QString& id) {
        if (!validId(id) || authoredIds.contains(id)) return false;
        authoredIds.insert(id);
        return true;
    };
    try {
        for (const auto& entry : parameters["points"].toArray()) {
            if (!entry.isObject()) return "Sketch point must be an object";
            const auto point = entry.toObject();
            const auto id = point["id"].toString();
            if (point.size() != 3 || !claimId(id)) return "Invalid or duplicate sketch point UUID";
            coordinate(point, "uMm");
            coordinate(point, "vMm");
            points.insert(id, point);
        }
        QSet<QString> usedPoints=derivedPointIds, entityIds;
        usedPoints.unite(SketchOpenChainOffset::derivedPointIds(parameters));
        const auto originId=parameters["originPointId"].toString();
        if (parameters.contains("originPointId")) {
            if (!parameters["originPointId"].isString() || !points.contains(originId)) return "Sketch origin reference point does not exist";
            const auto origin=points[originId];
            if (std::abs(origin["uMm"].toDouble())>endpointTolerance || std::abs(origin["vMm"].toDouble())>endpointTolerance) return "Sketch origin reference must remain at zero";
            usedPoints.insert(originId);
        }
        for (const auto& entry : parameters["entities"].toArray()) {
            if (!entry.isObject()) return "Sketch entity must be an object";
            const auto entity = entry.toObject();
            const auto type = entity["type"].toString();
            if (!claimId(entity["id"].toString())) return "Invalid or duplicate sketch entity UUID";
            if(type=="fitSpline"||type=="controlSpline"){const auto error=SketchSpline::validate(parameters,entity);if(!error.isEmpty())return error;for(const auto& id:entity["pointIds"].toArray())usedPoints.insert(id.toString());entityIds.insert(entity["id"].toString());continue;}
            if(type=="fitSplineOffset"){
                const auto error=SketchSplineOffset::validate(parameters,entity,false);if(!error.isEmpty())return error;
                if(!points.contains(entity["startPointId"].toString()) || !points.contains(entity["endPointId"].toString()))return "Spline Offset endpoints must be retained sketch points";
                usedPoints.insert(entity["startPointId"].toString());usedPoints.insert(entity["endPointId"].toString());
                entityIds.insert(entity["id"].toString());continue;
            }
            if(type=="conic"){const auto error=SketchConic::validate(parameters,entity);if(!error.isEmpty())return error;for(const auto& id:entity["pointIds"].toArray())usedPoints.insert(id.toString());entityIds.insert(entity["id"].toString());continue;}
            if(type=="ellipse"){
                if(entity.size()!=5 || !points.contains(entity["centerPointId"].toString()) ||
                    !entity["firstAxisId"].isString() || !entity["secondAxisId"].isString() ||
                    entity["firstAxisId"]==entity["secondAxisId"])
                    return "Ellipse requires one center and two distinct retained axes";
                usedPoints.insert(entity["centerPointId"].toString());entityIds.insert(entity["id"].toString());continue;
            }
            if (type != "line" && type != "circle" && type != "arc" && type != "point") return "Unsupported sketch entity type";
            if(type=="point") {
                if(entity.size()!=3 || !points.contains(entity["pointId"].toString()))return "Point entity requires exactly one existing point UUID";
                usedPoints.insert(entity["pointId"].toString());entityIds.insert(entity["id"].toString());continue;
            }
            const bool hasConstruction = entity.contains("construction");
            if (hasConstruction && ((type != "line" && type != "arc") || !entity["construction"].isBool()))
                return "Construction must be a boolean on a line or arc entity";
            const bool hasCenterline=entity.contains("centerline");
            if(hasCenterline && (type!="line" || !entity["centerline"].isBool()))return "Centerline must be a boolean on a line entity";
            if(entity["centerline"].toBool() && entity["construction"].toBool())return "Centerline and Construction are distinct mutually exclusive linetypes";
            const bool twoPointCircle=type=="circle" && entity.contains("diameterStartPointId");
            if (entity.size() != (type == "line" ? 4 : type == "circle" ? 4 : 8) + int(hasConstruction) + int(hasCenterline) + (twoPointCircle ? 3 : 0)) return "Unexpected sketch entity fields";
            if (type != "line") {
                const auto centerId = entity["centerPointId"].toString();
                if (!points.contains(centerId)) return "Curve center does not exist";
                usedPoints.insert(centerId);
                const double radius=coordinate(entity,"radiusMm");
                if (radius < minimumLength) return "Curve radius out of supported range";
                if(twoPointCircle) {
                    const auto startId=entity["diameterStartPointId"].toString();
                    if(!points.contains(startId) || startId==centerId)return "Two-point circle needs a distinct retained diameter endpoint";
                    const double du=coordinate(entity,"diameterDirectionU"),dv=coordinate(entity,"diameterDirectionV");
                    if(std::abs(std::hypot(du,dv)-1)>1e-9)return "Two-point circle direction must be unit length";
                    usedPoints.insert(startId);
                }
                if (type == "arc") {
                    const double start=coordinate(entity,"startAngleDeg"), sweep=coordinate(entity,"sweepAngleDeg");
                    if (std::abs(start) > 180 || std::abs(sweep) < 1e-6 || std::abs(sweep) >= 360 || radius*std::abs(sweep)*degrees < minimumLength)
                        return "Invalid normalized arc angles";
                    const auto center=points[centerId];
                    for (const auto& key : {"startPointId","endPointId"}) {
                        const auto endpoint=entity[key].toString(); if (!points.contains(endpoint)) return "Arc endpoint does not exist";
                        const double angle=(start+(QString(key)=="endPointId" ? sweep : 0))*degrees;
                        const auto p=points[endpoint];
                        if (std::hypot(p["uMm"].toDouble()-center["uMm"].toDouble()-radius*std::cos(angle),
                            p["vMm"].toDouble()-center["vMm"].toDouble()-radius*std::sin(angle)) > endpointTolerance)
                            return "Arc endpoints do not match its analytic radius and sweep";
                        usedPoints.insert(endpoint);
                    }
                    if (entity["startPointId"] == entity["endPointId"]) return "Arc endpoints must be distinct";
                }
            } else {
                const auto startId=entity["startPointId"].toString(), endId=entity["endPointId"].toString();
                if (!points.contains(startId) || !points.contains(endId)) return "Sketch line endpoint does not exist";
                const auto start=points[startId], end=points[endId];
                const double length=std::hypot(end["uMm"].toDouble()-start["uMm"].toDouble(),end["vMm"].toDouble()-start["vMm"].toDouble());
                if (length < minimumLength || length > maximumLength) return "Sketch line length out of supported range (0.001..100000 mm)";
                usedPoints.insert(startId); usedPoints.insert(endId);
            }
            entityIds.insert(entity["id"].toString());
        }
        // Offset endpoints are owned by exactly one retained evaluator. Lines
        // may share them to close the strip, but no other authored primitive
        // may silently take over a derived point.
        QSet<QString> ownedOffsetEndpoints,offsetConnectorIds;
        for(const auto& entry:parameters["entities"].toArray()){
            const auto offset=entry.toObject();if(offset["type"]!="fitSplineOffset")continue;
            QJsonObject source;
            for(const auto& value:parameters["entities"].toArray())if(value.toObject()["id"]==offset["sourceEntityId"]){source=value.toObject();break;}
            for(const auto& key:{"startPointId","endPointId"}){
                const auto id=offset[key].toString();
                if(id.isEmpty() || ownedOffsetEndpoints.contains(id) || id==originId || source["pointIds"].toArray().contains(id))
                    return "Spline Offset endpoint must be exclusively derived";
                for(const auto& value:parameters["pointReferences"].toArray())if(value.toObject()["id"]==id)
                    return "Spline Offset cannot overwrite a retained support point";
                ownedOffsetEndpoints.insert(id);
            }
        }
        for(const auto& entry:parameters["entities"].toArray()){
            const auto entity=entry.toObject();if(entity["type"]=="fitSplineOffset")continue;
            bool touches=false;
            for(const auto& key:{"pointId","centerPointId","startPointId","endPointId"})
                touches|=ownedOffsetEndpoints.contains(entity[key].toString());
            for(const auto& id:entity["pointIds"].toArray())touches|=ownedOffsetEndpoints.contains(id.toString());
            if(!touches)continue;
            if(entity["type"]!="line")return "Spline Offset endpoint cannot be shared with another primitive";
            offsetConnectorIds.insert(entity["id"].toString());
        }
        // A midpoint relation can retain a construction center point even when
        // that point is not an endpoint of any visible edge.
        for(const auto& value:parameters["constraints"].toArray()) {
            const auto constraint=value.toObject();
            if(constraint["type"]=="midpoint" && constraint["pointIds"].isArray()
                && constraint["pointIds"].toArray().size()==1)
                usedPoints.insert(constraint["pointIds"].toArray()[0].toString());
        }
        if (usedPoints.size() != points.size()) return "Sketch contains unreferenced points";
        auto checkReferences = [](const QJsonObject& object, const QString& field, int count, const QSet<QString>& valid) {
            if (!object[field].isArray() || object[field].toArray().size() != count) return false;
            QSet<QString> used;
            for (const auto& id : object[field].toArray()) {
                if (!id.isString() || !valid.contains(id.toString()) || used.contains(id.toString())) return false;
                used.insert(id.toString());
            }
            return true;
        };
        const QSet<QString> pointIds(points.keyBegin(), points.keyEnd());
        QSet<QString> lineIds, curveIds, arcIds;
        for (const auto& entry : parameters["entities"].toArray()) {const auto e=entry.toObject();
            if(e["type"]=="line") lineIds.insert(e["id"].toString());
            else {curveIds.insert(e["id"].toString()); if(e["type"]=="arc") arcIds.insert(e["id"].toString());}}
        QSet<QString> ownedEllipseAxes;
        for(const auto& value:parameters["entities"].toArray()){
            const auto ellipse=value.toObject();if(ellipse["type"]!="ellipse")continue;
            const auto center=ellipse["centerPointId"].toString();
            const auto first=ellipse["firstAxisId"].toString(),second=ellipse["secondAxisId"].toString();
            if(!lineIds.contains(first) || !lineIds.contains(second) || ownedEllipseAxes.contains(first) || ownedEllipseAxes.contains(second))
                return "Ellipse axes must be distinct retained construction lines";
            ownedEllipseAxes.insert(first);ownedEllipseAxes.insert(second);
            bool firstMid=false,secondMid=false,perpendicular=false;
            for(const auto& relationValue:parameters["constraints"].toArray()){
                const auto relation=relationValue.toObject();const auto ids=relation["entityIds"].toArray();
                if(relation["type"]=="midpoint" && relation["pointIds"].toArray()==QJsonArray{center}){
                    firstMid|=ids==QJsonArray{first};secondMid|=ids==QJsonArray{second};
                }
                if(relation["type"]=="perpendicular" && ids.size()==2 && ids.contains(first) && ids.contains(second))perpendicular=true;
            }
            if(!firstMid || !secondMid || !perpendicular)return "Ellipse lost its center or perpendicular axis relations";
            try{SketchEllipse::axes(parameters,ellipse);}catch(const std::exception& error){return QString::fromUtf8(error.what());}
        }
        if(parameters.contains("slotGroups") && !parameters["slotGroups"].isArray())return "Slot groups must be an array";
        for(const auto& value:parameters["slotGroups"].toArray()) {
            if(!value.isObject())return "Slot group must be an object";
            const auto group=value.toObject();
            if((group.size()!=5 && !(group.size()==6 && group["mode"]=="overall")) || !claimId(group["id"].toString()) ||
                !points.contains(group["centerPointId"].toString()) || !lineIds.contains(group["centerlineId"].toString()) ||
                !checkReferences(group,"sideIds",2,lineIds) || !checkReferences(group,"arcIds",2,arcIds))
                return "Invalid Center Point Slot references";
            const auto centerlineId=group["centerlineId"].toString();
            if(group["sideIds"].toArray().contains(centerlineId))return "Slot centerline cannot bound its profile";
            bool construction=false;
            for(const auto& entry:parameters["entities"].toArray())if(entry.toObject()["id"]==centerlineId)
                construction=entry.toObject()["construction"].toBool();
            if(!construction)return "Slot centerline must remain construction geometry";
            if(group["mode"]=="overall") {
                QJsonObject guide;for(const auto& entry:parameters["entities"].toArray())if(entry.toObject()["id"]==centerlineId){guide=entry.toObject();break;}
                const QJsonArray ends{guide["startPointId"],guide["endPointId"]};const auto arc=group["arcIds"].toArray().first();
                int lengths=0,widths=0,angles=0;for(const auto& entry:parameters["dimensions"].toArray()){
                    const auto d=entry.toObject();if(d["kind"]=="overallSlotLength" && d["pointIds"]==ends && d["entityId"]==arc && d["driving"].toBool())++lengths;
                    if(d["kind"]=="diameter" && d["entityId"]==arc && d["driving"].toBool())++widths;
                    if(d["kind"]=="overallSlotAngle" && d["pointIds"]==ends && d["driving"].toBool())++angles;
                }
                if(lengths!=1 || widths!=1 || angles!=1)return "Overall Slot needs one retained total length, width and angle dimension";
            }
        }
        if(parameters.contains("arcSlotGroups") && !parameters["arcSlotGroups"].isArray())return "Arc slot groups must be an array";
        QSet<QString> arcSlotOwnedIds;
        for(const auto& value:parameters["arcSlotGroups"].toArray()){
            if(!value.isObject())return "Arc slot group must be an object";
            const auto group=value.toObject();
            const auto sign=group["sweepSign"].toInt();
            if(group.size()!=6 || !claimId(group["id"].toString()) || !arcIds.contains(group["pathArcId"].toString()) ||
                !arcIds.contains(group["outerArcId"].toString()) || !arcIds.contains(group["innerArcId"].toString()) ||
                !checkReferences(group,"capIds",2,arcIds) || (sign!=1 && sign!=-1))
                return "Invalid Center Point Arc Slot references";
            for(const auto& id:QJsonArray{group["pathArcId"],group["outerArcId"],group["innerArcId"],
                group["capIds"].toArray().at(0),group["capIds"].toArray().at(1)}){
                if(arcSlotOwnedIds.contains(id.toString()))return "Arc slot curve belongs to more than one boundary role";
                arcSlotOwnedIds.insert(id.toString());
            }
        }
        const auto openOffsetError=SketchOpenChainOffset::validate(parameters);
        if(!openOffsetError.isEmpty())return openOffsetError;
        if(parameters.contains("closedRectOffsets")) {
            if(!parameters["closedRectOffsets"].isArray())return "Closed offsets must be an array";
            QSet<QString> sources;
            for(const auto& value:parameters["closedRectOffsets"].toArray()) {
                const auto group=value.toObject();
                if(group.size()!=5 || !claimId(group["id"].toString()) || !group["distanceMm"].isDouble()
                    || std::abs(group["distanceMm"].toDouble())<minimumLength
                    || !std::isfinite(group["distanceMm"].toDouble())
                    || group["sourceEntityIds"].toArray().size()!=4 || group["pointIds"].toArray().size()!=4
                    || group["entityIds"].toArray().size()!=4)return "Invalid retained closed rectangle Offset";
                for(const auto& source:group["sourceEntityIds"].toArray()) {
                    if(!validId(source.toString()) || sources.contains(source.toString()))return "Closed Offset source identity is repeated";
                    sources.insert(source.toString());
                }
                for(const auto& id:group["pointIds"].toArray())if(!validId(id.toString()))return "Closed Offset corner identity is invalid";
                for(const auto& id:group["entityIds"].toArray())if(!validId(id.toString()))return "Closed Offset edge identity is invalid";
            }
        }
        for(const auto& value:parameters["openChainOffsets"].toArray())
            if(!claimId(value.toObject()["id"].toString()))return "Duplicate open-chain offset UUID";
        const auto arcSlotDerivedPoints=SketchArcSlot::derivedPointIds(parameters);
        const auto offsetDerivedPoints=SketchSplineOffset::derivedPointIds(parameters);
        const auto arcSlotDerivedArcs=SketchArcSlot::derivedArcIds(parameters);
        const auto arcSlotCaps=SketchArcSlot::capIds(parameters);
        for (const auto& entry : parameters["constraints"].toArray()) {
            if (!entry.isObject()) return "Sketch constraint must be an object";
            const auto constraint = entry.toObject();
            for(const auto& id:constraint.value("pointIds").toArray())if(arcSlotDerivedPoints.contains(id.toString()))
                return "Arc slot tangent contacts cannot own independent constraints";
            for(const auto& id:constraint.value("pointIds").toArray())if(offsetDerivedPoints.contains(id.toString()))
                return "Spline Offset endpoints are derived; edit the source or signed distance";
            for(const auto& id:constraint.value("entityIds").toArray())if(offsetConnectorIds.contains(id.toString()))
                return "Spline Offset connector cannot own an independent relation";
            for(const auto& id:constraint.value("entityIds").toArray())if(arcSlotDerivedArcs.contains(id.toString()))
                return "Arc slot boundary arcs cannot own independent constraints";
            if (!claimId(constraint["id"].toString())) return "Invalid or duplicate constraint UUID";
            const auto type = constraint["type"].toString();
            if (type == "coincident" || type == "fixed") {
                const bool fixed = type == "fixed";
                if (constraint.size() != (fixed ? 5 : 3) || constraint.contains("entityIds")
                    || !checkReferences(constraint, "pointIds", fixed ? 1 : 2, pointIds)) return "Invalid point constraint references";
                if (fixed) { coordinate(constraint, "uMm"); coordinate(constraint, "vMm"); }
            } else if (type == "pointOnCurve" || type == "supportLine") {
                if(constraint.size()!=4 || !checkReferences(constraint,"pointIds",1,pointIds) || !checkReferences(constraint,"entityIds",1,entityIds))return "Point-on-curve requires one point and one curve";
                const auto id=constraint["entityIds"].toArray()[0].toString();for(const auto& entry:parameters["entities"].toArray())if(entry.toObject()["id"]==id &&
                    (type=="supportLine" ? entry.toObject()["type"]!="line" : entry.toObject()["type"]!="line" && entry.toObject()["type"]!="circle"))
                    return type=="supportLine" ? "Supporting-line incidence requires a line" : "Point-on-curve supports lines and circles";
            } else if (type == "arcTangent") {
                if (constraint.size()!=5 || !checkReferences(constraint,"entityIds",2,entityIds)
                    || !checkReferences(constraint,"pointIds",1,pointIds) || !constraint["side"].isDouble()
                    || (constraint["side"].toDouble()!=1 && constraint["side"].toDouble()!=-1))return "Arc tangency requires endpoint and signed side";
                const auto refs=constraint["entityIds"].toArray(); const auto contact=constraint["pointIds"].toArray()[0].toString();
                if(!arcIds.contains(refs[0].toString()) || !lineIds.contains(refs[1].toString()))return "Arc tangent references must be arc then line";
                for(const auto& value:parameters["entities"].toArray()) {const auto e=value.toObject();
                    if((e["id"]==refs[0] || e["id"]==refs[1]) && e["startPointId"]!=contact && e["endPointId"]!=contact)
                        return "Arc tangent contact must be a shared endpoint";}
            } else if (type == "tangent") {
                const bool endpointContact=constraint.contains("pointIds");
                if (constraint.size()!=(endpointContact ? 5 : 4) || !checkReferences(constraint,"entityIds",2,entityIds)
                    || !constraint["side"].isDouble() || (constraint["side"].toDouble()!=1 && constraint["side"].toDouble()!=-1))
                    return "Tangent requires one circle, one line and signed side";
                const auto refs=constraint["entityIds"].toArray();
                bool circle=false;
                for (const auto& entity:parameters["entities"].toArray())
                    if (entity.toObject()["id"]==refs[0] && entity.toObject()["type"]=="circle") circle=true;
                if (!circle || !lineIds.contains(refs[1].toString())) return "Tangent references must be ordered circle then line";
                if(endpointContact) {
                    if(!checkReferences(constraint,"pointIds",1,pointIds))return "Tangent contact needs one retained point";
                    const auto contact=constraint["pointIds"].toArray().first();bool lineEndpoint=false,onCircle=false;
                    for(const auto& value:parameters["entities"].toArray()){
                        const auto line=value.toObject();if(line["id"]==refs[1] &&
                            (line["startPointId"]==contact || line["endPointId"]==contact))lineEndpoint=true;
                    }
                    for(const auto& value:parameters["constraints"].toArray()){
                        const auto row=value.toObject();if(row["type"]=="pointOnCurve" &&
                            row["pointIds"]==QJsonArray{contact} && row["entityIds"]==QJsonArray{refs[0]})onCircle=true;
                    }
                    if(!lineEndpoint || !onCircle)return "Tangent contact needs its line endpoint on the retained circle";
                }
            } else if (type == "midpoint") {
                QSet<QString> midpointEntities=lineIds;
                midpointEntities.unite(arcIds);
                if (constraint.size() != 4 || !checkReferences(constraint,"pointIds",1,pointIds)
                    || !checkReferences(constraint,"entityIds",1,midpointEntities)) return "Midpoint requires one point and one line or arc";
                const auto refs = constraint["pointIds"].toArray();
                for (const auto& entity : parameters["entities"].toArray()) {
                    const auto line = entity.toObject();
                    if (line["id"] == constraint["entityIds"].toArray()[0]
                        && (line["startPointId"] == refs[0] || line["endPointId"] == refs[0]))
                        return "Midpoint point must be distinct from the line endpoints";
                }
            } else if(type=="concentric") {
                if(constraint.size()!=3 || !checkReferences(constraint,"entityIds",2,curveIds) || constraint.contains("pointIds"))
                    return "Concentric requires two distinct circles or arcs";
            } else if(type=="symmetry") {
                if(constraint.size()!=4 || !checkReferences(constraint,"entityIds",3,lineIds)
                    || !checkReferences(constraint,"pointIds",4,pointIds))return "Symmetry requires two lines, one construction axis and four endpoints";
                const auto refs=constraint["entityIds"].toArray(),pairs=constraint["pointIds"].toArray();
                QJsonObject first,second,axis;
                for(const auto& value:parameters["entities"].toArray()){
                    const auto e=value.toObject();
                    if(e["id"]==refs[0])first=e;
                    if(e["id"]==refs[1])second=e;
                    if(e["id"]==refs[2])axis=e;
                }
                if(!axis["construction"].toBool())return "Symmetry axis must be a construction line";
                if(pairs[0]!=first["startPointId"] || pairs[2]!=first["endPointId"]
                    || !((pairs[1]==second["startPointId"] && pairs[3]==second["endPointId"])
                      || (pairs[1]==second["endPointId"] && pairs[3]==second["startPointId"])))
                    return "Symmetry endpoints must match the retained line pairing";
            } else if((type=="horizontal" || type=="vertical") && constraint.contains("pointIds")) {
                if(constraint.size()!=3 || !checkReferences(constraint,"pointIds",2,pointIds) || constraint.contains("entityIds"))
                    return "Point alignment requires two distinct sketch points";
            } else if (type == "equal") {
                if (constraint.size() != 3 || !checkReferences(constraint,"entityIds",2,entityIds))
                    return "Equal requires two distinct sketch entities";
                const auto refs = constraint["entityIds"].toArray();
                const bool lines = lineIds.contains(refs[0].toString()) && lineIds.contains(refs[1].toString());
                const bool curves = curveIds.contains(refs[0].toString()) && curveIds.contains(refs[1].toString());
                if (!lines && !curves) return "Equal requires two lines or two circles/arcs";
            } else if(type=="circleOffset") {
                if(constraint.size()!=4 || !checkReferences(constraint,"entityIds",2,entityIds)
                    || constraint.contains("pointIds"))return "Circle offset requires source and result circle IDs";
                const auto refs=constraint["entityIds"].toArray();
                bool source=false,target=false;QString sourceCenter,targetCenter;
                for(const auto& value:parameters["entities"].toArray()) {
                    const auto entity=value.toObject();
                    if(entity["id"]==refs[0]){source=entity["type"]=="circle";sourceCenter=entity["centerPointId"].toString();}
                    if(entity["id"]==refs[1]){target=entity["type"]=="circle";targetCenter=entity["centerPointId"].toString();}
                }
                if(!source || !target)return "Circle offset requires two circles";
                bool concentric=sourceCenter==targetCenter;
                for(const auto& value:parameters["constraints"].toArray())if(value.toObject()["type"]=="coincident") {
                    const auto ids=value.toObject()["pointIds"].toArray();
                    concentric|=ids==QJsonArray{sourceCenter,targetCenter} || ids==QJsonArray{targetCenter,sourceCenter};
                }
                if(!concentric)return "Circle offset requires retained concentric centers";
                const double distance=coordinate(constraint,"distanceMm");
                if(std::abs(distance)<minimumLength || std::abs(distance)>maximumLength)return "Invalid circle offset distance";
                for(const auto& other:parameters["constraints"].toArray())if(other.toObject()["type"]=="circleOffset"
                    && other.toObject()["entityIds"].toArray()[1]==refs[0])return "Offset result cannot be used as a source in this slice";
            } else if (type == "horizontal" || type == "vertical" || type == "parallel" || type == "perpendicular" || type == "collinear") {
                if (constraint.size() != 3 || constraint.contains("pointIds")
                    || !checkReferences(constraint, "entityIds", (type == "parallel" || type == "perpendicular" || type == "collinear") ? 2 : 1, lineIds))
                    return "Invalid line constraint references";
            } else return "Unsupported sketch constraint type";
        }
        if(parameters.contains("mirrorGroups")){
            const auto mirrorError=SketchMirror::validate(parameters);if(!mirrorError.isEmpty())return mirrorError;
            for(const auto& value:parameters["mirrorGroups"].toArray())if(!claimId(value.toObject()["id"].toString()))return "Invalid or duplicate Mirror group UUID";
        }
        if (!originId.isEmpty()) {
            bool fixed=false;
            for (const auto& entry:parameters["constraints"].toArray()) {const auto c=entry.toObject();
                if (c["type"]=="fixed" && c["pointIds"].toArray()==QJsonArray{originId} && c["uMm"].toDouble()==0 && c["vMm"].toDouble()==0) fixed=true;}
            if (!fixed) return "Sketch origin reference requires a retained fixed relation at zero";
        }
        QSet<QString> localDimensionNames;int largestLocalName=0;
        for (const auto& entry : parameters["dimensions"].toArray()) {
            if (!entry.isObject()) return "Sketch dimension must be an object";
            const auto dimension = entry.toObject();
            for(const auto& id:dimension.value("pointIds").toArray())if(arcSlotDerivedPoints.contains(id.toString()))
                return "Arc slot tangent contacts cannot own independent dimensions";
            for(const auto& id:dimension.value("pointIds").toArray())if(offsetDerivedPoints.contains(id.toString()))
                return "Spline Offset endpoints cannot own independent dimensions";
            for(const auto& id:dimension.value("entityIds").toArray())if(offsetConnectorIds.contains(id.toString()))
                return "Spline Offset connector cannot own an independent dimension";
            if(offsetConnectorIds.contains(dimension.value("entityId").toString()))
                return "Spline Offset connector cannot own an independent dimension";
            const auto boundaryId=dimension.value("entityId").toString();
            if(arcSlotDerivedArcs.contains(boundaryId) && !(arcSlotCaps.contains(boundaryId) &&
                (dimension.value("kind")=="radius" || dimension.value("kind")=="diameter")))
                return "Arc slot boundary accepts only a rounded end radius dimension";
            if (!claimId(dimension["id"].toString())) return "Invalid or duplicate dimension UUID";
            const auto kind = dimension["kind"].toString();
            const bool curve = kind == "radius" || kind == "diameter" || kind == "arcSweep";
            const bool angle = kind == "angle" || kind == "overallSlotAngle";
            if (!dimension["driving"].isBool()) return "Dimension requires driving flag";
            if (curve) {
                if (!(kind=="arcSweep" ? arcIds : curveIds).contains(dimension["entityId"].toString())) return "Invalid curve dimension reference";
            } else if (kind=="overallSlotLength") {
                if(!checkReferences(dimension,"pointIds",2,pointIds) || !arcIds.contains(dimension["entityId"].toString()))return "Overall length needs two guide endpoints and one slot arc";
            } else if (kind=="overallSlotAngle") {
                if(!checkReferences(dimension,"pointIds",2,pointIds))return "Overall angle needs two guide endpoints";
            } else if (angle) {
                if (!checkReferences(dimension,"entityIds",2,lineIds)) return "Angle requires two distinct lines";
                if (dimension.contains("raySigns")) {
                    const auto signs=dimension["raySigns"].toArray();
                    if (!dimension["raySigns"].isArray() || signs.size()!=2) return "Angle raySigns requires two signs";
                    for (const auto& sign:signs) if (!sign.isDouble() || (sign.toDouble()!=1 && sign.toDouble()!=-1)) return "Angle ray signs must be +1 or -1";
                }
            } else if(kind=="centerlineDiameter") {
                if(!checkReferences(dimension,"entityIds",2,lineIds))return "Centerline diameter requires two distinct line references";
                const auto ids=dimension["entityIds"].toArray();QJsonObject axis,edge;
                for(const auto& value:parameters["entities"].toArray()){const auto line=value.toObject();if(line["id"]==ids[0])axis=line;if(line["id"]==ids[1])edge=line;}
                if(!axis["centerline"].toBool() || edge["centerline"].toBool() || axis["construction"].toBool() || edge["construction"].toBool())return "Diameter requires an ordered Centerline and ordinary profile line";
                if(!dimension["side"].isDouble() || (dimension["side"].toDouble()!=1 && dimension["side"].toDouble()!=-1))return "Centerline diameter side must be +1 or -1";
            } else if(kind=="parallelLineDistance") {
                if(!checkReferences(dimension,"entityIds",2,lineIds))return "Parallel distance requires two distinct straight lines";
                if(!dimension["side"].isDouble() || (dimension["side"].toDouble()!=1 && dimension["side"].toDouble()!=-1))return "Parallel distance side must be +1 or -1";
            } else if(kind=="pointLineDistance") {
                if(!checkReferences(dimension,"pointIds",1,pointIds) || !lineIds.contains(dimension["entityId"].toString()))return "Point-edge dimension requires one point and a straight line";
                if(!dimension["side"].isDouble() || (dimension["side"].toDouble()!=1 && dimension["side"].toDouble()!=-1))return "Point-edge dimension side must be +1 or -1";
            } else if ((kind != "linear" && kind != "horizontal" && kind != "vertical")
                || !checkReferences(dimension,"pointIds",2,pointIds)) return "Invalid dimension kind or references";
            const bool driving=dimension["driving"].toBool(), placed=dimension.contains("annotation");
            const auto valueKey=kind=="arcSweep" || angle ? "valueDeg" : "valueMm";
            const bool named=dimension.contains("name"),formula=dimension.contains("expression");
            if(named){
                static const QRegularExpression localName(QStringLiteral(R"(^d([1-9]\d*)$)"));
                const auto match=localName.match(dimension["name"].toString());
                if(!dimension["name"].isString() || !match.hasMatch() || !localLengthDimension(dimension) ||
                   localDimensionNames.contains(dimension["name"].toString()))return "Invalid or duplicate local dimension name";
                localDimensionNames.insert(dimension["name"].toString());
                largestLocalName=std::max(largestLocalName,match.captured(1).toInt());
            }
            if(formula && (!named || dimension.contains("parameterId") ||
               !dimension["expression"].isString() || dimension["expression"].toString().trimmed().isEmpty() ||
               !dimension["expressionUnits"].isString() ||
               (dimension["expressionUnits"]!="mm" && dimension["expressionUnits"]!="in") ||
               !dimension["referenceIds"].isArray()))return "Invalid local dimension expression";
            if(dimension.size() != (driving ? 5 : 4)+int(placed)+int(kind=="angle" && dimension.contains("raySigns"))+2*int(kind=="pointLineDistance")+int(kind=="centerlineDiameter" || kind=="parallelLineDistance" || kind=="overallSlotLength")+int(dimension.contains("parameterId"))+int(named)+3*int(formula) || (!driving && dimension.contains(valueKey))) return "Unexpected dimension fields";
            if(dimension.contains("parameterId") && (!driving || angle || kind=="arcSweep" || !validId(dimension["parameterId"].toString())))return "Only driving length dimensions accept stable parameter references";
            if (placed) {
                if (!dimension["annotation"].isObject()) return "Dimension annotation must be an object";
                const auto annotation = dimension["annotation"].toObject();
                if (annotation.size() != 2) return "Dimension annotation requires only uMm and vMm";
                coordinate(annotation, "uMm"); coordinate(annotation, "vMm");
            }
            if (driving) {
                const double value = coordinate(dimension, valueKey);
                if (kind=="arcSweep" && (value < 1e-6 || value >= 360)) return "Angular dimension must be between zero and 360 degrees";
                if (kind=="angle" && (value < 1e-6 || value >= 180)) return "Line angle must be between zero and 180 degrees";
                if (kind=="overallSlotAngle" && std::abs(value)>360) return "Overall Slot angle must be within 360 degrees";
                if (kind!="arcSweep" && !angle && value < minimumLength) return "Driving dimension must be positive (0.001..100000 mm)";
            }
        }
        if(parameters.contains("nextDimensionNameIndex") &&
            parameters["nextDimensionNameIndex"].toInt()<=largestLocalName)
            return "Sketch dimension name counter must exceed retained names";
    } catch (const std::invalid_argument& error) { return QString::fromUtf8(error.what()); }
    return {};
}

SketchGeometry::SolveResult SketchGeometry::solveDraft(const QJsonObject& sourceDraft, const QJsonObject& projectionContext, const QString& initialReferencePointId) {
    try {
        const auto draft=resolvedDimensionFeature(sourceDraft);
        validateDraft(draft);
        const auto authored=draft["parameters"].toObject();
        for(const auto& value:authored["closedRectOffsets"].toArray()) {
            const auto group=value.toObject();const auto sources=group["sourceEntityIds"].toArray(),pointIds=group["pointIds"].toArray(),entityIds=group["entityIds"].toArray();
            for(const auto& source:sources) {
                bool linked=false;
                for(const auto& row:authored["projections"].toArray())if(row.toObject()["id"]==source && row.toObject()["profile"]==false)
                    linked=true;
                if(!linked)reject("Closed Offset requires its retained non-profile linked source edges");
            }
            for(int i=0;i<4;++i) {
                bool found=false;for(const auto& row:authored["entities"].toArray()) {
                    const auto entity=row.toObject();if(entity["id"]==entityIds[i] && entity["type"]=="line"
                        && entity["startPointId"]==pointIds[i] && entity["endPointId"]==pointIds[(i+1)%4]
                        && !entity["construction"].toBool())found=true;
                }
                if(!found)reject("Closed Offset generated boundary identity changed");
            }
        }
        if(draft["parameters"].toObject().contains("texts") || draft["parameters"].toObject().contains("loopOffsets"))
            return {true,draft,{{"solver","direct text parameters"},{"status","accepted"}}, {}};
        auto expanded=expandedProjectionFeature(draft,projectionContext);
        if(expanded["parameters"].toObject().contains("openChainOffsets"))
            expanded["parameters"]=SketchOpenChainOffset::derive(expanded["parameters"].toObject());
        expanded["parameters"]=deriveClosedRectOffsets(expanded["parameters"].toObject());
        QSet<QString> lockedPoints,virtualEntities;
        for(const auto& value:draft["parameters"].toObject()["projections"].toArray()){const auto id=value.toObject()["id"].toString();
            virtualEntities.insert(id);lockedPoints.insert(projectionPointId(id,false));lockedPoints.insert(projectionPointId(id,true));}
        QSet<QString> virtualDimensions;
        for(const auto& value:draft["parameters"].toObject()["circleProjections"].toArray()) {
            const auto id=value.toObject()["id"].toString();virtualEntities.insert(id);lockedPoints.insert(projectionPointId(id,false));
            virtualDimensions.insert(QUuid::createUuidV5(QUuid(id),QByteArray("alloy-projected-circle-diameter/1")).toString(QUuid::WithoutBraces));
        }
        QSet<QString> derivedPoints;
        for(const auto& value:draft["parameters"].toObject()["pointReferences"].toArray()){const auto id=value.toObject()["id"].toString();lockedPoints.insert(id);derivedPoints.insert(id);}
        for(const auto& value:draft["parameters"].toObject()["pointProjections"].toArray()){const auto id=value.toObject()["id"].toString();lockedPoints.insert(id);derivedPoints.insert(id);}
        const auto externalLockedPoints=lockedPoints;
        lockedPoints.unite(SketchOpenChainOffset::derivedPointIds(draft["parameters"].toObject()));
        lockedPoints.unite(closedRectOffsetPointIds(draft["parameters"].toObject()));
        if(!initialReferencePointId.isEmpty())lockedPoints.insert(initialReferencePointId);
        validateDraft(expanded,derivedPoints);
        const auto result=SketchSolver::solve(expanded["parameters"].toObject(),lockedPoints);
        if(!result.accepted)return {false,sourceDraft,result.diagnostics,result.error};
        auto solved=draft;auto parameters=deriveClosedRectOffsets(SketchOpenChainOffset::derive(result.parameters));
        QJsonArray points,entities,dimensions;
        for(const auto& value:parameters["points"].toArray())if(!externalLockedPoints.contains(value.toObject()["id"].toString()))points.append(value);
        for(const auto& value:parameters["entities"].toArray())if(!virtualEntities.contains(value.toObject()["id"].toString()))entities.append(value);
        for(const auto& value:parameters["dimensions"].toArray())if(!virtualDimensions.contains(value.toObject()["id"].toString()))dimensions.append(value);
        parameters["points"]=points;parameters["entities"]=entities;parameters["dimensions"]=dimensions;
        if(draft["parameters"].toObject().contains("texts"))parameters["texts"]=draft["parameters"].toObject()["texts"];
        if(draft["parameters"].toObject().contains("projections"))parameters["projections"]=draft["parameters"].toObject()["projections"];
        if(draft["parameters"].toObject().contains("pointReferences"))parameters["pointReferences"]=draft["parameters"].toObject()["pointReferences"];
        if(draft["parameters"].toObject().contains("pointProjections"))parameters["pointProjections"]=draft["parameters"].toObject()["pointProjections"];
        if(draft["parameters"].toObject().contains("circleProjections"))parameters["circleProjections"]=draft["parameters"].toObject()["circleProjections"];
        parameters=SketchSplineOffset::refreshEndpoints(parameters);
        solved["parameters"]=parameters;validateDraft(solved);validateRoundedIntent(parameters);validateSlotGroups(parameters);validateArcSlotGroups(parameters);
        for(const auto& value:entities)if(value.toObject()["type"]=="fitSplineOffset"){
            const auto error=SketchSplineOffset::validate(parameters,value.toObject(),true);
            if(!error.isEmpty())reject(error);
        }
        for(const auto& value:entities)if(value.toObject()["type"]=="fitSpline"||value.toObject()["type"]=="controlSpline")SketchSpline::evaluate(parameters,value.toObject());
        for(const auto& value:entities)if(value.toObject()["type"]=="conic")SketchConic::evaluate(parameters,value.toObject());
        auto diagnostics=result.diagnostics;int handleDof=0;for(const auto& v:entities)if(v.toObject()["type"]=="fitSpline")handleDof+=2*v.toObject()["pointIds"].toArray().size();
        if(handleDof){diagnostics["pointSolverDof"]=diagnostics["dof"];diagnostics["splineHandleDof"]=handleDof;diagnostics["dof"]=diagnostics["dof"].toInt()+handleDof;diagnostics["splineSolverScope"]="Fit-point constraints only; tangent vectors are editable evaluator parameters";diagnostics["status"]="underconstrained";}
        int rhoDof=0;for(const auto& value:entities)if(value.toObject()["type"]=="conic")++rhoDof;
        if(rhoDof){diagnostics["conicRhoDof"]=rhoDof;diagnostics["dof"]=diagnostics["dof"].toInt()+rhoDof;diagnostics["status"]="underconstrained";}
        return {true,solved,diagnostics,{}};
    } catch(const std::exception& error){return invalidDraft(sourceDraft,QString::fromUtf8(error.what()));}
}

SketchGeometry::SolveResult SketchGeometry::mirrorEntities(const QJsonObject& draft,const QJsonObject& arguments,const QJsonObject& projectionContext){
    try{
        validateDraft(draft);checkFields(arguments,{"sourceEntityIds","axisEntityId"});
        auto candidate=draft;candidate["parameters"]=SketchMirror::create(draft["parameters"].toObject(),arguments);
        const auto result=solveDraft(candidate,projectionContext);
        return result.ok ? result : SolveResult{false,draft,result.diagnostics,result.error};
    }catch(const std::exception& error){return invalidDraft(draft,QString::fromUtf8(error.what()));}
}

SketchGeometry::SolveResult SketchGeometry::addConstraint(const QJsonObject& draft, const QJsonObject& arguments, const QJsonObject& projectionContext) {
    try {
        validateDraft(draft);
        checkFields(arguments, {"type", "pointIds", "entityIds", "side", "origin"});
        auto constraint = arguments;
        constraint["id"] = newId();
        QJsonObject current=draft;
        if(constraint["type"]=="symmetry" || constraint["type"]=="tangent"){
            if(arguments.contains("pointIds"))reject("Constraint contact points are derived from current solved geometry");
            const auto solved=solveDraft(draft,projectionContext);
            if(!solved.ok)return SolveResult{false,draft,solved.diagnostics,solved.error};
            current=solved.feature;
        }
        auto parameters = current["parameters"].toObject();
        if(constraint["type"]=="tangent"){
            const auto ids=constraint["entityIds"].toArray();
            if(ids.size()!=2 || ids[0]==ids[1])reject("Tangent needs one curve and one line");
            QJsonObject arc,circle,line;QMap<QString,QJsonObject> points;
            for(const auto& value:parameters["entities"].toArray()){
                const auto entity=value.toObject();if(entity["id"]!=ids[0] && entity["id"]!=ids[1])continue;
                if(entity["type"]=="arc")arc=entity;else if(entity["type"]=="circle")circle=entity;
                else if(entity["type"]=="line")line=entity;
            }
            for(const auto& value:parameters["points"].toArray()){const auto point=value.toObject();points[point["id"].toString()]=point;}
            if(line.isEmpty() || (arc.isEmpty()==circle.isEmpty()))reject("Tangent needs one arc or circle and one straight line");
            if(arguments.contains("side"))reject("Tangent derives its side from the current solved geometry");
            const auto a=points.value(line["startPointId"].toString()),b=points.value(line["endPointId"].toString());
            const auto curve=arc.isEmpty() ? circle : arc;
            const auto center=points.value(curve["centerPointId"].toString());
            if(a.isEmpty() || b.isEmpty() || center.isEmpty())reject("Tangent geometry is unavailable");
            const double dx=b["uMm"].toDouble()-a["uMm"].toDouble(),dy=b["vMm"].toDouble()-a["vMm"].toDouble();
            const double length=std::hypot(dx,dy);
            if(length<minimumLength)reject("Tangent line is degenerate");
            const double cross=dx*(center["vMm"].toDouble()-a["vMm"].toDouble())
                -dy*(center["uMm"].toDouble()-a["uMm"].toDouble());
            if(std::abs(cross)<1e-9*std::max(1.0,length*curve["radiusMm"].toDouble()))
                reject("Tangent side is ambiguous");
            const auto pair=QJsonArray{curve["id"],line["id"]};
            for(const auto& value:parameters["constraints"].toArray()){
                const auto relation=value.toObject();
                if(relation["type"]==(arc.isEmpty() ? "tangent" : "arcTangent") && relation["entityIds"]==pair)
                    reject("Tangent relation already exists for this pair");
            }
            constraint["entityIds"]=pair;constraint["side"]=cross>0 ? 1 : -1;
            if(!arc.isEmpty()){
                QStringList shared;
                for(const auto& key:{"startPointId","endPointId"})for(const auto& other:{"startPointId","endPointId"})
                    if(arc[key]==line[other])shared.append(arc[key].toString());
                shared.removeDuplicates();
                if(shared.size()!=1)reject("Arc and line must share exactly one authored endpoint");
                constraint["type"]="arcTangent";constraint["pointIds"]=QJsonArray{shared[0]};
            }
        }
        if(constraint["type"]=="symmetry"){
            const auto ids=constraint["entityIds"].toArray();
            if(ids.size()!=3 || ids[0]==ids[1] || ids[0]==ids[2] || ids[1]==ids[2])reject("Symmetry needs two distinct lines and one construction axis");
            QMap<QString,QJsonObject> lines,points;
            for(const auto& value:parameters["entities"].toArray()){
                const auto entity=value.toObject();if(entity["type"]=="line")lines[entity["id"].toString()]=entity;
            }
            for(const auto& value:parameters["points"].toArray()){const auto point=value.toObject();points[point["id"].toString()]=point;}
            for(const auto& id:ids)if(!lines.contains(id.toString()))reject("Symmetry supports three authored straight lines");
            const auto first=lines[ids[0].toString()],second=lines[ids[1].toString()],axis=lines[ids[2].toString()];
            if(!axis["construction"].toBool())reject("Symmetry axis must be a construction line");
            auto xy=[&](const QJsonValue& id){const auto point=points.value(id.toString());
                if(point.isEmpty())reject("Symmetry endpoint is unavailable");return QPointF(point["uMm"].toDouble(),point["vMm"].toDouble());};
            const auto axisStart=xy(axis["startPointId"]),axisEnd=xy(axis["endPointId"]),delta=axisEnd-axisStart;
            const double norm=QPointF::dotProduct(delta,delta);
            if(norm<1e-12)reject("Symmetry axis is degenerate");
            const auto reflect=[&](const QPointF& point){const auto foot=axisStart+delta*(QPointF::dotProduct(point-axisStart,delta)/norm);return foot*2-point;};
            const auto a=reflect(xy(first["startPointId"])),b=reflect(xy(first["endPointId"]));
            const auto c=xy(second["startPointId"]),d=xy(second["endPointId"]);
            const auto square=[](const QPointF& v){return QPointF::dotProduct(v,v);};
            const double forward=square(a-c)+square(b-d),reverse=square(a-d)+square(b-c);
            const double lineScale=square(xy(first["endPointId"])-xy(first["startPointId"]))
                +square(d-c);
            if(std::abs(forward-reverse)<=1e-8*std::max({1.0,lineScale,forward,reverse}))reject("Symmetry endpoint pairing is ambiguous");
            constraint["pointIds"]=forward<reverse
                ? QJsonArray{first["startPointId"],second["startPointId"],first["endPointId"],second["endPointId"]}
                : QJsonArray{first["startPointId"],second["endPointId"],first["endPointId"],second["startPointId"]};
        }
        if(arguments.contains("origin")){
            if(constraint["type"]!="midpoint" || arguments["origin"]!=true || arguments.contains("pointIds"))
                reject("Only Midpoint accepts the sketch origin as a point target");
            auto origin=parameters["originPointId"].toString();
            if(origin.isEmpty()){
                origin=newId();parameters["originPointId"]=origin;
                auto points=parameters["points"].toArray();points.append(QJsonObject{{"id",origin},{"uMm",0},{"vMm",0}});parameters["points"]=points;
                auto constraints=parameters["constraints"].toArray();constraints.append(QJsonObject{{"id",newId()},{"type","fixed"},
                    {"pointIds",QJsonArray{origin}},{"uMm",0},{"vMm",0}});parameters["constraints"]=constraints;
            }
            constraint.remove("origin");constraint["pointIds"]=QJsonArray{origin};
        }
        const auto derivedPoints=SketchArcSlot::derivedPointIds(parameters),derivedArcs=SketchArcSlot::derivedArcIds(parameters);
        for(const auto& value:constraint.value("pointIds").toArray())if(derivedPoints.contains(value.toString()))
            reject("Arc slot tangent contact is derived; constrain its path endpoint instead");
        for(const auto& value:constraint.value("entityIds").toArray())if(derivedArcs.contains(value.toString()))
            reject("Arc slot boundary is derived; constrain its path arc instead");
        if(constraint["type"]=="horizontalVertical") {
            const auto ids=constraint["pointIds"].toArray();
            if(ids.size()!=2)reject("Horizontal/Vertical requires two ordered points");
            QJsonObject a,b;for(const auto& value:parameters["points"].toArray()){const auto p=value.toObject();if(p["id"]==ids[0])a=p;if(p["id"]==ids[1])b=p;}
            if(a.isEmpty() || b.isEmpty())reject("Alignment point no longer exists");
            constraint["type"]=std::abs(a["uMm"].toDouble()-b["uMm"].toDouble())>=std::abs(a["vMm"].toDouble()-b["vMm"].toDouble()) ? "horizontal" : "vertical";
        }
        if (constraint["type"].toString() == "fixed") {
            const auto pointIds = constraint["pointIds"].toArray();
            if (pointIds.size() != 1) reject("Fixed constraint requires one point");
            bool found = false;
            for (const auto& entry : parameters["points"].toArray()) {
                const auto point = entry.toObject();
                if (point["id"] != pointIds[0]) continue;
                constraint["uMm"] = point["uMm"]; constraint["vMm"] = point["vMm"]; found = true; break;
            }
            if (!found) reject("Fixed point does not exist");
        }
        auto constraints = parameters["constraints"].toArray();
        if(constraint["type"]=="concentric" || constraint["type"]=="collinear")for(const auto& value:constraints) {
            const auto prior=value.toObject();if(prior["type"]!=constraint["type"])continue;
            const auto a=prior["entityIds"].toArray(),b=constraint["entityIds"].toArray();
            if(a.size()==2 && b.size()==2 && ((a[0]==b[0] && a[1]==b[1]) || (a[0]==b[1] && a[1]==b[0])))
                reject(constraint["type"]=="collinear" ? "Collinear relation already exists for this pair" : "Concentric relation already exists for this pair");
        }
        constraints.append(constraint);
        parameters["constraints"] = constraints;
        if((constraint["type"]=="horizontal" || constraint["type"]=="vertical") && constraint["entityIds"].toArray().size()==1) {
            const auto guideId=constraint["entityIds"].toArray()[0].toString();
            for(const auto& value:parameters.value("slotGroups").toArray()) {
                const auto group=value.toObject();if(group["centerlineId"]!=guideId)continue;
                auto entities=parameters["entities"].toArray(),points=parameters["points"].toArray();
                QJsonObject guide;for(const auto& entry:entities)if(entry.toObject()["id"]==guideId){guide=entry.toObject();break;}
                QJsonObject center,start,end;for(const auto& entry:points){const auto p=entry.toObject();
                    if(p["id"]==group["centerPointId"])center=p;
                    if(p["id"]==guide["startPointId"])start=p;
                    if(p["id"]==guide["endPointId"])end=p;
                }
                if(center.isEmpty() || start.isEmpty() || end.isEmpty())reject("Slot guide points are unavailable");
                const double dx=end["uMm"].toDouble()-start["uMm"].toDouble(),dy=end["vMm"].toDouble()-start["vMm"].toDouble();
                const double angle=std::atan2(dy,dx),target=constraint["type"]=="horizontal" ?
                    (dx>=0 ? 0.0 : std::numbers::pi) : (dy>=0 ? std::numbers::pi/2 : -std::numbers::pi/2);
                const double rotation=std::remainder(target-angle,2*std::numbers::pi),cs=std::cos(rotation),sn=std::sin(rotation);
                QSet<QString> owned{group["centerPointId"].toString(),guide["startPointId"].toString(),guide["endPointId"].toString()};
                for(const auto& id:group["sideIds"].toArray())for(const auto& entry:entities)if(entry.toObject()["id"]==id){
                    owned.insert(entry.toObject()["startPointId"].toString());owned.insert(entry.toObject()["endPointId"].toString());}
                const double cx=center["uMm"].toDouble(),cy=center["vMm"].toDouble();
                for(int i=0;i<points.size();++i){auto p=points[i].toObject();if(!owned.contains(p["id"].toString()))continue;
                    const double x=p["uMm"].toDouble()-cx,y=p["vMm"].toDouble()-cy;
                    p["uMm"]=cx+cs*x-sn*y;p["vMm"]=cy+sn*x+cs*y;points[i]=p;}
                for(int i=0;i<entities.size();++i){auto e=entities[i].toObject();if(!group["arcIds"].toArray().contains(e["id"]))continue;
                    e["startAngleDeg"]=std::remainder(e["startAngleDeg"].toDouble()+rotation/degrees,360.0);entities[i]=e;}
                parameters["points"]=points;parameters["entities"]=entities;break;
            }
        }
        auto candidate = current; candidate["parameters"] = parameters;
        const auto pointPair=(constraint["type"]=="horizontal" || constraint["type"]=="vertical") && constraint.contains("pointIds");
        if(pointPair && constraint["pointIds"].toArray().size()!=2)reject("Point alignment requires two ordered points");
        QString first=pointPair ? constraint["pointIds"].toArray().at(0).toString() : QString{};
        if(constraint["type"]=="concentric" && constraint["entityIds"].toArray().size()==2) {
            const auto source=constraint["entityIds"].toArray().at(0);
            for(const auto& value:parameters["entities"].toArray())if(value.toObject()["id"]==source)first=value.toObject()["centerPointId"].toString();
        }
        auto result=solveDraft(candidate,projectionContext,first);
        if(!result.ok && !first.isEmpty())result=solveDraft(candidate,projectionContext);
        if(result.ok && !first.isEmpty())result=solveDraft(result.feature,projectionContext);
        return result.ok ? result : SolveResult{false, draft, result.diagnostics, result.error};
    } catch (const std::exception& error) { return invalidDraft(draft, QString::fromUtf8(error.what())); }
}

SketchGeometry::SolveResult SketchGeometry::addDimension(const QJsonObject& draft, const QJsonObject& arguments,
    const QJsonObject& projectionContext,const QSet<QString>& userLengthNames) {
    try {
        validateDraft(draft);
        checkFields(arguments, {"kind", "pointIds", "entityId", "entityIds", "raySigns", "driving", "valueMm", "valueDeg", "annotation", "parameterId", "expression", "expressionUnits"});
        const bool formula=arguments.contains("expression");
        if(formula && (arguments.contains("valueMm") || arguments.contains("valueDeg") || arguments.contains("parameterId")))
            reject("Choose a local expression or a numeric/user-parameter dimension");
        if(formula && (!arguments["expression"].isString() || arguments["expression"].toString().trimmed().isEmpty() ||
            !arguments["expressionUnits"].isString()))reject("Local dimension expression and entry units are required");
        if (arguments.contains("driving") && !arguments["driving"].isBool()) reject("Dimension driving must be Boolean");
        auto parameters = draft["parameters"].toObject();
        QJsonArray pointIds = arguments["pointIds"].toArray();
        const auto kind=arguments["kind"].toString();
        const auto derivedPoints=SketchArcSlot::derivedPointIds(parameters),derivedArcs=SketchArcSlot::derivedArcIds(parameters);
        for(const auto& value:pointIds)if(derivedPoints.contains(value.toString()))
            reject("Arc slot tangent contact is derived; dimension its path instead");
        const auto entityId=arguments["entityId"].toString();
        if(derivedArcs.contains(entityId) && !(SketchArcSlot::capIds(parameters).contains(entityId) &&
            (kind=="radius" || kind=="diameter")))
            reject("Arc slot side is derived; dimension the path or rounded end radius instead");
        const bool curve=kind=="radius" || kind=="diameter" || kind=="arcSweep";
        const bool angle=kind=="angle";
        if ((angle || kind=="centerlineDiameter" || kind=="parallelLineDistance") && (arguments.contains("pointIds") || arguments.contains("entityId"))) reject("Line-pair dimensions require two line entityIds");
        if (!angle && kind!="centerlineDiameter" && kind!="parallelLineDistance" && (arguments.contains("entityIds") || arguments.contains("raySigns"))) reject("Only line-pair dimensions accept entityIds; raySigns are for angles");
        if (arguments.contains("entityId") && !curve && kind!="pointLineDistance") {
            if (arguments.contains("pointIds")) reject("Choose dimension points or one line, not both");
            bool found = false;
            for (const auto& entry : expandedProjectionFeature(draft,projectionContext)["parameters"].toObject()["entities"].toArray()) {
                const auto entity = entry.toObject();
                if (entity["id"] != arguments["entityId"]) continue;
                pointIds = {entity["startPointId"], entity["endPointId"]}; found = true; break;
            }
            if (!found) reject("Dimension line does not exist");
        }
        const bool driving = arguments.value("driving").toBool(true);
        QJsonObject dimension{{"id", newId()}, {"kind", arguments["kind"]}, {"pointIds", pointIds}, {"driving", driving}};
        if(arguments.contains("parameterId")) {
            if(!driving || angle || kind=="arcSweep" || !validId(arguments["parameterId"].toString()))reject("Only driving length dimensions accept a stable parameter ID");
            dimension["parameterId"]=arguments["parameterId"];
        }
        if(curve && arguments.contains("pointIds"))reject("Curve dimensions require one entity reference");
        if(curve) {dimension.remove("pointIds"); dimension["entityId"]=arguments["entityId"];}
        if(angle) {dimension.remove("pointIds");dimension["entityIds"]=arguments["entityIds"];
            if(arguments.contains("raySigns"))dimension["raySigns"]=arguments["raySigns"];}
        if(kind=="centerlineDiameter") {
            if(arguments.contains("raySigns"))reject("Centerline diameter does not accept angle ray signs");
            const auto pair=centerlinePair(expandedProjectionFeature(draft,projectionContext)["parameters"].toObject(),arguments["entityIds"].toArray());
            dimension.remove("pointIds");dimension["entityIds"]=pair["entityIds"];dimension["side"]=pair["signedOffsetMm"].toDouble()>0 ? 1 : -1;
        }
        if(kind=="parallelLineDistance") {
            if(arguments.contains("raySigns"))reject("Parallel distance does not accept angle ray signs");
            const auto pair=parallelLinePair(expandedProjectionFeature(draft,projectionContext)["parameters"].toObject(),arguments["entityIds"].toArray());
            dimension.remove("pointIds");dimension["entityIds"]=pair["entityIds"];dimension["side"]=pair["signedOffsetMm"].toDouble()>0 ? 1 : -1;
        }
        if(kind=="pointLineDistance") {
            if(pointIds.size()!=1 || !arguments["entityId"].isString())reject("Point-edge dimension requires one point and one line");
            const auto expanded=expandedProjectionFeature(draft,projectionContext)["parameters"].toObject();QMap<QString,QPointF> points;for(const auto& value:expanded["points"].toArray()){const auto p=value.toObject();points[p["id"].toString()]={p["uMm"].toDouble(),p["vMm"].toDouble()};}
            if(!points.contains(pointIds[0].toString()))reject("Point-edge point unavailable");QJsonObject line;for(const auto& value:expanded["entities"].toArray())if(value.toObject()["id"]==arguments["entityId"])line=value.toObject();
            if(line["type"]!="line")reject("Point-edge dimensions support straight lines only");const auto a=points.value(line["startPointId"].toString()),b=points.value(line["endPointId"].toString()),q=points.value(pointIds[0].toString());const auto d=b-a;const double length=std::hypot(d.x(),d.y());if(length<minimumLength)reject("Point-edge dimension line is degenerate");const double signedDistance=(d.x()*(q.y()-a.y())-d.y()*(q.x()-a.x()))/length;if(std::abs(signedDistance)<minimumLength)reject("Use Coincident for a point on the line");
            dimension["entityId"]=arguments["entityId"];dimension["side"]=signedDistance>0 ? 1 : -1;
        }
        const auto valueKey=kind=="arcSweep" || angle ? "valueDeg" : "valueMm";
        if (arguments.contains("annotation")) dimension["annotation"] = arguments["annotation"];
        if (driving) dimension[valueKey] = formula ? QJsonValue(1.0) : arguments[valueKey];
        else if (arguments.contains("valueMm") || arguments.contains("valueDeg")) reject("Driven dimensions measure geometry; omit its value");
        if (arguments.contains(kind=="arcSweep" || angle ? "valueMm" : "valueDeg")) reject("Dimension value has incorrect units");
        auto dimensions = parameters["dimensions"].toArray();
        if(kind=="parallelLineDistance")for(const auto& existing:dimensions){const auto row=existing.toObject();const auto prior=row["entityIds"].toArray(),chosen=dimension["entityIds"].toArray();if(row["kind"]==kind && (prior==chosen || (prior.size()==2 && prior[0]==chosen[1] && prior[1]==chosen[0])))reject("These lines already have a distance dimension; edit its value");}
        dimensions.append(dimension);
        parameters["dimensions"] = dimensions;
        if(formula){
            if(!parameters.contains("nextDimensionNameIndex"))parameters["nextDimensionNameIndex"]=1;
            parameters=allocateDimensionNames(parameters);dimensions=parameters["dimensions"].toArray();
            dimension=bindLocalExpression(dimensions.last().toObject(),dimensions,
                arguments["expression"].toString(),arguments["expressionUnits"].toString(),userLengthNames);
            dimensions[dimensions.size()-1]=dimension;parameters["dimensions"]=dimensions;
        }
        auto candidate = draft; candidate["parameters"] = parameters;
        const auto result = solveDraft(candidate,projectionContext);
        return result.ok ? result : SolveResult{false, draft, result.diagnostics, result.error};
    } catch (const std::exception& error) { return invalidDraft(draft, QString::fromUtf8(error.what())); }
}

SketchGeometry::SolveResult SketchGeometry::filletCorners(const QJsonObject& draft, const QJsonObject& arguments, const QJsonObject& projectionContext) {
    try {
        validateDraft(draft); checkFields(arguments,{"corners","radiusMm"});
        const double radius=coordinate(arguments,"radiusMm");
        if(radius<minimumLength || !arguments["corners"].isArray() || arguments["corners"].toArray().isEmpty())
            reject("Fillet requires corners and a positive radius");
        auto parameters=draft["parameters"].toObject(); auto entities=parameters["entities"].toArray();
        auto points=parameters["points"].toArray(); auto constraints=parameters["constraints"].toArray();
        auto dimensions=parameters["dimensions"].toArray();
        // This narrow repair applies only to the four-line typed rectangle and
        // its two overall point-pair dimensions as corners are rounded in turn.
        int boundaryLines=0,existingArcs=0,overallHorizontal=0,overallVertical=0;
        bool rectangleOutline=true;
        for(const auto& value:entities){const auto e=value.toObject();
            if(e["type"]=="line" && !e.value("construction").toBool())++boundaryLines;
            else if(e["type"]=="arc")++existingArcs;
            else rectangleOutline=false;
        }
        for(const auto& value:dimensions){const auto d=value.toObject();
            if(d["kind"]=="horizontal" && d["pointIds"].toArray().size()==2 && !d.contains("entityId"))++overallHorizontal;
            if(d["kind"]=="vertical" && d["pointIds"].toArray().size()==2 && !d.contains("entityId"))++overallVertical;
        }
        rectangleOutline &= boundaryLines==4 && existingArcs<=3 && overallHorizontal==1 && overallVertical==1;
        QMap<QString,QJsonObject> pointMap; QMap<QString,QString> roots;
        for(const auto& value:points){const auto p=value.toObject();pointMap[p["id"].toString()]=p;roots[p["id"].toString()]=p["id"].toString();}
        auto root=[&](QString id){while(roots[id]!=id)id=roots[id];return id;};
        for(const auto& value:constraints){const auto c=value.toObject();if(c["type"]!="coincident")continue;
            const auto refs=c["pointIds"].toArray();roots[root(refs[1].toString())]=root(refs[0].toString());}
        const auto position=[&](const QString& id){const auto p=pointMap[id];return QPointF(p["uMm"].toDouble(),p["vMm"].toDouble());};
        bool drivingRectangle=rectangleOutline;
        QJsonArray sideIds,sideDirections;
        QMap<QString,int> degree;
        QMap<QString,QSet<QString>> adjacency;
        QSet<int> axes;
        double minU=maximumLength,maxU=-maximumLength,minV=maximumLength,maxV=-maximumLength;
        for(const auto& value:entities){const auto e=value.toObject();
            if(e["type"]!="line" && e["type"]!="arc")continue;
            const auto a=position(e["startPointId"].toString()),b=position(e["endPointId"].toString());
            const auto start=root(e["startPointId"].toString()),end=root(e["endPointId"].toString());
            ++degree[start];++degree[end];adjacency[start].insert(end);adjacency[end].insert(start);
            if(e["type"]=="arc") {if(std::abs(std::abs(e["sweepAngleDeg"].toDouble())-90)>1e-4)drivingRectangle=false;continue;}
            const bool horizontal=std::abs(a.y()-b.y())<1e-6 && std::abs(a.x()-b.x())>minimumLength;
            const bool vertical=std::abs(a.x()-b.x())<1e-6 && std::abs(a.y()-b.y())>minimumLength;
            if(horizontal==vertical)drivingRectangle=false;
            axes.insert(horizontal ? 0 : 1);
            sideIds.append(e["id"]);sideDirections.append(horizontal ? (b.x()>a.x() ? 1 : -1) : (b.y()>a.y() ? 1 : -1));
            minU=std::min({minU,a.x(),b.x()});maxU=std::max({maxU,a.x(),b.x()});
            minV=std::min({minV,a.y(),b.y()});maxV=std::max({maxV,a.y(),b.y()});
        }
        for(const auto count:degree)if(count!=2)drivingRectangle=false;
        if(!degree.isEmpty()){
            QSet<QString> reached;QStringList pending{degree.firstKey()};
            while(!pending.isEmpty()){const auto node=pending.takeLast();if(reached.contains(node))continue;
                reached.insert(node);for(const auto& next:adjacency[node])pending.append(next);}
            if(reached.size()!=degree.size())drivingRectangle=false;
        }
        drivingRectangle &= axes.size()==2 && sideIds.size()==4 && minU<maxU-minimumLength && minV<maxV-minimumLength;
        const auto originId=parameters.value("originPointId").toString();
        bool originFixed=false;
        for(const auto& value:constraints){const auto c=value.toObject();if(c["type"]=="fixed" && c["pointIds"].toArray()==QJsonArray{originId}
            && std::abs(c["uMm"].toDouble())<1e-7 && std::abs(c["vMm"].toDouble())<1e-7)originFixed=true;}
        drivingRectangle &= originFixed && std::abs(minU)<1e-6 && std::abs(maxV)<1e-6;
        QString widthId,heightId;int horizontalSides=0,verticalSides=0;
        for(const auto& value:entities){const auto e=value.toObject();if(e["type"]!="line")continue;
            const auto a=position(e["startPointId"].toString()),b=position(e["endPointId"].toString());
            if(std::abs(a.y()-b.y())<1e-6){++horizontalSides;if(std::abs(a.y()-minV)>1e-6 && std::abs(a.y()-maxV)>1e-6)drivingRectangle=false;}
            else {++verticalSides;if(std::abs(a.x()-minU)>1e-6 && std::abs(a.x()-maxU)>1e-6)drivingRectangle=false;}
        }
        drivingRectangle &= horizontalSides==2 && verticalSides==2;
        for(const auto& value:dimensions){const auto d=value.toObject();
            if(d["kind"]!="horizontal" && d["kind"]!="vertical")continue;
            if(d.contains("entityId") || !d["driving"].toBool() || !d.contains("valueMm") || d["pointIds"].toArray().size()!=2)continue;
            const auto refs=d["pointIds"].toArray();const auto a=position(refs[0].toString()),b=position(refs[1].toString());
            if(d["kind"]=="horizontal" && std::abs(std::abs(a.x()-b.x())-(maxU-minU))<1e-5
                && std::abs(d["valueMm"].toDouble()-(maxU-minU))<1e-5)widthId=d["id"].toString();
            if(d["kind"]=="vertical" && std::abs(std::abs(a.y()-b.y())-(maxV-minV))<1e-5
                && std::abs(d["valueMm"].toDouble()-(maxV-minV))<1e-5)heightId=d["id"].toString();
        }
        drivingRectangle &= !widthId.isEmpty() && !heightId.isEmpty();
        if(parameters.contains("roundedRectangleIntent"))drivingRectangle &= parameters["roundedRectangleIntent"].toObject()["sideIds"].toArray()==sideIds;
        QSet<QString> touchedLines,retired,removedLines,discardedFresh;
        QJsonArray arcs,newRelations,removedDimensions,removedConstraints,convertedDimensions;
        const auto originalEntities=entities;
        struct Trim {QString contact,outer;int arc;double setback;};
        QMap<QString,QMap<QString,Trim>> trims;
        QMap<QString,QJsonArray> spanEndpoints;
        QMap<QString,QMap<QString,QString>> overallContacts;
        QJsonArray originCornerLines;
        auto xy=[&](const QString& id){const auto p=pointMap[id];return QPointF(p["uMm"].toDouble(),p["vMm"].toDouble());};
        auto fresh=[&](const QPointF& position){const auto id=newId();const QJsonObject p{{"id",id},{"uMm",position.x()},{"vMm",position.y()}};points.append(p);pointMap[id]=p;return id;};
        for(const auto& value:arguments["corners"].toArray()) {
            if(!value.isObject())reject("Fillet corner must be an object");const auto corner=value.toObject();checkFields(corner,{"entityIds"});
            const auto refs=corner["entityIds"].toArray();if(!corner["entityIds"].isArray() || refs.size()!=2 || refs[0]==refs[1])reject("Fillet needs two distinct lines");
            int indices[2]={-1,-1};QJsonObject lines[2];
            for(int i=0;i<originalEntities.size();++i)for(int j=0;j<2;++j)if(originalEntities[i].toObject()["id"]==refs[j]){indices[j]=i;lines[j]=originalEntities[i].toObject();}
            for(int j=0;j<2;++j){if(indices[j]<0 || lines[j]["type"]!="line" || lines[j].value("construction").toBool())reject("Fillet supports normal lines only");
                const auto id=lines[j]["id"].toString();touchedLines.insert(id);}
            QString keys[2];int matches=0;
            for(const auto& a:{QString("startPointId"),QString("endPointId")})for(const auto& b:{QString("startPointId"),QString("endPointId")})
                if(root(lines[0][a].toString())==root(lines[1][b].toString())){keys[0]=a;keys[1]=b;++matches;}
            if(matches!=1)reject("Fillet requires one unambiguous connected corner");
            const auto cornerRoot=root(lines[0][keys[0]].toString());
            if(drivingRectangle && cornerRoot==root(originId))originCornerLines=refs;
            for(auto it=roots.begin();it!=roots.end();++it)if(root(it.key())==cornerRoot)retired.insert(it.key());
            const QPointF vertex=xy(lines[0][keys[0]].toString()); QPointF directions[2]; double lengths[2];
            for(int j=0;j<2;++j){const auto other=keys[j]=="startPointId" ? "endPointId" : "startPointId";
                directions[j]=xy(lines[j][other].toString())-vertex;lengths[j]=std::hypot(directions[j].x(),directions[j].y());directions[j]/=lengths[j];}
            const double dot=std::clamp(QPointF::dotProduct(directions[0],directions[1]),-1.0,1.0),angle=std::acos(dot);
            if(angle<1e-6 || std::numbers::pi-angle<1e-6)reject("Fillet corner lines must not be collinear");
            const double setback=radius/std::tan(angle/2);
            for(double length:lengths)if(setback>length+1e-7)reject("Fillet radius exceeds available line length");
            QPointF bisector=directions[0]+directions[1];bisector/=std::hypot(bisector.x(),bisector.y());
            const QPointF center=vertex+bisector*(radius/std::sin(angle/2));
            const QPointF contacts[2]={vertex+directions[0]*setback,vertex+directions[1]*setback};
            const auto centerId=fresh(center),firstId=fresh(contacts[0]),lastId=fresh(contacts[1]),arcId=newId();
            if(rectangleOutline)for(auto it=roots.begin();it!=roots.end();++it)if(root(it.key())==cornerRoot) {
                for(int j=0;j<2;++j){
                    if(std::abs(contacts[j].x()-vertex.x())<1e-7)overallContacts[it.key()]["horizontal"]=j==0 ? firstId : lastId;
                    if(std::abs(contacts[j].y()-vertex.y())<1e-7)overallContacts[it.key()]["vertical"]=j==0 ? firstId : lastId;
                }
            }
            const double start=std::atan2(contacts[0].y()-center.y(),contacts[0].x()-center.x())/degrees;
            const double sweep=std::remainder(std::atan2(contacts[1].y()-center.y(),contacts[1].x()-center.x())/degrees-start,360.0);
            arcs.append(QJsonObject{{"id",arcId},{"type","arc"},{"centerPointId",centerId},{"startPointId",firstId},{"endPointId",lastId},
                {"radiusMm",radius},{"startAngleDeg",start},{"sweepAngleDeg",sweep}});
            for(int j=0;j<2;++j){const auto contactId=j==0 ? firstId : lastId;
                auto& lineTrims=trims[lines[j]["id"].toString()];
                if(lineTrims.contains(keys[j]))reject("Fillet corners cannot reuse the same line endpoint");
                lineTrims[keys[j]]={contactId,j==0 ? lastId : firstId,int(arcs.size()-1),setback};
                lines[j][keys[j]]=contactId;
                const auto a=keys[j]=="startPointId" ? contacts[j] : xy(lines[j]["startPointId"].toString());
                const auto b=keys[j]=="endPointId" ? contacts[j] : xy(lines[j]["endPointId"].toString());
                const double cross=(b.x()-a.x())*(center.y()-a.y())-(b.y()-a.y())*(center.x()-a.x());
                newRelations.append(QJsonObject{{"id",newId()},{"type","arcTangent"},{"entityIds",QJsonArray{arcId,lines[j]["id"]}},
                    {"pointIds",QJsonArray{contactId}},{"side",cross>0 ? 1 : -1}});
            }
        }
        // Resolve both setbacks against the original segment before mutating it.
        for(int i=0;i<entities.size();++i) {
            auto line=entities[i].toObject();const auto id=line["id"].toString();if(!trims.contains(id))continue;
            const auto a=xy(line["startPointId"].toString()),b=xy(line["endPointId"].toString());
            const auto edits=trims[id];double consumed=0;for(const auto& edit:edits)consumed+=edit.setback;
            const double remainder=std::hypot((b-a).x(),(b-a).y())-consumed;
            if(remainder < -1e-7)reject("Paired fillet setbacks exceed the shared line length");
            if(remainder<=1e-7) {
                if(edits.size()!=2)reject("Exhausted fillet line requires two supported corner trims");
                const auto first=edits["startPointId"],last=edits["endPointId"];
                auto left=arcs[first.arc].toObject(),right=arcs[last.arc].toObject();
                const auto centerLeft=left["centerPointId"].toString(),centerRight=right["centerPointId"].toString();
                const auto delta=xy(centerLeft)-xy(centerRight),contactDelta=xy(first.contact)-xy(last.contact);
                if(std::hypot(contactDelta.x(),contactDelta.y())>1e-7)reject("Exhausted fillet contacts do not coincide");
                if(std::hypot(delta.x(),delta.y())>1e-7)reject("Exhausted fillet requires coincident equal-radius return centers");
                for(const auto& field:{"startPointId","endPointId"})if(right[field]==last.contact)right[field]=first.contact;
                right["centerPointId"]=centerLeft;arcs[last.arc]=right;
                discardedFresh.insert(last.contact);discardedFresh.insert(centerRight);
                removedLines.insert(id);spanEndpoints[id]=QJsonArray{first.outer,last.outer};
            } else {
                if(remainder<minimumLength)reject("Fillet residual line is below the supported minimum length");
                for(auto edit=edits.begin();edit!=edits.end();++edit)line[edit.key()]=edit->contact;
                entities[i]=line;
            }
        }
        QJsonArray survivors;for(const auto& value:entities)if(!removedLines.contains(value.toObject()["id"].toString()))survivors.append(value);entities=survivors;
        QJsonArray survivingRelations;for(const auto& value:newRelations)if(!removedLines.contains(value.toObject()["entityIds"].toArray()[1].toString()))survivingRelations.append(value);newRelations=survivingRelations;
        // Retiring a corner never silently drops unrelated authored references.
        for(const auto& value:entities){const auto e=value.toObject();for(const auto& field:{"startPointId","endPointId","centerPointId"})
            if(retired.contains(e[field].toString()))reject("Fillet corner is referenced by unrelated geometry");}
        QJsonArray keptConstraints;
        bool releasedOriginAnchor=false;
        QJsonArray originSupportLines;
        for(const auto& value:constraints){const auto c=value.toObject();bool references=false,allRetired=true;
            for(const auto& id:c["pointIds"].toArray()){references|=retired.contains(id.toString());allRetired&=retired.contains(id.toString());}
            bool removedEntity=false;for(const auto& id:c["entityIds"].toArray())removedEntity|=removedLines.contains(id.toString());
            if(rectangleOutline && c["type"]=="fixed" && c["pointIds"].toArray()==QJsonArray{parameters["originPointId"]}) {
                keptConstraints.append(c);continue;
            }
            if(removedEntity){if(c["type"]!="horizontal" && c["type"]!="vertical")reject("Exhausted fillet line has an unsupported entity constraint");removedConstraints.append(c["id"]);continue;}
            if(references){if(c["type"]!="coincident" || !allRetired)reject("Fillet corner has an unrelated point constraint");
                if(rectangleOutline && c["pointIds"].toArray().contains(parameters["originPointId"])) {
                    if(drivingRectangle){
                        for(const auto& id:originCornerLines)if(!originSupportLines.contains(id))originSupportLines.append(id);
                    } else releasedOriginAnchor=true;
                }
                removedConstraints.append(c["id"]);continue;}keptConstraints.append(c);}
        QJsonArray keptDimensions;
        for(const auto& value:dimensions){const auto d=value.toObject();bool pointReference=false;for(const auto& id:d["pointIds"].toArray())pointReference|=retired.contains(id.toString());
            const bool lengthDimension=d["kind"]=="linear" || d["kind"]=="horizontal" || d["kind"]=="vertical";
            bool affected=lengthDimension && touchedLines.contains(d["entityId"].toString());
            const auto refs=d["pointIds"].toArray();
            if(lengthDimension && refs.size()==2)for(const auto& original:parameters["entities"].toArray()) {
                const auto line=original.toObject();if(!touchedLines.contains(line["id"].toString()))continue;
                const auto start=root(line["startPointId"].toString()),end=root(line["endPointId"].toString());
                const auto a=root(refs[0].toString()),b=root(refs[1].toString());
                if((a==start && b==end) || (a==end && b==start))affected=true;
            }
            QString exhausted;
            if(removedLines.contains(d["entityId"].toString()))exhausted=d["entityId"].toString();
            if(exhausted.isEmpty() && lengthDimension && refs.size()==2)for(const auto& value:originalEntities){const auto line=value.toObject();if(!removedLines.contains(line["id"].toString()))continue;
                const auto a=root(refs[0].toString()),b=root(refs[1].toString()),start=root(line["startPointId"].toString()),end=root(line["endPointId"].toString());
                if((a==start && b==end)||(a==end && b==start))exhausted=line["id"].toString();}
            if(!exhausted.isEmpty()) {
                QJsonObject original;for(const auto& value:originalEntities)if(value.toObject()["id"]==exhausted)original=value.toObject();
                if(!lengthDimension || d["kind"]=="vertical" || std::abs(xy(original["startPointId"].toString()).y()-xy(original["endPointId"].toString()).y())>1e-7)
                    reject("Exhausted fillet dimension cannot be converted to a horizontal reference span");
                auto span=d;span.remove("entityId");span.remove("valueMm");span["kind"]="horizontal";span["pointIds"]=spanEndpoints[exhausted];span["driving"]=false;
                keptDimensions.append(span);convertedDimensions.append(d["id"]);
            } else if(rectangleOutline && lengthDimension && (d["kind"]=="horizontal" || d["kind"]=="vertical")
                    && !d.contains("entityId") && pointReference) {
                auto span=d;auto mapped=refs;
                for(int i=0;i<mapped.size();++i)if(retired.contains(mapped[i].toString())) {
                    const auto replacement=overallContacts[mapped[i].toString()][d["kind"].toString()];
                    if(replacement.isEmpty())reject("Rounded rectangle overall span has no surviving outer tangent endpoint");
                    mapped[i]=replacement;
                }
                if(mapped[0]==mapped[1])reject("Rounded rectangle overall span endpoints coincide");
                span["pointIds"]=mapped;
                if(!drivingRectangle){span.remove("valueMm");span["driving"]=false;convertedDimensions.append(d["id"]);}
                keptDimensions.append(span);
            } else if(affected)removedDimensions.append(d["id"]);
            else {if(pointReference)reject("Fillet corner has an unrelated point dimension");keptDimensions.append(d);}}
        QJsonArray keptPoints;for(const auto& value:points)if((!retired.contains(value.toObject()["id"].toString()) ||
                (rectangleOutline && value.toObject()["id"]==parameters["originPointId"]))
                && !discardedFresh.contains(value.toObject()["id"].toString()))keptPoints.append(value);
        for(const auto& value:arcs)entities.append(value);for(const auto& value:newRelations)keptConstraints.append(value);
        if(drivingRectangle && originSupportLines.size()==2)for(const auto& lineId:originSupportLines)
            keptConstraints.append(QJsonObject{{"id",newId()},{"type","supportLine"},{"pointIds",QJsonArray{originId}},{"entityIds",QJsonArray{lineId}}});
        const auto firstArc=arcs.first().toObject()["id"].toString();
        keptDimensions.append(QJsonObject{{"id",newId()},{"kind","radius"},{"entityId",firstArc},{"driving",true},{"valueMm",radius}});
        for(int i=1;i<arcs.size();++i)if(arcs[i].toObject()["centerPointId"]!=arcs.first().toObject()["centerPointId"])keptConstraints.append(QJsonObject{{"id",newId()},{"type","equal"},{"entityIds",QJsonArray{firstArc,arcs[i].toObject()["id"]}}});
        parameters["points"]=keptPoints;parameters["entities"]=entities;parameters["constraints"]=keptConstraints;parameters["dimensions"]=keptDimensions;
        if(drivingRectangle){
            auto intent=parameters["roundedRectangleIntent"].toObject();
            if(intent.isEmpty())intent=QJsonObject{{"sideIds",sideIds},{"sideDirections",sideDirections},
                {"widthId",widthId},{"heightId",heightId},{"originPointId",originId}};
            auto cornerIds=intent["cornerArcIds"].toArray();
            for(const auto& value:arcs)cornerIds.append(value.toObject()["id"]);
            intent["cornerArcIds"]=cornerIds;parameters["roundedRectangleIntent"]=intent;
        }
        auto candidate=draft;candidate["parameters"]=parameters;auto result=solveDraft(candidate,projectionContext);
        if(!result.ok)return {false,draft,result.diagnostics,result.error};
        QJsonArray arcIds;for(const auto& value:arcs)arcIds.append(value.toObject()["id"]);
        result.diagnostics["createdArcIds"]=arcIds;result.diagnostics["removedDimensionIds"]=removedDimensions;result.diagnostics["removedConstraintIds"]=removedConstraints;result.diagnostics["convertedReferenceDimensionIds"]=convertedDimensions;result.diagnostics["releasedOriginCoincidence"]=releasedOriginAnchor;
        result.diagnostics["retainedDrivingDimensionIds"]=drivingRectangle ? QJsonArray{widthId,heightId} : QJsonArray{};
        result.diagnostics["originSupportLineIds"]=originSupportLines;
        QJsonArray retiredLines;for(const auto& id:removedLines)retiredLines.append(id);result.diagnostics["removedEntityIds"]=retiredLines;return result;
    }catch(const std::exception& error){return invalidDraft(draft,QString::fromUtf8(error.what()));}
}

SketchGeometry::SolveResult SketchGeometry::addPointerCircle(const QJsonObject& draft, const QJsonObject& arguments, const QJsonObject& projectionContext) {
    try {
        validateDraft(draft);
        checkFields(arguments, {"centerUMm", "centerVMm", "diameterMm", "centerSnapTarget", "tangentSnapTarget", "drivingDiameterMm"});
        double u = coordinate(arguments,"centerUMm"), v = coordinate(arguments,"centerVMm");
        double radius = coordinate(arguments,"diameterMm") / 2;
        if(arguments.contains("drivingDiameterMm")) {
            const double diameter=coordinate(arguments,"drivingDiameterMm");
            if(diameter<2*minimumLength)reject("Driving circle diameter must be at least 0.002 mm");
            radius=diameter/2;
        }
        if (radius < minimumLength || radius > maximumLength) reject("Circle radius out of supported range");
        auto parameters = draft["parameters"].toObject();
        auto points = parameters["points"].toArray(), entities = parameters["entities"].toArray();
        auto constraints = parameters.value("constraints").toArray();
        QString midpointLine,coincidentPoint;
        if (arguments.contains("centerSnapTarget")) {
            if (!arguments["centerSnapTarget"].isObject()) reject("Circle snap target must be an object");
            const auto target = arguments["centerSnapTarget"].toObject();
            if(target["kind"]=="origin") {
                if(target.size()!=1)reject("Origin snap target only accepts kind");
                coincidentPoint=parameters["originPointId"].toString();
                if(coincidentPoint.isEmpty()) {
                    coincidentPoint=newId();parameters["originPointId"]=coincidentPoint;
                    points.append(QJsonObject{{"id",coincidentPoint},{"uMm",0},{"vMm",0}});
                    constraints.append(QJsonObject{{"id",newId()},{"type","fixed"},{"pointIds",QJsonArray{coincidentPoint}},{"uMm",0},{"vMm",0}});
                }
                u=0;v=0;
            } else if(target["kind"]=="point") {
                if(target.size()!=2 || !target["pointId"].isString() || !validId(target["pointId"].toString()))reject("Circle center point snap requires a stable pointId");
                retainSupportPoint(parameters,projectionContext,target["pointId"].toString());
                for(const auto& value:expandedProjectionFeature(QJsonObject{{"parameters",parameters}},projectionContext)["parameters"].toObject()["points"].toArray()) {
                    const auto point=value.toObject();if(point["id"]!=target["pointId"])continue;
                    coincidentPoint=point["id"].toString();u=point["uMm"].toDouble();v=point["vMm"].toDouble();break;
                }
                if(coincidentPoint.isEmpty())reject("Circle center snap point no longer exists");
            } else {
            checkFields(target, {"kind","entityId"});
            if (target["kind"] != "midpoint" || !target["entityId"].isString() || !validId(target["entityId"].toString()))
                reject("Circle center currently supports a line midpoint target");
            for (const auto& entry : entities) {
                const auto line = entry.toObject();
                if (line["id"] != target["entityId"]) continue;
                if (line["type"] != "line") reject("Midpoint target must be a line");
                midpointLine = line["id"].toString();
                QJsonObject first,last;
                for (const auto& value : points) {
                    if (value.toObject()["id"] == line["startPointId"]) first = value.toObject();
                    if (value.toObject()["id"] == line["endPointId"]) last = value.toObject();
                }
                u = (first["uMm"].toDouble()+last["uMm"].toDouble())/2;
                v = (first["vMm"].toDouble()+last["vMm"].toDouble())/2;
                break;
            }
            if (midpointLine.isEmpty()) reject("Circle midpoint target no longer exists");
            }
        }
        QString tangentLine; int tangentSide = 0;
        if (arguments.contains("tangentSnapTarget")) {
            if (!arguments["tangentSnapTarget"].isObject()) reject("Tangent target must be an object");
            const auto target = arguments["tangentSnapTarget"].toObject();
            checkFields(target,{"kind","entityId"});
            if (target["kind"] != "tangent" || !target["entityId"].isString() || !validId(target["entityId"].toString()))
                reject("Circle tangent target requires a line UUID");
            for (const auto& entry : entities) {
                const auto line = entry.toObject(); if (line["id"] != target["entityId"]) continue;
                if (line["type"] != "line") reject("Circle tangent target must be a line");
                QJsonObject first,last;
                for (const auto& value : points) {
                    if (value.toObject()["id"] == line["startPointId"]) first = value.toObject();
                    if (value.toObject()["id"] == line["endPointId"]) last = value.toObject();
                }
                const double dx=last["uMm"].toDouble()-first["uMm"].toDouble(), dy=last["vMm"].toDouble()-first["vMm"].toDouble();
                const double px=u-first["uMm"].toDouble(), py=v-first["vMm"].toDouble(), length=std::hypot(dx,dy);
                const double foot=(px*dx+py*dy)/(length*length);
                if (foot < -endpointTolerance/length || foot > 1+endpointTolerance/length)
                    reject("Tangent contact is outside the finite line");
                const double distance=(dx*py-dy*px)/length;
                radius=std::abs(distance);
                if (radius < minimumLength || radius > maximumLength) reject("Tangent circle radius out of supported range");
                tangentSide=distance>0 ? 1 : -1; tangentLine=line["id"].toString(); break;
            }
            if (tangentLine.isEmpty()) reject("Circle tangent target no longer exists");
        }
        const auto centerId = newId();
        points.append(QJsonObject{{"id",centerId},{"uMm",u},{"vMm",v}});
        const auto circleId = newId();
        entities.append(QJsonObject{{"id",circleId},{"type","circle"},{"centerPointId",centerId},{"radiusMm",radius}});
        if (!tangentLine.isEmpty()) constraints.append(QJsonObject{{"id",newId()},{"type","tangent"},
            {"entityIds",QJsonArray{circleId,tangentLine}},{"side",tangentSide}});
        if(!coincidentPoint.isEmpty())constraints.append(QJsonObject{{"id",newId()},{"type","coincident"},{"pointIds",QJsonArray{centerId,coincidentPoint}}});
        if (!midpointLine.isEmpty()) constraints.append(QJsonObject{{"id",newId()},{"type","midpoint"},
            {"pointIds",QJsonArray{centerId}},{"entityIds",QJsonArray{midpointLine}}});
        QString diameterDimensionId;
        if(arguments.contains("drivingDiameterMm")) {
            diameterDimensionId=newId();auto dimensions=parameters["dimensions"].toArray();
            dimensions.append(QJsonObject{{"id",diameterDimensionId},{"kind","diameter"},{"entityId",circleId},{"driving",true},{"valueMm",arguments["drivingDiameterMm"]}});
            parameters["dimensions"]=dimensions;
        }
        parameters["points"] = points; parameters["entities"] = entities;
        if (!midpointLine.isEmpty() || !tangentLine.isEmpty() || !coincidentPoint.isEmpty()) parameters["constraints"] = constraints;
        auto candidate = draft; candidate["parameters"] = parameters;
        auto result = solveDraft(candidate,projectionContext);
        if(!result.ok)return {false,draft,result.diagnostics,result.error};
        result.diagnostics["createdCircleId"]=circleId;
        if(!diameterDimensionId.isEmpty())result.diagnostics["createdDimensionId"]=diameterDimensionId;
        return result;
    } catch (const std::exception& error) { return invalidDraft(draft,QString::fromUtf8(error.what())); }
}

SketchGeometry::SolveResult SketchGeometry::addTwoPointCircle(const QJsonObject& draft, const QJsonObject& arguments, const QJsonObject& projectionContext) {
    try {
        validateDraft(draft);
        checkFields(arguments,{"startUMm","startVMm","directionU","directionV","diameterMm"});
        const double u=coordinate(arguments,"startUMm"),v=coordinate(arguments,"startVMm");
        const double du=coordinate(arguments,"directionU"),dv=coordinate(arguments,"directionV");
        const double length=std::hypot(du,dv),diameter=coordinate(arguments,"diameterMm");
        if(length<1e-9 || diameter<2*minimumLength || diameter>2*maximumLength)
            reject("Two-point circle needs a direction and diameter from 0.002 to 200000 mm");
        const double unitU=du/length,unitV=dv/length,radius=diameter/2;
        auto parameters=draft["parameters"].toObject();
        auto points=parameters["points"].toArray(),entities=parameters["entities"].toArray(),dimensions=parameters["dimensions"].toArray();
        const auto startId=newId(),centerId=newId(),circleId=newId(),dimensionId=newId();
        points.append(QJsonObject{{"id",startId},{"uMm",u},{"vMm",v}});
        points.append(QJsonObject{{"id",centerId},{"uMm",u+unitU*radius},{"vMm",v+unitV*radius}});
        entities.append(QJsonObject{{"id",circleId},{"type","circle"},{"centerPointId",centerId},{"radiusMm",radius},
            {"diameterStartPointId",startId},{"diameterDirectionU",unitU},{"diameterDirectionV",unitV}});
        dimensions.append(QJsonObject{{"id",dimensionId},{"kind","diameter"},{"entityId",circleId},{"driving",true},{"valueMm",diameter}});
        parameters["points"]=points;parameters["entities"]=entities;parameters["dimensions"]=dimensions;
        auto candidate=draft;candidate["parameters"]=parameters;
        auto result=solveDraft(candidate,projectionContext);
        if(!result.ok)return {false,draft,result.diagnostics,result.error};
        result.diagnostics["createdCircleId"]=circleId;
        result.diagnostics["createdDimensionId"]=dimensionId;
        result.diagnostics["diameterStartPointId"]=startId;
        return result;
    } catch(const std::exception& error) {return invalidDraft(draft,QString::fromUtf8(error.what()));}
}

SketchGeometry::SolveResult SketchGeometry::setCenterline(const QJsonObject& draft, const QJsonObject& arguments, const QJsonObject& projectionContext) {
    try {
        validateDraft(draft);
        checkFields(arguments, {"entityIds", "centerline"});
        if (!arguments["centerline"].isBool()) reject("Centerline requires an explicit boolean");
        if (!arguments["entityIds"].isArray() || arguments["entityIds"].toArray().isEmpty())
            reject("Select at least one line for centerline conversion");
        QSet<QString> selected;
        for (const auto& value : arguments["entityIds"].toArray()) {
            if (!value.isString() || QUuid(value.toString()).isNull() || selected.contains(value.toString()))
                reject("Centerline selection requires distinct line UUIDs");
            selected.insert(value.toString());
        }
        auto parameters = draft["parameters"].toObject();
        auto entities = parameters["entities"].toArray();
        auto remaining = selected;
        bool changed = false;
        for (int i = 0; i < entities.size(); ++i) {
            auto entity = entities[i].toObject();
            if (!selected.contains(entity["id"].toString())) continue;
            if (entity["type"] != "line") reject("Centerline conversion currently supports lines only");
            remaining.remove(entity["id"].toString());
            if (entity.value("centerline").toBool() == arguments["centerline"].toBool()) continue;
            if (arguments["centerline"].toBool()) { entity["centerline"] = true; entity.remove("construction"); }
            else entity.remove("centerline");
            entities[i] = entity; changed = true;
        }
        if (!remaining.isEmpty()) reject("Centerline selection contains a missing entity");
        if (!changed) {
            auto result = solveDraft(draft,projectionContext);
            result.feature = draft;
            return result;
        }
        parameters["entities"] = entities;
        auto candidate = draft; candidate["parameters"] = parameters;
        const auto result = solveDraft(candidate,projectionContext);
        return result.ok ? result : SolveResult{false, draft, result.diagnostics, result.error};
    } catch (const std::exception& error) { return invalidDraft(draft, QString::fromUtf8(error.what())); }
}

SketchGeometry::SolveResult SketchGeometry::setConstruction(const QJsonObject& draft, const QJsonObject& arguments, const QJsonObject& projectionContext) {
    try {
        validateDraft(draft);
        checkFields(arguments, {"entityIds", "construction"});
        if (!arguments["construction"].isBool()) reject("Construction requires an explicit boolean");
        if (!arguments["entityIds"].isArray() || arguments["entityIds"].toArray().isEmpty())
            reject("Select at least one line for construction conversion");
        QSet<QString> selected;
        for (const auto& value : arguments["entityIds"].toArray()) {
            if (!value.isString() || QUuid(value.toString()).isNull() || selected.contains(value.toString()))
                reject("Construction selection requires distinct line UUIDs");
            selected.insert(value.toString());
        }
        auto parameters = draft["parameters"].toObject();
        auto entities = parameters["entities"].toArray();
        auto remaining = selected;
        bool changed = false;
        for (int i = 0; i < entities.size(); ++i) {
            auto entity = entities[i].toObject();
            if (!selected.contains(entity["id"].toString())) continue;
            if (entity["type"] != "line") reject("Construction conversion currently supports lines only");
            remaining.remove(entity["id"].toString());
            if (entity.value("construction").toBool() == arguments["construction"].toBool()) continue;
            if (arguments["construction"].toBool()) { entity["construction"] = true; entity.remove("centerline"); }
            else entity.remove("construction");
            entities[i] = entity; changed = true;
        }
        if (!remaining.isEmpty()) reject("Construction selection contains a missing entity");
        if (!changed) {
            auto result = solveDraft(draft,projectionContext);
            result.feature = draft;
            return result;
        }
        parameters["entities"] = entities;
        auto candidate = draft; candidate["parameters"] = parameters;
        const auto result = solveDraft(candidate,projectionContext);
        return result.ok ? result : SolveResult{false, draft, result.diagnostics, result.error};
    } catch (const std::exception& error) { return invalidDraft(draft, QString::fromUtf8(error.what())); }
}

SketchGeometry::SolveResult SketchGeometry::addCircleOffset(const QJsonObject& draft,const QJsonObject& arguments,const QJsonObject& projectionContext) {
    try {
        validateDraft(draft);
        checkFields(arguments,{"sourceEntityId","distanceMm"});
        const auto sourceId=arguments["sourceEntityId"].toString();
        if(!validId(sourceId))reject("Offset requires a stable source circle ID");
        const double distance=coordinate(arguments,"distanceMm");
        if(std::abs(distance)<minimumLength)reject("Circle offset distance must be at least 0.001 mm");
        auto parameters=draft["parameters"].toObject();auto entities=parameters["entities"].toArray();
        auto points=parameters["points"].toArray(),constraints=parameters["constraints"].toArray();
        QJsonObject source,sourceCenter;
        for(const auto& value:entities)if(value.toObject()["id"]==sourceId){source=value.toObject();break;}
        if(source.isEmpty())for(const auto& value:expandedProjectionFeature(draft,projectionContext)["parameters"].toObject()["entities"].toArray())
            if(value.toObject()["id"]==sourceId){source=value.toObject();break;}
        if(source["type"]!="circle")reject("Circle Offset requires one current circle");
        for(const auto& value:constraints)if(value.toObject()["type"]=="circleOffset"
            && value.toObject()["entityIds"].toArray()[1]==sourceId)reject("Offset result cannot be used as a source in this slice");
        for(const auto& value:points)if(value.toObject()["id"]==source["centerPointId"]){sourceCenter=value.toObject();break;}
        if(sourceCenter.isEmpty())for(const auto& value:expandedProjectionFeature(draft,projectionContext)["parameters"].toObject()["points"].toArray())
            if(value.toObject()["id"]==source["centerPointId"]){sourceCenter=value.toObject();break;}
        if(sourceCenter.isEmpty())reject("Source circle center no longer exists");
        const double radius=source["radiusMm"].toDouble()+distance;
        if(radius<minimumLength || radius>maximumLength)reject("Offset would create an invalid circle radius");
        const auto centerId=newId(),targetId=newId(),relationId=newId();
        points.append(QJsonObject{{"id",centerId},{"uMm",sourceCenter["uMm"]},{"vMm",sourceCenter["vMm"]}});
        entities.append(QJsonObject{{"id",targetId},{"type","circle"},{"centerPointId",centerId},{"radiusMm",radius}});
        constraints.append(QJsonObject{{"id",newId()},{"type","coincident"},{"pointIds",QJsonArray{source["centerPointId"],centerId}}});
        constraints.append(QJsonObject{{"id",relationId},{"type","circleOffset"},{"entityIds",QJsonArray{sourceId,targetId}},{"distanceMm",distance}});
        parameters["points"]=points;parameters["entities"]=entities;parameters["constraints"]=constraints;
        auto candidate=draft;candidate["parameters"]=parameters;
        const auto result=solveDraft(candidate,projectionContext);
        return result.ok ? result : SolveResult{false,draft,result.diagnostics,result.error};
    } catch(const std::exception& error){return invalidDraft(draft,QString::fromUtf8(error.what()));}
}

SketchGeometry::SolveResult SketchGeometry::updateCircleOffset(const QJsonObject& draft,const QJsonObject& arguments,const QJsonObject& projectionContext) {
    try {
        validateDraft(draft);
        checkFields(arguments,{"constraintId","distanceMm"});
        const auto id=arguments["constraintId"].toString();if(!validId(id))reject("Offset requires a stable constraint ID");
        const double distance=coordinate(arguments,"distanceMm");
        if(std::abs(distance)<minimumLength)reject("Circle offset distance must be at least 0.001 mm");
        auto parameters=draft["parameters"].toObject();auto constraints=parameters["constraints"].toArray();
        bool found=false;
        for(qsizetype i=0;i<constraints.size();++i) {
            auto relation=constraints[i].toObject();
            if(relation["id"]!=id)continue;
            if(relation["type"]!="circleOffset")reject("Selected constraint is not a circle offset");
            relation["distanceMm"]=distance;constraints[i]=relation;found=true;break;
        }
        if(!found)reject("Circle offset constraint no longer exists");
        parameters["constraints"]=constraints;auto candidate=draft;candidate["parameters"]=parameters;
        const auto result=solveDraft(candidate,projectionContext);
        return result.ok ? result : SolveResult{false,draft,result.diagnostics,result.error};
    } catch(const std::exception& error){return invalidDraft(draft,QString::fromUtf8(error.what()));}
}

SketchGeometry::SolveResult SketchGeometry::updateDimension(const QJsonObject& draft, const QJsonObject& arguments,
    const QJsonObject& projectionContext,const QSet<QString>& userLengthNames) {
    try {
        validateDraft(draft);
        checkFields(arguments, {"dimensionId", "valueMm", "valueDeg", "parameterId", "expression", "expressionUnits"});
        const bool formula=arguments.contains("expression");
        if(formula && (arguments.contains("valueMm") || arguments.contains("valueDeg") || arguments.contains("parameterId")))
            reject("Choose a local expression or a numeric/user-parameter dimension");
        if(arguments.contains("expressionUnits")!=formula)reject("Local expression and entry units must be supplied together");
        auto parameters = draft["parameters"].toObject();
        auto dimensions = parameters["dimensions"].toArray();
        bool found = false;
        for (int i = 0; i < dimensions.size(); ++i) {
            auto dimension = dimensions[i].toObject();
            if (dimension["id"] != arguments["dimensionId"]) continue;
            if (!dimension["driving"].toBool()) reject("A driven dimension cannot set geometry");
            const bool angular=dimension["kind"]=="arcSweep" || dimension["kind"]=="angle" || dimension["kind"]=="overallSlotAngle";
            const auto key=angular ? "valueDeg" : "valueMm";
            if(!formula)coordinate(arguments,key);
            if(arguments.contains(angular ? "valueMm" : "valueDeg")) reject("Dimension value has incorrect units");
            if(formula){
                if(angular || !arguments["expression"].isString() ||
                   arguments["expression"].toString().trimmed().isEmpty() ||
                   !arguments["expressionUnits"].isString())reject("Only driving lengths accept a local expression");
                if(dimension.value("expression")==arguments["expression"] &&
                   dimension.value("expressionUnits")==arguments["expressionUnits"] &&
                   dimension.contains("referenceIds")){
                    // Reopening the same text is not a new name binding.
                    dimensions[i]=dimension;found=true;break;
                }
                dimension.remove("parameterId");dimension.remove("expression");
                dimension.remove("expressionUnits");dimension.remove("referenceIds");
                dimension[key]=1.0;dimensions[i]=dimension;
                parameters["dimensions"]=dimensions;
                if(!parameters.contains("nextDimensionNameIndex"))parameters["nextDimensionNameIndex"]=1;
                parameters=allocateDimensionNames(parameters);dimensions=parameters["dimensions"].toArray();
                dimension=bindLocalExpression(dimensions[i].toObject(),dimensions,
                    arguments["expression"].toString(),arguments["expressionUnits"].toString(),userLengthNames);
            } else if(arguments.contains("parameterId")) {
                if(angular || !validId(arguments["parameterId"].toString()))reject("Only driving length dimensions accept a stable parameter ID");
                dimension[key]=arguments[key];
                dimension["parameterId"]=arguments["parameterId"];
            } else {dimension[key]=arguments[key];dimension.remove("parameterId");}
            if(!formula){dimension.remove("expression");dimension.remove("expressionUnits");dimension.remove("referenceIds");}
            dimensions[i] = dimension; found = true; break;
        }
        if (!found) reject("Driving dimension does not exist");
        parameters["dimensions"] = dimensions;
        auto candidate = draft; candidate["parameters"] = parameters;
        const auto result = solveDraft(candidate,projectionContext);
        return result.ok ? result : SolveResult{false, draft, result.diagnostics, result.error};
    } catch (const std::exception& error) { return invalidDraft(draft, QString::fromUtf8(error.what())); }
}

SketchGeometry::SolveResult SketchGeometry::updateAnnotation(const QJsonObject& draft, const QJsonObject& arguments, const QJsonObject& projectionContext) {
    try {
        validateDraft(draft);
        checkFields(arguments, {"dimensionId", "annotation"});
        if (!arguments["annotation"].isObject()) reject("Dimension annotation must be an object");
        auto parameters = draft["parameters"].toObject();
        auto dimensions = parameters["dimensions"].toArray();
        bool found = false;
        for (int i = 0; i < dimensions.size(); ++i) {
            auto dimension = dimensions[i].toObject();
            if (dimension["id"] != arguments["dimensionId"]) continue;
            dimension["annotation"] = arguments["annotation"];
            dimensions[i] = dimension; found = true; break;
        }
        if (!found) reject("Dimension does not exist");
        parameters["dimensions"] = dimensions;
        auto candidate = draft; candidate["parameters"] = parameters;
        const auto result = solveDraft(candidate,projectionContext);
        // A presentation edit preserves accepted authored coordinates verbatim.
        // Obtain diagnostics without applying any solver normalization to intent.
        return result.ok ? SolveResult{true, candidate, result.diagnostics, {}}
                         : SolveResult{false, draft, result.diagnostics, result.error};
    } catch (const std::exception& error) { return invalidDraft(draft, QString::fromUtf8(error.what())); }
}

SketchGeometry::SolveResult SketchGeometry::movePoint(const QJsonObject& draft, const QJsonObject& arguments, const QJsonObject& projectionContext) {
    try {
        validateDraft(draft);
        checkFields(arguments, {"pointId", "uMm", "vMm"});
        coordinate(arguments, "uMm"); coordinate(arguments, "vMm");
        auto parameters = draft["parameters"].toObject();
        if (arguments["pointId"]==parameters.value("originPointId") && parameters.contains("originPointId")) reject("Sketch origin reference cannot be moved");
        if(SketchArcSlot::derivedPointIds(parameters).contains(arguments["pointId"].toString()))
            reject("Arc slot tangent contact is derived; move a path endpoint instead");
        if(SketchSplineOffset::derivedPointIds(parameters).contains(arguments["pointId"].toString()))
            reject("Spline Offset endpoint is derived; edit its source or signed distance");
        if(SketchOpenChainOffset::derivedPointIds(parameters).contains(arguments["pointId"].toString()))
            reject("Open-chain Offset point is derived; edit its source or signed distance");
        for(const auto& groupValue:parameters.value("mirrorGroups").toArray()){
            const auto group=groupValue.toObject();
            for(const auto& pairValue:group["pointPairs"].toArray()){
                const auto pair=pairValue.toObject();if(pair["resultPointId"]!=arguments["pointId"])continue;
                QJsonObject axis;for(const auto& value:parameters["entities"].toArray())if(value.toObject()["id"]==group["axisEntityId"]){axis=value.toObject();break;}
                if(axis["type"]!="line")reject("Mirror axis line is missing");
                QPointF ends[2];int found=0;
                for(const auto& value:parameters["points"].toArray()){
                    const auto point=value.toObject();if(point["id"]==axis["startPointId"]){ends[0]={point["uMm"].toDouble(),point["vMm"].toDouble()};++found;}
                    if(point["id"]==axis["endPointId"]){ends[1]={point["uMm"].toDouble(),point["vMm"].toDouble()};++found;}
                }
                if(found!=2)reject("Mirror axis points are missing");
                const QPointF target(arguments["uMm"].toDouble(),arguments["vMm"].toDouble()),direction=ends[1]-ends[0];
                const double norm=QPointF::dotProduct(direction,direction);
                if(norm<minimumLength*minimumLength)reject("Mirror axis is degenerate");
                const auto source=2*(ends[0]+direction*(QPointF::dotProduct(target-ends[0],direction)/norm))-target;
                return movePoint(draft,{{"pointId",pair["sourcePointId"]},{"uMm",source.x()},{"vMm",source.y()}},projectionContext);
            }
        }
        auto points = parameters["points"].toArray();
        // Moving the familiar circle-center handle translates its retained
        // first diameter endpoint. The solver then derives the new center.
        for(const auto& value:parameters["entities"].toArray()) {
            const auto circle=value.toObject();
            if(circle["type"]!="circle" || !circle.contains("diameterStartPointId") || circle["centerPointId"]!=arguments["pointId"])continue;
            QJsonObject oldCenter;for(const auto& pointValue:points)if(pointValue.toObject()["id"]==arguments["pointId"]){oldCenter=pointValue.toObject();break;}
            if(oldCenter.isEmpty())reject("Two-point circle center does not exist");
            const double du=arguments["uMm"].toDouble()-oldCenter["uMm"].toDouble();
            const double dv=arguments["vMm"].toDouble()-oldCenter["vMm"].toDouble();
            for(int i=0;i<points.size();++i)if(points[i].toObject()["id"]==circle["diameterStartPointId"]) {
                auto start=points[i].toObject();start["uMm"]=start["uMm"].toDouble()+du;start["vMm"]=start["vMm"].toDouble()+dv;points[i]=start;break;
            }
        }
        bool found = false;
        for (int i = 0; i < points.size(); ++i) {
            auto point = points[i].toObject();
            if (point["id"] != arguments["pointId"]) continue;
            point["uMm"] = arguments["uMm"]; point["vMm"] = arguments["vMm"]; points[i] = point; found = true; break;
        }
        if (!found) reject("Sketch point does not exist");
        parameters["points"] = points;
        auto candidate = draft; candidate["parameters"] = parameters;
        // Point manipulation supplies a solver seed. ArcRules restore normalized
        // analytic intent before final validation; no inconsistent state is accepted.
        if(parameters.value("projections").toArray().isEmpty() && parameters.value("circleProjections").toArray().isEmpty()
            && parameters.value("pointReferences").toArray().isEmpty()) {
            const auto result=SketchSolver::solve(parameters);
            if(!result.accepted)return {false,draft,result.diagnostics,result.error};
            candidate["parameters"]=result.parameters;
        }
        auto solved=solveDraft(candidate,projectionContext);
        if(!solved.ok)solved.feature=draft;
        return solved;
    } catch (const std::exception& error) { return invalidDraft(draft, QString::fromUtf8(error.what())); }
}

QJsonObject SketchGeometry::frame(const QString& plane) {
    QJsonArray u, v, normal;
    if (plane == "XY") { u = {1, 0, 0}; v = {0, 1, 0}; normal = {0, 0, 1}; }
    else if (plane == "XZ") { u = {1, 0, 0}; v = {0, 0, 1}; normal = {0, -1, 0}; }
    else if (plane == "YZ") { u = {0, 1, 0}; v = {0, 0, 1}; normal = {1, 0, 0}; }
    else reject("Unsupported sketch origin plane");
    return {{"origin", QJsonArray{0, 0, 0}}, {"uAxis", u}, {"vAxis", v}, {"normal", normal}};
}

QJsonObject SketchGeometry::evaluationFrame(const QJsonObject& parameters,const QJsonObject& resolvedFrame) {
    if(parameters["plane"]!="face" && parameters["plane"]!="construction")return frame(parameters["plane"].toString());
    if(resolvedFrame.size()!=4)reject("Face sketch requires an explicit resolved support frame");
    std::array<std::array<double,3>,4> vectors;int index=0;
    for(const auto& key:{"origin","uAxis","vAxis","normal"}){
        const auto values=resolvedFrame[key].toArray();if(!resolvedFrame[key].isArray() || values.size()!=3)reject("Invalid resolved support frame");
        for(int j=0;j<3;++j){if(!values[j].isDouble() || !std::isfinite(values[j].toDouble()))reject("Nonfinite support frame");vectors[index][j]=values[j].toDouble();}++index;}
    auto dot=[](const auto& a,const auto& b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];};
    for(int i=1;i<4;++i)if(std::abs(dot(vectors[i],vectors[i])-1)>1e-9)reject("Support frame axes must be unit length");
    if(std::abs(dot(vectors[1],vectors[2]))>1e-9 || std::abs(dot(vectors[1],vectors[3]))>1e-9 || std::abs(dot(vectors[2],vectors[3]))>1e-9)reject("Support frame axes must be orthogonal");
    const std::array<double,3> cross{vectors[1][1]*vectors[2][2]-vectors[1][2]*vectors[2][1],vectors[1][2]*vectors[2][0]-vectors[1][0]*vectors[2][2],vectors[1][0]*vectors[2][1]-vectors[1][1]*vectors[2][0]};
    if(dot(cross,vectors[3])<1-1e-9)reject("Support frame must be right handed");return resolvedFrame;
}

EvaluatedSketch SketchGeometry::evaluateFeature(const QJsonObject& feature, const QJsonObject& resolvedFrame, const QJsonObject& projectionContext) {
    const auto solution = solveDraft(feature,projectionContext);
    if (!solution.ok) reject(solution.error);
    const auto parameters = deriveClosedRectOffsets(SketchOpenChainOffset::derive(
        expandedProjectionFeature(solution.feature,projectionContext)["parameters"].toObject()));
    const auto plane = parameters["plane"].toString();
    const auto sketchFrame=evaluationFrame(parameters,resolvedFrame);
    QMap<QString, gp_Pnt> points;
    QJsonArray worldPoints;
    for (const auto& entry : parameters["points"].toArray()) {
        const auto point = entry.toObject();
        const auto world = worldPoint(sketchFrame, point["uMm"].toDouble(), point["vMm"].toDouble());
        points.insert(point["id"].toString(), world);
        worldPoints.append(QJsonObject{{"id", point["id"]}, {"positionMm", QJsonArray{world.X(), world.Y(), world.Z()}}});
    }
    TopoDS_Shape shape;
    QMap<QString,TopoDS_Edge> entityEdges;
    QJsonValue bounds(QJsonValue::Null);
    const auto entities = parameters["entities"].toArray();
    if (!entities.isEmpty()) {
        TopoDS_Compound compound;
        BRep_Builder builder;
        builder.MakeCompound(compound);
        for (const auto& entry : entities) {
            const auto entity = entry.toObject();
            if(entity["type"]=="point") {
                builder.Add(compound,BRepBuilderAPI_MakeVertex(points[entity["pointId"].toString()]).Vertex());continue;
            }
            const auto edge = analyticEdge(parameters,entity,sketchFrame);
            builder.Add(compound, edge);
            entityEdges.insert(entity["id"].toString(),edge);
        }
        if (!BRepCheck_Analyzer(compound).IsValid()) throw std::runtime_error("Kernel did not produce valid sketch edges");
        shape = compound;
        Bnd_Box box;
        BRepBndLib::AddOptimal(shape, box, false, false);
        double x0, y0, z0, x1, y1, z1;
        box.Get(x0, y0, z0, x1, y1, z1);
        bounds = QJsonObject{{"min", QJsonArray{x0, y0, z0}}, {"max", QJsonArray{x1, y1, z1}},
            {"size", QJsonArray{x1-x0, y1-y0, z1-z0}}};
    }
    QJsonArray geometry;
    for(const auto& value : entities) {const auto e=value.toObject(); auto detail=e;
        if(e["type"]=="line") detail["lengthMm"]=points[e["startPointId"].toString()].Distance(points[e["endPointId"].toString()]);
        else if(e["type"]=="fitSpline"){const auto spline=SketchSpline::evaluate(parameters,e);detail["degree"]=5;detail["continuity"]="C2";detail["fitParameters"]=QJsonArray::fromVariantList([&]{QVariantList list;for(double t:spline.parameters)list.append(t);return list;}());detail["tangentHandles"]=spline.handles;detail["curvatureComb"]=SketchSpline::comb(parameters,e);}
        else if(e["type"]=="fitSplineOffset"){
            const auto evaluated=SketchSplineOffset::evaluate(parameters,e,sketchFrame);
            detail["startMm"]=QJsonArray{evaluated.start.X(),evaluated.start.Y(),evaluated.start.Z()};
            detail["endMm"]=QJsonArray{evaluated.end.X(),evaluated.end.Y(),evaluated.end.Z()};
            detail["curveType"]="analytic_offset";
        }
        else if(e["type"]=="controlSpline"){const auto spline=SketchSpline::evaluate(parameters,e);detail["degree"]=spline.curve->Degree();detail["continuity"]=spline.curve->IsCN(2)?"C2":spline.curve->IsCN(1)?"C1":"C0";detail["curveSamples"]=SketchSpline::curveSamples(spline);QJsonArray poles;for(const auto& id:e["pointIds"].toArray())for(const auto& v:parameters["points"].toArray()){const auto p=v.toObject();if(p["id"]==id)poles.append(QJsonObject{{"pointId",id},{"uMm",p["uMm"]},{"vMm",p["vMm"]}});}detail["controlPoints"]=poles;detail["curvatureComb"]=SketchSpline::comb(parameters,e);}
        else if(e["type"]=="conic"){const auto curve=SketchConic::evaluate(parameters,e);const auto shoulder=curve->Value(0.5);detail["degree"]=2;detail["shoulderLocalMm"]=QJsonArray{shoulder.X(),shoulder.Y()};}
        else if(e["type"]=="ellipse"){
            const auto axes=SketchEllipse::axes(parameters,e);const auto center=points[e["centerPointId"].toString()];
            detail["centerMm"]=QJsonArray{center.X(),center.Y(),center.Z()};
            detail["firstRadiusMm"]=axes.firstRadius;detail["secondRadiusMm"]=axes.secondRadius;
            detail["majorRadiusMm"]=std::max(axes.firstRadius,axes.secondRadius);
            detail["minorRadiusMm"]=std::min(axes.firstRadius,axes.secondRadius);
        }
        else if(e["type"]=="point") {const auto p=points[e["pointId"].toString()];detail["positionMm"]=QJsonArray{p.X(),p.Y(),p.Z()};}
        else {const auto center=points[e["centerPointId"].toString()]; detail["centerMm"]=QJsonArray{center.X(),center.Y(),center.Z()};
            detail["lengthMm"]=e["radiusMm"].toDouble()*(e["type"]=="circle" ? 2*std::numbers::pi : std::abs(e["sweepAngleDeg"].toDouble())*degrees);
            if(e["type"]=="arc") {const auto first=points[e["startPointId"].toString()],last=points[e["endPointId"].toString()];
                detail["startMm"]=QJsonArray{first.X(),first.Y(),first.Z()};detail["endMm"]=QJsonArray{last.X(),last.Y(),last.Z()};}}
        geometry.append(detail);
    }
    QJsonObject summary{{"kernel", "OCCT"}, {"version", ALLOY_OCCT_VERSION}, {"type", "sketch"}, {"valid", true},
        {"lengthUnit", "mm"}, {"plane", plane}, {"frame", sketchFrame}, {"solverIntegrated", true}, {"solver", solution.diagnostics},
        {"entities",geometry}, {"pointCount", points.size()}, {"entityCount", entities.size()}, {"worldPoints", worldPoints}, {"boundsMm", bounds}};
    auto profileFeature=solution.feature;
    bool linkedProfile=false;
    for(const auto& collection:{"projections","circleProjections"})for(const auto& value:solution.feature["parameters"].toObject()[collection].toArray())
        linkedProfile|=value.toObject()["profile"].toBool();
    if(linkedProfile) {
        auto profileParameters=parameters;
        // Region extraction consumes solved geometry. Constraints and retained
        // point anchors have already done their work and are not boundaries.
        QSet<QString> boundaryPoints;
        for(const auto& entry:parameters["entities"].toArray()) {
            const auto entity=entry.toObject();
            for(const auto& key:{"pointId","centerPointId","startPointId","endPointId"})
                if(entity[key].isString())boundaryPoints.insert(entity[key].toString());
            for(const auto& id:entity["pointIds"].toArray())boundaryPoints.insert(id.toString());
        }
        if(parameters["originPointId"].isString())boundaryPoints.insert(parameters["originPointId"].toString());
        QJsonArray profilePoints;for(const auto& entry:parameters["points"].toArray())
            if(boundaryPoints.contains(entry.toObject()["id"].toString()))profilePoints.append(entry);
        // Ellipse axes are structural geometry: their internal midpoint and
        // perpendicular relations are required by profile validation even
        // though external solved anchors are not profile boundaries.
        QJsonArray structural;
        for(const auto& value:parameters["constraints"].toArray()) {
            const auto relation=value.toObject();const auto ids=relation["entityIds"].toArray();bool retain=false;
            for(const auto& item:parameters["entities"].toArray()) {
                const auto ellipse=item.toObject();if(ellipse["type"]!="ellipse")continue;
                const auto first=ellipse["firstAxisId"],second=ellipse["secondAxisId"];
                retain|=relation["type"]=="midpoint" && relation["pointIds"].toArray()==QJsonArray{ellipse["centerPointId"]}
                    && (ids==QJsonArray{first} || ids==QJsonArray{second});
                retain|=relation["type"]=="perpendicular" && ids.size()==2 && ids.contains(first) && ids.contains(second);
            }
            if(retain)structural.append(relation);
        }
        profileParameters["points"]=profilePoints;profileParameters["constraints"]=structural;
        profileParameters["dimensions"]=QJsonArray{};
        profileFeature["parameters"]=profileParameters;
    }
    const auto profile = SketchProfiles::deriveRegions(profileFeature,sketchFrame);
    QJsonArray profileSummaries;
    std::vector<EvaluatedProfile> profiles;
    if (profile.ok) {
        profiles = profile.profiles;
        for (const auto& region : profiles) profileSummaries.append(region.summary);
    }
    if(parameters.contains("texts")){
        const auto generated=SketchText::evaluate(solution.feature,sketchFrame);
        shape=generated.shape;profiles=generated.profiles;profileSummaries=generated.summary["profiles"].toArray();
        summary["texts"]=QJsonArray{generated.summary};summary["boundsMm"]=GeometryKernel::summarize(shape)["boundsMm"];
        summary["solverIntegrated"]=false;
    }
    QJsonArray retainedSupportPoints;for(const auto& point:projectionContext["points"].toArray()) {
        for(const auto& collection:{"pointReferences","pointProjections"})for(const auto& reference:solution.feature["parameters"].toObject()[collection].toArray())if(point.toObject()["id"]==reference.toObject()["id"]){auto retained=point.toObject();retained["retained"]=true;retainedSupportPoints.append(retained);}
    }
    summary["projectedPoints"]=retainedSupportPoints;summary["availableSupportPoints"]=projectionContext["points"].toArray();
    summary["projectedLines"]=projectionContext["lines"].toArray();
    summary["projectedCircles"]=projectionContext["circles"].toArray();
    summary["unsupportedProjectedEdges"]=projectionContext["unsupportedEdges"].toArray();
    QJsonArray standalonePoints;for(const auto& item:geometry)if(item.toObject()["type"]=="point")standalonePoints.append(item);
    summary["standalonePoints"]=standalonePoints;
    summary["standalonePointCount"]=standalonePoints.size();
    summary["profiles"] = profileSummaries;
    const auto authoredEntities=solution.feature["parameters"].toObject()["entities"].toArray();
    summary["profileDiagnostics"] = !standalonePoints.isEmpty() && std::all_of(authoredEntities.begin(),authoredEntities.end(),[](const auto& e){return e.toObject()["type"]=="point";})
        ? QJsonObject{{"code","point_only_sketch"},{"message","Standalone points do not bound a profile"},{"profileCount",0}}
        : parameters.contains("texts")
        ? QJsonObject{{"code","text_profiles"},{"profileCount",static_cast<int>(profiles.size())}}
        : profile.diagnostics;
    return {feature["id"].toString(), feature["name"].toString(), plane, feature["visible"].toBool(), shape, summary, profiles, sketchFrame, {}, entityEdges};
}
