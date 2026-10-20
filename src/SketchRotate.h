// SPDX-License-Identifier: MIT
#pragma once
#include <QJsonObject>
#include <QSet>
namespace SketchRotate {
QString validate(const QJsonObject&);
QSet<QString> owned(const QJsonObject&,bool points=false);
bool touches(const QJsonObject&,const QJsonValue&);
QJsonObject source(const QJsonObject&);
QJsonObject materialize(const QJsonObject&,const QJsonArray&);
QJsonObject create(const QJsonObject&,const QJsonArray&,const QString& axisId,double angle,const QString& groupId={});
}
