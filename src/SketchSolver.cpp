// SPDX-License-Identifier: MIT
#include "SketchSolver.h"
#include "GCS.h"
#include <QJsonArray>
#include <QMap>
#include <QSet>
#include <algorithm>
#include <cmath>
#include <deque>
#include <map>
#include <stdexcept>
#include <vector>

namespace {
constexpr double lengthTolerance = 1e-6;
constexpr double angularTolerance = 1e-8;
constexpr double minimumLength = 0.001;
constexpr double maximumCoordinate = 100000;
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
        {"dimensionValues", QJsonArray{}}, {"maxResidual", QJsonValue::Null},
        {"lengthToleranceMm", lengthTolerance}, {"angularTolerance", angularTolerance}};
    try {
        require(parameters.value("points").isArray() && parameters.value("entities").isArray(), "Missing sketch points/entities");
        const auto points = parameters.value("points").toArray();
        const auto entities = parameters.value("entities").toArray();
        const auto constraints = parameters.value("constraints").toArray();
        const auto dimensions = parameters.value("dimensions").toArray();
        // All storage is allocated before pointers enter the solver. std::map
        // nodes and deque elements retain addresses as their containers grow.
        std::vector<double> coordinates(static_cast<size_t>(points.size()) * 2);
        std::map<QString, GCS::Point> pointMap;
        std::map<QString, GCS::Line> lineMap;
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
            require(entity.value("type") == "line" && !id.isEmpty() && !ids.contains(id), "Unsupported or duplicate sketch entity");
            ids.insert(id);
            GCS::Line line; line.p1 = point(entity.value("startPointId").toString()); line.p2 = point(entity.value("endPointId").toString());
            lineMap.emplace(id, line);
        }
        auto line = [&](const QString& id) -> GCS::Line& {
            const auto found = lineMap.find(id);
            require(found != lineMap.end(), "Unknown sketch line reference"); return found->second;
        };
        auto constant = [&](double value) { constants.push_back(value); return &constants.back(); };
        auto tag = [&](const QJsonObject& item) {
            const auto id = item.value("id").toString();
            require(!id.isEmpty() && !ids.contains(id), "Duplicate or empty constraint/dimension ID");
            ids.insert(id); const int value = tagIds.size()+1; tagIds[value] = id; return value;
        };
        GCS::System solver;
        for (const auto& entry : constraints) {
            const auto constraint = entry.toObject();
            const auto type = constraint.value("type").toString(); const int t = tag(constraint);
            if (type == "coincident") {
                solver.addConstraintP2PCoincident(point(reference(constraint,"pointIds",0)), point(reference(constraint,"pointIds",1)), t);
            } else if (type == "fixed") {
                auto& p = point(reference(constraint,"pointIds",0));
                solver.addConstraintEqual(p.x,constant(number(constraint,"uMm")),t);
                solver.addConstraintEqual(p.y,constant(number(constraint,"vMm")),t);
            } else if (type == "horizontal") {
                solver.addConstraintHorizontal(line(reference(constraint,"entityIds",0)),t);
            } else if (type == "vertical") {
                solver.addConstraintVertical(line(reference(constraint,"entityIds",0)),t);
            } else if (type == "parallel") {
                solver.addConstraintParallel(line(reference(constraint,"entityIds",0)),line(reference(constraint,"entityIds",1)),t);
            } else if (type == "perpendicular") {
                solver.addConstraintPerpendicular(line(reference(constraint,"entityIds",0)),line(reference(constraint,"entityIds",1)),t);
            } else require(false,"Unsupported sketch constraint");
        }
        for (const auto& entry : dimensions) {
            const auto dimension = entry.toObject();
            const auto id = dimension.value("id").toString();
            require(!id.isEmpty() && !ids.contains(id), "Duplicate or empty dimension ID");
            auto& a = point(reference(dimension,"pointIds",0)); auto& b = point(reference(dimension,"pointIds",1));
            const auto kind = dimension.value("kind").toString();
            require(kind == "linear" || kind == "horizontal" || kind == "vertical", "Unsupported dimension kind");
            require(dimension.value("driving").isBool(), "Dimension requires driving flag");
            if (!dimension.value("driving").toBool()) {ids.insert(id); continue;}
            const double value = number(dimension,"valueMm");
            require(value >= minimumLength && value <= maximumCoordinate, "Driving dimension outside supported range");
            const int t = tag(dimension);
            if (kind == "linear") solver.addConstraintP2PDistance(a,b,constant(value),t);
            else {
                double* first = kind == "horizontal" ? a.x : a.y;
                double* second = kind == "horizontal" ? b.x : b.y;
                // Positive lengths keep the currently seeded orientation. A zero
                // projection chooses positive; arbitrary branch control is deferred.
                solver.addConstraintDifference(first,second,constant(*second < *first ? -value : value),t);
            }
        }
        result.diagnostics["constraintCount"] = tagIds.size();
        int dof = static_cast<int>(unknowns.size());
        if (!tagIds.isEmpty()) {
            solver.declareUnknowns(unknowns); solver.initSolution(GCS::DogLeg);
            const auto status = solver.solve(GCS::DogLeg);
            GCS::VEC_I conflicts,redundant,partial;
            solver.getConflicting(conflicts); solver.getRedundant(redundant); solver.getPartiallyRedundant(partial);
            auto mapped = [&](const GCS::VEC_I& values) {
                QJsonArray array; QSet<QString> emitted;
                for (int value : values) if (tagIds.contains(value) && !emitted.contains(tagIds[value])) {
                    array.append(tagIds[value]); emitted.insert(tagIds[value]);
                }
                return array;
            };
            redundant.insert(redundant.end(),partial.begin(),partial.end());
            result.diagnostics["conflictingConstraintIds"] = mapped(conflicts);
            result.diagnostics["redundantConstraintIds"] = mapped(redundant);
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
            else {
                const auto& a=line(reference(c,"entityIds",0)); const double ax=*a.p2.x-*a.p1.x,ay=*a.p2.y-*a.p1.y;
                if(type=="horizontal")check(ay,lengthTolerance);
                else if(type=="vertical")check(ax,lengthTolerance);
                else {const auto& b=line(reference(c,"entityIds",1));const double bx=*b.p2.x-*b.p1.x,by=*b.p2.y-*b.p1.y;
                    check((type=="parallel" ? ax*by-ay*bx : ax*bx+ay*by)/(std::hypot(ax,ay)*std::hypot(bx,by)),angularTolerance);}
            }
        }
        QJsonArray measurements;
        for (const auto& entry : dimensions) {
            const auto d=entry.toObject();const auto kind=d.value("kind").toString();
            const double value=separation(kind,point(reference(d,"pointIds",0)),point(reference(d,"pointIds",1)));
            if(d.value("driving").toBool())check(value-number(d,"valueMm"),lengthTolerance);
            measurements.append(QJsonObject{{"id",d.value("id")},{"kind",kind},{"driving",d.value("driving")},{"valueMm",value}});
        }
        QJsonArray solvedPoints;
        for (qsizetype i=0;i<points.size();++i) {auto p=points[i].toObject();p["uMm"]=coordinates[2*i];p["vMm"]=coordinates[2*i+1];solvedPoints.append(p);}
        result.parameters["points"] = solvedPoints;
        result.diagnostics["dof"] = dof;
        result.diagnostics["status"] = dof == 0 ? "fully_constrained" : "underconstrained";
        result.diagnostics["dimensionValues"] = measurements;
        result.diagnostics["maxResidual"] = maxResidual;
        result.diagnostics["residualUnits"] = "mm for lengths; normalized sine/cosine for line angles";
        result.accepted = true;
    } catch (const std::exception& error) {
        result.parameters = parameters;
        result.error = QString::fromUtf8(error.what());
        result.diagnostics["status"] = "invalid";
    }
    return result;
}
