#include "SketchGeometry.h"
#include <Geom2dAPI_InterCurveCurve.hxx>
#include <Geom2d_Circle.hxx>
#include <GCE2d_MakeSegment.hxx>
#include <gp_Ax2d.hxx>
#include <QMap>
#include <QUuid>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace {
QString freshId(){return QUuid::createUuid().toString(QUuid::WithoutBraces);}
struct Cut {double t;QStringList boundaries;};
}
SketchGeometry::SolveResult SketchGeometry::trimLine(const QJsonObject& draft,const QJsonObject& arguments,const QJsonObject& context) {
    try {
        for(auto it=arguments.begin();it!=arguments.end();++it)if(it.key()!="entityId" && it.key()!="uMm" && it.key()!="vMm")throw std::invalid_argument("Unknown Trim argument");
        if(!arguments["uMm"].isDouble() || !arguments["vMm"].isDouble())throw std::invalid_argument("Trim needs a finite sketch point");
        const double pickedU=arguments["uMm"].toDouble(),pickedV=arguments["vMm"].toDouble();
        if(!std::isfinite(pickedU) || !std::isfinite(pickedV))throw std::invalid_argument("Trim point is nonfinite");
        auto parameters=draft["parameters"].toObject();const auto originalEntities=parameters["entities"].toArray();
        QMap<QString,QJsonObject> points;for(const auto& entry:parameters["points"].toArray()){const auto point=entry.toObject();points[point["id"].toString()]=point;}
        auto xy=[&](const QString& id){const auto p=points.value(id);if(p.isEmpty())throw std::invalid_argument("Trim point reference unavailable");return gp_Pnt2d(p["uMm"].toDouble(),p["vMm"].toDouble());};
        const auto targetId=arguments["entityId"].toString();QJsonObject target;
        for(const auto& entry:originalEntities)if(entry.toObject()["id"]==targetId)target=entry.toObject();
        if(target.isEmpty() || target["type"]!="line")throw std::invalid_argument("This Trim slice requires an authored line; curves and linked projections cannot be trimmed");
        const auto start=xy(target["startPointId"].toString()),end=xy(target["endPointId"].toString());
        const gp_Vec2d direction(start,end);const auto squared=direction.SquareMagnitude();
        const double pick=std::clamp(gp_Vec2d(start,gp_Pnt2d(pickedU,pickedV)).Dot(direction)/squared,0.0,1.0);
        const auto curve=GCE2d_MakeSegment(start,end).Value();std::vector<Cut> cuts{{0,{}},{1,{}}};std::vector<std::pair<double,double>> overlaps;
        for(const auto& entry:originalEntities){const auto other=entry.toObject();if(other["id"]==targetId)continue;
            Handle(Geom2d_Curve) boundary;
            if(other["type"]=="line")boundary=GCE2d_MakeSegment(xy(other["startPointId"].toString()),xy(other["endPointId"].toString())).Value();
            else if(other["type"]=="circle")boundary=new Geom2d_Circle(gp_Ax2d(xy(other["centerPointId"].toString()),gp_Dir2d(1,0)),other["radiusMm"].toDouble());
            else continue;
            Geom2dAPI_InterCurveCurve intersection(curve,boundary,1e-8);
            auto parameter=[&](const gp_Pnt2d& point){return gp_Vec2d(start,point).Dot(direction)/squared;};
            for(int i=1;i<=intersection.NbPoints();++i){const double t=parameter(intersection.Point(i));if(t>1e-8 && t<1-1e-8)cuts.push_back({t,{other["id"].toString()}});}
            for(int i=1;i<=intersection.NbSegments();++i){Handle(Geom2d_Curve) a,b;intersection.Segment(i,a,b);if(!a.IsNull()){double low=parameter(a->Value(a->FirstParameter())),high=parameter(a->Value(a->LastParameter()));overlaps.push_back({std::min(low,high),std::max(low,high)});}}
        }
        std::sort(cuts.begin(),cuts.end(),[](const auto& a,const auto& b){return a.t<b.t;});std::vector<Cut> unique;
        for(const auto& cut:cuts){if(!unique.empty() && std::abs(cut.t-unique.back().t)<1e-8)unique.back().boundaries.append(cut.boundaries);else unique.push_back(cut);}
        int interval=-1;for(int i=0;i+1<int(unique.size());++i)if(pick>unique[i].t+1e-8 && pick<unique[i+1].t-1e-8){interval=i;break;}
        if(interval<0)throw std::invalid_argument("Pick inside a line segment, away from an intersection");
        const double low=unique[interval].t,high=unique[interval+1].t;
        for(const auto& overlap:overlaps)if(std::min(high,overlap.second)-std::max(low,overlap.first)>1e-8)throw std::invalid_argument("Selected Trim interval overlaps another line; ambiguous ownership is unsupported");
        if((high-low)*std::sqrt(squared)<.001)throw std::invalid_argument("Trim interval is too short");
        QJsonArray entities,created,removedEntities,newRelations;QStringList retainedIds;
        const bool split=low>1e-8 && high<1-1e-8;
        auto endpoint=[&](const Cut& cut){const auto p=start.Translated(direction*cut.t);const auto id=freshId();points[id]={{"id",id},{"uMm",p.X()},{"vMm",p.Y()}};
            QSet<QString> boundaries(cut.boundaries.begin(),cut.boundaries.end());for(const auto& boundary:boundaries)newRelations.append(QJsonObject{{"id",freshId()},{"type","pointOnCurve"},{"pointIds",QJsonArray{id}},{"entityIds",QJsonArray{boundary}}});return id;};
        auto retain=[&](double a,double b){auto line=target;const auto id=split ? freshId() : targetId;line["id"]=id;
            if(a>1e-8)line["startPointId"]=endpoint(unique[interval+1]);if(b<1-1e-8)line["endPointId"]=endpoint(unique[interval]);
            entities.append(line);retainedIds.append(id);if(id!=targetId)created.append(id);};
        for(const auto& entry:originalEntities)if(entry.toObject()["id"]!=targetId)entities.append(entry);
        if(low>1e-8)retain(0,low);if(high<1-1e-8)retain(high,1);
        if(retainedIds.isEmpty() || split)removedEntities.append(targetId);
        QSet<QString> used;for(const auto& entry:entities){const auto e=entry.toObject();for(const auto& key:{"startPointId","endPointId","centerPointId"})if(e.contains(key))used.insert(e[key].toString());}
        // Preserve standalone origin references; all other retired endpoints lose their annotations explicitly.
        if(parameters.contains("originPointId"))used.insert(parameters["originPointId"].toString());
        QSet<QString> retired;QJsonArray keptPoints;for(auto it=points.begin();it!=points.end();++it)if(used.contains(it.key()))keptPoints.append(it.value());else retired.insert(it.key());
        QJsonArray constraints,dimensions,removedConstraints,removedDimensions;
        for(const auto& entry:parameters["constraints"].toArray()){const auto c=entry.toObject();bool invalid=false;
            for(const auto& id:c["pointIds"].toArray())invalid|=retired.contains(id.toString());
            const bool source=c["entityIds"].toArray().contains(targetId);
            if(source && (split || retainedIds.isEmpty()))invalid=true;
            // Length- and endpoint-sensitive relations cannot silently migrate to a shortened segment.
            if(source && (c["type"]=="equal" || c["type"]=="midpoint" || c["type"]=="arcTangent"))invalid=true;
            if(invalid){removedConstraints.append(c["id"]);if(split && (c["type"]=="horizontal" || c["type"]=="vertical"))for(const auto& id:retainedIds){auto orientation=c;orientation["id"]=freshId();orientation["entityIds"]=QJsonArray{id};constraints.append(orientation);}}
            else constraints.append(c);
        }
        for(const auto& entry:newRelations)constraints.append(entry);
        for(const auto& entry:parameters["dimensions"].toArray()){const auto d=entry.toObject();bool invalid=d["entityId"]==targetId || d["entityIds"].toArray().contains(targetId);
            const auto refs=d["pointIds"].toArray();
            // End/whole trim invalidates a length taken from that line's original endpoints;
            // middle trim preserves dimensions spanning the two surviving outer endpoints.
            if(!split && refs.size()==2 && refs.contains(target["startPointId"]) && refs.contains(target["endPointId"]))invalid=true;
            for(const auto& id:refs)invalid|=retired.contains(id.toString());
            if(invalid)removedDimensions.append(d["id"]);else dimensions.append(d);
        }
        parameters["points"]=keptPoints;parameters["entities"]=entities;parameters["constraints"]=constraints;parameters["dimensions"]=dimensions;
        auto candidate=draft;candidate["parameters"]=parameters;auto result=solveDraft(candidate,context);if(!result.ok)return {false,draft,result.diagnostics,result.error};
        const auto from=start.Translated(direction*low),to=start.Translated(direction*high);
        result.diagnostics["trimmedSegment"]=QJsonObject{{"entityId",targetId},{"u1Mm",from.X()},{"v1Mm",from.Y()},{"u2Mm",to.X()},{"v2Mm",to.Y()}};
        result.diagnostics["removedEntityIds"]=removedEntities;result.diagnostics["createdEntityIds"]=created;result.diagnostics["removedConstraintIds"]=removedConstraints;result.diagnostics["removedDimensionIds"]=removedDimensions;return result;
    }catch(const std::exception& error){return {false,draft,{{"operation","trim"},{"status","invalid"}},QString::fromUtf8(error.what())};}
}
