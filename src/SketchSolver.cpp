// SPDX-License-Identifier: MIT
#include "SketchSolver.h"
#include "SketchMirror.h"
#include "SketchArcSlot.h"
#include "SketchPolygon.h"
#include "GCS.h"
#include "Constraints.h"
#include <QJsonArray>
#include <QMap>
#include <QSet>
#include <algorithm>
#include <array>
#include <cmath>
#include <deque>
#include <map>
#include <set>
#include <numbers>
#include <stdexcept>
#include <vector>

namespace {
constexpr double lengthTolerance = 1e-6;
constexpr double angularTolerance = 1e-8;
constexpr double minimumLength = 0.001;
constexpr double maximumCoordinate = 100000;
constexpr double degrees = std::numbers::pi/180.0;
void require(bool condition, const char* message) {
    if (!condition) throw std::invalid_argument(message);
}
double number(const QJsonObject& object, const char* key) {
    const auto value = object.value(QLatin1String(key));
    require(value.isDouble() && std::isfinite(value.toDouble()), "Expected finite sketch parameter");
    return value.toDouble();
}
QString reference(const QJsonObject& object, const char* key, int index) {
    const auto array = object.value(QLatin1String(key)).toArray();
    require(index < array.size() && array[index].isString(), "Missing sketch reference");
    return array[index].toString();
}
double pointLineDistance(const GCS::Point& p,const GCS::Line& l) {
    const double dx=*l.p2.x-*l.p1.x,dy=*l.p2.y-*l.p1.y,length=std::hypot(dx,dy);
    require(length>=minimumLength,"Point-edge dimension needs a nondegenerate straight line");
    return (dx*(*p.y-*l.p1.y)-dy*(*p.x-*l.p1.x))/length;
}
bool sharesEndpointId(const QJsonArray& entities,const QString& first,const QString& second) {
    QJsonObject a,b;
    for(const auto& value:entities){const auto entity=value.toObject();
        if(entity["id"]==first)a=entity;else if(entity["id"]==second)b=entity;}
    require(!a.isEmpty() && !b.isEmpty(),"Collinear line references are unavailable");
    return a["startPointId"]==b["startPointId"] || a["startPointId"]==b["endPointId"]
        || a["endPointId"]==b["startPointId"] || a["endPointId"]==b["endPointId"];
}
double separation(const QString& kind, const GCS::Point& a, const GCS::Point& b) {
    if (kind == "horizontal") return std::abs(*b.x - *a.x);
    if (kind == "vertical") return std::abs(*b.y - *a.y);
    require(kind == "linear", "Unsupported dimension kind");
    return std::hypot(*b.x - *a.x, *b.y - *a.y);
}
}

SketchSolver::Result SketchSolver::solve(const QJsonObject& parameters, const QSet<QString>& lockedPointIds) {
    Result result;
    result.parameters = parameters;
    result.diagnostics = {{"solver", "PlaneGCS"}, {"status", "invalid"}, {"dof", -1},
        {"conflictingConstraintIds", QJsonArray{}}, {"redundantConstraintIds", QJsonArray{}},
        {"conflictingEntityIds",QJsonArray{}},{"redundantEntityIds",QJsonArray{}},
        {"dimensionValues", QJsonArray{}}, {"maxResidual", QJsonValue::Null},
        {"lengthToleranceMm", lengthTolerance}, {"angularTolerance", angularTolerance}};
    try {
        require(parameters.value("points").isArray() && parameters.value("entities").isArray(), "Missing sketch points/entities");
        auto points = parameters.value("points").toArray();
        auto entities = parameters.value("entities").toArray();
        const auto constraints = parameters.value("constraints").toArray();
        const auto dimensions = parameters.value("dimensions").toArray();
        // Radius/sweep edits seed the requested analytic geometry without fixing
        // any free center or rotation. Shared endpoint line intent follows the seed;
        // all authored constraints still participate in the authoritative solve.
        for(const auto& value : dimensions) {
            const auto d=value.toObject(); const auto kind=d["kind"].toString();
            if(!d["driving"].toBool() || (kind!="radius" && kind!="diameter" && kind!="arcSweep")) continue;
            for(qsizetype i=0;i<entities.size();++i) {auto e=entities[i].toObject(); if(e["id"]!=d["entityId"])continue;
                const double requested=number(d,kind=="arcSweep" ? "valueDeg" : "valueMm")/(kind=="diameter" ? 2 : 1);
                const double existing=kind=="arcSweep" ? std::abs(number(e,"sweepAngleDeg")) : number(e,"radiusMm");
                if(std::abs(requested-existing)<1e-10)continue;
                if(kind=="arcSweep") e["sweepAngleDeg"]=(e["sweepAngleDeg"].toDouble()<0 ? -1 : 1)*number(d,"valueDeg");
                else e["radiusMm"]=number(d,"valueMm")/(kind=="diameter" ? 2 : 1);
                entities[i]=e;
                if(e["type"]!="arc")continue;
                double u=0,v=0; for(const auto& pv:points) {const auto p=pv.toObject();if(p["id"]==e["centerPointId"]){u=number(p,"uMm");v=number(p,"vMm");}}
                for(qsizetype j=0;j<points.size();++j){auto p=points[j].toObject();if(p["id"]!=e["startPointId"] && p["id"]!=e["endPointId"])continue;
                    const double angle=(e["startAngleDeg"].toDouble()+(p["id"]==e["endPointId"] ? e["sweepAngleDeg"].toDouble() : 0))*degrees;
                    p["uMm"]=u+e["radiusMm"].toDouble()*std::cos(angle);p["vMm"]=v+e["radiusMm"].toDouble()*std::sin(angle);points[j]=p;}
            }
        }
        if(parameters.contains("arcSlotGroups")) {
            const auto capPrimary=SketchArcSlot::capPrimaryIds(parameters);
            for(const auto& value:dimensions) {
                const auto d=value.toObject();
                const auto capId=d["entityId"].toString();
                if(!d["driving"].toBool() || !capPrimary.contains(capId) ||
                    (d["kind"]!="radius" && d["kind"]!="diameter"))continue;
                const double requested=number(d,"valueMm")/(d["kind"]=="diameter" ? 2 : 1);
                for(int i=0;i<entities.size();++i)if(entities[i].toObject()["id"]==capPrimary.value(capId)){
                    auto cap=entities[i].toObject();cap["radiusMm"]=requested;entities[i]=cap;break;
                }
            }
            auto seeded=SketchArcSlot::derive(QJsonObject{{"points",points},{"entities",entities},
                {"arcSlotGroups",parameters["arcSlotGroups"]}});
            points=seeded["points"].toArray();entities=seeded["entities"].toArray();
        }
        // The authored source circle owns its radius. Seed each retained result
        // from that source before PlaneGCS solves; editing the offset must not
        // make the least-norm step resize an otherwise free source circle.
        for(const auto& value:constraints) {
            const auto relation=value.toObject();if(relation["type"]!="circleOffset")continue;
            const auto refs=relation["entityIds"].toArray();
            require(refs.size()==2,"Circle offset requires source and result");
            double sourceRadius=0;bool foundSource=false,foundTarget=false;
            for(const auto& entity:entities)if(entity.toObject()["id"]==refs[0]) {
                require(entity.toObject()["type"]=="circle","Circle offset source must be a circle");
                sourceRadius=number(entity.toObject(),"radiusMm");foundSource=true;break;
            }
            require(foundSource,"Circle offset source no longer exists");
            const double targetRadius=sourceRadius+number(relation,"distanceMm");
            require(targetRadius>=minimumLength && targetRadius<=maximumCoordinate,"Circle offset would create an invalid radius");
            for(qsizetype i=0;i<entities.size();++i)if(entities[i].toObject()["id"]==refs[1]) {
                auto target=entities[i].toObject();require(target["type"]=="circle","Circle offset result must be a circle");
                target["radiusMm"]=targetRadius;entities[i]=target;foundTarget=true;break;
            }
            require(foundTarget,"Circle offset result no longer exists");
        }
        QSet<QString> diameterStartIds;
        for(const auto& value:entities) {
            const auto circle=value.toObject();
            if(circle["type"]!="circle" || !circle.contains("diameterStartPointId"))continue;
            const auto startId=circle["diameterStartPointId"].toString(),centerId=circle["centerPointId"].toString();
            diameterStartIds.insert(startId);
            QJsonObject start;int centerIndex=-1;
            for(qsizetype i=0;i<points.size();++i) {
                const auto p=points[i].toObject();
                if(p["id"]==startId)start=p;
                if(p["id"]==centerId)centerIndex=i;
            }
            require(!start.isEmpty() && centerIndex>=0,"Two-point circle endpoint or center is unavailable");
            auto center=points[centerIndex].toObject();
            center["uMm"]=number(start,"uMm")+number(circle,"diameterDirectionU")*number(circle,"radiusMm");
            center["vMm"]=number(start,"vMm")+number(circle,"diameterDirectionV")*number(circle,"radiusMm");
            points[centerIndex]=center;
        }
        if(parameters.contains("mirrorGroups")){
            auto seeded=SketchMirror::seed({{"points",points},{"entities",entities},{"mirrorGroups",parameters["mirrorGroups"]}});
            points=seeded["points"].toArray();entities=seeded["entities"].toArray();
        }
        if(parameters.contains("polygonGroups")){
            auto intent=parameters;intent["points"]=points;intent["entities"]=entities;
            const auto seeded=SketchPolygon::seed(intent);points=seeded["points"].toArray();
        }
        // All storage is allocated before pointers enter the solver. std::map
        // nodes and deque elements retain addresses as their containers grow.
        std::vector<double> coordinates(static_cast<size_t>(points.size()) * 2);
        std::map<QString, GCS::Point> pointMap;
        std::map<QString, GCS::Line> lineMap;
        std::map<QString,GCS::Circle> circleMap;
        std::map<QString,GCS::Arc> arcMap;
        std::deque<double> curveParameters;
        std::deque<double> constants;
        GCS::VEC_pD unknowns;
        QMap<int, QString> tagIds;
        QMap<QString,QString> mirrorSources;
        const auto derivedArcIds=SketchArcSlot::derivedArcIds(parameters);
        auto derivedPointIds=SketchArcSlot::derivedPointIds(parameters);
        for(const auto& entry:entities){const auto entity=entry.toObject();if(entity["type"]!="fitSplineOffset")continue;
            derivedPointIds.insert(entity["startPointId"].toString());derivedPointIds.insert(entity["endPointId"].toString());}
        const auto capPrimary=SketchArcSlot::capPrimaryIds(parameters);
        for(const auto& groupValue:parameters["mirrorGroups"].toArray())for(const auto& pairValue:groupValue.toObject()["entityPairs"].toArray()){
            const auto pair=pairValue.toObject();mirrorSources[pair["resultEntityId"].toString()]=pair["sourceEntityId"].toString();
        }
        QSet<QString> ids;
        for (qsizetype i = 0; i < points.size(); ++i) {
            const auto point = points[i].toObject();
            const auto id = point.value("id").toString();
            require(!id.isEmpty() && !ids.contains(id), "Duplicate or empty sketch point ID");
            ids.insert(id);
            coordinates[2*i] = number(point, "uMm"); coordinates[2*i+1] = number(point, "vMm");
            require(std::abs(coordinates[2*i]) <= maximumCoordinate && std::abs(coordinates[2*i+1]) <= maximumCoordinate, "Point outside supported range");
            pointMap.emplace(id, GCS::Point(&coordinates[2*i], &coordinates[2*i+1]));
            if(!lockedPointIds.contains(id) && !diameterStartIds.contains(id) && !derivedPointIds.contains(id)){
                unknowns.push_back(&coordinates[2*i]); unknowns.push_back(&coordinates[2*i+1]);}
        }
        auto point = [&](const QString& id) -> GCS::Point& {
            const auto found = pointMap.find(id);
            require(found != pointMap.end(), "Unknown sketch point reference"); return found->second;
        };
        for (const auto& entry : entities) {
            const auto entity = entry.toObject();
            const auto id = entity.value("id").toString();
            const auto type=entity["type"].toString();
            require((type=="line" || type=="circle" || type=="arc" || type=="point" || type=="fitSpline" || type=="fitSplineOffset" || type=="controlSpline" || type=="conic" || type=="ellipse") && !id.isEmpty() && !ids.contains(id), "Unsupported or duplicate sketch entity");
            ids.insert(id);
            if(type=="point") {point(entity["pointId"].toString());continue;}
            if(type=="fitSpline"||type=="controlSpline" || type=="conic"){for(const auto& id:entity["pointIds"].toArray())point(id.toString());continue;}
            if(type=="fitSplineOffset"){point(entity["startPointId"].toString());point(entity["endPointId"].toString());continue;}
            if(type=="ellipse"){point(entity["centerPointId"].toString());continue;}
            if(type=="line") {GCS::Line line;line.p1=point(entity["startPointId"].toString());line.p2=point(entity["endPointId"].toString());lineMap.emplace(id,line);}
            else {
                curveParameters.push_back(number(entity,"radiusMm"));auto* radius=&curveParameters.back();
                const bool derivedArc=type=="arc" && (mirrorSources.contains(id) || derivedArcIds.contains(id));
                if(!derivedArc || capPrimary.value(id)==id)unknowns.push_back(radius);
                if(type=="circle") {GCS::Circle circle;circle.center=point(entity["centerPointId"].toString());circle.rad=radius;circleMap.emplace(id,circle);}
                else {GCS::Arc arc;arc.center=point(entity["centerPointId"].toString());arc.rad=radius;
                    arc.start=point(entity["startPointId"].toString());arc.end=point(entity["endPointId"].toString());
                    curveParameters.push_back(number(entity,"startAngleDeg")*degrees);arc.startAngle=&curveParameters.back();if(!derivedArc)unknowns.push_back(arc.startAngle);
                    curveParameters.push_back((number(entity,"startAngleDeg")+number(entity,"sweepAngleDeg"))*degrees);arc.endAngle=&curveParameters.back();if(!derivedArc)unknowns.push_back(arc.endAngle);
                    arcMap.emplace(id,arc);}
            }
        }
        for(auto it=capPrimary.cbegin();it!=capPrimary.cend();++it)if(it.key()!=it.value())
            arcMap.at(it.key()).rad=arcMap.at(it.value()).rad;
        auto line = [&](const QString& id) -> GCS::Line& {
            const auto found = lineMap.find(id);
            require(found != lineMap.end(), "Unknown sketch line reference"); return found->second;
        };
        auto radius = [&](const QString& id) -> double* {
            if (const auto found = circleMap.find(id); found != circleMap.end()) return found->second.rad;
            if (const auto found = arcMap.find(id); found != arcMap.end()) return found->second.rad;
            require(false,"Unknown sketch circle/arc reference");return nullptr;
        };
        auto raySigns = [&](const QJsonObject& dimension) {
            std::array<int,2> signs{1,1};
            if (dimension.contains("raySigns")) {
                const auto values=dimension["raySigns"].toArray();
                require(dimension["raySigns"].isArray() && values.size()==2,"Angle requires two ray signs");
                for(int i=0;i<2;++i) {require(values[i].isDouble() && (values[i].toDouble()==1 || values[i].toDouble()==-1),"Angle ray sign must be +1 or -1");signs[i]=values[i].toInt();}
            }
            return signs;
        };
        auto signedLineAngle = [&](const QJsonObject& dimension) {
            const auto& a=line(reference(dimension,"entityIds",0));const auto& b=line(reference(dimension,"entityIds",1));
            const auto signs=raySigns(dimension);
            const double ax=(*a.p2.x-*a.p1.x)*signs[0],ay=(*a.p2.y-*a.p1.y)*signs[0];
            const double bx=(*b.p2.x-*b.p1.x)*signs[1],by=(*b.p2.y-*b.p1.y)*signs[1];
            return std::atan2(ax*by-ay*bx,ax*bx+ay*by);
        };
        auto constant = [&](double value) { constants.push_back(value); return &constants.back(); };
        auto tag = [&](const QJsonObject& item) {
            const auto id = item.value("id").toString();
            require(!id.isEmpty() && !ids.contains(id), "Duplicate or empty constraint/dimension ID");
            ids.insert(id); const int value = tagIds.size()+1; tagIds[value] = id; return value;
        };
        GCS::System solver;
        // A pivot-selected particular step can erase unconstrained geometry
        // (for example a free cap length while adding a line angle). PlaneGCS's
        // least-norm step changes the constrained directions without adding
        // motion in the Jacobian nullspace. No extra authored constraint is added.
        solver.dogLegGaussStep = GCS::LeastNormFullPivLU;
        result.diagnostics["dogLegStep"] = "least_norm_full_piv_lu";
        QSet<int> internalTags;int internalRuleCount=0;
        for(auto& [id,arc]:arcMap) {if(mirrorSources.contains(id) || derivedArcIds.contains(id))continue;const int t=tagIds.size()+1;tagIds[t]=id;internalTags.insert(t);solver.addConstraintArcRules(arc,t);internalRuleCount+=4;}
        for(const auto& value:entities) {
            const auto e=value.toObject();if(e["type"]!="circle" || !e.contains("diameterStartPointId"))continue;
            const auto id=e["id"].toString();auto& center=point(e["centerPointId"].toString());
            auto& start=point(e["diameterStartPointId"].toString());auto* radius=circleMap.at(id).rad;
            const int t=tagIds.size()+1;tagIds[t]=id;internalTags.insert(t);
            solver.addConstraintP2PDistance(start,center,radius,t);
            solver.addConstraintP2PAngle(start,center,constant(std::atan2(number(e,"diameterDirectionV"),number(e,"diameterDirectionU"))),t);
            internalRuleCount+=2;
        }
        for (const auto& entry : constraints) {
            const auto constraint = entry.toObject();
            const auto type = constraint.value("type").toString(); const int t = tag(constraint);
            if (type == "coincident") {
                solver.addConstraintP2PCoincident(point(reference(constraint,"pointIds",0)), point(reference(constraint,"pointIds",1)), t);
            } else if(type=="polygonAffineVertex") {
                auto& a=point(reference(constraint,"pointIds",0));auto& b=point(reference(constraint,"pointIds",1));auto& v=point(reference(constraint,"pointIds",2));
                const double x=number(constraint,"coefficientU"),y=number(constraint,"coefficientV");
                auto equation=[&](double* q,const std::vector<double>& factors){
                    std::vector<double*> params{q,a.x,a.y,b.x,b.y};for(int i=0;i<4;++i)params.push_back(constant(1));
                    auto* c=new GCS::ConstraintWeightedLinearCombination(4,params,factors);c->setTag(t);solver.addConstraint(c);
                };
                equation(v.x,{1-x,y,x,-y});equation(v.y,{-y,1-x,y,x});
            } else if(type=="polygonVertex") {
                auto& anchor=point(reference(constraint,"pointIds",0));auto& vertex=point(reference(constraint,"pointIds",1));
                solver.addConstraintDifference(anchor.x,vertex.x,constant(number(constraint,"offsetUMm")),t);
                solver.addConstraintDifference(anchor.y,vertex.y,constant(number(constraint,"offsetVMm")),t);
            } else if(type=="pointOnCurve" || type=="supportLine") {
                const auto id=reference(constraint,"entityIds",0);auto& p=point(reference(constraint,"pointIds",0));
                if(lineMap.contains(id))solver.addConstraintPointOnLine(p,line(id),t);else solver.addConstraintPointOnCircle(p,circleMap.at(id),t);
            } else if (type == "fixed") {
                auto& p = point(reference(constraint,"pointIds",0));
                solver.addConstraintEqual(p.x,constant(number(constraint,"uMm")),t);
                solver.addConstraintEqual(p.y,constant(number(constraint,"vMm")),t);
            } else if(type=="circleTangent") {
                auto& a=circleMap.at(reference(constraint,"entityIds",0));auto& b=circleMap.at(reference(constraint,"entityIds",1));
                solver.addConstraintTangentCircumf(a.center,b.center,a.rad,b.rad,number(constraint,"side")<0,t);
            } else if (type == "arcTangent") {
                // Shared contact and arc rules already retain the radius. A radial
                // perpendicular gives tangency full first-order rank at contact.
                solver.addConstraintPerpendicular(arcMap.at(reference(constraint,"entityIds",0)).center,
                    point(reference(constraint,"pointIds",0)),line(reference(constraint,"entityIds",1)),t);
            } else if (type == "tangent") {
                auto& circle=circleMap.at(reference(constraint,"entityIds",0));
                if(constraint.contains("pointIds"))
                    solver.addConstraintPerpendicular(circle.center,point(reference(constraint,"pointIds",0)),
                        line(reference(constraint,"entityIds",1)),t);
                else solver.addConstraintTangent(line(reference(constraint,"entityIds",1)),circle,number(constraint,"side")>0,t);
            } else if (type == "midpoint") {
                const auto id=reference(constraint,"entityIds",0);
                auto& p=point(reference(constraint,"pointIds",0));
                if(lineMap.contains(id)){
                    auto& target=line(id);solver.addConstraintP2PSymmetric(target.p1,target.p2,p,t);
                }else{
                    auto& arc=arcMap.at(id);
                    solver.addConstraintPointOnArc(p,arc,t);
                    solver.addConstraintPointOnPerpBisector(p,arc.start,arc.end,t);
                }
            } else if(type=="concentric") {
                auto center=[&](const QString& id) -> GCS::Point& {
                    if(auto found=circleMap.find(id);found!=circleMap.end())return found->second.center;
                    if(auto found=arcMap.find(id);found!=arcMap.end())return found->second.center;
                    require(false,"Concentric requires two circles or arcs");return point(id);
                };
                solver.addConstraintP2PCoincident(center(reference(constraint,"entityIds",0)),center(reference(constraint,"entityIds",1)),t);
            } else if(type=="symmetry") {
                auto& axis=line(reference(constraint,"entityIds",2));
                solver.addConstraintP2PSymmetric(point(reference(constraint,"pointIds",0)),point(reference(constraint,"pointIds",1)),axis,t);
                if(constraint["pointIds"].toArray().size()==6){
                    // Centers and one endpoint reflect; a shared signed sweep supplies
                    // the remaining angular equation without redundant ArcRules.
                    solver.addConstraintP2PSymmetric(point(reference(constraint,"pointIds",4)),point(reference(constraint,"pointIds",5)),axis,t);
                    auto& first=arcMap.at(reference(constraint,"entityIds",0));
                    auto& second=arcMap.at(reference(constraint,"entityIds",1));
                    curveParameters.push_back(*first.endAngle-*first.startAngle);auto* sweep=&curveParameters.back();unknowns.push_back(sweep);
                    solver.addConstraintDifference(first.startAngle,first.endAngle,sweep,t);
                    const bool sameDirection=(*first.endAngle-*first.startAngle)*(*second.endAngle-*second.startAngle)>0;
                    solver.addConstraintDifference(sameDirection ? second.startAngle : second.endAngle,sameDirection ? second.endAngle : second.startAngle,sweep,t);
                }else solver.addConstraintP2PSymmetric(point(reference(constraint,"pointIds",2)),point(reference(constraint,"pointIds",3)),axis,t);
            } else if (type == "horizontal") {
                if(constraint.contains("pointIds"))solver.addConstraintEqual(point(reference(constraint,"pointIds",0)).y,point(reference(constraint,"pointIds",1)).y,t);
                else solver.addConstraintHorizontal(line(reference(constraint,"entityIds",0)),t);
            } else if (type == "vertical") {
                if(constraint.contains("pointIds"))solver.addConstraintEqual(point(reference(constraint,"pointIds",0)).x,point(reference(constraint,"pointIds",1)).x,t);
                else solver.addConstraintVertical(line(reference(constraint,"entityIds",0)),t);
            } else if (type == "parallel") {
                solver.addConstraintParallel(line(reference(constraint,"entityIds",0)),line(reference(constraint,"entityIds",1)),t);
            } else if (type == "collinear") {
                const auto first=reference(constraint,"entityIds",0),second=reference(constraint,"entityIds",1);
                solver.addConstraintParallel(line(first),line(second),t);
                if(!sharesEndpointId(entities,first,second))solver.addConstraintPointOnLine(line(second).p1,line(first),t);
            } else if (type == "perpendicular") {
                solver.addConstraintPerpendicular(line(reference(constraint,"entityIds",0)),line(reference(constraint,"entityIds",1)),t);
            } else if (type == "equal") {
                const auto a = reference(constraint,"entityIds",0), b = reference(constraint,"entityIds",1);
                require(a != b,"Equal requires distinct sketch entities");
                if (lineMap.contains(a) && lineMap.contains(b)) solver.addConstraintEqualLength(line(a),line(b),t);
                else solver.addConstraintEqual(radius(a),radius(b),t);
            } else if(type=="circleOffset") {
                solver.addConstraintDifference(radius(reference(constraint,"entityIds",0)),
                    radius(reference(constraint,"entityIds",1)),constant(number(constraint,"distanceMm")),t);
            } else require(false,"Unsupported sketch constraint");
        }
        // A mirror relation uses one point-pair symmetry equation per distinct
        // source point. Arc center/endpoints and ArcRules imply equal radii, so
        // an additional radius equation would be redundant.
        for(const auto& groupValue:parameters["mirrorGroups"].toArray()){
            const auto group=groupValue.toObject();auto& axis=line(group["axisEntityId"].toString());
            for(const auto& pairValue:group["pointPairs"].toArray()){
                const auto pair=pairValue.toObject();const int t=tagIds.size()+1;tagIds[t]=group["id"].toString();
                solver.addConstraintP2PSymmetric(point(pair["sourcePointId"].toString()),point(pair["resultPointId"].toString()),axis,t);
            }
        }
        for (const auto& entry : dimensions) {
            const auto dimension = entry.toObject();
            const auto id = dimension.value("id").toString();
            require(!id.isEmpty() && !ids.contains(id), "Duplicate or empty dimension ID");
            const auto kind=dimension["kind"].toString();
            const bool curve=kind=="radius" || kind=="diameter" || kind=="arcSweep";
            const bool angle=kind=="angle" || kind=="overallSlotAngle";
            require(curve || angle || kind=="overallSlotLength" || kind=="centerlineDiameter" || kind=="parallelLineDistance" || kind=="pointLineDistance" || kind=="linear" || kind=="horizontal" || kind=="vertical", "Unsupported dimension kind");
            require(dimension["driving"].isBool(), "Dimension requires driving flag");
            if(kind=="angle") {require(dimension["entityIds"].isArray() && dimension["entityIds"].toArray().size()==2 && reference(dimension,"entityIds",0)!=reference(dimension,"entityIds",1),"Angle requires two distinct line IDs");signedLineAngle(dimension);}
            if(!dimension["driving"].toBool()) {ids.insert(id);continue;}
            const double value=number(dimension,kind=="arcSweep" || angle ? "valueDeg" : "valueMm");
            require(kind=="arcSweep" ? value>=1e-6 && value<360 : kind=="overallSlotAngle" ? std::abs(value)<=360 : angle ? value>=1e-6 && value<180 : value>=minimumLength && value<=maximumCoordinate, "Driving dimension outside supported range");
            const int t=tag(dimension);
            if(kind=="overallSlotAngle") {
                solver.addConstraintP2PAngle(point(reference(dimension,"pointIds",0)),point(reference(dimension,"pointIds",1)),constant(value*degrees),t);
            } else if(kind=="angle") {
                const auto signs=raySigns(dimension);
                const double signedTarget=(signedLineAngle(dimension)<0 ? -1 : 1)*value*degrees;
                const double rayOffset=((signs[1]<0 ? 1 : 0)-(signs[0]<0 ? 1 : 0))*std::numbers::pi;
                const double target=std::remainder(signedTarget-rayOffset,2*std::numbers::pi);
                solver.addConstraintL2LAngle(line(reference(dimension,"entityIds",0)),line(reference(dimension,"entityIds",1)),constant(target),t);
            } else if(kind=="overallSlotLength") {
                const auto arcId=dimension["entityId"].toString();double width=0;
                for(const auto& row:dimensions){const auto d=row.toObject();if(d["kind"]=="diameter" && d["entityId"]==arcId && d["driving"].toBool()){width=number(d,"valueMm");break;}}
                require(width>=minimumLength && value-width>=minimumLength,"Overall Slot length must exceed its width");
                solver.addConstraintP2PDistance(point(reference(dimension,"pointIds",0)),point(reference(dimension,"pointIds",1)),constant(value-width),t);
            } else if(kind=="centerlineDiameter") {
                auto& axis=line(reference(dimension,"entityIds",0));auto& edge=line(reference(dimension,"entityIds",1));
                require(dimension["side"].toInt()==1 || dimension["side"].toInt()==-1,"Centerline diameter requires retained side");
                // Parallelism is retained by independent sketch intent, not a
                // hidden relation. The diameter contributes one distance DOF.
                solver.addConstraintP2LDistance(edge.p1,axis,constant(value/2),dimension["side"].toInt()>0,t);
            } else if(kind=="parallelLineDistance") {
                auto& first=line(reference(dimension,"entityIds",0));auto& second=line(reference(dimension,"entityIds",1));
                require(dimension["side"].toInt()==1 || dimension["side"].toInt()==-1,"Parallel distance requires retained side");
                // One perpendicular-distance equation; parallelism must come from authored line relations.
                solver.addConstraintP2LDistance(second.p1,first,constant(value),dimension["side"].toInt()>0,t);
            } else if(kind=="pointLineDistance") {
                auto& p=point(reference(dimension,"pointIds",0));auto& l=line(dimension["entityId"].toString());pointLineDistance(p,l);
                require(dimension["side"].toInt()==1 || dimension["side"].toInt()==-1,"Point-edge dimension needs retained side");
                solver.addConstraintP2LDistance(p,l,constant(value),dimension["side"].toInt()==1,t);
            } else if(curve) {
                const auto entityId=dimension["entityId"].toString();
                if(arcMap.contains(entityId)) {auto& arc=arcMap.at(entityId);
                    if(kind=="radius")solver.addConstraintArcRadius(arc,constant(value),t);
                    else if(kind=="diameter")solver.addConstraintArcDiameter(arc,constant(value),t);
                    else solver.addConstraintDifference(arc.startAngle,arc.endAngle,constant((*arc.endAngle<*arc.startAngle ? -1 : 1)*value*degrees),t);
                } else {require(kind!="arcSweep" && circleMap.contains(entityId), "Unknown curve dimension reference");auto& circle=circleMap.at(entityId);
                    if(kind=="radius")solver.addConstraintCircleRadius(circle,constant(value),t);else solver.addConstraintCircleDiameter(circle,constant(value),t);}
            } else {
                auto& a=point(reference(dimension,"pointIds",0));auto& b=point(reference(dimension,"pointIds",1));
                if(kind=="linear")solver.addConstraintP2PDistance(a,b,constant(value),t);
                else {double* first=kind=="horizontal" ? a.x : a.y;double* second=kind=="horizontal" ? b.x : b.y;
                    solver.addConstraintDifference(first,second,constant(*second<*first ? -value : value),t);}
            }
        }
        result.diagnostics["constraintCount"] = tagIds.size()-internalTags.size();
        result.diagnostics["internalCurveRuleCount"] = internalRuleCount;
        int dof = static_cast<int>(unknowns.size());
        if (!tagIds.isEmpty()) {
            solver.declareUnknowns(unknowns); solver.initSolution(GCS::DogLeg);
            const auto status = solver.solve(GCS::DogLeg);
            GCS::VEC_I conflicts,redundant,partial;
            solver.getConflicting(conflicts); solver.getRedundant(redundant); solver.getPartiallyRedundant(partial);
            auto mapped = [&](const GCS::VEC_I& values,bool internal=false) {
                QJsonArray array; QSet<QString> emitted;
                for (int value : values) if (tagIds.contains(value) && internalTags.contains(value)==internal && !emitted.contains(tagIds[value])) {
                    array.append(tagIds[value]); emitted.insert(tagIds[value]);
                }
                return array;
            };
            redundant.insert(redundant.end(),partial.begin(),partial.end());
            QSet<QString> concentricIds;for(const auto& value:constraints)if(value.toObject()["type"]=="concentric")concentricIds.insert(value.toObject()["id"].toString());
            bool scopedConcentricRedundancy=!redundant.empty() && conflicts.empty() && status==GCS::SolveStatus::Success && !concentricIds.isEmpty();
            if(scopedConcentricRedundancy) {
                // PlaneGCS may tag an older line relation, rather than the new
                // Concentric equation, as redundant in the same rank cycle.
                // Remove only Concentric in a separate diagnostic solve. If the
                // remaining authored system is clean, the rank loss depends on
                // Concentric; all equations still participate below and their
                // independently checked geometric residuals must pass.
                auto base=parameters;QJsonArray kept;
                for(const auto& value:constraints)if(value.toObject()["type"]!="concentric")kept.append(value);
                base["constraints"]=kept;
                const auto baseline=SketchSolver::solve(base,lockedPointIds);
                scopedConcentricRedundancy=baseline.accepted;
            }
            if(scopedConcentricRedundancy)result.diagnostics["acceptedConcentricDependentRedundancyIds"]=mapped(redundant);
            bool scopedReferenceCoincidence=false;
            QString referenceCoincidenceId;
            if(!redundant.empty() && conflicts.empty() && status==GCS::SolveStatus::Success) {
                int referenceCoincidenceCandidates=0;
                QSet<QString> horizontalIds,angleReferenceIds,anchoredIds=lockedPointIds;
                for(const auto& value:constraints) {
                    const auto c=value.toObject();
                    if(c["type"]=="horizontal" && c["entityIds"].isArray())horizontalIds.insert(reference(c,"entityIds",0));
                    if(c["type"]=="coincident") {
                        const auto a=reference(c,"pointIds",0),b=reference(c,"pointIds",1);
                        if(lockedPointIds.contains(a))anchoredIds.insert(b);
                        if(lockedPointIds.contains(b))anchoredIds.insert(a);
                    }
                }
                for(const auto& value:dimensions) {
                    const auto d=value.toObject();
                    if(d["kind"]=="angle" && d["driving"].toBool())
                        angleReferenceIds.insert(reference(d,"entityIds",0));
                }
                for(const auto& value:constraints) {
                    const auto c=value.toObject();if(c["type"]!="coincident")continue;
                    const auto a=reference(c,"pointIds",0),b=reference(c,"pointIds",1);
                    for(const auto& entityValue:entities) {
                        const auto e=entityValue.toObject();const auto id=e["id"].toString();
                        if(e["type"]!="line" || !e["construction"].toBool() ||
                           !horizontalIds.contains(id) || !angleReferenceIds.contains(id))continue;
                        const auto start=e["startPointId"].toString(),end=e["endPointId"].toString();
                        if(!anchoredIds.contains(start) || lockedPointIds.contains(end))continue;
                        if(!((a==end && lockedPointIds.contains(b)) || (b==end && lockedPointIds.contains(a))))continue;
                        ++referenceCoincidenceCandidates;
                        referenceCoincidenceId=c["id"].toString();
                    }
                }
                if(referenceCoincidenceCandidates==1 && !referenceCoincidenceId.isEmpty()) {
                    auto base=parameters;QJsonArray kept;
                    for(const auto& value:constraints)if(value.toObject()["id"]!=referenceCoincidenceId)kept.append(value);
                    base["constraints"]=kept;
                    const auto baseline=SketchSolver::solve(base,lockedPointIds);
                    // The horizontal helper already supplies the vertical
                    // component of this corner coincidence. Accept that single
                    // dependent equation only if the corner removes exactly one
                    // free DOF; the full coincidence residual is checked below.
                    scopedReferenceCoincidence=baseline.accepted &&
                        baseline.diagnostics["dof"].toInt()==solver.dofsNumber()+1;
                }
            }
            if(scopedReferenceCoincidence)
                result.diagnostics["acceptedReferenceCoincidenceId"]=referenceCoincidenceId;
            result.diagnostics["conflictingConstraintIds"] = mapped(conflicts);
            result.diagnostics["redundantConstraintIds"] = mapped(redundant);
            result.diagnostics["conflictingEntityIds"] = mapped(conflicts,true);
            result.diagnostics["redundantEntityIds"] = mapped(redundant,true);
            result.diagnostics["solverStatus"] = static_cast<int>(status);
            dof = solver.dofsNumber(); result.diagnostics["dof"] = dof;
            if (!conflicts.empty() || (!redundant.empty() && !scopedConcentricRedundancy && !scopedReferenceCoincidence) || status != GCS::SolveStatus::Success) {
                result.diagnostics["status"] = !conflicts.empty() ? "conflicting" : !redundant.empty() ? "redundant" : "failed";
                result.error = "Sketch solve rejected: " + result.diagnostics.value("status").toString(); return result;
            }
            solver.applySolution();
        }
        // Result arcs are an analytic consequence of source arc plus mirrored
        // center/endpoints. Repeating ArcRules on them is partially redundant.
        for(const auto& groupValue:parameters["mirrorGroups"].toArray()){
            const auto group=groupValue.toObject();const auto& axis=line(group["axisEntityId"].toString());
            const double angle=std::atan2(*axis.p2.y-*axis.p1.y,*axis.p2.x-*axis.p1.x);
            for(const auto& pairValue:group["entityPairs"].toArray()){
                const auto pair=pairValue.toObject();const auto sourceId=pair["sourceEntityId"].toString(),resultId=pair["resultEntityId"].toString();
                if(!arcMap.contains(resultId))continue;
                const auto& source=arcMap.at(sourceId);auto& target=arcMap.at(resultId);
                *target.rad=*source.rad;*target.startAngle=2*angle-*source.startAngle;*target.endAngle=2*angle-*source.endAngle;
            }
        }
        if(parameters.contains("arcSlotGroups")) {
            QJsonArray solvedPoints,solvedEntities;
            for(int i=0;i<points.size();++i){auto p=points[i].toObject();p["uMm"]=coordinates[2*i];p["vMm"]=coordinates[2*i+1];solvedPoints.append(p);}
            for(const auto& value:entities){auto e=value.toObject();const auto id=e["id"].toString();
                if(e["type"]=="arc"){
                    const auto& arc=arcMap.at(id);e["radiusMm"]=*arc.rad;
                    e["startAngleDeg"]=*arc.startAngle/degrees;
                    e["sweepAngleDeg"]=(*arc.endAngle-*arc.startAngle)/degrees;
                }
                solvedEntities.append(e);
            }
            const auto derived=SketchArcSlot::derive(QJsonObject{{"points",solvedPoints},{"entities",solvedEntities},
                {"arcSlotGroups",parameters["arcSlotGroups"]}});
            for(const auto& value:derived["points"].toArray()){
                const auto p=value.toObject();const auto id=p["id"].toString();if(!derivedPointIds.contains(id))continue;
                auto& target=pointMap.at(id);*target.x=number(p,"uMm");*target.y=number(p,"vMm");
            }
            for(const auto& value:derived["entities"].toArray()){
                const auto e=value.toObject();const auto id=e["id"].toString();if(!derivedArcIds.contains(id))continue;
                auto& target=arcMap.at(id);*target.rad=number(e,"radiusMm");
                *target.startAngle=number(e,"startAngleDeg")*degrees;
                *target.endAngle=(number(e,"startAngleDeg")+number(e,"sweepAngleDeg"))*degrees;
            }
        }
        // Independent checks avoid accepting an algorithm status with invalid
        // coordinates or a collapsed edge. Residuals below use geometry, not the
        // solver's differently scaled per-constraint error functions.
        for (double value : coordinates) require(std::isfinite(value) && std::abs(value) <= maximumCoordinate, "Solved coordinate outside supported range");
        for (const auto& [id,l] : lineMap) {
            const double length = separation("linear",l.p1,l.p2);
            require(length >= minimumLength && length <= maximumCoordinate, "Solve collapsed a line or exceeded its supported range");
        }
        double maxResidual = 0;
        auto check = [&](double residual,double tolerance) {
            require(std::isfinite(residual), "Nonfinite constraint residual");
            maxResidual = std::max(maxResidual,std::abs(residual));
            require(std::abs(residual) <= tolerance, "Independent geometric residual exceeds tolerance");
        };
        for(const auto& value:entities) {
            const auto e=value.toObject();if(e["type"]!="circle" || !e.contains("diameterStartPointId"))continue;
            const auto& center=point(e["centerPointId"].toString()),&start=point(e["diameterStartPointId"].toString());
            const double radius=*circleMap.at(e["id"].toString()).rad;
            check(*center.x-*start.x-number(e,"diameterDirectionU")*radius,lengthTolerance);
            check(*center.y-*start.y-number(e,"diameterDirectionV")*radius,lengthTolerance);
        }
        for (const auto& entry : constraints) {
            const auto c=entry.toObject(); const auto type=c.value("type").toString();
            if(type=="coincident") check(separation("linear",point(reference(c,"pointIds",0)),point(reference(c,"pointIds",1))),lengthTolerance);
            else if(type=="pointOnCurve" || type=="supportLine") {
                const auto id=reference(c,"entityIds",0);const auto& p=point(reference(c,"pointIds",0));
                if(lineMap.contains(id)){const auto& l=line(id);double dx=*l.p2.x-*l.p1.x,dy=*l.p2.y-*l.p1.y,len=std::hypot(dx,dy);check((dx*(*p.y-*l.p1.y)-dy*(*p.x-*l.p1.x))/len,lengthTolerance);double t=((*p.x-*l.p1.x)*dx+(*p.y-*l.p1.y)*dy)/(len*len);if(type=="pointOnCurve")require(t>=-1e-6 && t<=1+1e-6,"Trim contact left its finite boundary line");}
                else {const auto& circle=circleMap.at(id);check(std::hypot(*p.x-*circle.center.x,*p.y-*circle.center.y)-*circle.rad,lengthTolerance);}
            }
            else if(type=="polygonAffineVertex") {
                const auto& a=point(reference(c,"pointIds",0));const auto& b=point(reference(c,"pointIds",1));const auto& v=point(reference(c,"pointIds",2));
                const double x=number(c,"coefficientU"),y=number(c,"coefficientV"),du=*b.x-*a.x,dv=*b.y-*a.y;
                check(*v.x-*a.x-x*du+y*dv,lengthTolerance);check(*v.y-*a.y-y*du-x*dv,lengthTolerance);
            }
            else if(type=="polygonVertex") {
                const auto& anchor=point(reference(c,"pointIds",0));const auto& vertex=point(reference(c,"pointIds",1));
                check(*vertex.x-*anchor.x-number(c,"offsetUMm"),lengthTolerance);check(*vertex.y-*anchor.y-number(c,"offsetVMm"),lengthTolerance);
            }
            else if(type=="fixed") {const auto& p=point(reference(c,"pointIds",0));check(*p.x-number(c,"uMm"),lengthTolerance);check(*p.y-number(c,"vMm"),lengthTolerance);}
            else if(type=="circleTangent") {
                const auto& a=circleMap.at(reference(c,"entityIds",0));const auto& b=circleMap.at(reference(c,"entityIds",1));
                const double distance=std::hypot(*a.center.x-*b.center.x,*a.center.y-*b.center.y);
                require(distance>=minimumLength,"Circle tangent centers became coincident");
                if(number(c,"side")<0)require(*a.rad>*b.rad,"Internal tangent outer circle changed");
                check(distance-(*a.rad+number(c,"side") * *b.rad),lengthTolerance);
            }
            else if(type=="arcTangent") {
                const auto& arc=arcMap.at(reference(c,"entityIds",0));const auto& target=line(reference(c,"entityIds",1));
                const auto& contact=point(reference(c,"pointIds",0));
                const double dx=*target.p2.x-*target.p1.x,dy=*target.p2.y-*target.p1.y,length=std::hypot(dx,dy);
                check((dx*(*arc.center.y-*target.p1.y)-dy*(*arc.center.x-*target.p1.x))/length-number(c,"side")*(*arc.rad),lengthTolerance);
                check((dx*(*contact.x-*arc.center.x)+dy*(*contact.y-*arc.center.y))/length,lengthTolerance);
            }
            else if(type=="collinear") {
                const auto& a=line(reference(c,"entityIds",0));const auto& b=line(reference(c,"entityIds",1));
                const double ax=*a.p2.x-*a.p1.x,ay=*a.p2.y-*a.p1.y,bx=*b.p2.x-*b.p1.x,by=*b.p2.y-*b.p1.y;
                const double al=std::hypot(ax,ay),bl=std::hypot(bx,by);
                require(al>=minimumLength && bl>=minimumLength,"Collinear lines became degenerate");
                check((ax*by-ay*bx)/(al*bl),angularTolerance);
                check((ax*(*b.p1.y-*a.p1.y)-ay*(*b.p1.x-*a.p1.x))/al,lengthTolerance);
            }
            else if(type=="tangent") {
                const auto& circle=circleMap.at(reference(c,"entityIds",0)); const auto& target=line(reference(c,"entityIds",1));
                const double dx=*target.p2.x-*target.p1.x,dy=*target.p2.y-*target.p1.y,length=std::hypot(dx,dy);
                require(length>=minimumLength,"Tangent line became degenerate");
                const double px=*circle.center.x-*target.p1.x,py=*circle.center.y-*target.p1.y;
                check((dx*py-dy*px)/length-number(c,"side") * (*circle.rad),lengthTolerance);
                const double foot=(px*dx+py*dy)/(length*length);
                require(foot>=-lengthTolerance/length && foot<=1+lengthTolerance/length,"Tangent contact left the finite line");
                if(c.contains("pointIds")) {
                    const auto& contact=point(reference(c,"pointIds",0));
                    const double rx=*contact.x-*circle.center.x,ry=*contact.y-*circle.center.y;
                    check(std::hypot(rx,ry)-*circle.rad,lengthTolerance);
                    check((dx*rx+dy*ry)/length,lengthTolerance);
                }
            }
            else if(type=="midpoint") {
                const auto& p=point(reference(c,"pointIds",0));const auto id=reference(c,"entityIds",0);
                if(lineMap.contains(id)){
                    const auto& target=line(id);
                    check(*p.x-(*target.p1.x+*target.p2.x)/2,lengthTolerance);
                    check(*p.y-(*target.p1.y+*target.p2.y)/2,lengthTolerance);
                }else{
                    const auto& arc=arcMap.at(id);
                    const double half=(*arc.startAngle+*arc.endAngle)/2;
                    check(*p.x-*arc.center.x-*arc.rad*std::cos(half),lengthTolerance);
                    check(*p.y-*arc.center.y-*arc.rad*std::sin(half),lengthTolerance);
                }
            }
            else if(type=="concentric") {
                auto center=[&](const QString& id) -> GCS::Point& {
                    if(auto found=circleMap.find(id);found!=circleMap.end())return found->second.center;
                    return arcMap.at(id).center;
                };
                const auto& a=center(reference(c,"entityIds",0));const auto& b=center(reference(c,"entityIds",1));
                check(*a.x-*b.x,lengthTolerance);check(*a.y-*b.y,lengthTolerance);
            }
            else if(type=="symmetry") {
                const auto& axis=line(reference(c,"entityIds",2));
                const double dx=*axis.p2.x-*axis.p1.x,dy=*axis.p2.y-*axis.p1.y,norm=dx*dx+dy*dy;
                require(norm>=minimumLength*minimumLength,"Symmetry axis collapsed");
                for(int pair=0;pair<c["pointIds"].toArray().size()/2;++pair){
                    const auto& source=point(reference(c,"pointIds",2*pair));
                    const auto& target=point(reference(c,"pointIds",2*pair+1));
                    const double along=((*source.x-*axis.p1.x)*dx+(*source.y-*axis.p1.y)*dy)/norm;
                    check(*target.x-(2*(*axis.p1.x+along*dx)-*source.x),lengthTolerance);
                    check(*target.y-(2*(*axis.p1.y+along*dy)-*source.y),lengthTolerance);
                }
            }
            else if(type=="equal") {
                const auto a = reference(c,"entityIds",0), b = reference(c,"entityIds",1);
                if (lineMap.contains(a) && lineMap.contains(b))
                    check(separation("linear",line(a).p1,line(a).p2)-separation("linear",line(b).p1,line(b).p2),lengthTolerance);
                else check(*radius(a)-*radius(b),lengthTolerance);
            }
            else if(type=="circleOffset") {
                check(*radius(reference(c,"entityIds",1))-*radius(reference(c,"entityIds",0))-number(c,"distanceMm"),lengthTolerance);
            }
            else if((type=="horizontal" || type=="vertical") && c.contains("pointIds")) {
                const auto& a=point(reference(c,"pointIds",0));const auto& b=point(reference(c,"pointIds",1));
                check(type=="horizontal" ? *a.y-*b.y : *a.x-*b.x,lengthTolerance);
            }
            else {
                const auto& a=line(reference(c,"entityIds",0)); const double ax=*a.p2.x-*a.p1.x,ay=*a.p2.y-*a.p1.y;
                if(type=="horizontal")check(ay,lengthTolerance);
                else if(type=="vertical")check(ax,lengthTolerance);
                else {const auto& b=line(reference(c,"entityIds",1));const double bx=*b.p2.x-*b.p1.x,by=*b.p2.y-*b.p1.y;
                    check((type=="parallel" ? ax*by-ay*bx : ax*bx+ay*by)/(std::hypot(ax,ay)*std::hypot(bx,by)),angularTolerance);}
            }
        }
        for(const auto& groupValue:parameters["mirrorGroups"].toArray()){
            const auto group=groupValue.toObject();const auto& axis=line(group["axisEntityId"].toString());
            const double dx=*axis.p2.x-*axis.p1.x,dy=*axis.p2.y-*axis.p1.y,norm=dx*dx+dy*dy;
            require(norm>=minimumLength*minimumLength,"Mirror axis collapsed");
            for(const auto& pairValue:group["pointPairs"].toArray()){
                const auto pair=pairValue.toObject();const auto& source=point(pair["sourcePointId"].toString());const auto& target=point(pair["resultPointId"].toString());
                const double t=((*source.x-*axis.p1.x)*dx+(*source.y-*axis.p1.y)*dy)/norm;
                check(*target.x-(2*(*axis.p1.x+t*dx)-*source.x),lengthTolerance);
                check(*target.y-(2*(*axis.p1.y+t*dy)-*source.y),lengthTolerance);
            }
        }
        QJsonArray solvedEntities;
        for(const auto& value:entities) {auto e=value.toObject();const auto id=e["id"].toString();
            if(e["type"]=="circle" || e["type"]=="arc") {
                const auto& curve=e["type"]=="arc" ? static_cast<const GCS::Circle&>(arcMap.at(id)) : circleMap.at(id);
                require(std::isfinite(*curve.rad) && *curve.rad>=minimumLength && *curve.rad<=maximumCoordinate,"Solved curve radius outside supported range");
                e["radiusMm"]=*curve.rad;
                if(e["type"]=="arc") {const auto& arc=arcMap.at(id);const double sweep=(*arc.endAngle-*arc.startAngle)/degrees;
                    require(std::isfinite(sweep) && std::abs(sweep)>1e-6 && std::abs(sweep)<360 && *arc.rad*std::abs(sweep)*degrees>=minimumLength,"Solved arc has invalid sweep");
                    require((sweep<0)==(e["sweepAngleDeg"].toDouble()<0),"Solve reversed the arc sweep direction");
                    check(*arc.start.x-*arc.center.x-*arc.rad*std::cos(*arc.startAngle),lengthTolerance);
                    check(*arc.start.y-*arc.center.y-*arc.rad*std::sin(*arc.startAngle),lengthTolerance);
                    check(*arc.end.x-*arc.center.x-*arc.rad*std::cos(*arc.endAngle),lengthTolerance);
                    check(*arc.end.y-*arc.center.y-*arc.rad*std::sin(*arc.endAngle),lengthTolerance);
                    // Preserve authored degrees when the solver left both radian
                    // parameters unchanged; converting back otherwise causes
                    // tiny authored drift on every save/reopen evaluation.
                    const double seededStart=e["startAngleDeg"].toDouble()*degrees;
                    const double seededEnd=(e["startAngleDeg"].toDouble()+e["sweepAngleDeg"].toDouble())*degrees;
                    if(*arc.startAngle!=seededStart || *arc.endAngle!=seededEnd) {
                        e["startAngleDeg"]=std::remainder(*arc.startAngle/degrees,360.0);e["sweepAngleDeg"]=sweep;
                    }
                }
            } solvedEntities.append(e);
        }
        QJsonArray measurements;
        for(const auto& entry:dimensions) {
            const auto d=entry.toObject();const auto kind=d["kind"].toString();const bool angular=kind=="arcSweep" || kind=="angle" || kind=="overallSlotAngle";
            double value;
            if(kind=="angle")value=std::abs(signedLineAngle(d))/degrees;
            else if(kind=="overallSlotAngle") {const auto& a=point(reference(d,"pointIds",0));const auto& b=point(reference(d,"pointIds",1));value=std::atan2(*b.y-*a.y,*b.x-*a.x)/degrees;}
            else if(kind=="overallSlotLength") {const auto& a=point(reference(d,"pointIds",0));const auto& b=point(reference(d,"pointIds",1));value=separation("linear",a,b)+2*(*arcMap.at(d["entityId"].toString()).rad);}
            else if(kind=="radius" || kind=="diameter" || kind=="arcSweep") {const auto id=d["entityId"].toString();
                const auto& curve=arcMap.contains(id) ? static_cast<const GCS::Circle&>(arcMap.at(id)) : circleMap.at(id);
                value=angular ? std::abs(*arcMap.at(id).endAngle-*arcMap.at(id).startAngle)/degrees : *curve.rad*(kind=="diameter" ? 2 : 1);
            } else if(kind=="centerlineDiameter") {
                const auto& axis=line(reference(d,"entityIds",0));const auto& edge=line(reference(d,"entityIds",1));
                const auto first=pointLineDistance(edge.p1,axis),second=pointLineDistance(edge.p2,axis);
                require(std::abs(first-second)<=lengthTolerance,"Centerline diameter lost parallel orientation; retain Parallel/Horizontal/Vertical constraints");
                const auto side=d["side"].toInt();require(side==1 || side==-1,"Centerline diameter side is invalid");
                require(std::abs(first)>1e-9,"Centerline diameter requires nonzero offset");
                value=2*(d["driving"].toBool() ? side*first : std::abs(first));
            } else if(kind=="parallelLineDistance") {
                const auto& first=line(reference(d,"entityIds",0));const auto& second=line(reference(d,"entityIds",1));
                const auto near=pointLineDistance(second.p1,first),far=pointLineDistance(second.p2,first);
                require(std::abs(near-far)<=lengthTolerance,"Parallel distance lost line orientation; retain Parallel/Horizontal/Vertical constraints");
                require(std::abs(near)>=minimumLength,"Parallel distance requires separated lines");
                const auto side=d["side"].toInt();require(side==1 || side==-1,"Parallel distance side is invalid");
                value=d["driving"].toBool() ? side*near : std::abs(near);
            } else if(kind=="pointLineDistance"){const auto signedValue=pointLineDistance(point(reference(d,"pointIds",0)),line(d["entityId"].toString()));value=d["driving"].toBool() ? d["side"].toInt()*signedValue : std::abs(signedValue);}
            else value=separation(kind,point(reference(d,"pointIds",0)),point(reference(d,"pointIds",1)));
            if(d["driving"].toBool())check(value-number(d,angular ? "valueDeg" : "valueMm"),angular ? 1e-6 : lengthTolerance);
            QJsonObject measurement{{"id",d["id"]},{"kind",kind},{"driving",d["driving"]},{angular ? "valueDeg" : "valueMm",value}};
            measurements.append(measurement);
        }
        for(qsizetype i=0;i<points.size();++i)if(lockedPointIds.contains(points[i].toObject()["id"].toString())) {
            check(coordinates[2*i]-number(points[i].toObject(),"uMm"),lengthTolerance);
            check(coordinates[2*i+1]-number(points[i].toObject(),"vMm"),lengthTolerance);
        }
        result.parameters["entities"]=solvedEntities;
        QJsonArray solvedPoints;
        for (qsizetype i=0;i<points.size();++i) {auto p=points[i].toObject();p["uMm"]=coordinates[2*i];p["vMm"]=coordinates[2*i+1];solvedPoints.append(p);}
        result.parameters["points"] = solvedPoints;
        result.parameters=SketchPolygon::refreshAnchors(result.parameters);
        // Tangent handles are evaluator vectors, outside PlaneGCS's fit-point
        // equations. Derive their reflection using the final solved axis.
        if(parameters.contains("mirrorGroups"))result.parameters=SketchMirror::refreshSplineHandles(result.parameters);
        // PlaneGCS returns every parameter participating in a Jacobian nullspace
        // direction, including parameters coupled to the independent pivots.
        // Diagnose at the accepted coordinates, rather than the initial guess.
        GCS::VEC_pD movable;
        if(tagIds.isEmpty())movable=unknowns;
        else {solver.invalidatedDiagnosis();solver.diagnose(GCS::DogLeg);solver.getDependentParams(movable);}
        const std::set<double*> freeParameters(movable.begin(),movable.end());
        QJsonObject pointFreedom,entityFreedom;
        for(const auto& [id,p]:pointMap){
            const bool determinate=dof==0||(!derivedPointIds.contains(id)&&!diameterStartIds.contains(id));
            const bool uFree=freeParameters.contains(p.x),vFree=freeParameters.contains(p.y);
            pointFreedom[id]=QJsonObject{{"uFree",uFree},{"vFree",vFree},{"determinate",determinate},{"fullyConstrained",determinate&&!uFree&&!vFree}};
        }
        for(const auto& value:entities){
            const auto e=value.toObject();const auto id=e["id"].toString(),type=e["type"].toString();QSet<QString> owned;
            bool determinate=true,fixed=true;
            auto own=[&](const QString& p){if(!p.isEmpty())owned.insert(p);};
            if(type=="point")own(e["pointId"].toString());
            else if(type=="fitSpline"||type=="controlSpline"||type=="conic")for(const auto& p:e["pointIds"].toArray())own(p.toString());
            else {for(const auto& key:{"centerPointId","startPointId","endPointId"})own(e[key].toString());}
            if(type=="ellipse")for(const auto& key:{"firstAxisId","secondAxisId"}){const auto& axis=lineMap.at(e[key].toString());fixed&=!freeParameters.contains(axis.p1.x)&&!freeParameters.contains(axis.p1.y)&&!freeParameters.contains(axis.p2.x)&&!freeParameters.contains(axis.p2.y);}
            for(const auto& p:owned){const auto freedom=pointFreedom[p].toObject();determinate&=freedom["determinate"].toBool();fixed&=freedom["fullyConstrained"].toBool();}
            if(type=="circle")fixed&=!freeParameters.contains(circleMap.at(id).rad);
            if(type=="arc"){const auto& arc=arcMap.at(id);fixed&=!freeParameters.contains(arc.rad)&&!freeParameters.contains(arc.startAngle)&&!freeParameters.contains(arc.endAngle);determinate&=dof==0||(!derivedArcIds.contains(id)&&!mirrorSources.contains(id));}
            // These authored shape parameters are intentionally outside PlaneGCS.
            if(type=="fitSpline"||type=="fitSplineOffset"||type=="conic")fixed=false;
            entityFreedom[id]=QJsonObject{{"determinate",determinate},{"fullyConstrained",determinate&&fixed}};
        }
        result.diagnostics["freedomSource"]="PlaneGCS solved-coordinate Jacobian nullspace";
        result.diagnostics["pointFreedom"]=pointFreedom;result.diagnostics["entityFreedom"]=entityFreedom;
        result.diagnostics["dof"] = dof;
        result.diagnostics["status"] = dof == 0 ? "fully_constrained" : "underconstrained";
        result.diagnostics["dimensionValues"] = measurements;
        result.diagnostics["maxResidual"] = maxResidual;
        result.diagnostics["residualUnits"] = "mm for lengths; degrees for angular dimensions; normalized sine/cosine for line relations";
        result.accepted = true;
    } catch (const std::exception& error) {
        result.parameters = parameters;
        result.error = QString::fromUtf8(error.what());
        result.diagnostics["status"] = "invalid";
    }
    return result;
}
