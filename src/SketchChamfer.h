// SPDX-License-Identifier: MIT
#pragma once
#include <QJsonObject>
#include <QSet>
namespace SketchChamfer {
QString validate(const QJsonObject& parameters,QSet<QString>& authoredIds);
QJsonObject derive(const QJsonObject& parameters);
}
