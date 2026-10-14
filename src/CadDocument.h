#pragma once
#include <QJsonArray>
#include <QJsonObject>
#include <QUndoStack>
#include "GeometryKernel.h"
#include <memory>
#include <optional>

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
    QString activeOwnerId() const;
    QString activateOwner(const QString& ownerId);
    static bool hasOwner(const QJsonObject& authored, const QString& ownerId);
    static QJsonObject componentCandidate(const QJsonObject& authored, const QString& name);
    static QJsonObject renameBodyCandidate(const QJsonObject& authored,const EvaluatedDocument& evaluated,
        const QString& bodyId,const QString& name);
    static QJsonObject reparentCandidate(const QJsonObject& authored,const EvaluatedDocument& evaluated,
        const QString& kind,const QJsonArray& objectIds,const QString& destinationId);
    static QJsonObject lengthParameterCandidate(const QJsonObject& authored,const QJsonObject& arguments);
    static QJsonArray modelLengthRows(const QJsonObject& authored);
    static QJsonObject modelLengthCandidate(const QJsonObject& authored,const QJsonObject& arguments);
    static QJsonObject resolveLengthBindings(const QJsonObject& authored);
    quint64 revision() const { return revision_; }
    QUndoStack& history() { return history_; }
    void reset();
    QString save(const QString& path);
    QString open(const QString& path);
    QString path() const { return path_; }
    const std::vector<EvaluatedBody>& bodies() const { return evaluation_->bodies; }
    std::vector<EvaluatedBody> worldBodies(std::optional<double> transientPositionMm = {}) const;
    QJsonObject assemblyState(std::optional<double> transientPositionMm = {}) const;
    const std::vector<EvaluatedSketch>& sketches() const { return evaluation_->sketches; }
    QString commitAnalyses(const QJsonArray& analyses, const QString& label);
    static QJsonObject sectionCandidate(const QJsonObject& authored, const QJsonObject& args);
    QString commit(const QJsonObject& candidate, const QString& label);
    QJsonObject lastEvaluationError() const { return lastEvaluationError_; }
    static CandidateEvaluation evaluateCandidate(const QJsonObject& candidate);
    // Reuse only the exact accepted prefix for one bounded append command.
    CandidateEvaluation prepareCandidate(const QJsonObject& candidate) const;
    // Canonical args: sketchId, profileId, distanceMm, optional name; either an
    // existing featureId edit or a reserved newFeatureId/newBodyId New Body pair.
    // Cut adds operation=cut and exactly one targetBodyIds entry; it owns no body.
    const EvaluatedDocument& evaluation() const { return *evaluation_; }
    EvaluatedDocument evaluationBefore(const QString& featureId) const;
    // Reads and embeds one bounded local STEP source. The caller commits the
    // returned candidate through the ordinary validated document transaction.
    static QJsonObject stepImportCandidate(const QJsonObject& authored, const QJsonObject& arguments);
    static QJsonObject localReferenceCandidate(const QJsonObject& authored,const QJsonObject& arguments);
    static QJsonObject localReferencePlacementCandidate(const QJsonObject& authored,const QString& featureId,const QJsonObject& placement);
    static QJsonObject componentOpacityCandidate(const QJsonObject& authored,const QString& definitionId,double opacity);
    static QJsonObject extrusionCandidate(const QJsonObject& authored, const QJsonObject& arguments);
    // Canonical args: operation, targetBodyId, toolBodyIds, keepTools, optional
    // name; featureId edits an existing Boolean, newFeatureId reserves a create.
    // The target retains its producer body ID; this feature owns no new body.
    static QJsonObject filletCandidate(const QJsonObject& authored, const QJsonObject& arguments);
    static QJsonObject chamferCandidate(const QJsonObject& authored, const QJsonObject& arguments);
    static QJsonObject patternCandidate(const QJsonObject& authored,const QJsonObject& arguments);
    static QJsonObject sweepCandidate(const QJsonObject& authored,const QJsonObject& arguments);
    static QJsonObject revolveCandidate(const QJsonObject& authored,const QJsonObject& arguments);
    static QJsonObject loftCandidate(const QJsonObject& authored,const QJsonObject& arguments);
    static QJsonObject surfaceExtrudeCandidate(const QJsonObject& authored,const QJsonObject& arguments);
    static QJsonObject thickenCandidate(const QJsonObject& authored,const QJsonObject& arguments);
    static QJsonObject sphereCandidate(const QJsonObject& authored, const QJsonObject& arguments);
    static QJsonObject holeCandidate(const QJsonObject& authored, const QJsonObject& arguments);
    static QJsonObject shellCandidate(const QJsonObject& authored, const QJsonObject& arguments);
    static QJsonObject splitBodyCandidate(const QJsonObject& authored,const QJsonObject& arguments);
    static QJsonObject circularPatternCandidate(const QJsonObject& authored,const QJsonObject& arguments);
    static QJsonObject mirrorCandidate(const QJsonObject& authored, const QJsonObject& arguments);
    static QJsonObject featureMirrorCandidate(const QJsonObject& authored, const QJsonObject& arguments);
    static QJsonObject reorderCandidate(const QJsonObject& authored,const QString& featureId,const QString& afterFeatureId);
    static QJsonObject moveCandidate(const QJsonObject& authored, const QJsonObject& arguments);
    static QJsonObject booleanCandidate(const QJsonObject& authored, const QJsonObject& arguments);
    static QString validate(const QJsonObject& candidate);
    static double millimeters(double value, const QString& units);
    static QString newId();
private:
    friend class DocumentEdit;
    static CandidateEvaluation evaluateCandidateFromPrefix(const QJsonObject& candidate,
        const EvaluatedDocument* prefix, int firstFeature);
    static QString validateStructure(const QJsonObject& candidate, QJsonObject* errorContext = nullptr);
    void apply(const QJsonObject& value, std::shared_ptr<const EvaluatedDocument> evaluation, const QString& transaction = {});
    QJsonObject authored_;
    QUndoStack history_;
    quint64 revision_ = 0;
    QString lastTransaction_;
    QString path_;
    QString activeOwnerId_;
    QJsonObject lastEvaluationError_;
    std::shared_ptr<const EvaluatedDocument> evaluation_ = std::make_shared<const EvaluatedDocument>();
};
