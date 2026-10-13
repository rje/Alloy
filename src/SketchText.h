#pragma once
#include "SketchProfiles.h"
#include <QJsonObject>
#include <QSet>
#include <TopoDS_Shape.hxx>

// A bounded, generated text object. Only the parameters are authored; glyph
// curves are rebuilt from the pinned font for every evaluation.
class SketchText final {
public:
    struct Result { TopoDS_Shape shape; std::vector<EvaluatedProfile> profiles; QJsonObject summary; };
    static QString validate(const QJsonObject& text, QSet<QString>& ids);
    static Result evaluate(const QJsonObject& sketch, const QJsonObject& frame);
    static QJsonObject create(const QJsonObject& arguments);
    static QString fontId();
    static QString fontSha256();
};
