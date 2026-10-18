#pragma once
#include <QJsonObject>
namespace SketchPolygon {
// Retained regular polygon intent evaluated to ordinary closed line geometry.
QJsonObject apply(const QJsonObject& parameters,const QJsonObject& arguments,bool update);
QJsonObject seed(const QJsonObject& parameters);
QJsonObject refreshAnchors(const QJsonObject& parameters);
QString validate(const QJsonObject& parameters);
}
