// SPDX-License-Identifier: MIT
#pragma once
#include "SketchGeometry.h"
#include <Geom_BSplineCurve.hxx>
#include <QVector>
class SketchSpline final {
public:
    struct Evaluation { Handle(Geom_BSplineCurve) curve; QVector<double> parameters; QJsonArray handles; };
    static Evaluation evaluate(const QJsonObject& parameters,const QJsonObject& entity,const QJsonObject& frame = {});
    static QJsonObject comb(const QJsonObject& parameters,const QJsonObject& entity);
    static QJsonArray curveSamples(const Evaluation& evaluated,int count = 17);
    static SketchGeometry::SolveResult addControl(const QJsonObject& draft,const QJsonObject& inputs,const QJsonObject& context = {});
    static SketchGeometry::SolveResult setComb(const QJsonObject& draft,const QJsonObject& inputs,const QJsonObject& context = {});
    static QString validate(const QJsonObject& parameters,const QJsonObject& entity);
    static SketchGeometry::SolveResult add(const QJsonObject& draft,const QJsonObject& inputs,const QJsonObject& context = {});
    static SketchGeometry::SolveResult edit(const QJsonObject& draft,const QString& operation,const QJsonObject& inputs,const QJsonObject& context = {});
};
