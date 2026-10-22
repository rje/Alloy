// SPDX-License-Identifier: MIT
#pragma once
#include <QJsonObject>
#include <stdexcept>
#include <functional>
namespace MeshImport {
class Error : public std::runtime_error {
public:
    explicit Error(QJsonObject details);
    QJsonObject details;
};
// Strict STL adapter; geometry stays separate from OCCT B-rep bodies.
using Progress = std::function<void(const QString&,int,int)>;
QJsonObject readStl(const QString& path, const QString& units, const Progress& progress = {});
QJsonObject withUnits(const QJsonObject& millimeterMesh, const QString& units);
QString validate(const QJsonObject& mesh);
QJsonObject summary(const QJsonObject& mesh);
}
