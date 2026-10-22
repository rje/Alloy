// SPDX-License-Identifier: MIT
#pragma once
#include <QJsonObject>
#include <QByteArray>
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
QJsonObject readObj(const QString& path, const QString& units, const Progress& progress = {});
// Owned bounded source snapshots let native unit previews use the exact same
// canonical-mm polygon policy as direct import without rereading the source.
QByteArray readObjSource(const QString& path);
QJsonObject readObjData(const QByteArray& bytes, const QString& sourcePath, const QString& units, const Progress& progress = {});
QJsonObject withUnits(const QJsonObject& millimeterMesh, const QString& units);
QString validate(const QJsonObject& mesh);
QJsonObject summary(const QJsonObject& mesh);
}
