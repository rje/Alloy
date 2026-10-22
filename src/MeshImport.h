// SPDX-License-Identifier: MIT
#pragma once
#include <QJsonObject>
#include <stdexcept>
namespace MeshImport {
class Error : public std::runtime_error {
public:
    explicit Error(QJsonObject details);
    QJsonObject details;
};
// Strict STL adapter; geometry stays separate from OCCT B-rep bodies.
QJsonObject readStl(const QString& path, const QString& units);
QJsonObject withUnits(const QJsonObject& millimeterMesh, const QString& units);
QString validate(const QJsonObject& mesh);
QJsonObject summary(const QJsonObject& mesh);
}
