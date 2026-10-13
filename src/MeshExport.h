#pragma once
#include "GeometryKernel.h"

// Export tessellation is detached from the document and viewport caches.
class MeshExport final {
public:
    static QJsonObject writeStl(const std::vector<EvaluatedBody>& bodies, const QString& path,
        const QString& units, bool binary, double surfaceDeviationMm,
        double normalDeviationDeg);
    static QJsonObject writeStl(const EvaluatedBody& body, const QString& path,
        const QString& units, bool binary, double surfaceDeviationMm,
        double normalDeviationDeg);
};
