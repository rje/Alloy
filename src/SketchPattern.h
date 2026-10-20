// SPDX-License-Identifier: MIT
#pragma once
#include <QJsonObject>
#include <QSet>
namespace SketchPattern {
QString validate(const QJsonObject& parameters);
QJsonObject create(const QJsonObject& parameters,const QJsonObject& arguments);
QJsonObject derive(const QJsonObject& parameters);
QJsonObject sourceOnly(const QJsonObject& parameters);
QSet<QString> resultIds(const QJsonObject& parameters,bool points=false);
}
