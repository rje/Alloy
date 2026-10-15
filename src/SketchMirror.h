// SPDX-License-Identifier: MIT
#pragma once
#include <QJsonObject>
#include <QString>
#include <QStringList>

// Retained sketch-entity reflection. The group records identity; PlaneGCS
// enforces its point and radius relationships during every sketch solve.
namespace SketchMirror {
QString validate(const QJsonObject& parameters);
QJsonObject create(const QJsonObject& parameters,const QJsonObject& arguments);
QJsonObject seed(const QJsonObject& parameters);
QJsonObject refreshSplineHandles(const QJsonObject& parameters);
bool supportedSource(const QJsonObject& entity);
QStringList pointIds(const QJsonObject& entity);
}
