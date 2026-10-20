// SPDX-License-Identifier: MIT
#pragma once
#include <QJsonObject>
#include <QSet>
#include "SketchSolver.h"
namespace SketchRotate {
QString validate(const QJsonObject&);
QSet<QString> owned(const QJsonObject&,bool points=false);
bool touches(const QJsonObject&,const QJsonValue&);
QJsonObject source(const QJsonObject&);
QJsonObject materialize(const QJsonObject&,const QJsonArray&);
std::optional<SketchSolver::SourceOrientation> orientation(const QJsonObject&);
QSet<QString> anchors(const QJsonObject&);
QJsonObject resizeSource(const QJsonObject&,const QString& dimensionId,double valueMm);
void verifySource(const QJsonObject& authoritative,const QJsonObject& solved);
QJsonObject create(const QJsonObject&,const QJsonArray&,const QString& axisId,double angle,const QString& groupId={});
}
