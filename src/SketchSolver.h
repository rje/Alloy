// SPDX-License-Identifier: MIT
#pragma once
#include <QJsonObject>
#include <QString>

namespace SketchSolver {
struct Result {
    bool accepted = false;
    QJsonObject parameters;
    QJsonObject diagnostics;
    QString error;
};
// Coordinates and driving lengths are canonical millimeters; line-angle and
// arc-sweep dimensions use degrees. The input is never
// mutated; rejected results retain it unchanged. UUIDs stay outside PlaneGCS.
Result solve(const QJsonObject& parameters);
}
