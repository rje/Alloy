#pragma once
#include <QJsonObject>
#include <QString>
#include <TopoDS_Face.hxx>
#include <vector>

// Derived geometry, never a separately authored feature or persisted display cache.
struct EvaluatedProfile {
    QString profileId, sketchId, plane;
    TopoDS_Face face;
    QJsonObject summary;
};

class SketchProfiles final {
public:
    struct Result {
        bool ok = false;
        EvaluatedProfile profile;
        QJsonObject diagnostics;
        QString error;
    };
    struct RegionsResult {
        bool ok = false;
        std::vector<EvaluatedProfile> profiles;
        QJsonObject diagnostics;
        QString error;
    };
    static RegionsResult deriveRegions(const QJsonObject& solvedFeature);
    // Input is already validated/solved authored sketch intent. No mutation or
    // solver call occurs here. Open or unsupported regions return diagnostics,
    // leaving the source sketch valid and available for further drawing.
    static Result derive(const QJsonObject& solvedFeature);
};
