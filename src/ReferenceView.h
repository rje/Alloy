#pragma once
#include <QWidget>
#include <gp_Lin.hxx>
#include <QImage>
#include <QRectF>
#include <QJsonObject>
#include <QJsonArray>
#include <functional>
#include <optional>
#include <memory>
#include <AIS_InteractiveContext.hxx>
#include <AIS_Trihedron.hxx>
#include <AIS_ViewCube.hxx>
#include <V3d_View.hxx>
#include <V3d_Viewer.hxx>
#include <AIS_Shape.hxx>
#include <AIS_Point.hxx>
#include <Graphic3d_ClipPlane.hxx>
#include <QMap>
#include <QSet>
#include <QStringList>
#include "GeometryKernel.h"
class QContextMenuEvent;
class QKeyEvent;
class QRubberBand;
class QToolButton;
class QLabel;

// Value-only pick geometry captured on the GUI thread. No view/camera handle.
struct ViewPickSnapshot {
    Graphic3d_Mat4d orientation, projection, inverseOrientation, inverseProjection;
    QPointF deviceScale, pixel;
    int physicalWidth=0, physicalHeight=0, logicalHeight=0;
    double cameraScale=0;
    gp_Lin ray(double x,double y) const;
    QPointF project(const gp_Pnt& world) const;
};

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
    void setCanvases(const QJsonArray& rows,const EvaluatedDocument& evaluation);
    void setCanvasPicking(bool active) { canvasPicking_=active; }
    QJsonArray canvasState() const { return canvasState_; }
    TopoDS_Shape canvasShape(const QString& id) const { return canvasModels_.contains(id) ? canvasModels_[id]->Shape() : TopoDS_Shape{}; }
    QJsonObject diagnostics() const;
    QJsonObject cameraState() const;
    QJsonObject viewConvention() const;
    void setViewConvention(const QJsonObject& convention,bool preserveCamera=true);
    void freezeWorldCamera();
    QJsonObject conventionFromCurrentView(const QString& face) const;
    void restoreCameraState(const QJsonObject& state);
    void setProjectionPolicy(const QString& policy);
    QString projectionPolicy() const { return projectionPolicy_; }
    void orbitFree(double deltaX,double deltaY,bool useCustomPivot=true);
    void orbitConstrained(double deltaX,double deltaY,bool useCustomPivot=true);
    void orbitByMode(double deltaX,double deltaY,bool useCustomPivot=true);
    bool zoomAt(double factor,std::optional<QPointF> cursor=std::nullopt);
    bool zoomWindow(const QRectF& rectangle);
    void setNavigationTool(const QString& mode);
    QString navigationTool() const { return navigationTool_; }
    void setInputPreset(const QString& preset);
    QString inputPreset() const { return inputPreset_; }
    bool inputGestureActive() const { return panButton_!=Qt::NoButton || orbitButton_!=Qt::NoButton || zoomButton_!=Qt::NoButton; }
    void cancelInputGesture();
    bool navigationDragging() const { return navigationPressActive_; }
    void cancelNavigationGesture(bool suppressRelease=false);
    void setOrbitMode(const QString& mode);
    void centerNamedView() { viewCenter_=sceneCenter_; }
    QString orbitMode() const { return orbitMode_; }
    QJsonObject lastNavigationInput() const { return lastNavigationInput_; }
    QJsonObject cubeHit(double x,double y);
    bool setCubeOrientation(const QString& region);
    static QStringList cubeRegions();
    QJsonObject cubeFaceState() const;
    bool cubeAdjacent(const QString& side);
    bool cubeRoll(const QString& side);
    bool cubeGestureActive() const { return cubePressHandled_; }
    void cancelCubeGesture(bool suppressRelease=false);
    void beginOrbitCenterPick();
    void cancelOrbitCenterPick();
    bool orbitCenterPicking() const { return orbitCenterPicking_; }
    bool bodyStageActive() const { return !bodyStageKey_.isEmpty(); }
    QJsonObject pickOrbitCenter(double x,double y) const;
    void setOrbitCenter(const QJsonObject& hit);
    void resetOrbitCenter();
    void setFreeOrbitTool(bool active) { freeOrbitTool_=active; if(!active && orbitButton_!=Qt::NoButton){orbitButton_=Qt::NoButton;unsetCursor();} }
    bool freeOrbitTool() const { return freeOrbitTool_; }
    bool orbitDragging() const { return orbitButton_!=Qt::NoButton; }
    void rebindDocument();
    QImage framebuffer();
    void setSectionAnalysis(const QJsonObject& analysis);
    // Only these explicitly worker-prepared shapes may bypass AIS remeshing.
    // Replaced on the next Thread result; unrelated/changed shapes use normal AIS.
    void setPreparedThreadMeshes(const std::vector<TopoDS_Shape>& shapes);
    void setBodies(const std::vector<EvaluatedBody>& bodies, quint64 revision);
    void setBodyStage(const std::vector<EvaluatedBody>& bodies, const QString& stageKey);
    void clearBodyStage();
    void setSketches(const std::vector<EvaluatedSketch>& sketches, quint64 revision);
    void setConstructionPlanes(const std::vector<EvaluatedConstructionPlane>& planes, quint64 revision);
    void selectConstructionPlane(const QString& planeId);
    QString pickConstructionPlane(double x,double y) const;
    void showSketchDraft(const EvaluatedSketch& sketch);
    void clearSketchDraft();
    QJsonObject pickFinishedPoint(double x,double y) const;
    void selectFinishedPoint(const QJsonObject& reference);
    QJsonObject finishedPointSelection() const { return selectedFinishedPoint_; }
    void selectProfile(const QString& sketchId, const QString& profileId);
    void selectLoftProfiles(const QJsonArray& references);
    void selectProfiles(const QString& sketchId, const QJsonArray& profileIds);
    QJsonObject pickFinishedProfile(double x, double y) const;
    QJsonObject pickSupportFace(double x, double y, const std::vector<EvaluatedBody>& bodies) const;
    std::shared_ptr<const ViewPickSnapshot> capturePick(double x,double y) const;
    static QJsonObject pickSupportFace(const ViewPickSnapshot& pick,const std::vector<EvaluatedBody>& bodies);
    static QJsonObject pickSolidEdge(const ViewPickSnapshot& pick,const std::vector<EvaluatedBody>& bodies);
    void selectSupportFace(const QJsonObject& support, const TopoDS_Face& face = {}, bool outline = false);
    void setCommandSketches(const QSet<QString>& ids);
    void showSweepProfile(const EvaluatedProfile& profile);
    void showSweepPath(const EvaluatedSketch& sketch,const QJsonArray& entityIds);
    void clearSweepPath();
    void setEditDisplay(const QString& sourceSketchId, const QJsonArray& hiddenBodyIds);
    QJsonObject pickSketch(const QJsonObject& feature, double x, double y, const QString& filter = {}, const QJsonObject& resolvedFrame = {}) const;
    QJsonObject selectSketchWindow(const QJsonObject& feature, double x1, double y1, double x2, double y2,
        const QJsonObject& resolvedFrame = {}) const;
    void decorateSketch(const QJsonObject& feature, const QJsonObject& selection,
        const QJsonObject& diagnostics, const QString& units, const QJsonObject& pending = {});
    QJsonObject dimensionLayout(const QJsonObject& feature, const QJsonObject& dimension,
        double valueMm, const QString& units) const;
    void decorateConicHandle(const QJsonObject& feature,const QJsonObject& selection);
    void clearConicDrag() { conicDragging_=false;conicHandleState_={}; }
    QJsonObject conicHandle() const { return conicHandleState_; }
    void decorateSplineHandles(const QJsonObject& feature,const QJsonObject& selection);
    void clearSplineDrag() { splineDrag_={}; }
    void clearSketchLineArcDrag() { sketchLineArcDrag_={}; }
    QJsonObject splineHandles() const { return splineHandleState_; }
    QJsonObject pickAnnotation(double x, double y) const;
    QJsonObject pickConstraint(double x, double y) const;
    void setSketchAnnotationVisibility(bool showConstraints, bool showDimensions);
    QJsonArray annotations() const { return annotationLayouts_; }
    void setSketchPlane(const QString& plane, const QJsonObject& resolvedFrame = {});
    void lookAtSketch(const QString& plane, const QJsonObject& resolvedFrame = {}, const TopoDS_Shape& content = {});
    bool fitSelection(const TopoDS_Shape& shape);
    bool fitFinishedSketch(const EvaluatedSketch& sketch);
    bool lookAtPlane(const QJsonObject& frame,const TopoDS_Shape& shape);
    bool lookAtFinishedSketch(const EvaluatedSketch& sketch);
    QJsonObject sketchPoint(double x, double y, const QString& plane, const QJsonObject& resolvedFrame = {}) const;
    QJsonObject projectSketchPoint(double uMm, double vMm, const QString& plane, const QJsonObject& resolvedFrame = {}) const;
    void showTrimPreview(const QString& plane,const QJsonObject& segment);
    void showExtendPreview(const QString& plane,const QJsonObject& segment);
    void showSketchCursor(const QString& plane, const QPointF& first, const QPointF& second, bool rectangle, bool centered = false);
    void showSketchSnap(const QString& plane, const QJsonObject& snap);
    void showSketchCurveCursor(const QJsonObject& feature);
    void showSketchSlotCursor(const QJsonObject& feature,bool arcSlot);
    void showSketchEllipseCursor(const QJsonObject& feature);
    void showSketchEdgesCursor(const QJsonObject& feature,const QJsonArray& ids);
    void showSketchPolygonCursor(const QJsonObject& feature);
    void cancelSlotPointDrag();
    void showSlotDragPreview(const EvaluatedSketch& evaluated,const QJsonObject& draft,const QJsonObject& selection,const QJsonObject& diagnostics,const QString& units);
    void clearSketchCursor();
    void showSketchTextHandles(const QJsonObject& text,const QString& plane,const QJsonObject& frame);
    void clearSketchTextHandles();
    void fit(const std::vector<EvaluatedBody>& bodies, const std::vector<EvaluatedSketch>& sketches = {});
    void selectBody(const QString& id,const QString& occurrenceId = {});
    void recordContextMenu(const QJsonObject& state,bool shown = false);
    void highlightCombineInputs(const QString& targetBodyId, const QJsonArray& toolBodyIds);
    QString pickBody(double x,double y);
    QJsonObject pickBodyReference(double x,double y);
    QJsonObject pickSolidEndpoint(double x,double y,const std::vector<EvaluatedBody>& bodies,const QJsonObject& face) const;
    QJsonObject pickSolidEdge(double x,double y,const std::vector<EvaluatedBody>& bodies) const;
    QJsonObject pickSupportBoundaryEdge(double x,double y,const std::vector<EvaluatedBody>& bodies) const;
    QJsonObject projectSolidEdge(const EvaluatedSolidEdge& edge) const;
    void showPatternAxes(const QJsonObject& summary);
    void clearPatternAxes();
    void highlightSolidEdges(const std::vector<EvaluatedBody>& bodies,const QString& bodyId,const QJsonArray& edgeIds,
        const QString& occurrenceId = {});
    void clearSolidEdgeHighlights();
    void setSliderCue(const QJsonObject& joint);
    void setJointOriginPicking(bool active,const QString& occurrenceId = {},const QString& axis = {});
    QJsonObject jointOriginState() const;
    void setJointOrigins(const QJsonArray& origins,bool visible);
    void setAssemblyDragCue(const QJsonObject& cue);
    QJsonObject projectBody(const EvaluatedBody& body) const;
    QPointF projectWorldPoint(const QJsonArray& pointMm) const;
    void showHolePreview(const EvaluatedBody& result, const TopoDS_Shape& removedMaterial);
    void showSpherePreview(const TopoDS_Shape& tool, bool cut);
    void showPreview(const TopoDS_Shape& shape,const QString& replacedBody = {}, bool neutral = false);
    void showBoundaryFillPreview(const TopoDS_Shape& cells);
    void showExtrudePreview(const EvaluatedBody& body, const EvaluatedProfile& profile, double distanceMm, const QJsonArray& sourceProfileIds = {}, const QString& units = "mm", double startOffsetMm = 0);
    void showCutPreview(const EvaluatedBody& body, const std::vector<EvaluatedProfile>& profiles, double distanceMm,
        const QJsonArray& sourceProfileIds, const QString& units, double startOffsetMm = 0);
    void showSurfaceTrimPreview(const TopoDS_Shape& splitShape, const QString& replacedBody,
        const std::vector<TopoDS_Face>& removedRegions);
    void clearExtrusionCue();
    void showExtrudeHandle(const EvaluatedProfile& profile,const QJsonArray& sourceProfileIds,const QString& units,const QString& operation,double distanceMm = 0,double startOffsetMm = 0);
    void showRevolveCue(const QJsonObject& geometry,const EvaluatedProfile& profile);
    void clearRevolveCue(bool resetGesture = false);
    QJsonObject revolvePointerAngle(double x,double y,const QString& phase);
    QJsonObject revolveCueState() const { return revolveCueState_; }
    void showMovePreview(const TopoDS_Shape& shape,const QString& bodyId);
    void showMoveCue(const QJsonObject& inputs);
    void clearMoveCue(bool resetGesture = false);
    QJsonObject moveCueState() const { if(moveCueState_.isEmpty())return {};auto state=moveCueState_;state["activeDrag"]=moveDrag_;return state; }
    void clearPreview();
    QPaintEngine* paintEngine() const override { return nullptr; }
protected:
    void showEvent(QShowEvent*) override;
    void paintEvent(QPaintEvent*) override;
    void resizeEvent(QResizeEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void keyPressEvent(QKeyEvent*) override;
    void keyReleaseEvent(QKeyEvent*) override;
    void leaveEvent(QEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void mouseDoubleClickEvent(QMouseEvent*) override;
    void contextMenuEvent(QContextMenuEvent*) override;
    void wheelEvent(QWheelEvent*) override;
    bool event(QEvent*) override;
    bool eventFilter(QObject*,QEvent*) override;
private:
    QMap<QString,Handle(AIS_Shape)> canvasModels_;
    QJsonArray canvasState_;
    QJsonObject canvasStamp_;
    bool canvasPicking_=false;
    std::vector<Handle(AIS_Shape)> solidEdgeHighlights_;
    std::vector<Handle(AIS_Shape)> sliderCue_;
    bool jointOriginPicking_=false,jointOriginLocked_=false,jointOriginsVisible_=false,jointOriginPressHandled_=false;
    QString jointOriginOccurrence_,jointOriginAxis_;
    QJsonObject jointOriginHover_,jointOriginLock_;
    QPointF lastJointPointer_;
    QJsonArray jointOrigins_;
    std::vector<Handle(AIS_Shape)> jointOriginCue_,jointOriginMarkers_;
    void updateJointOriginHover(const QPointF& point);
    void refreshJointOriginCue();
    void refreshJointOriginMarkers();
    QJsonObject assemblyDragCue_, assemblyDrag_;
    std::optional<double> assemblyPointerAngle(const QPointF& position) const;
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
    void setExtrusionCue(const EvaluatedProfile& profile,double distanceMm,const QJsonArray& sourceProfileIds,const QString& units,const QString& operation,double startOffsetMm = 0);
    void updateExtrusionDrag(const QPointF& position,const QString& phase);
    bool preservePreparedThreadMesh(const Handle(AIS_Shape)& presentation) const;
    std::vector<TopoDS_Shape> preparedThreadMeshes_;
    void replacePreview(const TopoDS_Shape& shape, const QString& replacedBody, bool neutral);
    QPointF windowScale() const;
    Handle(V3d_Viewer) viewer_;
    Handle(V3d_View) view_;
    Handle(AIS_InteractiveContext) context_;
    Handle(AIS_Trihedron) axes_;
    Handle(Graphic3d_Camera) baseCamera_;
    QString error_;
    QPointF last_;
    QString inputPreset_ = "default";
    Qt::MouseButton panButton_ = Qt::NoButton;
    QPointF orbitLast_;
    Qt::MouseButton orbitButton_ = Qt::NoButton;
    QPointF zoomLast_,zoomAnchor_;
    Qt::MouseButton zoomButton_ = Qt::NoButton;
    bool alternateRightContextSuppressed_ = false;
    bool alternateOrbitSuppressRelease_ = false;
    bool freeOrbitTool_ = false;
    QString navigationTool_ = "select";
    bool navigationPressActive_ = false;
    bool navigationSuppressRelease_ = false;
    bool navigationMoved_ = false;
    QPointF navigationPress_, navigationLast_;
    QRubberBand* navigationWindowBand_ = nullptr;
    QString orbitMode_ = "free";
    QJsonObject lastNavigationInput_;
    bool nativeGestureActive_=false;
    Handle(AIS_ViewCube) viewCube_;
    QWidget* cubeControls_=nullptr;
    QToolButton* cubeAccessoryToggle_=nullptr;
    std::vector<QToolButton*> cubeFaceControls_;
    void updateCubeControls();
    void updateCubeAccessoryVisibility();
    bool cubeAccessoriesDismissed_=false;
    bool cubePressHandled_=false;
    bool cubeSuppressRelease_=false;
    bool cubeDragging_=false;
    QPointF cubePress_,cubeLast_;
    QString cubePressedRegion_;
    bool cubeHoverActive_=false;
    bool orbitCenterPicking_ = false;
    bool orbitCenterPressHandled_ = false;
    std::optional<gp_Pnt> customOrbitCenter_;
    gp_Pnt viewCenter_;
    Handle(AIS_Point) orbitCenterMarker_;
    void refreshOrbitCenterMarker();
    gp_Dir customDirection_{0,0,-1};
    gp_Dir customUp_{0,1,0};
    QJsonObject sectionAnalysis_;
    Handle(Graphic3d_ClipPlane) sectionClip_;
    QMap<QString,Handle(AIS_Shape)> models_;
    QMap<QString,QJsonObject> modelReferences_;
    QMap<QString,bool> bodyVisibility_;
    QString combineTargetId_;
    QJsonObject selectedSupportFace_;
    Handle(AIS_Shape) selectedSupportFaceOverlay_;
    QJsonObject hoveredSupportFace_;
    Handle(AIS_Shape) hoveredSupportFaceOverlay_;
    QJsonArray combineToolIds_;
    QMap<QString,Handle(AIS_Shape)> sketchModels_;
    std::vector<Handle(AIS_Shape)> sketchTextHandles_;
    QJsonObject sketchTextHandleState_, sketchTextDrag_;
    QJsonObject slotPointDrag_,sketchLineArcDrag_;
    QMap<QString,Handle(AIS_Shape)> constructionSketchModels_, centerlineSketchModels_;
    QMap<QString,Handle(AIS_Shape)> constructionPlaneModels_;
    std::vector<EvaluatedConstructionPlane> constructionPlanes_;
    QString selectedConstructionPlane_;
    quint64 constructionPlaneRevision_ = 0;
    std::vector<Handle(AIS_Shape)> patternAxes_;
    QMap<QString,Handle(AIS_Shape)> finishedProfileModels_;
    std::vector<EvaluatedSketch> finishedSketches_;
    QMap<QString,Handle(AIS_InteractiveObject)> finishedPointModels_;
    QJsonObject selectedFinishedPoint_;
    QSet<QString> selectedLoftProfileIds_;
    QSet<QString> commandSketches_;
    std::vector<Handle(AIS_Shape)> sweepPathOverlays_;
    Handle(AIS_Shape) sweepProfileOverlay_;
    QJsonObject sweepPathState_;
    QString selectedProfile_, selectedProfileSketch_, editSourceSketch_;
    QJsonArray selectedProfileIds_;
    QJsonArray hiddenBodyIds_;
    Handle(AIS_Shape) sketchDraft_, sketchConstructionDraft_, sketchCenterlineDraft_, projectedDraft_, sketchCursor_, sketchSnapCue_, splineCageCursor_;
    int draftNormalEdgeCount_ = 0;
    QJsonArray projectedLines_, projectedCircles_, supportPoints_;
    QJsonArray constructionDraftLineIds_, centerlineDraftLineIds_;
    int rightMousePressCount_ = 0;
    QJsonObject splineHandleState_,splineDrag_;
    QJsonObject conicHandleState_;
    bool conicDragging_=false;
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
    QPointF mirrorWindowPress_;
    bool mirrorWindowActive_ = false;
    QRubberBand* mirrorWindowBand_ = nullptr;
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
    QString projectionPolicy_ = "orthographic";
    int msaaRequested_ = 4, msaaEnabled_ = 0, msaaMaximum_ = 0;
    bool msaaTextures_ = false;
    std::vector<Handle(AIS_Shape)> moveCue_;
    QJsonObject moveCueState_;
    QJsonObject moveDrag_;
    std::vector<Handle(AIS_Shape)> revolveCue_;
    QJsonObject revolveCueState_,revolveDrag_,revolvePointerState_;
    void refreshRevolveCue();
    void updateRevolveDrag(const QPointF& position,const QString& phase);
    gp_Pnt sceneCenter_;
    gp_Dir conventionRight_{1,0,0},conventionBack_{0,1,0},conventionUp_{0,0,1};
    QString conventionDefaultUp_="Z";
    QLabel* cubeUpLabel_=nullptr;
    gp_Dir conventionDirection(const gp_Dir& local) const;
};
