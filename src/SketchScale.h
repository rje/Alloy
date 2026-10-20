// SPDX-License-Identifier: MIT
#pragma once
#include <QJsonObject>
#include <QSet>
namespace SketchScale {
QString validate(const QJsonObject&);
QSet<QString> owned(const QJsonObject&, bool points = false);
bool touches(const QJsonObject&, const QJsonValue&);
QJsonObject source(const QJsonObject&);
QJsonObject materialize(const QJsonObject&, const QJsonArray&);
void verifySource(const QJsonObject&, const QJsonObject&);
QJsonObject create(const QJsonObject&, const QJsonArray&, const QString& pivotId,
                   double factor, const QString& groupId = {});
// Explicit visible-point edit: inverse map to source, never assign derived cache.
QJsonObject editPoint(const QJsonObject&, const QString& pointId, double u, double v);
}
