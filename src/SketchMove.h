// SPDX-License-Identifier: MIT
#pragma once
#include <QJsonObject>
#include <QSet>
namespace SketchMove {
QString validate(const QJsonObject& parameters);
QSet<QString> owned(const QJsonObject& parameters,bool points=false);
QJsonObject source(const QJsonObject& parameters);
QJsonObject materialize(const QJsonObject& source,const QJsonArray& groups,bool refreshSource=true);
QJsonObject create(const QJsonObject& parameters,const QJsonArray& entities,double du,double dv,const QString& groupId={});
QSet<QString> anchors(const QJsonObject& parameters);
bool touches(const QJsonObject& parameters,const QJsonValue& value);
}
