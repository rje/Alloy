#pragma once
#include <QJsonObject>
#include <QSet>
#include <QString>

// One retained straight, open chain. Its source IDs are sketch or projected
// lines; the five generated lines and their points keep authored identities.
namespace SketchOpenChainOffset {
QSet<QString> derivedPointIds(const QJsonObject& parameters);
QSet<QString> derivedEntityIds(const QJsonObject& parameters);
QString validate(const QJsonObject& parameters);
QJsonObject derive(const QJsonObject& expandedParameters);
QJsonObject add(const QJsonObject& draft,const QJsonObject& expandedParameters,
                const QJsonObject& arguments);
QJsonObject update(const QJsonObject& draft,const QJsonObject& expandedParameters,
                   const QJsonObject& arguments);
}
