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
#include "GeometryKernel.h"

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
    QImage framebuffer();
    void setBodies(const std::vector<EvaluatedBody>& bodies, quint64 revision);
    void setBodyStage(const std::vector<EvaluatedBody>& bodies, const QString& stageKey);
    void clearBodyStage();
    void setSketches(const std::vector<EvaluatedSketch>& sketches, quint64 revision);
    void showSketchDraft(const EvaluatedSketch& sketch);
    void clearSketchDraft();
    void selectProfile(const QString& sketchId, const QString& profileId);
    QJsonObject pickFinishedProfile(double x, double y) const;
    void setEditDisplay(const QString& sourceSketchId, const QJsonArray& hiddenBodyIds);
    QJsonObject pickSketch(const QJsonObject& feature, double x, double y, const QString& filter = {}) const;
    void decorateSketch(const QJsonObject& feature, const QJsonObject& selection,
        const QJsonObject& diagnostics, const QString& units, const QJsonObject& pending = {});
    QJsonObject dimensionLayout(const QJsonObject& feature, const QJsonObject& dimension,
        double valueMm, const QString& units) const;
    QJsonObject pickAnnotation(double x, double y) const;
    QJsonArray annotations() const { return annotationLayouts_; }
    void setSketchPlane(const QString& plane);
    void lookAtSketch(const QString& plane);
    QJsonObject sketchPoint(double x, double y, const QString& plane) const;
    QJsonObject projectSketchPoint(double uMm, double vMm, const QString& plane) const;
    void showSketchCursor(const QString& plane, const QPointF& first, const QPointF& second, bool rectangle);
    void showSketchCurveCursor(const QJsonObject& feature);
    void clearSketchCursor();
    void fit(const std::vector<EvaluatedBody>& bodies, const std::vector<EvaluatedSketch>& sketches = {});
    void selectBody(const QString& id);
    void highlightCombineInputs(const QString& targetBodyId, const QJsonArray& toolBodyIds);
    QString pickBody(double x,double y);
    QJsonObject projectBody(const EvaluatedBody& body) const;
    void showPreview(const TopoDS_Shape& shape,const QString& replacedBody = {}, bool neutral = false);
    void showExtrudePreview(const EvaluatedBody& body, const EvaluatedProfile& profile, double distanceMm);
    void clearPreview();
    QPaintEngine* paintEngine() const override { return nullptr; }
protected:
    void showEvent(QShowEvent*) override;
    void paintEvent(QPaintEvent*) override;
    void resizeEvent(QResizeEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void mouseDoubleClickEvent(QMouseEvent*) override;
    void wheelEvent(QWheelEvent*) override;
private:
    void initialize();
    void displayBodies(const std::vector<EvaluatedBody>& bodies);
    QJsonObject curveDimensionLayout(const QJsonObject& feature, const QJsonObject& dimension,
        double value, const QString& units) const;
    QJsonObject angleDimensionLayout(const QJsonObject& feature, const QJsonObject& dimension,
        double value) const;
    void clearSketchDecorations();
    void applyEditDisplay(bool redraw = true);
    void clearPreviewPresentation();
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
    QJsonArray combineToolIds_;
    QMap<QString,Handle(AIS_Shape)> sketchModels_;
    QMap<QString,Handle(AIS_Shape)> finishedProfileModels_;
    std::vector<EvaluatedSketch> finishedSketches_;
    QString selectedProfile_, selectedProfileSketch_, editSourceSketch_;
    QJsonArray hiddenBodyIds_;
    Handle(AIS_Shape) sketchDraft_, sketchCursor_;
    std::vector<Handle(AIS_InteractiveObject)> sketchDecorations_;
    QJsonArray annotationLayouts_;
    QJsonArray constraintGlyphs_;
    std::vector<EvaluatedProfile> draftProfiles_;
    QMap<QString,Handle(AIS_Shape)> profileModels_;
    QJsonObject decorationFeature_, decorationSelection_, decorationDiagnostics_, decorationPending_;
    QString decorationUnits_, draggedAnnotation_;
    QPointF annotationPress_;
    bool annotationDragged_ = false;
    QString sketchPlane_, draftSketchId_;
    quint64 sketchRevision_ = 0;
    Handle(AIS_Shape) preview_;
    std::vector<Handle(AIS_Shape)> extrusionCue_;
    QJsonObject extrusionCueState_;
    QString previewReplacedBody_, previewProfile_;
    quint64 modelRevision_ = 0;
    std::vector<EvaluatedBody> documentBodies_;
    QString bodyStageKey_;
    double sceneScale_ = 400;
    int msaaRequested_ = 4, msaaEnabled_ = 0, msaaMaximum_ = 0;
    bool msaaTextures_ = false;
    gp_Pnt sceneCenter_;
};
