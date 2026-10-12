// SPDX-License-Identifier: MIT
#include "SketchSolver.h"
#include "GCS.h"
#include <QJsonArray>
#include <QMap>
#include <QSet>
#include <algorithm>
#include <array>
#include <cmath>
#include <deque>
#include <map>
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
double separation(const QString& kind, const GCS::Point& a, const GCS::Point& b) {
    if (kind == "horizontal") return std::abs(*b.x - *a.x);
    if (kind == "vertical") return std::abs(*b.y - *a.y);
    require(kind == "linear", "Unsupported dimension kind");
    return std::hypot(*b.x - *a.x, *b.y - *a.y);
}
}

SketchSolver::Result SketchSolver::solve(const QJsonObject& parameters) {
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
        QSet<QString> ids;
        for (qsizetype i = 0; i < points.size(); ++i) {
            const auto point = points[i].toObject();
            const auto id = point.value("id").toString();
            require(!id.isEmpty() && !ids.contains(id), "Duplicate or empty sketch point ID");
            ids.insert(id);
            coordinates[2*i] = number(point, "uMm"); coordinates[2*i+1] = number(point, "vMm");
            require(std::abs(coordinates[2*i]) <= maximumCoordinate && std::abs(coordinates[2*i+1]) <= maximumCoordinate, "Point outside supported range");
            pointMap.emplace(id, GCS::Point(&coordinates[2*i], &coordinates[2*i+1]));
            unknowns.push_back(&coordinates[2*i]); unknowns.push_back(&coordinates[2*i+1]);
        }
        auto point = [&](const QString& id) -> GCS::Point& {
            const auto found = pointMap.find(id);
            require(found != pointMap.end(), "Unknown sketch point reference"); return found->second;
        };
        for (const auto& entry : entities) {
            const auto entity = entry.toObject();
            const auto id = entity.value("id").toString();
            const auto type=entity["type"].toString();
            require((type=="line" || type=="circle" || type=="arc") && !id.isEmpty() && !ids.contains(id), "Unsupported or duplicate sketch entity");
            ids.insert(id);
            if(type=="line") {GCS::Line line;line.p1=point(entity["startPointId"].toString());line.p2=point(entity["endPointId"].toString());lineMap.emplace(id,line);}
            else {
                curveParameters.push_back(number(entity,"radiusMm"));auto* radius=&curveParameters.back();unknowns.push_back(radius);
                if(type=="circle") {GCS::Circle circle;circle.center=point(entity["centerPointId"].toString());circle.rad=radius;circleMap.emplace(id,circle);}
                else {GCS::Arc arc;arc.center=point(entity["centerPointId"].toString());arc.rad=radius;
                    arc.start=point(entity["startPointId"].toString());arc.end=point(entity["endPointId"].toString());
                    curveParameters.push_back(number(entity,"startAngleDeg")*degrees);arc.startAngle=&curveParameters.back();unknowns.push_back(arc.startAngle);
                    curveParameters.push_back((number(entity,"startAngleDeg")+number(entity,"sweepAngleDeg"))*degrees);arc.endAngle=&curveParameters.back();unknowns.push_back(arc.endAngle);
                    arcMap.emplace(id,arc);}
            }
        }
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
        QSet<int> internalTags;
        for(auto& [id,arc]:arcMap) {const int t=tagIds.size()+1;tagIds[t]=id;internalTags.insert(t);solver.addConstraintArcRules(arc,t);}
        for (const auto& entry : constraints) {
            const auto constraint = entry.toObject();
            const auto type = constraint.value("type").toString(); const int t = tag(constraint);
            if (type == "coincident") {
                solver.addConstraintP2PCoincident(point(reference(constraint,"pointIds",0)), point(reference(constraint,"pointIds",1)), t);
            } else if (type == "fixed") {
                auto& p = point(reference(constraint,"pointIds",0));
                solver.addConstraintEqual(p.x,constant(number(constraint,"uMm")),t);
                solver.addConstraintEqual(p.y,constant(number(constraint,"vMm")),t);
            } else if (type == "midpoint") {
                auto& target = line(reference(constraint,"entityIds",0));
                solver.addConstraintP2PSymmetric(target.p1,target.p2,point(reference(constraint,"pointIds",0)),t);
            } else if (type == "horizontal") {
                solver.addConstraintHorizontal(line(reference(constraint,"entityIds",0)),t);
            } else if (type == "vertical") {
                solver.addConstraintVertical(line(reference(constraint,"entityIds",0)),t);
            } else if (type == "parallel") {
                solver.addConstraintParallel(line(reference(constraint,"entityIds",0)),line(reference(constraint,"entityIds",1)),t);
            } else if (type == "perpendicular") {
                solver.addConstraintPerpendicular(line(reference(constraint,"entityIds",0)),line(reference(constraint,"entityIds",1)),t);
            } else if (type == "equal") {
                const auto a = reference(constraint,"entityIds",0), b = reference(constraint,"entityIds",1);
                require(a != b,"Equal requires distinct sketch entities");
                if (lineMap.contains(a) && lineMap.contains(b)) solver.addConstraintEqualLength(line(a),line(b),t);
                else solver.addConstraintEqual(radius(a),radius(b),t);
            } else require(false,"Unsupported sketch constraint");
        }
        for (const auto& entry : dimensions) {
            const auto dimension = entry.toObject();
            const auto id = dimension.value("id").toString();
            require(!id.isEmpty() && !ids.contains(id), "Duplicate or empty dimension ID");
            const auto kind=dimension["kind"].toString();
            const bool curve=kind=="radius" || kind=="diameter" || kind=="arcSweep";
            const bool angle=kind=="angle";
            require(curve || angle || kind=="linear" || kind=="horizontal" || kind=="vertical", "Unsupported dimension kind");
            require(dimension["driving"].isBool(), "Dimension requires driving flag");
            if(angle) {require(dimension["entityIds"].isArray() && dimension["entityIds"].toArray().size()==2 && reference(dimension,"entityIds",0)!=reference(dimension,"entityIds",1),"Angle requires two distinct line IDs");signedLineAngle(dimension);}
            if(!dimension["driving"].toBool()) {ids.insert(id);continue;}
            const double value=number(dimension,kind=="arcSweep" || angle ? "valueDeg" : "valueMm");
            require(kind=="arcSweep" ? value>=1e-6 && value<360 : angle ? value>=1e-6 && value<180 : value>=minimumLength && value<=maximumCoordinate, "Driving dimension outside supported range");
            const int t=tag(dimension);
            if(angle) {
                const auto signs=raySigns(dimension);
                const double signedTarget=(signedLineAngle(dimension)<0 ? -1 : 1)*value*degrees;
                const double rayOffset=((signs[1]<0 ? 1 : 0)-(signs[0]<0 ? 1 : 0))*std::numbers::pi;
                const double target=std::remainder(signedTarget-rayOffset,2*std::numbers::pi);
                solver.addConstraintL2LAngle(line(reference(dimension,"entityIds",0)),line(reference(dimension,"entityIds",1)),constant(target),t);
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
        result.diagnostics["internalCurveRuleCount"] = internalTags.size()*4;
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
            result.diagnostics["conflictingConstraintIds"] = mapped(conflicts);
            result.diagnostics["redundantConstraintIds"] = mapped(redundant);
            result.diagnostics["conflictingEntityIds"] = mapped(conflicts,true);
            result.diagnostics["redundantEntityIds"] = mapped(redundant,true);
            result.diagnostics["solverStatus"] = static_cast<int>(status);
            dof = solver.dofsNumber(); result.diagnostics["dof"] = dof;
            if (!conflicts.empty() || !redundant.empty() || status != GCS::SolveStatus::Success) {
                result.diagnostics["status"] = !conflicts.empty() ? "conflicting" : !redundant.empty() ? "redundant" : "failed";
                result.error = "Sketch solve rejected: " + result.diagnostics.value("status").toString(); return result;
            }
            solver.applySolution();
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
        for (const auto& entry : constraints) {
            const auto c=entry.toObject(); const auto type=c.value("type").toString();
            if(type=="coincident") check(separation("linear",point(reference(c,"pointIds",0)),point(reference(c,"pointIds",1))),lengthTolerance);
            else if(type=="fixed") {const auto& p=point(reference(c,"pointIds",0));check(*p.x-number(c,"uMm"),lengthTolerance);check(*p.y-number(c,"vMm"),lengthTolerance);}
            else if(type=="midpoint") {
                const auto& p=point(reference(c,"pointIds",0)); const auto& target=line(reference(c,"entityIds",0));
                check(*p.x-(*target.p1.x+*target.p2.x)/2,lengthTolerance);
                check(*p.y-(*target.p1.y+*target.p2.y)/2,lengthTolerance);
            }
            else if(type=="equal") {
                const auto a = reference(c,"entityIds",0), b = reference(c,"entityIds",1);
                if (lineMap.contains(a) && lineMap.contains(b))
                    check(separation("linear",line(a).p1,line(a).p2)-separation("linear",line(b).p1,line(b).p2),lengthTolerance);
                else check(*radius(a)-*radius(b),lengthTolerance);
            }
            else {
                const auto& a=line(reference(c,"entityIds",0)); const double ax=*a.p2.x-*a.p1.x,ay=*a.p2.y-*a.p1.y;
                if(type=="horizontal")check(ay,lengthTolerance);
                else if(type=="vertical")check(ax,lengthTolerance);
                else {const auto& b=line(reference(c,"entityIds",1));const double bx=*b.p2.x-*b.p1.x,by=*b.p2.y-*b.p1.y;
                    check((type=="parallel" ? ax*by-ay*bx : ax*bx+ay*by)/(std::hypot(ax,ay)*std::hypot(bx,by)),angularTolerance);}
            }
        }
        QJsonArray solvedEntities;
        for(const auto& value:entities) {auto e=value.toObject();const auto id=e["id"].toString();
            if(e["type"]!="line") {
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
                    e["startAngleDeg"]=std::remainder(*arc.startAngle/degrees,360.0);e["sweepAngleDeg"]=sweep;
                }
            } solvedEntities.append(e);
        }
        QJsonArray measurements;
        for(const auto& entry:dimensions) {
            const auto d=entry.toObject();const auto kind=d["kind"].toString();const bool angular=kind=="arcSweep" || kind=="angle";
            double value;
            if(kind=="angle")value=std::abs(signedLineAngle(d))/degrees;
            else if(kind=="radius" || kind=="diameter" || kind=="arcSweep") {const auto id=d["entityId"].toString();
                const auto& curve=arcMap.contains(id) ? static_cast<const GCS::Circle&>(arcMap.at(id)) : circleMap.at(id);
                value=angular ? std::abs(*arcMap.at(id).endAngle-*arcMap.at(id).startAngle)/degrees : *curve.rad*(kind=="diameter" ? 2 : 1);
            } else value=separation(kind,point(reference(d,"pointIds",0)),point(reference(d,"pointIds",1)));
            if(d["driving"].toBool())check(value-number(d,angular ? "valueDeg" : "valueMm"),angular ? 1e-6 : lengthTolerance);
            QJsonObject measurement{{"id",d["id"]},{"kind",kind},{"driving",d["driving"]},{angular ? "valueDeg" : "valueMm",value}};
            measurements.append(measurement);
        }
        result.parameters["entities"]=solvedEntities;
        QJsonArray solvedPoints;
        for (qsizetype i=0;i<points.size();++i) {auto p=points[i].toObject();p["uMm"]=coordinates[2*i];p["vMm"]=coordinates[2*i+1];solvedPoints.append(p);}
        result.parameters["points"] = solvedPoints;
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
