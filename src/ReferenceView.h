#pragma once
#include <QWidget>
#include <QImage>
#include <QJsonObject>
#include <QJsonArray>
#include <functional>
#include <AIS_InteractiveContext.hxx>
#include <AIS_Trihedron.hxx>
#include <V3d_View.hxx>
#include <V3d_Viewer.hxx>
#include <AIS_Shape.hxx>
#include <QMap>
#include <QSet>
#include "GeometryKernel.h"
class QContextMenuEvent;

// GPU presentation only. Reference axes/grid are not authored document objects.
class ReferenceView final : public QWidget {
public:
    explicit ReferenceView(QWidget* parent = nullptr);
    ~ReferenceView() override;
    QString orientation = "isometric";
    bool grid = true, origin = true;
    double zoom = 1.0;
    QPointF pan;
    std::function<void(const QString&, const QJsonObject&)> dispatch;
    void sync();
    QJsonObject diagnostics() const;
    QJsonObject cameraState() const;
    void restoreCameraState(const QJsonObject& state);
    void rebindDocument();
    QImage framebuffer();
    void setBodies(const std::vector<EvaluatedBody>& bodies, quint64 revision);
    void setBodyStage(const std::vector<EvaluatedBody>& bodies, const QString& stageKey);
    void clearBodyStage();
    void setSketches(const std::vector<EvaluatedSketch>& sketches, quint64 revision);
    void setConstructionPlanes(const std::vector<EvaluatedConstructionPlane>& planes, quint64 revision);
    void selectConstructionPlane(const QString& planeId);
    QString pickConstructionPlane(double x,double y) const;
    void showSketchDraft(const EvaluatedSketch& sketch);
    void clearSketchDraft();
    void selectProfile(const QString& sketchId, const QString& profileId);
    void selectLoftProfiles(const QJsonArray& references);
    void selectProfiles(const QString& sketchId, const QJsonArray& profileIds);
    QJsonObject pickFinishedProfile(double x, double y) const;
    QJsonObject pickSupportFace(double x, double y, const std::vector<EvaluatedBody>& bodies) const;
    void selectSupportFace(const QJsonObject& support, const TopoDS_Face& face = {});
    void setEditDisplay(const QString& sourceSketchId, const QJsonArray& hiddenBodyIds);
    QJsonObject pickSketch(const QJsonObject& feature, double x, double y, const QString& filter = {}) const;
    void decorateSketch(const QJsonObject& feature, const QJsonObject& selection,
        const QJsonObject& diagnostics, const QString& units, const QJsonObject& pending = {});
    QJsonObject dimensionLayout(const QJsonObject& feature, const QJsonObject& dimension,
        double valueMm, const QString& units) const;
    QJsonObject pickAnnotation(double x, double y) const;
    QJsonObject pickConstraint(double x, double y) const;
    void setSketchAnnotationVisibility(bool showConstraints, bool showDimensions);
    QJsonArray annotations() const { return annotationLayouts_; }
    void setSketchPlane(const QString& plane, const QJsonObject& resolvedFrame = {});
    void lookAtSketch(const QString& plane, const QJsonObject& resolvedFrame = {}, const TopoDS_Shape& content = {});
    QJsonObject sketchPoint(double x, double y, const QString& plane) const;
    QJsonObject projectSketchPoint(double uMm, double vMm, const QString& plane, const QJsonObject& resolvedFrame = {}) const;
    void showTrimPreview(const QString& plane,const QJsonObject& segment);
    void showSketchCursor(const QString& plane, const QPointF& first, const QPointF& second, bool rectangle, bool centered = false);
    void showSketchSnap(const QString& plane, const QJsonObject& snap);
    void showSketchCurveCursor(const QJsonObject& feature);
    void clearSketchCursor();
    void fit(const std::vector<EvaluatedBody>& bodies, const std::vector<EvaluatedSketch>& sketches = {});
    void selectBody(const QString& id);
    void recordContextMenu(const QJsonObject& state,bool shown = false);
    void highlightCombineInputs(const QString& targetBodyId, const QJsonArray& toolBodyIds);
    QString pickBody(double x,double y);
    QJsonObject pickSolidEdge(double x,double y,const std::vector<EvaluatedBody>& bodies) const;
    QJsonObject projectSolidEdge(const EvaluatedSolidEdge& edge) const;
    void showPatternAxes(const QJsonObject& summary);
    void clearPatternAxes();
    void highlightSolidEdges(const std::vector<EvaluatedBody>& bodies,const QString& bodyId,const QJsonArray& edgeIds);
    void clearSolidEdgeHighlights();
    QJsonObject projectBody(const EvaluatedBody& body) const;
    void showPreview(const TopoDS_Shape& shape,const QString& replacedBody = {}, bool neutral = false);
    void showExtrudePreview(const EvaluatedBody& body, const EvaluatedProfile& profile, double distanceMm, const QJsonArray& sourceProfileIds = {}, const QString& units = "mm");
    void showCutPreview(const EvaluatedBody& body, const std::vector<EvaluatedProfile>& profiles, double distanceMm,
        const QJsonArray& sourceProfileIds, const QString& units);
    void showExtrudeHandle(const EvaluatedProfile& profile,const QJsonArray& sourceProfileIds,const QString& units,const QString& operation,double distanceMm = 0);
    void showMovePreview(const TopoDS_Shape& shape,const QString& bodyId);
    void showMoveCue(const QJsonObject& inputs);
    void clearMoveCue(bool resetGesture = false);
    QJsonObject moveCueState() const { return moveCueState_; }
    void clearPreview();
    QPaintEngine* paintEngine() const override { return nullptr; }
protected:
    void showEvent(QShowEvent*) override;
    void paintEvent(QPaintEvent*) override;
    void resizeEvent(QResizeEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void leaveEvent(QEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void mouseDoubleClickEvent(QMouseEvent*) override;
    void contextMenuEvent(QContextMenuEvent*) override;
    void wheelEvent(QWheelEvent*) override;
private:
    std::vector<Handle(AIS_Shape)> solidEdgeHighlights_;
    gp_Pnt sketchWorld(const QString& plane,double u,double v) const;
    QJsonObject frameFor(const QString& plane) const;
    void initialize();
    void displayBodies(const std::vector<EvaluatedBody>& bodies);
    void hoverSupportFace(const QJsonObject& support);
    QJsonObject curveDimensionLayout(const QJsonObject& feature, const QJsonObject& dimension,
        double value, const QString& units) const;
    QJsonObject angleDimensionLayout(const QJsonObject& feature, const QJsonObject& dimension,
        double value) const;
    void clearSketchDecorations();
    void applyEditDisplay(bool redraw = true);
    void clearPreviewPresentation();
    void refreshExtrusionCue();
    void setExtrusionCue(const EvaluatedProfile& profile,double distanceMm,const QJsonArray& sourceProfileIds,const QString& units,const QString& operation);
    void updateExtrusionDrag(const QPointF& position,const QString& phase);
    void replacePreview(const TopoDS_Shape& shape, const QString& replacedBody, bool neutral);
    QPointF windowScale() const;
    Handle(V3d_Viewer) viewer_;
    Handle(V3d_View) view_;
    Handle(AIS_InteractiveContext) context_;
    Handle(AIS_Trihedron) axes_;
    Handle(Graphic3d_Camera) baseCamera_;
    QString error_;
    QPointF last_;
    Qt::MouseButton panButton_ = Qt::NoButton;
    QMap<QString,Handle(AIS_Shape)> models_;
    QMap<QString,bool> bodyVisibility_;
    QString combineTargetId_;
    QJsonObject selectedSupportFace_;
    Handle(AIS_Shape) selectedSupportFaceOverlay_;
    QJsonObject hoveredSupportFace_;
    Handle(AIS_Shape) hoveredSupportFaceOverlay_;
    QJsonArray combineToolIds_;
    QMap<QString,Handle(AIS_Shape)> sketchModels_;
    QMap<QString,Handle(AIS_Shape)> constructionSketchModels_;
    QMap<QString,Handle(AIS_Shape)> constructionPlaneModels_;
    std::vector<EvaluatedConstructionPlane> constructionPlanes_;
    QString selectedConstructionPlane_;
    quint64 constructionPlaneRevision_ = 0;
    std::vector<Handle(AIS_Shape)> patternAxes_;
    QMap<QString,Handle(AIS_Shape)> finishedProfileModels_;
    std::vector<EvaluatedSketch> finishedSketches_;
    QSet<QString> selectedLoftProfileIds_;
    QString selectedProfile_, selectedProfileSketch_, editSourceSketch_;
    QJsonArray selectedProfileIds_;
    QJsonArray hiddenBodyIds_;
    Handle(AIS_Shape) sketchDraft_, sketchConstructionDraft_, projectedDraft_, sketchCursor_, sketchSnapCue_;
    QJsonArray projectedLines_, supportPoints_;
    QJsonArray constructionDraftLineIds_;
    int rightMousePressCount_ = 0;
    int contextMenuEventCount_ = 0;
    int contextMenuShownCount_ = 0;
    QJsonObject lastContextMenu_;
    QJsonObject sketchSnapState_;
    std::vector<Handle(AIS_InteractiveObject)> sketchDecorations_;
    QJsonArray annotationLayouts_;
    QJsonArray constraintGlyphs_;
    bool showConstraints_ = true;
    bool showDimensions_ = true;
    std::vector<EvaluatedProfile> draftProfiles_;
    QMap<QString,Handle(AIS_Shape)> profileModels_;
    QJsonObject decorationFeature_, decorationSelection_, decorationDiagnostics_, decorationPending_;
    QString decorationUnits_, draggedAnnotation_;
    QString annotationGestureId_;
    quint64 nextAnnotationGesture_ = 0;
    QPointF annotationPress_;
    bool annotationDragged_ = false;
    QString sketchPlane_, draftSketchId_;
    QJsonObject sketchFrame_;
    quint64 sketchRevision_ = 0;
    Handle(AIS_Shape) preview_;
    std::vector<Handle(AIS_Shape)> cutTools_;
    std::vector<Handle(AIS_Shape)> cutProfileOverlays_;
    std::vector<Handle(AIS_InteractiveObject)> extrusionCue_;
    QJsonObject extrusionCueState_;
    bool extrusionDragActive_ = false;
    QPointF extrusionDragPress_;
    double extrusionDragInitialMm_ = 0;
    QString previewReplacedBody_, previewProfile_;
    QJsonArray previewProfileIds_;
    quint64 modelRevision_ = 0;
    std::vector<EvaluatedBody> documentBodies_;
    QString bodyStageKey_;
    double gridCenterU_ = 0, gridCenterV_ = 0, gridStep_ = 10;
    double gridWidth_ = 200, gridHeight_ = 200;
    bool adaptiveGrid_ = false;
    double sceneScale_ = 400;
    int msaaRequested_ = 4, msaaEnabled_ = 0, msaaMaximum_ = 0;
    bool msaaTextures_ = false;
    std::vector<Handle(AIS_Shape)> moveCue_;
    QJsonObject moveCueState_;
    QJsonObject moveDrag_;
    gp_Pnt sceneCenter_;
};
