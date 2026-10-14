// SPDX-License-Identifier: MIT
#pragma once
#include "GeometryKernel.h"
#include <QImage>
namespace ReferenceCanvas {
QJsonObject candidate(const QJsonObject& document,const QJsonObject& args);
QString validate(const QJsonObject& row);
QJsonObject frame(const QJsonObject& row,const EvaluatedDocument& evaluation);
QJsonObject calibrate(const QJsonObject& row,const QJsonArray& first,const QJsonArray& second,double distanceMm);
QImage image(const QJsonObject& row);
QJsonObject inspect(const QJsonObject& row,const EvaluatedDocument& evaluation);
QPointF pixelToPlane(const QJsonObject& row,double x,double y);
QPointF planeToPixel(const QJsonObject& row,double u,double v);
}
