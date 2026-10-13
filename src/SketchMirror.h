// SPDX-License-Identifier: MIT
#pragma once
#include <QJsonObject>
#include <QString>

// Retained sketch-entity reflection. The group records identity; PlaneGCS
// enforces its point and radius relationships during every sketch solve.
namespace SketchMirror {
QString validate(const QJsonObject& parameters);
QJsonObject create(const QJsonObject& parameters,const QJsonObject& arguments);
QJsonObject seed(const QJsonObject& parameters);
}
