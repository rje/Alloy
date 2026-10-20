// SPDX-License-Identifier: MIT
#pragma once
#include <QJsonObject>
#include <QSet>
namespace SketchAxis3d {
QString validate(const QJsonObject& parameters);
QJsonObject upsert(const QJsonObject& parameters,double u,double v,double height,const QString& axisId={});
QJsonObject erase(const QJsonObject& parameters,const QString& axisId);
QSet<QString> ids(const QJsonObject& parameters);
bool referenced(const QJsonObject& parameters,const QString& axisId);
QJsonArray inspect(const QJsonObject& parameters);
}
