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
    static QJsonObject renameComponentCandidate(const QJsonObject& authored,const QString& definitionId,const QString& name);
    static QJsonObject componentsFromBodiesCandidate(const QJsonObject& authored,const EvaluatedDocument& evaluated,
        const QJsonArray& bodyIds);
    static QJsonObject reparentOccurrenceCandidate(const QJsonObject& authored,const QJsonArray& occurrenceIds,
        const QString& destinationId);
    static QJsonObject copyOccurrenceCandidate(const QJsonObject& authored,const QString& occurrenceId,const QJsonObject& placement);
    static QJsonObject pinOccurrenceCandidate(const QJsonObject& authored,const QString& occurrenceId);
    static QJsonObject renameBodyCandidate(const QJsonObject& authored,const EvaluatedDocument& evaluated,
        const QString& bodyId,const QString& name);
    static QJsonObject reparentCandidate(const QJsonObject& authored,const EvaluatedDocument& evaluated,
        const QString& kind,const QJsonArray& objectIds,const QString& destinationId);
    static QJsonObject lengthParameterCandidate(const QJsonObject& authored,const QJsonObject& arguments);
    static QJsonArray evaluatedLengthParameters(const QJsonObject& authored);
    static QJsonArray modelLengthRows(const QJsonObject& authored);
    static QJsonObject modelLengthCandidate(const QJsonObject& authored,const QJsonObject& arguments);
    static QJsonObject resolveLengthBindings(const QJsonObject& authored);
    static QJsonObject bindExtrudeDistanceExpression(const QJsonObject& authored,const QString& expression,const QString& units);
    quint64 revision() const { return revision_; }
    QUndoStack& history() { return history_; }
    void reset();
    QString save(const QString& path);
    QString open(const QString& path);
    static CandidateEvaluation prepareOpen(const QString& path);
    QString adoptOpen(const CandidateEvaluation& prepared, const QString& path,
        const QJsonObject& expectedBase, quint64 expectedRevision);
    QString recover(const QJsonObject& authored);
    bool isDirty() const { return recoveredDirty_ || !history_.isClean(); }
    QString path() const { return path_; }
    QString label() const;
    const std::vector<EvaluatedBody>& bodies() const { return evaluation().bodies; }
    std::vector<EvaluatedBody> worldBodies(std::optional<double> transientPositionMm = {}) const;
    QJsonObject assemblyState(std::optional<double> transientPositionMm = {}) const;
    const std::vector<EvaluatedSketch>& sketches() const { return evaluation().sketches; }
    QString commitCanvases(const QJsonArray& canvases,const QString& label);
    QString commitNamedViews(const QJsonArray& views,const QString& label);
    QString commitHomeCamera(std::optional<QJsonObject> camera,const QString& label);
    QString commitViewConvention(const QJsonObject& convention,const QString& label);
    QString commitAnalyses(const QJsonArray& analyses, const QString& label);
    static QJsonObject sectionCandidate(const QJsonObject& authored, const QJsonObject& args);
    QString commit(const QJsonObject& candidate, const QString& label);
    // Adopt a worker's fully evaluated Surface Extrude draft only at its exact base.
    QString commitSurfaceExtrudeEvaluation(const CandidateEvaluation& prepared,
        const QJsonObject& requestedCandidate, const QJsonObject& expectedBase, quint64 expectedRevision, const QString& label);
    QString commitSketchEvaluation(const CandidateEvaluation& prepared,
        const QJsonObject& requestedCandidate, const QJsonObject& expectedBase, quint64 expectedRevision, const QString& label="Edit sketch");
    QString commitThreadEvaluation(const CandidateEvaluation& prepared, const QJsonObject& requestedCandidate,
        const QJsonObject& expectedBase, quint64 expectedRevision, const QString& label="Thread");
    QString commitLoftEvaluation(const CandidateEvaluation& prepared,
        const QJsonObject& requestedCandidate, const QJsonObject& expectedBase, quint64 expectedRevision);
    QString commitSurfaceTrimEvaluation(const CandidateEvaluation& prepared,
        const QJsonObject& requestedCandidate, const QJsonObject& expectedBase, quint64 expectedRevision);
    QString commitBoundaryFillEvaluation(const CandidateEvaluation& prepared,
        const QJsonObject& requestedCandidate, const QJsonObject& expectedBase, quint64 expectedRevision, const QString& label);
    QJsonObject lastEvaluationError() const { return lastEvaluationError_; }
    static CandidateEvaluation evaluateCandidate(const QJsonObject& candidate);
    // Reuse only the exact accepted prefix for one bounded append command.
    CandidateEvaluation prepareCandidate(const QJsonObject& candidate) const;
    // Canonical args: sketchId, profileId, distanceMm, optional name; either an
    // existing featureId edit or a reserved newFeatureId/newBodyId New Body pair.
    // Cut adds operation=cut and exactly one targetBodyIds entry; it owns no body.
    const EvaluatedDocument& evaluation() const { return rollbackEvaluation_ ? *rollbackEvaluation_ : *evaluation_; }
    // Read-only transient prefix inspection; never authored, serialized or undoable.
    QString rollbackBefore(const QString& featureId);
    void returnToLatest();
    bool isRolledBack() const { return bool(rollbackEvaluation_); }
    QJsonObject rollbackState() const;
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
    static QJsonObject boundaryFillCandidate(const QJsonObject& authored,const QJsonObject& arguments);
    static QJsonObject surfaceTrimCandidate(const QJsonObject& authored,const QJsonObject& arguments);
    static QJsonObject removeBodyCandidate(const QJsonObject& authored,const QJsonObject& arguments);
    static QJsonObject thickenCandidate(const QJsonObject& authored,const QJsonObject& arguments);
    static QJsonObject sphereCandidate(const QJsonObject& authored, const QJsonObject& arguments);
    static QJsonObject faceOffsetCandidate(const QJsonObject& authored, const QJsonObject& arguments);
    static QJsonObject threadCandidate(const QJsonObject& authored, const QJsonObject& arguments);
    static QJsonObject holeCandidate(const QJsonObject& authored, const QJsonObject& arguments);
    static QJsonObject shellCandidate(const QJsonObject& authored, const QJsonObject& arguments);
    static QJsonObject splitBodyCandidate(const QJsonObject& authored,const QJsonObject& arguments);
    static QJsonObject rectangularBodyPatternCandidate(const QJsonObject& authored,const QJsonObject& arguments);
    static QJsonObject circularPatternCandidate(const QJsonObject& authored,const QJsonObject& arguments);
    static QJsonObject mirrorCandidate(const QJsonObject& authored, const QJsonObject& arguments);
    static QJsonObject featureMirrorCandidate(const QJsonObject& authored, const QJsonObject& arguments);
    static QJsonObject reorderCandidate(const QJsonObject& authored,const QString& featureId,const QString& afterFeatureId);
    static QJsonObject bodyCopyCandidate(const QJsonObject& authored,const QJsonObject& snapshot,const QJsonObject& arguments);
    static QJsonObject alignCandidate(const QJsonObject& authored, const QJsonObject& arguments);
    static QJsonObject moveCandidate(const QJsonObject& authored, const QJsonObject& arguments);
    static QJsonObject booleanCandidate(const QJsonObject& authored, const QJsonObject& arguments);
    static QString validate(const QJsonObject& candidate);
    static QString faceSketchOriginContextError(const QJsonObject& authored,const EvaluatedDocument& evaluated,
        const QString& ownerId,const QString& supportBodyId);
    static double millimeters(double value, const QString& units);
    static QString newId();
private:
    QString commitEvaluatedFeature(const CandidateEvaluation& prepared,
        const QJsonObject& requestedCandidate, const QJsonObject& expectedBase, quint64 expectedRevision,
        const QString& featureType, const QString& label);
    friend class DocumentEdit;
    static CandidateEvaluation evaluateCandidateFromPrefix(const QJsonObject& candidate,
        const EvaluatedDocument* prefix, int firstFeature);
    static QString validateStructure(const QJsonObject& candidate, QJsonObject* errorContext = nullptr);
    void apply(const QJsonObject& value, std::shared_ptr<const EvaluatedDocument> evaluation, const QString& transaction = {});
    std::shared_ptr<const EvaluatedDocument> rollbackEvaluation_;
    QString rollbackBeforeFeatureId_;
    QJsonObject authored_;
    QUndoStack history_;
    quint64 revision_ = 0;
    QString lastTransaction_;
    QString path_;
    bool recoveredDirty_ = false;
    QString activeOwnerId_;
    QJsonObject lastEvaluationError_;
    std::shared_ptr<const EvaluatedDocument> evaluation_ = std::make_shared<const EvaluatedDocument>();
};
