// SPDX-License-Identifier: MIT
#pragma once
#include "SketchGeometry.h"
#include <Geom_OffsetCurve.hxx>

class SketchSplineOffset final {
public:
    struct Evaluation {
        Handle(Geom_OffsetCurve) curve;
        gp_Pnt start;
        gp_Pnt end;
    };
    static Evaluation evaluate(const QJsonObject& parameters,const QJsonObject& entity,
                               const QJsonObject& frame = {});
    static QSet<QString> derivedPointIds(const QJsonObject& parameters);
    static QJsonObject refreshEndpoints(const QJsonObject& parameters);
    static QString validate(const QJsonObject& parameters,const QJsonObject& entity,bool checkGeometry);
    static SketchGeometry::SolveResult add(const QJsonObject& draft,const QJsonObject& arguments,
                                           const QJsonObject& context = {});
    static SketchGeometry::SolveResult breakLink(const QJsonObject& draft,const QString& entityId,
                                                 const QJsonObject& context = {});
    static SketchGeometry::SolveResult update(const QJsonObject& draft,const QJsonObject& arguments,
                                              const QJsonObject& context = {});
};
