#pragma once
#include <QJsonObject>
#include <QUndoStack>
#include "GeometryKernel.h"
#include <memory>

class DocumentEdit;

// Authored intent only. Evaluated BReps and viewport objects are separate caches.
class CadDocument final {
public:
    struct CandidateEvaluation {
        bool ok = false;
        QJsonObject authored;
        std::shared_ptr<const EvaluatedDocument> evaluation;
        QJsonObject diagnostics;
        QString error;
    };
    CadDocument();
    const QJsonObject& authored() const { return authored_; }
    QJsonObject state() const;
    quint64 revision() const { return revision_; }
    QUndoStack& history() { return history_; }
    void reset();
    QString save(const QString& path);
    QString open(const QString& path);
    QString path() const { return path_; }
    const std::vector<EvaluatedBody>& bodies() const { return evaluation_->bodies; }
    const std::vector<EvaluatedSketch>& sketches() const { return evaluation_->sketches; }
    QString commit(const QJsonObject& candidate, const QString& label);
    QJsonObject lastEvaluationError() const { return lastEvaluationError_; }
    static CandidateEvaluation evaluateCandidate(const QJsonObject& candidate);
    // Canonical args: sketchId, profileId, distanceMm, optional name; either an
    // existing featureId edit or a reserved newFeatureId/newBodyId New Body pair.
    // Cut adds operation=cut and exactly one targetBodyIds entry; it owns no body.
    const EvaluatedDocument& evaluation() const { return *evaluation_; }
    EvaluatedDocument evaluationBefore(const QString& featureId) const;
    // Reads and embeds one bounded local STEP source. The caller commits the
    // returned candidate through the ordinary validated document transaction.
    static QJsonObject stepImportCandidate(const QJsonObject& authored, const QJsonObject& arguments);
    static QJsonObject extrusionCandidate(const QJsonObject& authored, const QJsonObject& arguments);
    // Canonical args: operation, targetBodyId, toolBodyIds, keepTools, optional
    // name; featureId edits an existing Boolean, newFeatureId reserves a create.
    // The target retains its producer body ID; this feature owns no new body.
    static QJsonObject booleanCandidate(const QJsonObject& authored, const QJsonObject& arguments);
    static QString validate(const QJsonObject& candidate);
    static double millimeters(double value, const QString& units);
    static QString newId();
private:
    friend class DocumentEdit;
    static QString validateStructure(const QJsonObject& candidate, QJsonObject* errorContext = nullptr);
    void apply(const QJsonObject& value, std::shared_ptr<const EvaluatedDocument> evaluation, const QString& transaction = {});
    QJsonObject authored_;
    QUndoStack history_;
    quint64 revision_ = 0;
    QString lastTransaction_;
    QString path_;
    QJsonObject lastEvaluationError_;
    std::shared_ptr<const EvaluatedDocument> evaluation_ = std::make_shared<const EvaluatedDocument>();
};
