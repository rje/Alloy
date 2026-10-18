#pragma once
#include <QJsonObject>
namespace SketchPolygon {
// Retained regular polygon intent evaluated to ordinary closed line geometry.
QJsonObject apply(const QJsonObject& parameters,const QJsonObject& arguments,bool update);
QString validate(const QJsonObject& parameters);
}
