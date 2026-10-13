// SPDX-License-Identifier: MIT
#pragma once
#include "SketchGeometry.h"
#include <Geom_BezierCurve.hxx>

class SketchConic final {
public:
    static QString validate(const QJsonObject& parameters, const QJsonObject& entity);
    static Handle(Geom_BezierCurve) evaluate(const QJsonObject& parameters,
                                            const QJsonObject& entity,
                                            const QJsonObject& frame = {});
    static SketchGeometry::SolveResult add(const QJsonObject& draft,
                                          const QJsonObject& inputs,
                                          const QJsonObject& context = {});
    static SketchGeometry::SolveResult setRho(const QJsonObject& draft,
                                             const QString& entityId, double rho,
                                             const QJsonObject& context = {});
};
