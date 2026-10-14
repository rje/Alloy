#pragma once
#include "GeometryKernel.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <vector>

// Select evaluated, already world-placed bodies. An occurrence is an instance,
// so source body IDs may appear more than once in a valid result.
namespace ExportScope {
struct Selection {
    std::vector<EvaluatedBody> bodies;
    QJsonArray instances;
    QString label;
};
Selection select(const QJsonObject& authored,const std::vector<EvaluatedBody>& placed,
    const QJsonObject& request);
}
