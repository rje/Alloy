// SPDX-License-Identifier: MIT
#include "SketchConic.h"
#include <QJsonArray>
#include <QMap>
#include <QSet>
#include <QUuid>
#include <Standard_Failure.hxx>
#include <TColgp_Array1OfPnt.hxx>
#include <TColStd_Array1OfReal.hxx>
#include <cmath>
#include <stdexcept>

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::invalid_argument(message);
}
double coordinate(const QJsonObject& point, const char* key) {
    const auto value = point[key];
    require(value.isDouble() && std::isfinite(value.toDouble()) &&
            std::abs(value.toDouble()) <= 100000, "Conic coordinates must be finite and within 100000 mm");
    return value.toDouble();
}
QMap<QString, gp_Pnt> pointMap(const QJsonObject& parameters) {
    QMap<QString, gp_Pnt> result;
    for (const auto& value : parameters["points"].toArray()) {
        const auto point = value.toObject();
        result[point["id"].toString()] = {coordinate(point,"uMm"), coordinate(point,"vMm"), 0};
    }
    return result;
}
void validateRho(double rho) {
    require(std::isfinite(rho) && rho > 0 && rho < 1, "Conic Rho must be strictly between 0 and 1");
    require(rho > gp::Resolution() && 1-rho > gp::Resolution(),
            "Conic Rho is too close to a boundary for the geometry kernel");
}
gp_Pnt toWorld(const gp_Pnt& point, const QJsonObject& frame) {
    if (frame.isEmpty()) return point;
    const auto origin=frame["origin"].toArray(), u=frame["uAxis"].toArray(), v=frame["vAxis"].toArray();
    require(origin.size()==3 && u.size()==3 && v.size()==3, "Conic needs a valid sketch frame");
    return {origin[0].toDouble()+u[0].toDouble()*point.X()+v[0].toDouble()*point.Y(),
            origin[1].toDouble()+u[1].toDouble()*point.X()+v[1].toDouble()*point.Y(),
            origin[2].toDouble()+u[2].toDouble()*point.X()+v[2].toDouble()*point.Y()};
}
SketchGeometry::SolveResult failure(const QJsonObject& draft, const QString& error) {
    return {false,draft,{{"status","invalid"}},error};
}
SketchGeometry::SolveResult solve(const QJsonObject& draft, const QJsonObject& parameters,
                                 const QJsonObject& context) {
    auto candidate=draft; candidate["parameters"]=parameters;
    auto result=SketchGeometry::solveDraft(candidate,context);
    if (!result.ok) result.feature=draft;
    return result;
}
}

QString SketchConic::validate(const QJsonObject& parameters, const QJsonObject& entity) {
    try {
        require(entity.size()==8 && entity["type"]=="conic" && entity["version"]==1 &&
                entity["rho"].isDouble(), "Invalid retained conic intent");
        validateRho(entity["rho"].toDouble());
        const auto points=pointMap(parameters);
        const auto ids=entity["pointIds"].toArray();
        require(ids.size()==3 && ids[0]==entity["startPointId"] && ids[1]==entity["endPointId"] &&
                ids[2]==entity["vertexPointId"], "Conic requires ordered start, end and tangent vertex identities");
        QSet<QString> seen;
        for (const auto& id : ids) {
            require(id.isString() && points.contains(id.toString()) && !seen.contains(id.toString()),
                    "Conic points must have distinct existing identities");
            seen.insert(id.toString());
        }
        const auto a=points[ids[0].toString()], b=points[ids[1].toString()], v=points[ids[2].toString()];
        const gp_Vec chord(a,b), arm(a,v);
        require(chord.Magnitude()>=0.001 && arm.Magnitude()>=0.001 && b.Distance(v)>=0.001,
                "Conic points must be separated by at least 0.001 mm");
        require(chord.Crossed(arm).Magnitude()/chord.Magnitude()>=0.001,
                "Conic tangent vertex must lie off the endpoint chord");
        return {};
    } catch (const std::exception& error) { return QString::fromUtf8(error.what()); }
}

Handle(Geom_BezierCurve) SketchConic::evaluate(const QJsonObject& parameters,
                                              const QJsonObject& entity,
                                              const QJsonObject& frame) {
    try {
        const auto error=validate(parameters,entity);
        if (!error.isEmpty()) throw std::invalid_argument(error.toStdString());
        const auto points=pointMap(parameters);
        const double rho=entity["rho"].toDouble();
        TColgp_Array1OfPnt poles(1,3);
        poles.SetValue(1,toWorld(points[entity["startPointId"].toString()],frame));
        poles.SetValue(2,toWorld(points[entity["vertexPointId"].toString()],frame));
        poles.SetValue(3,toWorld(points[entity["endPointId"].toString()],frame));
        TColStd_Array1OfReal weights(1,3);
        weights.SetValue(1,1-rho); weights.SetValue(2,rho); weights.SetValue(3,1-rho);
        Handle(Geom_BezierCurve) curve=new Geom_BezierCurve(poles,weights);
        const auto shoulder=curve->Value(0.5);
        require(std::isfinite(shoulder.X()) && std::isfinite(shoulder.Y()) && std::isfinite(shoulder.Z()),
                "Conic evaluation is not finite");
        return curve;
    } catch (const Standard_Failure& error) { throw std::invalid_argument(error.GetMessageString()); }
}

SketchGeometry::SolveResult SketchConic::add(const QJsonObject& draft,
                                             const QJsonObject& inputs,
                                             const QJsonObject& context) {
    try {
        require(inputs.size()==4 && inputs["rho"].isDouble(), "Conic creation needs start, end, vertex and Rho");
        auto parameters=draft["parameters"].toObject();
        auto points=parameters["points"].toArray(), entities=parameters["entities"].toArray();
        const auto existing=pointMap(parameters);
        QJsonArray ids;
        for (const auto* key : {"start","end","vertex"}) {
            require(inputs[key].isObject(), "Conic control point is missing");
            const auto point=inputs[key].toObject();
            const double u=coordinate(point,"uMm"), v=coordinate(point,"vMm");
            require(point.size()==2 || (point.size()==3 && point["pointId"].isString()), "Invalid conic control point");
            QString id=point["pointId"].toString();
            if (!id.isEmpty()) {
                require(existing.contains(id) && existing[id].Distance(gp_Pnt(u,v,0))<1e-7,
                        "Conic snap must name an existing point at the requested position");
            } else {
                id=QUuid::createUuid().toString(QUuid::WithoutBraces);
                points.append(QJsonObject{{"id",id},{"uMm",u},{"vMm",v}});
            }
            ids.append(id);
        }
        const auto id=QUuid::createUuid().toString(QUuid::WithoutBraces);
        entities.append(QJsonObject{{"id",id},{"type","conic"},{"version",1},{"pointIds",ids},
            {"startPointId",ids[0]},{"endPointId",ids[1]},{"vertexPointId",ids[2]},{"rho",inputs["rho"]}});
        parameters["points"]=points; parameters["entities"]=entities;
        auto result=solve(draft,parameters,context);
        if(result.ok) result.diagnostics["createdConicId"]=id;
        return result;
    } catch (const std::exception& error) { return failure(draft,QString::fromUtf8(error.what())); }
}

SketchGeometry::SolveResult SketchConic::setRho(const QJsonObject& draft,
                                                const QString& entityId, double rho,
                                                const QJsonObject& context) {
    try {
        validateRho(rho);
        auto parameters=draft["parameters"].toObject(); auto entities=parameters["entities"].toArray();
        bool found=false;
        for (int i=0;i<entities.size();++i) {
            auto entity=entities[i].toObject();
            if(entity["id"]!=entityId || entity["type"]!="conic") continue;
            entity["rho"]=rho; entities[i]=entity; found=true; break;
        }
        require(found,"Select an existing conic");
        parameters["entities"]=entities;
        return solve(draft,parameters,context);
    } catch (const std::exception& error) { return failure(draft,QString::fromUtf8(error.what())); }
}
