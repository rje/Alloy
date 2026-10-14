// SPDX-License-Identifier: MIT
#pragma once
#include "GeometryKernel.h"
#include <QJsonObject>
#include <gp_Trsf.hxx>
namespace LocalOccurrences {
QString validate(const QJsonObject& authored);
QString parentOccurrenceId(const QJsonObject& authored,const QString& occurrenceId);
gp_Trsf worldTransform(const QJsonObject& authored,const QString& occurrenceId);
QJsonArray worldPose(const QJsonObject& authored,const QString& occurrenceId);
std::vector<EvaluatedBody> worldBodies(const QJsonObject& authored,const EvaluatedDocument& definitions);
QJsonObject inspect(const QJsonObject& authored,const EvaluatedDocument& definitions);
}
