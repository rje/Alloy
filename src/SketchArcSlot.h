// SPDX-License-Identifier: MIT
#pragma once
#include <QJsonObject>
#include <QMap>
#include <QSet>

namespace SketchArcSlot {
// Derive the analytic boundary from the retained path arc and end radius.
// Throws std::invalid_argument for a collapsed path or a changed sweep branch.
QJsonObject derive(const QJsonObject& parameters);
QSet<QString> derivedPointIds(const QJsonObject& parameters);
QSet<QString> derivedArcIds(const QJsonObject& parameters);
QSet<QString> capIds(const QJsonObject& parameters);
QMap<QString,QString> capPrimaryIds(const QJsonObject& parameters);
}
