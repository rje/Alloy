#include "ReferenceView.h"
#include "SketchGeometry.h"
#include <QMouseEvent>
#include <QWheelEvent>
#include <QContextMenuEvent>
#include <QMenu>
#include <QApplication>
#include <QTimer>
#include <QJsonArray>
#include <QFontMetricsF>
#include <QFont>
#include <QLineF>
#include <Aspect_DisplayConnection.hxx>
#include <Aspect_Grid.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <OpenGl_Caps.hxx>
#include <OpenGl_Context.hxx>
#include <V3d_DirectionalLight.hxx>
#include <V3d_AmbientLight.hxx>
#include <Geom_Axis2Placement.hxx>
#include <Graphic3d_DiagnosticInfo.hxx>
#include <Image_PixMap.hxx>
#include <Standard_Failure.hxx>
#include <TColStd_IndexedDataMapOfStringString.hxx>
#ifdef __APPLE__
#include <Cocoa_Window.hxx>
#elif defined(_WIN32)
#include <WNT_Window.hxx>
#else
#include <Xw_Window.hxx>
#endif
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <numbers>
#include <stdexcept>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <Prs3d_Drawer.hxx>
#include <Prs3d_LineAspect.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <TopoDS_Wire.hxx>
#include <gp_Ax3.hxx>
#include <gp_Vec.hxx>
#include <AIS_Point.hxx>
#include <AIS_TextLabel.hxx>
#include <Geom_CartesianPoint.hxx>
#include <Prs3d_PointAspect.hxx>
#include <Graphic3d_ZLayerId.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <TopoDS_Edge.hxx>
#include <PrsDim_LengthDimension.hxx>
#include <Prs3d_DimensionAspect.hxx>
#include <Prs3d_ArrowAspect.hxx>
#include <BRepClass_FaceClassifier.hxx>
#include <IntCurvesFace_ShapeIntersector.hxx>
#include <gp_Lin.hxx>
#include <gp_Pln.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRep_Tool.hxx>
#include <Poly_Triangulation.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <GeomAPI_ProjectPointOnCurve.hxx>
#include <Geom_Curve.hxx>
#include <BRep_Builder.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Iterator.hxx>

namespace {
Handle(AIS_Shape) sketchPresentation(const TopoDS_Shape& shape) {
    Handle(AIS_Shape) presentation=new AIS_Shape(shape);
    presentation->SetColor(Quantity_Color(0.10,0.34,0.84,Quantity_TOC_RGB));
    presentation->SetWidth(2);
    return presentation;
}
struct SketchStrokes { TopoDS_Shape normal, construction; int constructionCount = 0; };
SketchStrokes splitSketchStrokes(const EvaluatedSketch& sketch) {
    if(sketch.shape.IsNull())return {};
    const auto entities=sketch.summary.value("entities").toArray();
    QSet<QString> projectedIds;
    for(const auto& value:sketch.summary.value("projectedLines").toArray())projectedIds.insert(value.toObject()["id"].toString());
    TopoDS_Iterator child(sketch.shape);
    TopoDS_Compound normal,construction;BRep_Builder builder;builder.MakeCompound(normal);builder.MakeCompound(construction);
    int ordinaryCount=0,constructionCount=0;
    for(const auto& entry:entities) {
        if(!child.More())return {sketch.shape,{}};
        if(projectedIds.contains(entry.toObject()["id"].toString())){child.Next();continue;}
        if(entry.toObject().value("construction").toBool()) {builder.Add(construction,child.Value());++constructionCount;}
        else {builder.Add(normal,child.Value());++ordinaryCount;}
        child.Next();
    }
    if(child.More())return {sketch.shape,{}};
    return {ordinaryCount ? TopoDS_Shape(normal) : TopoDS_Shape{},
        constructionCount ? TopoDS_Shape(construction) : TopoDS_Shape{},constructionCount};
}
Handle(AIS_Shape) constructionPresentation(const TopoDS_Shape& shape,bool selected=false) {
    Handle(AIS_Shape) presentation=new AIS_Shape(shape);
    const Quantity_Color color=selected ? Quantity_Color(0.98,0.43,0.08,Quantity_TOC_RGB)
        : Quantity_Color(0.91,0.47,0.11,Quantity_TOC_RGB);
    const Handle(Prs3d_LineAspect) dashed=new Prs3d_LineAspect(color,Aspect_TOL_DASH,selected ? 4 : 1.5);
    presentation->SetColor(color);
    presentation->Attributes()->SetWireAspect(dashed);presentation->Attributes()->SetLineAspect(dashed);
    presentation->Attributes()->SetFreeBoundaryAspect(dashed);presentation->Attributes()->SetUnFreeBoundaryAspect(dashed);
    presentation->SetDisplayMode(AIS_WireFrame);
    return presentation;
}
}

ReferenceView::ReferenceView(QWidget* parent) : QWidget(parent) {
    setMinimumSize(400, 300);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setAttribute(Qt::WA_NativeWindow);
    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_NoSystemBackground);
}
ReferenceView::~ReferenceView() {
    if (!view_.IsNull()) view_->Remove();
}
QJsonObject ReferenceView::cameraState() const {
    return {{"orientation",orientation},{"zoom",zoom},{"panX",pan.x()},{"panY",pan.y()},
        {"grid",grid},{"origin",origin},{"sketchPlane",sketchPlane_},{"sketchFrame",sketchFrame_},
        {"centerX",sceneCenter_.X()},{"centerY",sceneCenter_.Y()},{"centerZ",sceneCenter_.Z()},
        {"scale",sceneScale_}};
}
void ReferenceView::restoreCameraState(const QJsonObject& state) {
    orientation=state.value("orientation").toString("isometric");
    zoom=state.value("zoom").toDouble(1);pan={state.value("panX").toDouble(),state.value("panY").toDouble()};
    grid=state.value("grid").toBool(true);origin=state.value("origin").toBool(true);
    sketchPlane_=state.value("sketchPlane").toString();
    sketchFrame_=state.value("sketchFrame").toObject();
    sceneCenter_=gp_Pnt(state.value("centerX").toDouble(),state.value("centerY").toDouble(),state.value("centerZ").toDouble());
    sceneScale_=state.value("scale").toDouble(400);
    sync();
}
void ReferenceView::rebindDocument() {
    if(context_.IsNull())return;
    clearPreview();clearSketchDraft();clearBodyStage();clearSketchCursor();
    selectSupportFace({},{});hoverSupportFace({});sketchFrame_={};
    setEditDisplay({},{});selectProfile({},{});highlightCombineInputs({},{});
    for(const auto& model:models_)context_->Remove(model,false);
    for(const auto& model:sketchModels_)context_->Remove(model,false);
    for(const auto& model:constructionSketchModels_)context_->Remove(model,false);
    for(const auto& model:finishedProfileModels_)context_->Remove(model,false);
    models_.clear();bodyVisibility_.clear();sketchModels_.clear();constructionSketchModels_.clear();finishedProfileModels_.clear();finishedSketches_.clear();
    modelRevision_=std::numeric_limits<quint64>::max();sketchRevision_=modelRevision_;
}
void ReferenceView::initialize() {
    if (!view_.IsNull() || !error_.isEmpty()) return;
    try {
        Handle(Aspect_DisplayConnection) display = new Aspect_DisplayConnection;
        Handle(OpenGl_GraphicDriver) driver = new OpenGl_GraphicDriver(display, false);
        driver->ChangeOptions().contextCompatible = false;
        driver->ChangeOptions().ffpEnable = false;
        driver->InitContext();
        viewer_ = new V3d_Viewer(driver);
        // World-space rays give an isometric solid distinct face tones.
        Handle(V3d_DirectionalLight) key=new V3d_DirectionalLight(gp_Dir(-0.35,0.8,-1),Quantity_NOC_WHITE,false);
        key->SetIntensity(0.9f);
        Handle(V3d_AmbientLight) ambient=new V3d_AmbientLight(Quantity_NOC_WHITE);
        ambient->SetIntensity(0.45f);
        viewer_->AddLight(key);viewer_->AddLight(ambient);viewer_->SetLightOn();
        context_ = new AIS_InteractiveContext(viewer_);
        // Make body selection visible on shaded solids, independent of OCCT's
        // default (gray) selection color. Hover remains a lighter blue.
        context_->SelectionStyle()->SetColor(Quantity_Color(0.12,0.43,0.82,Quantity_TOC_RGB));
        context_->SelectionStyle()->SetDisplayMode(AIS_Shaded);
        context_->HighlightStyle()->SetColor(Quantity_Color(0.40,0.67,0.94,Quantity_TOC_RGB));
        context_->HighlightStyle()->SetDisplayMode(AIS_Shaded);
        view_ = viewer_->CreateView();
        // Publish only complete camera states. OCCT otherwise redraws after
        // each setter, flashing the reset camera during clicks and navigation.
        view_->SetImmediateUpdate(false);
#ifdef __APPLE__
        Handle(Cocoa_Window) window = new Cocoa_Window(reinterpret_cast<NSView*>(winId()));
#elif defined(_WIN32)
        Handle(WNT_Window) window = new WNT_Window(reinterpret_cast<Aspect_Handle>(winId()));
#else
        Handle(Xw_Window) window = new Xw_Window(display, static_cast<Aspect_Drawable>(winId()));
#endif
        view_->SetWindow(window);
        const auto gl=driver->GetSharedContext();
        if(!gl.IsNull()) {
            msaaMaximum_=gl->MaxMsaaSamples();msaaTextures_=gl->HasTextureMultisampling();
            msaaEnabled_=msaaTextures_ && msaaMaximum_>=4 ? 4 : msaaTextures_ && msaaMaximum_>=2 ? 2 : 0;
            view_->ChangeRenderingParams().NbMsaaSamples=msaaEnabled_;
        }
        view_->SetBgGradientColors(Quantity_Color(1,1,1,Quantity_TOC_RGB),
            Quantity_Color(0.91,0.93,0.95,Quantity_TOC_RGB), Aspect_GradientFillMethod_Vertical, false);
        viewer_->SetRectangularGridValues(0, 0, 10, 10, 0);
        viewer_->SetRectangularGridGraphicValues(200, 200, 0);
        viewer_->Grid(Aspect_GT_Rectangular)->SetColors(Quantity_Color(0.77,0.81,0.84,Quantity_TOC_RGB), Quantity_Color(0.64,0.70,0.75,Quantity_TOC_RGB));
        axes_ = new AIS_Trihedron(new Geom_Axis2Placement(gp::XOY()));
        axes_->SetSize(100);
        axes_->SetDatumPartColor(Prs3d_DatumParts_XAxis, Quantity_Color(0.60,0.18,0.16,Quantity_TOC_RGB));
        axes_->SetDatumPartColor(Prs3d_DatumParts_YAxis, Quantity_Color(0.18,0.42,0.24,Quantity_TOC_RGB));
        axes_->SetDatumPartColor(Prs3d_DatumParts_ZAxis, Quantity_Color(0.18,0.33,0.62,Quantity_TOC_RGB));
        axes_->SetTextColor(Quantity_Color(0.18,0.22,0.26,Quantity_TOC_RGB));
        context_->Display(axes_, false);
        context_->Deactivate(axes_); // Decoration, not selectable document geometry.
        view_->SetProj(V3d_XposYnegZpos);
        view_->SetSize(400);
        baseCamera_ = new Graphic3d_Camera(view_->Camera());
        sync();
    } catch (const Standard_Failure& e) {
        error_ = QString::fromUtf8(e.GetMessageString());
        if (!view_.IsNull()) { view_->Remove(); view_.Nullify(); }
    }
}
void ReferenceView::showEvent(QShowEvent* e) {
    QWidget::showEvent(e); initialize();
    // Qt finishes native NSView geometry after the QWidget event on macOS.
    QTimer::singleShot(0, this, [this] { sync(); });
}
void ReferenceView::paintEvent(QPaintEvent*) { if (!view_.IsNull()) { view_->MustBeResized(); view_->Redraw(); } }
void ReferenceView::resizeEvent(QResizeEvent* e) {
    QWidget::resizeEvent(e);
    if (!view_.IsNull()) { sync(); QTimer::singleShot(0, this, [this] { sync(); }); }
}
void ReferenceView::sync() {
    if (view_.IsNull()) return;
    view_->ChangeRenderingParams().Resolution = qRound(72 * devicePixelRatioF());
    view_->SetCamera(new Graphic3d_Camera(baseCamera_));
    // Resize after installing the camera so its aspect matches this drawable.
    view_->MustBeResized();
    if(orientation=="face" && !sketchFrame_.isEmpty()) {
        const auto n=sketchFrame_.value("normal").toArray(),v=sketchFrame_.value("vAxis").toArray();
        view_->Camera()->SetDirection(gp_Dir(-n[0].toDouble(),-n[1].toDouble(),-n[2].toDouble()));
        view_->Camera()->SetUp(gp_Dir(v[0].toDouble(),v[1].toDouble(),v[2].toDouble()));
        view_->Camera()->OrthogonalizeUp();
    } else view_->SetProj(orientation == "top" ? V3d_Zpos : orientation == "front" ? V3d_Yneg : orientation == "right" ? V3d_Xpos : V3d_XposYnegZpos);
    // Move Eye/Center together: SetCenter alone changes the view direction,
    // while SetProj preserves the origin's screen offset rather than Center.
    const auto camera=view_->Camera();
    camera->MoveEyeTo(gp_Pnt(sceneCenter_.XYZ()-camera->Direction().XYZ()*camera->Distance()));
    // Fit uses the smaller viewport dimension; V3d::SetSize treats its input
    // as horizontal on a wide window and can crop a tall model vertically.
    camera->SetScale(sceneScale_ / (zoom * std::min(camera->Aspect(), 1.0)));
    const auto scale=windowScale();
    view_->Pan(qRound(pan.x()*scale.x()),-qRound(pan.y()*scale.y()));
    const auto frame=frameFor(sketchPlane_.isEmpty() ? "XY" : sketchPlane_);
    const auto normal=frame["normal"].toArray(),u=frame["uAxis"].toArray(),o=frame["origin"].toArray();
    const gp_Pnt frameOrigin(o[0].toDouble(),o[1].toDouble(),o[2].toDouble());
    const gp_Dir frameNormal(normal[0].toDouble(),normal[1].toDouble(),normal[2].toDouble());
    const gp_Dir frameU(u[0].toDouble(),u[1].toDouble(),u[2].toDouble());
    viewer_->SetPrivilegedPlane(gp_Ax3(frameOrigin,frameNormal,frameU));
    axes_->SetComponent(new Geom_Axis2Placement(gp_Ax2(frameOrigin,frameNormal,frameU)));
    context_->Redisplay(axes_,false);
    // Keep the sketch grid under the whole drawable. Its graphic center moves
    // only in whole grid steps, so panning never shifts the origin lattice.
    double gridCenterU=0,gridCenterV=0,gridStep=10,gridWidth=200,gridHeight=200;
    bool adaptive=false;
    if(!sketchPlane_.isEmpty() && width()>1 && height()>1) {
        try {
            const auto corners=std::array<QJsonObject,4>{
                sketchPoint(0,0,sketchPlane_),sketchPoint(width()-1,0,sketchPlane_),
                sketchPoint(0,height()-1,sketchPlane_),sketchPoint(width()-1,height()-1,sketchPlane_)};
            double minU=std::numeric_limits<double>::infinity(),maxU=-minU;
            double minV=minU,maxV=-minU;
            for(const auto& corner:corners) {
                const double cu=corner["uMm"].toDouble(),cv=corner["vMm"].toDouble();
                minU=std::min(minU,cu);maxU=std::max(maxU,cu);
                minV=std::min(minV,cv);maxV=std::max(maxV,cv);
            }
            const double mmPerPixel=std::max((maxU-minU)/(width()-1),(maxV-minV)/(height()-1));
            if(std::isfinite(mmPerPixel) && mmPerPixel>0) {
                // Ten logical pixels is Alloy's fine-grid presentation choice.
                const double desired=std::clamp(mmPerPixel*10.0,1e-6,1e9);
                const double decade=std::pow(10.0,std::floor(std::log10(desired)));
                const double scaled=desired/decade;
                gridStep=(scaled<=1 ? 1 : scaled<=2 ? 2 : scaled<=5 ? 5 : 10)*decade;
                gridCenterU=std::round(((minU+maxU)/2)/gridStep)*gridStep;
                gridCenterV=std::round(((minV+maxV)/2)/gridStep)*gridStep;
                // OCCT's graphic sizes are half-extents about the grid center.
                gridWidth=std::ceil((std::max(std::abs(minU-gridCenterU),std::abs(maxU-gridCenterU))+2*gridStep)/gridStep)*gridStep;
                gridHeight=std::ceil((std::max(std::abs(minV-gridCenterV),std::abs(maxV-gridCenterV))+2*gridStep)/gridStep)*gridStep;
                adaptive=true;
            }
        } catch(const std::exception&) { /* An edge-on plane cannot show a useful sketch grid. */ }
    }
    if(gridCenterU!=gridCenterU_ || gridCenterV!=gridCenterV_ || gridStep!=gridStep_) {
        // V3d_RectangularGrid applies the negated origin to its graphic
        // transformation. Pass -center to place the finite grid at +center.
        viewer_->SetRectangularGridValues(-gridCenterU,-gridCenterV,gridStep,gridStep,0);
        gridCenterU_=gridCenterU;gridCenterV_=gridCenterV;gridStep_=gridStep;
    }
    if(gridWidth!=gridWidth_ || gridHeight!=gridHeight_) {
        viewer_->SetRectangularGridGraphicValues(gridWidth,gridHeight,0);
        gridWidth_=gridWidth;gridHeight_=gridHeight;
    }
    adaptiveGrid_=adaptive;
    if (grid) viewer_->ActivateGrid(Aspect_GT_Rectangular, Aspect_GDM_Lines);
    else viewer_->DeactivateGrid();
    if (origin) context_->Display(axes_, false); else context_->Erase(axes_, false);
    context_->Deactivate(axes_);
    if(!decorationFeature_.isEmpty())decorateSketch(decorationFeature_,decorationSelection_,decorationDiagnostics_,decorationUnits_,decorationPending_);
    if(!extrusionCueState_.isEmpty())refreshExtrusionCue();
    view_->Redraw();
}
void ReferenceView::setBodies(const std::vector<EvaluatedBody>& bodies,quint64 revision) {
    if(context_.IsNull() || modelRevision_==revision)return;
    hoverSupportFace({});
    documentBodies_=bodies;modelRevision_=revision;
    if(bodyStageKey_.isEmpty())displayBodies(documentBodies_);
}
void ReferenceView::setBodyStage(const std::vector<EvaluatedBody>& bodies,const QString& stageKey) {
    if(stageKey.isEmpty() || context_.IsNull() || bodyStageKey_==stageKey)return;
    bodyStageKey_=stageKey;displayBodies(bodies);
}
void ReferenceView::clearBodyStage() {
    if(bodyStageKey_.isEmpty())return;bodyStageKey_.clear();displayBodies(documentBodies_);
}
void ReferenceView::displayBodies(const std::vector<EvaluatedBody>& bodies) {
    if(context_.IsNull())return;
    for(const auto& model:models_)context_->Remove(model,false);
    models_.clear();bodyVisibility_.clear();
    for(const auto& body:bodies) {
        Handle(AIS_Shape) model=new AIS_Shape(body.shape);model->SetDisplayMode(AIS_Shaded);
        model->SetOwnDeviationCoefficient(0.0001);
        model->SetOwnDeviationAngle(0.04);
        model->SetColor(Quantity_Color(0.67,0.72,0.76,Quantity_TOC_RGB));
        model->Attributes()->SetFaceBoundaryDraw(true);
        model->Attributes()->SetFaceBoundaryAspect(new Prs3d_LineAspect(Quantity_Color(0.20,0.25,0.29,Quantity_TOC_RGB),Aspect_TOL_SOLID,1));
        context_->Display(model,AIS_Shaded,0,false);models_[body.bodyId]=model;bodyVisibility_[body.bodyId]=body.visible;
    }
    applyEditDisplay();
}
void ReferenceView::setSketches(const std::vector<EvaluatedSketch>& sketches,quint64 revision) {
    if(context_.IsNull() || sketchRevision_==revision)return;
    for(const auto& model:sketchModels_)context_->Remove(model,false);
    for(const auto& model:constructionSketchModels_)context_->Remove(model,false);
    for(const auto& model:finishedProfileModels_)context_->Remove(model,false);
    sketchModels_.clear();constructionSketchModels_.clear();finishedProfileModels_.clear();finishedSketches_=sketches;
    for(const auto& sketch:sketches) {
        const auto strokes=splitSketchStrokes(sketch);
        if(!strokes.normal.IsNull()) {
            const auto model=sketchPresentation(strokes.normal);model->SetDisplayMode(AIS_WireFrame);
            if(sketch.plane=="face")model->SetZLayer(Graphic3d_ZLayerId_Topmost);
            if(sketch.summary["solver"].toObject()["dof"].toInt(-1)==0)model->SetColor(Quantity_Color(0.12,0.14,0.16,Quantity_TOC_sRGB));
            context_->Display(model,AIS_WireFrame,-1,false);context_->Deactivate(model);sketchModels_[sketch.featureId]=model;
        }
        if(!strokes.construction.IsNull()) {
            const auto model=constructionPresentation(strokes.construction);
            if(sketch.plane=="face")model->SetZLayer(Graphic3d_ZLayerId_Topmost);
            context_->Display(model,AIS_WireFrame,-1,false);context_->Deactivate(model);constructionSketchModels_[sketch.featureId]=model;
        }
        for(const auto& profile:sketch.profiles) {
            BRepMesh_IncrementalMesh mesh(profile.face,0.1,false,0.5,false);
            Handle(AIS_Shape) fill=new AIS_Shape(profile.face);fill->SetDisplayMode(AIS_Shaded);fill->Attributes()->SetAutoTriangulation(false);
            fill->Attributes()->SetShadingModel(Graphic3d_TypeOfShadingModel_Unlit,true);
            if(sketch.plane=="face")fill->SetZLayer(Graphic3d_ZLayerId_Topmost);
            fill->SetTransparency(0.58);context_->Display(fill,AIS_Shaded,-1,false);context_->Deactivate(fill);
            finishedProfileModels_[profile.profileId]=fill;
        }
    }
    sketchRevision_=revision;applyEditDisplay();
}
void ReferenceView::applyEditDisplay(bool redraw) {
    if(context_.IsNull())return;
    for(const auto& sketch:finishedSketches_) {
        const bool visible=(sketch.visible || sketch.featureId==editSourceSketch_) && sketch.featureId!=draftSketchId_;
        if(sketchModels_.contains(sketch.featureId)) {
            if(visible)context_->Display(sketchModels_.value(sketch.featureId),false);
            else context_->Erase(sketchModels_.value(sketch.featureId),false);
        }
        if(constructionSketchModels_.contains(sketch.featureId)) {
            if(visible)context_->Display(constructionSketchModels_.value(sketch.featureId),false);
            else context_->Erase(constructionSketchModels_.value(sketch.featureId),false);
        }
        for(const auto& profile:sketch.profiles)if(finishedProfileModels_.contains(profile.profileId)) {
            const auto fill=finishedProfileModels_.value(profile.profileId);
            fill->SetColor(selectedProfileIds_.contains(profile.profileId) && sketch.featureId==selectedProfileSketch_
                ? Quantity_Color(0.17,0.48,0.90,Quantity_TOC_sRGB) : Quantity_Color(0.39,0.68,0.91,Quantity_TOC_sRGB));
            // The extrusion base cap occupies this same plane. Keep its sketch
            // outline, but suppress the coplanar transparent profile fill.
            if(visible && !previewProfileIds_.contains(profile.profileId))context_->Display(fill,false);else context_->Erase(fill,false);
        }
    }
    for(auto i=models_.begin();i!=models_.end();++i) {
        const bool visible=bodyVisibility_.value(i.key()) || i.key()==combineTargetId_ || combineToolIds_.contains(i.key());
        if(!visible || hiddenBodyIds_.contains(i.key()) || i.key()==previewReplacedBody_)context_->Erase(i.value(),false);
        else context_->Display(i.value(),false);
    }
    if(redraw)context_->UpdateCurrentViewer();
}
void ReferenceView::setEditDisplay(const QString& sourceSketchId,const QJsonArray& hiddenBodyIds) {
    editSourceSketch_=sourceSketchId;hiddenBodyIds_=hiddenBodyIds;applyEditDisplay();
}
void ReferenceView::selectProfile(const QString& sketchId,const QString& profileId) {
    selectProfiles(sketchId,profileId.isEmpty() ? QJsonArray{} : QJsonArray{profileId});
}
void ReferenceView::selectProfiles(const QString& sketchId,const QJsonArray& profileIds) {
    selectedProfileSketch_=sketchId;selectedProfileIds_=profileIds;
    selectedProfile_=profileIds.isEmpty() ? QString{} : profileIds.first().toString();applyEditDisplay();
}
QJsonObject ReferenceView::pickFinishedProfile(double x,double y) const {
    if(view_.IsNull() || x<0 || y<0 || x>=width() || y>=height())return {};
    const auto scale=windowScale();double px,py,pz,dx,dy,dz;
    view_->ConvertWithProj(qRound(x*scale.x()),qRound(y*scale.y()),px,py,pz,dx,dy,dz);
    const gp_Vec eye(px,py,pz),ray(dx,dy,dz);
    for(const auto& sketch:finishedSketches_) {
        if((!sketch.visible && sketch.featureId!=editSourceSketch_) || sketch.featureId==draftSketchId_)continue;
        const auto frame=sketch.resolvedFrame.isEmpty() ? SketchGeometry::frame(sketch.plane) : sketch.resolvedFrame;
        const auto o=frame.value("origin").toArray(),n=frame.value("normal").toArray();
        const gp_Vec origin(o[0].toDouble(),o[1].toDouble(),o[2].toDouble());
        const gp_Vec normal(n[0].toDouble(),n[1].toDouble(),n[2].toDouble());
        const double denominator=normal.Dot(ray);if(std::abs(denominator)<1e-10)continue;
        const double distance=normal.Dot(origin-eye)/denominator;if(distance<0)continue;
        const auto world=gp_Pnt((eye+ray*distance).XYZ());
        for(const auto& profile:sketch.profiles) {
            BRepClass_FaceClassifier classifier(profile.face,world,1e-7);
            if(classifier.State()==TopAbs_IN)return {{"kind","profile"},{"sketchId",sketch.featureId},{"profileId",profile.profileId}};
        }
    }
    return {};
}
QJsonObject ReferenceView::pickSupportFace(double x,double y,const std::vector<EvaluatedBody>& bodies) const {
    if(view_.IsNull() || x<0 || y<0 || x>=width() || y>=height())return {};
    const auto scale=windowScale();double px,py,pz,dx,dy,dz;
    view_->ConvertWithProj(qRound(x*scale.x()),qRound(y*scale.y()),px,py,pz,dx,dy,dz);
    const gp_Lin ray(gp_Pnt(px,py,pz),gp_Dir(dx,dy,dz));
    double nearest=std::numeric_limits<double>::infinity();const EvaluatedBody* hitBody=nullptr;TopoDS_Face hitFace;
    try {
        for(const auto& body:bodies)if(body.visible && !body.shape.IsNull()) {
            IntCurvesFace_ShapeIntersector intersections;intersections.Load(body.shape,1e-6);
            intersections.PerformNearest(ray,0,1e9);
            if(!intersections.IsDone() || intersections.NbPnt()<1 || intersections.WParameter(1)>=nearest)continue;
            nearest=intersections.WParameter(1);hitBody=&body;hitFace=intersections.Face(1);
        }
    } catch(const Standard_Failure&) {return {};}
    if(!hitBody)return {};
    for(const auto& face:hitBody->supportFaces)if(hitFace.IsSame(face.face))
        return {{"producerFeatureId",face.producerFeatureId},{"bodyId",face.bodyId},{"faceId",face.faceId}};
    return {};
}
void ReferenceView::selectSupportFace(const QJsonObject& support,const TopoDS_Face& face) {
    hoverSupportFace({});
    if(!context_.IsNull() && !selectedSupportFaceOverlay_.IsNull())context_->Remove(selectedSupportFaceOverlay_,false);
    selectedSupportFaceOverlay_.Nullify();selectedSupportFace_=support;
    if(!context_.IsNull() && !face.IsNull()) {
        selectedSupportFaceOverlay_=new AIS_Shape(face);
        selectedSupportFaceOverlay_->SetDisplayMode(AIS_Shaded);
        selectedSupportFaceOverlay_->SetColor(Quantity_Color(0.16,0.47,0.86,Quantity_TOC_sRGB));
        selectedSupportFaceOverlay_->SetTransparency(0.48);
        selectedSupportFaceOverlay_->SetZLayer(Graphic3d_ZLayerId_Topmost);
        context_->Display(selectedSupportFaceOverlay_,AIS_Shaded,-1,false);
        context_->Deactivate(selectedSupportFaceOverlay_);
        context_->UpdateCurrentViewer();
    }
}
void ReferenceView::hoverSupportFace(const QJsonObject& support) {
    if(support==hoveredSupportFace_)return;
    if(!context_.IsNull() && !hoveredSupportFaceOverlay_.IsNull())context_->Remove(hoveredSupportFaceOverlay_,false);
    hoveredSupportFaceOverlay_.Nullify();hoveredSupportFace_=support;
    if(context_.IsNull() || support.isEmpty() || support==selectedSupportFace_)return;
    for(const auto& body:documentBodies_)for(const auto& face:body.supportFaces)
        if(face.bodyId==support.value("bodyId").toString() && face.faceId==support.value("faceId").toString()) {
            hoveredSupportFaceOverlay_=new AIS_Shape(face.face);
            hoveredSupportFaceOverlay_->SetDisplayMode(AIS_Shaded);
            hoveredSupportFaceOverlay_->SetColor(Quantity_Color(0.40,0.67,0.94,Quantity_TOC_sRGB));
            hoveredSupportFaceOverlay_->SetTransparency(0.62);
            hoveredSupportFaceOverlay_->SetZLayer(Graphic3d_ZLayerId_Topmost);
            context_->Display(hoveredSupportFaceOverlay_,AIS_Shaded,-1,false);
            context_->Deactivate(hoveredSupportFaceOverlay_);
            context_->UpdateCurrentViewer();return;
        }
}
void ReferenceView::showSketchDraft(const EvaluatedSketch& sketch) {
    if(context_.IsNull())return;
    for(const auto& model:profileModels_)context_->Remove(model,false);profileModels_.clear();draftProfiles_=sketch.profiles;
    for(const auto& profile:sketch.profiles) {
        BRepMesh_IncrementalMesh mesh(profile.face,0.1,false,0.5,false);
        Handle(AIS_Shape) fill=new AIS_Shape(profile.face);fill->SetDisplayMode(AIS_Shaded);fill->Attributes()->SetAutoTriangulation(false);
        fill->Attributes()->SetShadingModel(Graphic3d_TypeOfShadingModel_Unlit,true);
        if(sketch.plane=="face")fill->SetZLayer(Graphic3d_ZLayerId_Topmost);
        fill->SetColor(Quantity_Color(0.39,0.68,0.91,Quantity_TOC_sRGB));fill->SetTransparency(0.58);
        context_->Display(fill,AIS_Shaded,-1,false);context_->Deactivate(fill);profileModels_[profile.profileId]=fill;
    }
    if(!sketchDraft_.IsNull())context_->Remove(sketchDraft_,false);
    if(!sketchConstructionDraft_.IsNull())context_->Remove(sketchConstructionDraft_,false);
    if(!projectedDraft_.IsNull())context_->Remove(projectedDraft_,false);
    if(draftSketchId_!=sketch.featureId && sketchModels_.contains(draftSketchId_))context_->Display(sketchModels_.value(draftSketchId_),false);
    if(draftSketchId_!=sketch.featureId && constructionSketchModels_.contains(draftSketchId_))context_->Display(constructionSketchModels_.value(draftSketchId_),false);
    draftSketchId_=sketch.featureId;sketchDraft_.Nullify();sketchConstructionDraft_.Nullify();projectedDraft_.Nullify();
    projectedLines_=sketch.summary.value("projectedLines").toArray();
    supportPoints_=sketch.summary.value("availableSupportPoints").toArray();constructionDraftLineIds_={};
    if(sketchModels_.contains(draftSketchId_))context_->Erase(sketchModels_.value(draftSketchId_),false);
    if(constructionSketchModels_.contains(draftSketchId_))context_->Erase(constructionSketchModels_.value(draftSketchId_),false);
    const auto strokes=splitSketchStrokes(sketch);
    if(!strokes.normal.IsNull()) {
        sketchDraft_=sketchPresentation(strokes.normal);
        if(sketch.plane=="face")sketchDraft_->SetZLayer(Graphic3d_ZLayerId_Topmost);
        if(sketch.summary["solver"].toObject()["dof"].toInt(-1)==0)sketchDraft_->SetColor(Quantity_Color(0.12,0.14,0.16,Quantity_TOC_sRGB));
        context_->Display(sketchDraft_,AIS_WireFrame,-1,false);context_->Deactivate(sketchDraft_);
    }
    if(!strokes.construction.IsNull()) {
        sketchConstructionDraft_=constructionPresentation(strokes.construction);
        if(sketch.plane=="face")sketchConstructionDraft_->SetZLayer(Graphic3d_ZLayerId_Topmost);
        context_->Display(sketchConstructionDraft_,AIS_WireFrame,-1,false);context_->Deactivate(sketchConstructionDraft_);
        for(const auto& entry:sketch.summary.value("entities").toArray()) {
            const auto entity=entry.toObject();
            if(entity.value("construction").toBool() && entity.value("type")=="line"
                && !std::any_of(projectedLines_.begin(),projectedLines_.end(),[&](const QJsonValue& value){return value.toObject().value("id")==entity.value("id");}))
                constructionDraftLineIds_.append(entity.value("id"));
        }
    }
    if(!projectedLines_.isEmpty()) {
        TopoDS_Compound compound;BRep_Builder builder;builder.MakeCompound(compound);
        for(const auto& value:projectedLines_) {
            const auto line=value.toObject();const auto a=sketchWorld(sketch.plane,line["u1Mm"].toDouble(),line["v1Mm"].toDouble());
            const auto b=sketchWorld(sketch.plane,line["u2Mm"].toDouble(),line["v2Mm"].toDouble());
            if(a.Distance(b)>1e-9)builder.Add(compound,BRepBuilderAPI_MakeEdge(a,b).Edge());
        }
        projectedDraft_=sketchPresentation(compound);
        projectedDraft_->SetColor(Quantity_Color(0.55,0.22,0.72,Quantity_TOC_sRGB));projectedDraft_->SetWidth(2.2*devicePixelRatioF());
        projectedDraft_->SetZLayer(Graphic3d_ZLayerId_Topmost);
        context_->Display(projectedDraft_,AIS_WireFrame,-1,false);context_->Deactivate(projectedDraft_);
    }
    applyEditDisplay();
}
void ReferenceView::clearSketchDraft() {
    clearSketchDecorations();
    decorationFeature_={};decorationPending_={};annotationLayouts_={};draggedAnnotation_.clear();
    if(!context_.IsNull()) {
        for(const auto& model:profileModels_)context_->Remove(model,false);
        if(!sketchDraft_.IsNull())context_->Remove(sketchDraft_,false);
        if(!sketchConstructionDraft_.IsNull())context_->Remove(sketchConstructionDraft_,false);
        if(!projectedDraft_.IsNull())context_->Remove(projectedDraft_,false);
        if(sketchModels_.contains(draftSketchId_))context_->Display(sketchModels_.value(draftSketchId_),false);
        if(constructionSketchModels_.contains(draftSketchId_))context_->Display(constructionSketchModels_.value(draftSketchId_),false);
        context_->UpdateCurrentViewer();
    }
    profileModels_.clear();draftProfiles_.clear();
    sketchDraft_.Nullify();sketchConstructionDraft_.Nullify();projectedDraft_.Nullify();projectedLines_={};supportPoints_={};constructionDraftLineIds_={};draftSketchId_.clear();clearSketchCursor();applyEditDisplay();
}
QJsonObject ReferenceView::pickSketch(const QJsonObject& feature,double x,double y,const QString& filter) const {
    if(view_.IsNull() || x<0 || y<0 || x>=width() || y>=height())return {};
    const auto parameters=feature["parameters"].toObject();const auto plane=parameters["plane"].toString();
    QMap<QString,QPointF> points;
    double best=8;QString picked;QJsonObject pickedProjectedPoint;
    for(const auto& entry:parameters["points"].toArray()) {
        const auto point=entry.toObject();const auto projection=projectSketchPoint(point["uMm"].toDouble(),point["vMm"].toDouble(),plane);
        const QPointF p(projection["x"].toDouble(),projection["y"].toDouble());points[point["id"].toString()]=p;
        const double distance=std::hypot(p.x()-x,p.y()-y);
        if(filter!="entity" && distance<=best){best=distance;picked=point["id"].toString();pickedProjectedPoint={};}
    }
    if(filter!="entity")for(const auto& value:projectedLines_) {
        const auto line=value.toObject();
        for(const bool end:{false,true}) {
            const auto id=line[end ? "endPointId" : "startPointId"].toString();
            const auto projection=projectSketchPoint(line[end ? "u2Mm" : "u1Mm"].toDouble(),line[end ? "v2Mm" : "v1Mm"].toDouble(),plane);
            const double distance=std::hypot(projection["x"].toDouble()-x,projection["y"].toDouble()-y);
            // Coincident cap corners may have several virtual IDs. Choose a
            // stable one; authored points retain precedence at equal distance.
            if(distance<best-1e-6 || (picked.isEmpty() && distance<=best)
                || (!pickedProjectedPoint.isEmpty() && std::abs(distance-best)<=1e-6 && id<picked)) {
                best=distance;picked=id;
                pickedProjectedPoint={{"projectionId",line["id"]},{"source",line["source"]},{"endpoint",end ? "end" : "start"}};
            }
        }
    }
    if(filter!="entity")for(const auto& value:supportPoints_) {
        const auto point=value.toObject();const auto id=point["id"].toString();
        const auto projection=projectSketchPoint(point["uMm"].toDouble(),point["vMm"].toDouble(),plane);
        const double distance=std::hypot(projection["x"].toDouble()-x,projection["y"].toDouble()-y);
        if(distance<best-1e-6 || (picked.isEmpty() && distance<=best)
            || (!pickedProjectedPoint.isEmpty() && std::abs(distance-best)<=1e-6 && id<picked)) {
            best=distance;picked=id;
            pickedProjectedPoint={{"source",point["source"]},{"supportPoint",true},{"retained",point["retained"]}};
        }
    }
    if(!picked.isEmpty()){
        QJsonObject result{{"kind","point"},{"pointId",picked},{"distancePixels",best}};
        if(!pickedProjectedPoint.isEmpty()){
            result["projected"]=true;
            for(auto it=pickedProjectedPoint.begin();it!=pickedProjectedPoint.end();++it)result[it.key()]=it.value();
        }
        return result;
    }
    if(filter=="point")return {};
    best=6;QJsonObject pickedProjection;
    const auto planePoint=sketchPoint(x,y,plane);const auto hitWorld=sketchWorld(plane,planePoint["uMm"].toDouble(),planePoint["vMm"].toDouble());
    for(const auto& entry:parameters["entities"].toArray()) {
        const auto entity=entry.toObject();QPointF nearest;
        if(entity["type"]=="line") {
            const auto start=points.value(entity["startPointId"].toString()),end=points.value(entity["endPointId"].toString());
            const auto delta=end-start;const double length2=delta.x()*delta.x()+delta.y()*delta.y();if(length2<1e-12)continue;
            const double t=std::clamp(((x-start.x())*delta.x()+(y-start.y())*delta.y())/length2,0.0,1.0);nearest=start+delta*t;
        } else {
            const auto edge=SketchGeometry::analyticEdge(parameters,entity,sketchFrame_);double first,last;const auto curve=BRep_Tool::Curve(edge,first,last);
            GeomAPI_ProjectPointOnCurve projection(hitWorld,curve,first,last);if(projection.NbPoints()==0)continue;
            const auto p=projection.NearestPoint();int px,py;view_->Convert(p.X(),p.Y(),p.Z(),px,py);const auto scale=windowScale();nearest={px/scale.x(),py/scale.y()};
        }
        const double distance=std::hypot(nearest.x()-x,nearest.y()-y);
        if(distance<=best){best=distance;picked=entity["id"].toString();pickedProjection={};}
    }
    for(const auto& value:projectedLines_) {
        const auto line=value.toObject();
        const auto first=projectSketchPoint(line["u1Mm"].toDouble(),line["v1Mm"].toDouble(),plane);
        const auto last=projectSketchPoint(line["u2Mm"].toDouble(),line["v2Mm"].toDouble(),plane);
        const QPointF start(first["x"].toDouble(),first["y"].toDouble()),end(last["x"].toDouble(),last["y"].toDouble());
        const auto delta=end-start;const double length2=delta.x()*delta.x()+delta.y()*delta.y();if(length2<1e-12)continue;
        const double t=std::clamp(((x-start.x())*delta.x()+(y-start.y())*delta.y())/length2,0.0,1.0);
        const auto nearest=start+delta*t;const double distance=std::hypot(nearest.x()-x,nearest.y()-y);
        if(distance<=best){best=distance;picked=line["id"].toString();pickedProjection=line;}
    }
    if(!picked.isEmpty()){
        QJsonObject result{{"kind","entity"},{"entityId",picked},{"distancePixels",best}};
        if(!pickedProjection.isEmpty()){result["projected"]=true;result["source"]=pickedProjection["source"];}
        return result;
    }
    if(!filter.isEmpty())return {};
    const auto hit=sketchPoint(x,y,parameters.value("plane").toString());
    const auto world=sketchWorld(parameters.value("plane").toString(),hit.value("uMm").toDouble(),hit.value("vMm").toDouble());
    for(const auto& profile:draftProfiles_) {
        BRepClass_FaceClassifier classifier(profile.face,world,1e-7);
        if(classifier.State()==TopAbs_IN)return {{"kind","profile"},{"profileId",profile.profileId}};
    }
    return {};
}
void ReferenceView::clearSketchDecorations() {
    if(!context_.IsNull())for(const auto& decoration:sketchDecorations_)context_->Remove(decoration,false);
    sketchDecorations_.clear();
    annotationLayouts_={};
    constraintGlyphs_={};
}
QJsonObject ReferenceView::dimensionLayout(const QJsonObject& feature,const QJsonObject& dimension,double valueMm,const QString& units) const {
    if(view_.IsNull())return {};
    const auto parameters=feature.value("parameters").toObject();const auto plane=parameters.value("plane").toString();
    if(dimension["kind"]=="angle")return angleDimensionLayout(feature,dimension,valueMm);
    if(dimension["kind"]=="radius" || dimension["kind"]=="diameter" || dimension["kind"]=="arcSweep")return curveDimensionLayout(feature,dimension,valueMm,units);
    const auto ids=dimension.value("pointIds").toArray();if(ids.size()!=2)return {};
    QMap<QString,QPointF> points;
    for(const auto& entry:parameters.value("points").toArray()){const auto p=entry.toObject();points[p.value("id").toString()]={p.value("uMm").toDouble(),p.value("vMm").toDouble()};}
    if(!points.contains(ids[0].toString()) || !points.contains(ids[1].toString()))return {};
    const QPointF a=points.value(ids[0].toString()),b=points.value(ids[1].toString()),span=b-a;
    const auto kind=dimension.value("kind").toString();const double length=std::hypot(span.x(),span.y());if(length<1e-9)return {};
    const QPointF d=kind=="horizontal" ? QPointF(1,0) : kind=="vertical" ? QPointF(0,1) : span/length;
    const QPointF f(-d.y(),d.x());auto dot=[](const QPointF& p,const QPointF& q){return p.x()*q.x()+p.y()*q.y();};
    auto project=[&](const QPointF& p){const auto q=projectSketchPoint(p.x(),p.y(),plane);return QPointF(q.value("x").toDouble(),q.value("y").toDouble());};
    const QPointF screenA=project(a),axis=project(a+d*100)-screenA;
    const double pixelsPerMm=std::hypot(axis.x(),axis.y())/100;if(pixelsPerMm<1e-7)return {};
    const double mmPerPixel=1/pixelsPerMm;
    QFont font("Arial");font.setPixelSize(13);const QFontMetricsF metrics(font);
    const QPointF perpendicular=project(a+f*100)-screenA;
    const double normalPixelsPerMm=std::abs(axis.x()*perpendicular.y()-axis.y()*perpendicular.x())/(std::hypot(axis.x(),axis.y())*100);
    if(normalPixelsPerMm<1e-7)return {};
    const double clearancePixels=metrics.height()/2+3;
    auto annotation=dimension.value("annotation").toObject();
    const QPointF c=annotation.isEmpty() ? (a+b)/2+f*(36*mmPerPixel) : QPointF(annotation.value("uMm").toDouble(),annotation.value("vMm").toDouble());
    annotation={{"uMm",c.x()},{"vMm",c.y()}};
    // The authored point remains the exact text anchor. Offset only the line,
    // in logical pixels, so clearance survives zoom and native display scale.
    const QPointF lineAnchor=c-f*(clearancePixels/normalPixelsPerMm);
    const QPointF lineA=a+f*dot(lineAnchor-a,f),lineB=b+f*dot(lineAnchor-b,f);if(std::hypot((lineB-lineA).x(),(lineB-lineA).y())<1e-9)return {};
    double angle=std::atan2(-axis.y(),axis.x())*180/std::acos(-1.0);
    while(angle>90)angle-=180;while(angle<=-90)angle+=180;
    const double scale=units=="in" ? 25.4 : 1;const bool driving=dimension.value("driving").toBool(true);
    const QString text=(driving ? "" : "(")+QString::number(valueMm/scale,'g',6)+(driving ? "" : ")");
    const auto label=project(c);
    auto point=[](const QPointF& p){return QJsonObject{{"uMm",p.x()},{"vMm",p.y()}};};
    auto screen=[](const QPointF& p){return QJsonObject{{"x",p.x()},{"y",p.y()}};};
    return {{"dimensionId",dimension.value("id")},{"kind",kind},{"driving",driving},{"annotation",annotation},{"valueMm",valueMm},{"mmPerPixel",mmPerPixel},{"lineClearancePixels",clearancePixels},
        {"line",QJsonArray{point(lineA),point(lineB)}},{"witnesses",QJsonArray{QJsonArray{point(a),point(lineA)},QJsonArray{point(b),point(lineB)}}},
        {"linePixels",QJsonArray{screen(project(lineA)),screen(project(lineB))}},
        {"label",QJsonObject{{"text",text},{"x",label.x()},{"y",label.y()},{"width",metrics.horizontalAdvance(text)+12},{"height",metrics.height()+10},{"angleDegrees",angle}}}};
}
QJsonObject ReferenceView::angleDimensionLayout(const QJsonObject& feature,const QJsonObject& dimension,double value) const {
    const auto parameters=feature["parameters"].toObject();const auto ids=dimension["entityIds"].toArray();if(ids.size()!=2)return {};
    const auto plane=parameters["plane"].toString();QMap<QString,QPointF> points;
    for(const auto& entry:parameters["points"].toArray()){const auto p=entry.toObject();points[p["id"].toString()]={p["uMm"].toDouble(),p["vMm"].toDouble()};}
    QJsonArray lines;for(const auto& id:ids)for(const auto& entry:parameters["entities"].toArray())if(entry.toObject()["id"]==id){lines.append(entry);break;}
    if(lines.size()!=2 || lines[0].toObject()["type"]!="line" || lines[1].toObject()["type"]!="line")return {};
    auto start=[&](const QJsonValue& line){return points.value(line.toObject()["startPointId"].toString());};
    auto end=[&](const QJsonValue& line){return points.value(line.toObject()["endPointId"].toString());};
    const auto a=start(lines[0]),b=start(lines[1]),da=end(lines[0])-a,db=end(lines[1])-b;
    const double cross=da.x()*db.y()-da.y()*db.x();if(std::abs(cross)<1e-9)return {};
    const auto offset=b-a;const QPointF center=a+da*((offset.x()*db.y()-offset.y()*db.x())/cross);
    const auto signs=dimension["raySigns"].toArray();const int sa=signs.size()==2 ? signs[0].toInt(1) : 1,sb=signs.size()==2 ? signs[1].toInt(1) : 1;
    auto unit=[](QPointF v){const double length=std::hypot(v.x(),v.y());return v/length;};
    const auto u=unit(da)*sa,v=unit(db)*sb;const double degrees=180/std::numbers::pi;
    const double startAngle=std::atan2(u.y(),u.x())*degrees;
    const double sweep=std::atan2(u.x()*v.y()-u.y()*v.x(),u.x()*v.x()+u.y()*v.y())*degrees;
    if(std::abs(sweep)<1e-6 || std::abs(sweep)>179.999999)return {};
    auto project=[&](const QPointF& p){const auto q=projectSketchPoint(p.x(),p.y(),plane);return QPointF(q["x"].toDouble(),q["y"].toDouble());};
    const auto screenCenter=project(center),screenUnit=project(center+QPointF(100,0));
    const double pixelsPerMm=std::hypot((screenUnit-screenCenter).x(),(screenUnit-screenCenter).y())/100;if(pixelsPerMm<1e-7)return {};
    const double mmPerPixel=1/pixelsPerMm;
    const auto middle=unit(u+v);auto annotation=dimension["annotation"].toObject();
    const QPointF anchor=annotation.isEmpty() ? center+middle*(46*mmPerPixel) : QPointF(annotation["uMm"].toDouble(),annotation["vMm"].toDouble());
    annotation={{"uMm",anchor.x()},{"vMm",anchor.y()}};
    const double radius=std::max(std::hypot((anchor-center).x(),(anchor-center).y()),22*mmPerPixel);
    const bool driving=dimension.value("driving").toBool(true);
    const QString content=QString::number(value,'g',6)+QString::fromUtf8("°"),text=driving ? content : "("+content+")";
    QFont font("Arial");font.setPixelSize(13);const QFontMetricsF metrics(font);
    const auto screenLabel=project(anchor);auto point=[](const QPointF& p){return QJsonObject{{"uMm",p.x()},{"vMm",p.y()}};};
    auto radial=[&](double angle,double r){const double rad=angle/degrees;return center+QPointF(std::cos(rad),std::sin(rad))*r;};
    const auto radialDirection=std::hypot((anchor-center).x(),(anchor-center).y())<1e-9 ? middle : unit(anchor-center);
    const auto screenTangent=project(center+QPointF(-radialDirection.y(),radialDirection.x())*100)-screenCenter;
    double labelAngle=std::atan2(-screenTangent.y(),screenTangent.x())*degrees;
    while(labelAngle>90)labelAngle-=180;while(labelAngle<=-90)labelAngle+=180;
    QJsonObject result{{"dimensionId",dimension["id"]},{"kind","angle"},{"driving",driving},{"annotation",annotation},{"raySigns",signs},
        {"valueDeg",value},{"mmPerPixel",mmPerPixel},{"center",point(center)},{"arcRadiusMm",radius},{"startAngleDeg",startAngle},{"sweepAngleDeg",sweep},
        {"label",QJsonObject{{"text",text},{"x",screenLabel.x()},{"y",screenLabel.y()},{"width",metrics.horizontalAdvance(text)+12},{"height",metrics.height()+10},{"angleDegrees",labelAngle}}}};
    const double gap=std::min(std::abs(sweep)/4,(metrics.horizontalAdvance(text)/2+7)*mmPerPixel/radius*degrees);
    const double labelOffset=std::atan2(u.x()*(anchor-center).y()-u.y()*(anchor-center).x(),u.x()*(anchor-center).x()+u.y()*(anchor-center).y())*degrees;
    QJsonArray arcs;if((sweep>0 && labelOffset>gap && labelOffset<sweep-gap) || (sweep<0 && labelOffset< -gap && labelOffset>sweep+gap)){
        const double sense=sweep>0 ? 1 : -1;
        arcs.append(QJsonObject{{"startAngleDeg",startAngle},{"sweepAngleDeg",labelOffset-sense*gap}});
        arcs.append(QJsonObject{{"startAngleDeg",startAngle+labelOffset+sense*gap},{"sweepAngleDeg",sweep-labelOffset-sense*gap}});
    }else arcs.append(QJsonObject{{"startAngleDeg",startAngle},{"sweepAngleDeg",sweep}});
    result["arcSegments"]=arcs;
    result["witnesses"]=QJsonArray{QJsonArray{point(center+u*(8*mmPerPixel)),point(center+u*radius)},QJsonArray{point(center+v*(8*mmPerPixel)),point(center+v*radius)}};
    result["line"]=QJsonArray{point(radial(startAngle,radius)),point(radial(startAngle+sweep,radius))};
    return result;
}
QJsonObject ReferenceView::curveDimensionLayout(const QJsonObject& feature,const QJsonObject& dimension,double value,const QString& units) const {
    const auto parameters=feature["parameters"].toObject();const auto plane=parameters["plane"].toString();
    QJsonObject entity;for(const auto& entry:parameters["entities"].toArray())if(entry.toObject()["id"]==dimension["entityId"]){entity=entry.toObject();break;}
    if(entity.isEmpty())return {};
    QMap<QString,QPointF> points;for(const auto& value:parameters["points"].toArray()){const auto p=value.toObject();points[p["id"].toString()]={p["uMm"].toDouble(),p["vMm"].toDouble()};}
    const auto center=points[entity["centerPointId"].toString()];const double radius=entity["radiusMm"].toDouble();
    const auto kind=dimension["kind"].toString();const bool driving=dimension.value("driving").toBool(true),angular=kind=="arcSweep";
    auto project=[&](const QPointF& p){const auto q=projectSketchPoint(p.x(),p.y(),plane);return QPointF(q["x"].toDouble(),q["y"].toDouble());};
    const auto screenCenter=project(center),screenUnit=project(center+QPointF(100,0));
    const double pixelsPerMm=std::hypot((screenUnit-screenCenter).x(),(screenUnit-screenCenter).y())/100;if(pixelsPerMm<1e-7)return {};
    const double mmPerPixel=1/pixelsPerMm,degrees=std::numbers::pi/180;
    const double start=entity["startAngleDeg"].toDouble(),sweep=entity["sweepAngleDeg"].toDouble();
    const double middle=entity["type"]=="circle" ? 45 : start+sweep/2;
    const QPointF direction(std::cos(middle*degrees),std::sin(middle*degrees));
    auto annotation=dimension["annotation"].toObject();const QPointF anchor=annotation.isEmpty() ? center+direction*(radius+40*mmPerPixel) : QPointF(annotation["uMm"].toDouble(),annotation["vMm"].toDouble());
    annotation={{"uMm",anchor.x()},{"vMm",anchor.y()}};
    const QString content=(kind=="radius" ? "R" : kind=="diameter" ? QString::fromUtf8("Ø") : QString{})+QString::number(value/(angular || units!="in" ? 1 : 25.4),'g',6)+(angular ? QString::fromUtf8("°") : QString{});
    const QString text=driving ? content : "("+content+")";QFont font("Arial");font.setPixelSize(13);const QFontMetricsF metrics(font);
    auto point=[](const QPointF& p){return QJsonObject{{"uMm",p.x()},{"vMm",p.y()}};};
    const auto label=project(anchor);QJsonObject result{{"dimensionId",dimension["id"]},{"kind",kind},{"driving",driving},{"annotation",annotation},
        {angular ? "valueDeg" : "valueMm",value},{"mmPerPixel",mmPerPixel},{"center",point(center)},
        {"label",QJsonObject{{"text",text},{"x",label.x()},{"y",label.y()},{"width",metrics.horizontalAdvance(text)+12},{"height",metrics.height()+10},{"angleDegrees",0}}}};
    if(angular) {
        const double dimensionRadius=std::max(std::hypot((anchor-center).x(),(anchor-center).y()),radius+12*mmPerPixel);
        const auto radial=[&](double angle,double r){return center+QPointF(std::cos(angle*degrees),std::sin(angle*degrees))*r;};
        result["arcRadiusMm"]=dimensionRadius;result["startAngleDeg"]=start;result["sweepAngleDeg"]=sweep;
        QJsonArray arcs;
        const double gap=std::min(std::abs(sweep)/4,(metrics.horizontalAdvance(text)/2+7)*mmPerPixel/dimensionRadius/degrees);
        double labelOffset=std::remainder(std::atan2((anchor-center).y(),(anchor-center).x())/degrees-start,360.0);
        if(sweep>0 && labelOffset<0)labelOffset+=360;if(sweep<0 && labelOffset>0)labelOffset-=360;
        if((sweep>0 ? labelOffset>gap && labelOffset<sweep-gap : labelOffset< -gap && labelOffset>sweep+gap)) {
            const double sense=sweep>0 ? 1 : -1;arcs.append(QJsonObject{{"startAngleDeg",start},{"sweepAngleDeg",labelOffset-sense*gap}});
            arcs.append(QJsonObject{{"startAngleDeg",start+labelOffset+sense*gap},{"sweepAngleDeg",sweep-labelOffset-sense*gap}});
        } else arcs.append(QJsonObject{{"startAngleDeg",start},{"sweepAngleDeg",sweep}});
        result["arcSegments"]=arcs;
        result["witnesses"]=QJsonArray{QJsonArray{point(radial(start,radius)),point(radial(start,dimensionRadius))},QJsonArray{point(radial(start+sweep,radius)),point(radial(start+sweep,dimensionRadius))}};
        result["line"]=QJsonArray{point(radial(start,dimensionRadius)),point(radial(start+sweep,dimensionRadius))};
    } else {
        double angle=std::atan2((anchor-center).y(),(anchor-center).x())/degrees;
        if(entity["type"]=="arc") {double offset=std::remainder(angle-start,360.0);if(sweep>0 && offset<0)offset+=360;if(sweep<0 && offset>0)offset-=360;
            if(std::abs(offset)>std::abs(sweep)) {const auto a=points[entity["startPointId"].toString()],b=points[entity["endPointId"].toString()];
                angle=std::hypot((anchor-a).x(),(anchor-a).y())<std::hypot((anchor-b).x(),(anchor-b).y()) ? start : start+sweep;}}
        const QPointF ray(std::cos(angle*degrees),std::sin(angle*degrees)),tip=center+ray*radius;
        const double side=anchor.x()>=tip.x() ? 1 : -1;
        const QPointF end=anchor-QPointF(side*(metrics.horizontalAdvance(text)/2+6)*mmPerPixel,0);
        result["line"]=QJsonArray{point(kind=="diameter" ? center-ray*radius : tip),point(kind=="diameter" ? tip : end)};
        result["leader"]=QJsonArray{point(tip),point(end)};result["witnesses"]=QJsonArray{};
    }
    return result;
}
QJsonObject ReferenceView::pickAnnotation(double x,double y) const {
    for(auto i=annotationLayouts_.size()-1;i>=0;--i) {
        const auto layout=annotationLayouts_[i].toObject();if(layout.value("dimensionId")=="pending")continue;
        const auto label=layout.value("label").toObject();const double angle=label.value("angleDegrees").toDouble()*std::acos(-1.0)/180;
        const double dx=x-label.value("x").toDouble(),dy=y-label.value("y").toDouble();
        if(std::abs(dx*std::cos(angle)-dy*std::sin(angle))<=label.value("width").toDouble()/2
            && std::abs(dx*std::sin(angle)+dy*std::cos(angle))<=label.value("height").toDouble()/2)
            return {{"kind","dimension"},{"dimensionId",layout.value("dimensionId")},{"part","label"},{"annotation",layout.value("annotation")}};
    }
    return {};
}
void ReferenceView::decorateSketch(const QJsonObject& feature,const QJsonObject& selection,const QJsonObject& diagnostics,const QString& units,const QJsonObject& pending) {
    decorationFeature_=feature;decorationSelection_=selection;decorationDiagnostics_=diagnostics;decorationUnits_=units;decorationPending_=pending;
    clearSketchDecorations();if(context_.IsNull())return;
    for(auto i=profileModels_.begin();i!=profileModels_.end();++i) {
        i.value()->SetColor(selection.value("profileIds").toArray().contains(i.key()) ? Quantity_Color(0.17,0.48,0.90,Quantity_TOC_sRGB) : Quantity_Color(0.39,0.68,0.91,Quantity_TOC_sRGB));
        context_->Redisplay(i.value(),false);
    }
    const auto parameters=feature["parameters"].toObject();const auto plane=parameters["plane"].toString();
    const bool fullyConstrained=diagnostics["dof"].toInt(-1)==0;
    if(!sketchDraft_.IsNull()){sketchDraft_->SetColor(fullyConstrained ? Quantity_Color(0.12,0.14,0.16,Quantity_TOC_sRGB) : Quantity_Color(0.10,0.34,0.84,Quantity_TOC_RGB));context_->Redisplay(sketchDraft_,false);}
    QMap<QString,QJsonObject> points;QSet<QString> fixed;
    for(const auto& entry:parameters["constraints"].toArray()) {
        const auto constraint=entry.toObject();if(constraint["type"]=="fixed")for(const auto& id:constraint["pointIds"].toArray())fixed.insert(id.toString());
    }
    for(const auto& entry:parameters["points"].toArray()) {
        const auto point=entry.toObject();const auto id=point["id"].toString();points[id]=point;
        const bool selected=selection["pointIds"].toArray().contains(id);
        const Quantity_Color color=selected ? Quantity_Color(0.98,0.43,0.08,Quantity_TOC_RGB)
            : fixed.contains(id) ? Quantity_Color(0.10,0.52,0.24,Quantity_TOC_RGB)
            : fullyConstrained ? Quantity_Color(0.12,0.14,0.16,Quantity_TOC_sRGB) : Quantity_Color(0.10,0.34,0.84,Quantity_TOC_RGB);
        Handle(AIS_Point) marker=new AIS_Point(new Geom_CartesianPoint(sketchWorld(plane,point["uMm"].toDouble(),point["vMm"].toDouble())));
        marker->Attributes()->SetPointAspect(new Prs3d_PointAspect(Aspect_TOM_O_POINT,color,selected ? 3 : 2));
        marker->SetZLayer(Graphic3d_ZLayerId_Topmost);context_->Display(marker,0,-1,false);context_->Deactivate(marker);sketchDecorations_.push_back(marker);
    }
    QSet<QString> markedProjectedPoints;
    for(const auto& value:projectedLines_) {
        const auto line=value.toObject();
        for(const bool end:{false,true}) {
            const auto id=line[end ? "endPointId" : "startPointId"].toString();
            if(markedProjectedPoints.contains(id) || !selection["pointIds"].toArray().contains(id))continue;
            markedProjectedPoints.insert(id);
            const auto u=line[end ? "u2Mm" : "u1Mm"].toDouble(),v=line[end ? "v2Mm" : "v1Mm"].toDouble();
            Handle(AIS_Point) marker=new AIS_Point(new Geom_CartesianPoint(sketchWorld(plane,u,v)));
            marker->Attributes()->SetPointAspect(new Prs3d_PointAspect(Aspect_TOM_O_POINT,
                Quantity_Color(0.81,0.40,0.98,Quantity_TOC_sRGB),4));
            marker->SetZLayer(Graphic3d_ZLayerId_Topmost);context_->Display(marker,0,-1,false);context_->Deactivate(marker);sketchDecorations_.push_back(marker);
        }
    }
    for(const auto& value:supportPoints_) {
        const auto point=value.toObject();const bool selected=selection["pointIds"].toArray().contains(point["id"]);
        const bool retained=point.value("retained").toBool();
        const auto color=Quantity_Color(selected ? 0.84 : retained ? 0.66 : 0.55,
            selected ? 0.44 : retained ? 0.31 : 0.27,selected ? 0.98 : retained ? 0.85 : 0.70,Quantity_TOC_sRGB);
        Handle(AIS_Point) marker=new AIS_Point(new Geom_CartesianPoint(sketchWorld(plane,point["uMm"].toDouble(),point["vMm"].toDouble())));
        marker->Attributes()->SetPointAspect(new Prs3d_PointAspect(Aspect_TOM_PLUS,color,(selected ? 7.0 : 5.0)*devicePixelRatioF()));
        marker->SetZLayer(Graphic3d_ZLayerId_Topmost);context_->Display(marker,0,-1,false);context_->Deactivate(marker);sketchDecorations_.push_back(marker);
    }
    for(const auto& entry:parameters["entities"].toArray()) {
        const auto entity=entry.toObject();if(!selection["entityIds"].toArray().contains(entity["id"]))continue;
        const auto edge=SketchGeometry::analyticEdge(parameters,entity,sketchFrame_);
        const auto highlight=entity.value("construction").toBool() ? constructionPresentation(edge,true) : sketchPresentation(edge);
        if(!entity.value("construction").toBool()){highlight->SetColor(Quantity_Color(0.98,0.43,0.08,Quantity_TOC_RGB));highlight->SetWidth(4);}
        highlight->SetZLayer(Graphic3d_ZLayerId_Topmost);
        context_->Display(highlight,AIS_WireFrame,-1,false);context_->Deactivate(highlight);sketchDecorations_.push_back(highlight);
    }
    QJsonArray projectedUsedIds;QSet<QString> projectedIds;
    for(const auto& value:projectedLines_)projectedIds.insert(value.toObject()["id"].toString());
    for(const auto& entry:parameters["constraints"].toArray())
        for(const auto& id:entry.toObject()["entityIds"].toArray())projectedUsedIds.append(id);
    for(const auto& value:projectedLines_) {
        const auto line=value.toObject();const auto id=line["id"];
        const bool selected=selection["entityIds"].toArray().contains(id);
        const bool used=projectedUsedIds.contains(id);
        if(!selected && !used)continue;
        const auto a=sketchWorld(plane,line["u1Mm"].toDouble(),line["v1Mm"].toDouble());
        const auto b=sketchWorld(plane,line["u2Mm"].toDouble(),line["v2Mm"].toDouble());
        if(a.Distance(b)<1e-9)continue;
        auto highlight=sketchPresentation(BRepBuilderAPI_MakeEdge(a,b).Edge());
        highlight->SetColor(Quantity_Color(selected ? 0.81 : 0.68,selected ? 0.40 : 0.30,selected ? 0.98 : 0.84,Quantity_TOC_sRGB));
        highlight->SetWidth((selected ? 4.0 : 3.0)*devicePixelRatioF());
        highlight->SetZLayer(Graphic3d_ZLayerId_Topmost);
        context_->Display(highlight,AIS_WireFrame,-1,false);context_->Deactivate(highlight);sketchDecorations_.push_back(highlight);
    }
    // Independently authored retained-relation badges, anchored to their real
    // targets. The public tutorial scenes show bars, double bars and corner
    // marks; exact hover/selection and collision behavior remain unqualified.
    QMap<QString,QJsonObject> entities;for(const auto& entry:parameters["entities"].toArray())entities[entry.toObject()["id"].toString()]=entry.toObject();
    for(const auto& value:projectedLines_) {
        const auto line=value.toObject();const auto start=line["startPointId"].toString(),end=line["endPointId"].toString();
        points[start]={{"id",start},{"uMm",line["u1Mm"]},{"vMm",line["v1Mm"]}};
        points[end]={{"id",end},{"uMm",line["u2Mm"]},{"vMm",line["v2Mm"]}};
        entities[line["id"].toString()]={{"id",line["id"]},{"type","line"},{"startPointId",start},{"endPointId",end}};
    }
    for(const auto& value:supportPoints_) {
        const auto point=value.toObject();
        if(point.value("retained").toBool())points[point["id"].toString()]=point;
    }
    QMap<QString,int> badgeSlots;
    for(const auto& entry:parameters["constraints"].toArray()) {
        const auto relation=entry.toObject();const auto kind=relation["type"].toString();
        if(kind=="tangent" || kind=="arcTangent") {
            const auto ids=relation["entityIds"].toArray();if(ids.size()!=2)continue;
            const auto circle=entities.value(ids[0].toString()),line=entities.value(ids[1].toString());
            if(line["type"]!="line")continue;
            QPointF contact;
            if(kind=="arcTangent") {
                if(circle["type"]!="arc" || relation["pointIds"].toArray().size()!=1)continue;
                const auto shared=points.value(relation["pointIds"].toArray()[0].toString());
                if(shared.isEmpty())continue;
                contact=QPointF(shared["uMm"].toDouble(),shared["vMm"].toDouble());
            } else {
                if(circle["type"]!="circle")continue;
                const auto center=points.value(circle["centerPointId"].toString());
                const auto first=points.value(line["startPointId"].toString()),last=points.value(line["endPointId"].toString());
                if(center.isEmpty() || first.isEmpty() || last.isEmpty())continue;
                const QPointF c(center["uMm"].toDouble(),center["vMm"].toDouble());
                const QPointF a(first["uMm"].toDouble(),first["vMm"].toDouble());
                const QPointF d(last["uMm"].toDouble()-a.x(),last["vMm"].toDouble()-a.y());
                const double squared=QPointF::dotProduct(d,d);if(squared<1e-18)continue;
                const double t=std::clamp(QPointF::dotProduct(c-a,d)/squared,0.0,1.0);
                contact=a+d*t;
            }
            const auto screen=projectSketchPoint(contact.x(),contact.y(),plane);
            const auto offset=projectSketchPoint(contact.x()+100,contact.y(),plane);
            const double pixels=std::hypot(offset["x"].toDouble()-screen["x"].toDouble(),offset["y"].toDouble()-screen["y"].toDouble());
            if(pixels<1e-6)continue;
            const double size=100/pixels;
            const double u=contact.x(),v=contact.y()+(10+15*badgeSlots[ids[1].toString()]++)*size;
            TopoDS_Compound strokes;BRep_Builder builder;builder.MakeCompound(strokes);
            auto stroke=[&](double x1,double y1,double x2,double y2){builder.Add(strokes,BRepBuilderAPI_MakeEdge(sketchWorld(plane,u+x1*size,v+y1*size),sketchWorld(plane,u+x2*size,v+y2*size)).Edge());};
            stroke(-4,-4,4,-4);stroke(4,-4,4,4);stroke(4,4,-4,4);stroke(-4,4,-4,-4);
            const bool selected=selection["entityIds"].toArray().contains(ids[0]) || selection["entityIds"].toArray().contains(ids[1]);
            auto badge=sketchPresentation(strokes);badge->SetColor(selected ? Quantity_Color(0.98,0.43,0.08,Quantity_TOC_RGB) : Quantity_Color(0.36,0.39,0.42,Quantity_TOC_sRGB));badge->SetWidth(2*devicePixelRatioF());badge->SetZLayer(Graphic3d_ZLayerId_Topmost);
            context_->Display(badge,AIS_WireFrame,-1,false);context_->Deactivate(badge);sketchDecorations_.push_back(badge);
            double left=1e100,top=1e100,right=-1e100,bottom=-1e100;
            for(const double dx:{-6.0,6.0})for(const double dy:{-6.0,6.0}) {
                const auto corner=projectSketchPoint(u+dx*size,v+dy*size,plane);
                left=std::min(left,corner["x"].toDouble());right=std::max(right,corner["x"].toDouble());
                top=std::min(top,corner["y"].toDouble());bottom=std::max(bottom,corner["y"].toDouble());
            }
            constraintGlyphs_.append(QJsonObject{{"constraintId",relation["id"]},{"entityIds",ids},{"pointIds",relation["pointIds"]},{"anchorEntityId",ids[1]},
                {"kind",kind},{"selected",selected},{"contactMm",QJsonObject{{"uMm",contact.x()},{"vMm",contact.y()}}},
                {"anchorMm",QJsonObject{{"uMm",u},{"vMm",v}}},{"bounds",QJsonObject{{"x",left},{"y",top},
                    {"width",right-left},{"height",bottom-top}}},
                {"viewportLocal",true},{"units","logicalPixels"}});
            continue;
        }
        if(kind!="equal" && kind!="horizontal" && kind!="vertical" && kind!="perpendicular" && kind!="parallel" && kind!="midpoint")continue;
        for(const auto& target:relation["entityIds"].toArray()) {
            const auto entity=entities.value(target.toString());if(entity.isEmpty())continue;
            double u,v;
            if(kind=="midpoint") {
                const auto pointId=relation["pointIds"].toArray().at(0).toString();
                const auto point=points.value(pointId);if(entity["type"]!="line" || point.isEmpty())continue;
                u=point["uMm"].toDouble();v=point["vMm"].toDouble();
            } else if(entity["type"]=="line") {
                const auto a=points.value(entity["startPointId"].toString()),b=points.value(entity["endPointId"].toString());
                u=(a["uMm"].toDouble()+b["uMm"].toDouble())/2;v=(a["vMm"].toDouble()+b["vMm"].toDouble())/2;
            } else {
                const auto center=points.value(entity["centerPointId"].toString());
                const double angle=(entity["type"]=="arc" ? entity["startAngleDeg"].toDouble()+entity["sweepAngleDeg"].toDouble()/2 : 45)*std::numbers::pi/180;
                u=center["uMm"].toDouble()+entity["radiusMm"].toDouble()*std::cos(angle);v=center["vMm"].toDouble()+entity["radiusMm"].toDouble()*std::sin(angle);
            }
            // Measure a longer basis: a one-mm projection can round to the same
            // framebuffer pixel when zoomed out and incorrectly hide the badge.
            const auto p=projectSketchPoint(u,v,plane),q=projectSketchPoint(u+100,v,plane);
            const double pixels=std::hypot(q["x"].toDouble()-p["x"].toDouble(),q["y"].toDouble()-p["y"].toDouble());if(pixels<1e-6)continue;
            const double size=100/pixels;v+=(10+15*badgeSlots[target.toString()]++)*size;
            TopoDS_Compound strokes;BRep_Builder builder;builder.MakeCompound(strokes);
            auto stroke=[&](double x1,double y1,double x2,double y2){builder.Add(strokes,BRepBuilderAPI_MakeEdge(sketchWorld(plane,u+x1*size,v+y1*size),sketchWorld(plane,u+x2*size,v+y2*size)).Edge());};
            if(kind=="midpoint"){stroke(-5,4,5,4);stroke(5,4,0,-5);stroke(0,-5,-5,4);}
            else if(kind=="equal"){stroke(-5,-2,5,-2);stroke(-5,2,5,2);}
            else if(kind=="horizontal")stroke(-5,0,5,0);
            else if(kind=="vertical")stroke(0,-5,0,5);
            else if(kind=="perpendicular"){stroke(-4,4,-4,-4);stroke(-4,-4,4,-4);}
            else {stroke(-4,-4,0,4);stroke(0,-4,4,4);}
            const bool selected=selection["entityIds"].toArray().contains(target) ||
                (kind=="midpoint" && selection["pointIds"].toArray().contains(relation["pointIds"].toArray().at(0)));
            auto badge=sketchPresentation(strokes);badge->SetColor(projectedIds.contains(target.toString()) ? Quantity_Color(0.73,0.34,0.91,Quantity_TOC_sRGB)
                : selected ? Quantity_Color(0.98,0.43,0.08,Quantity_TOC_RGB) : Quantity_Color(0.36,0.39,0.42,Quantity_TOC_sRGB));badge->SetWidth(2*devicePixelRatioF());badge->SetZLayer(Graphic3d_ZLayerId_Topmost);
            context_->Display(badge,AIS_WireFrame,-1,false);context_->Deactivate(badge);sketchDecorations_.push_back(badge);
            double left=1e100,top=1e100,right=-1e100,bottom=-1e100;
            for(const double x:{-6.0,6.0})for(const double y:{-6.0,6.0}) {
                const auto corner=projectSketchPoint(u+x*size,v+y*size,plane);left=std::min(left,corner["x"].toDouble());right=std::max(right,corner["x"].toDouble());top=std::min(top,corner["y"].toDouble());bottom=std::max(bottom,corner["y"].toDouble());
            }
            constraintGlyphs_.append(QJsonObject{{"constraintId",relation["id"]},{"pointIds",relation["pointIds"]},{"entityIds",relation["entityIds"]},{"anchorEntityId",target},{"kind",kind},{"selected",selected},{"anchorMm",QJsonObject{{"uMm",u},{"vMm",v}}},{"bounds",QJsonObject{{"x",left},{"y",top},{"width",right-left},{"height",bottom-top}}},{"viewportLocal",true},{"units","logicalPixels"}});
        }
    }
    QMap<QString,QJsonObject> measurements;
    for(const auto& entry:diagnostics["dimensionValues"].toArray()){const auto value=entry.toObject();measurements[value["id"].toString()]=value;}
    const auto layoutFeature=SketchGeometry::expandedProjectionFeature(feature,{{"lines",projectedLines_},{"points",supportPoints_}});
    auto dimensions=parameters.value("dimensions").toArray();
    if(!pending.isEmpty() && !pending.contains("dimensionId")){auto preview=pending;preview["id"]="pending";dimensions.append(preview);}
    for(const auto& entry:dimensions) {
        const auto dimension=entry.toObject();const auto measured=measurements.value(dimension.value("id").toString());
        const bool angular=dimension["kind"]=="arcSweep" || dimension["kind"]=="angle";const auto valueKey=angular ? "valueDeg" : "valueMm";
        const double value=dimension.value("id")=="pending" ? pending.value(angular ? "measuredValueDeg" : "measuredValueMm").toDouble() : measured.value(valueKey).toDouble(dimension.value(valueKey).toDouble());
        const auto layout=dimensionLayout(layoutFeature,dimension,value,units);if(layout.isEmpty())continue;annotationLayouts_.append(layout);
        const bool selected=selection.value("dimensionId")==dimension.value("id");
        const Quantity_Color color=selected || dimension.value("id")=="pending" ? Quantity_Color(0.10,0.34,0.84,Quantity_TOC_RGB) : Quantity_Color(0.10,0.11,0.12,Quantity_TOC_sRGB);
        const auto line=layout.value("line").toArray();auto world=[&](const QJsonValue& v){const auto p=v.toObject();return sketchWorld(plane,p.value("uMm").toDouble(),p.value("vMm").toDouble());};
        const auto frame=frameFor(plane);const auto n=frame.value("normal").toArray(),u=frame.value("uAxis").toArray(),origin=frame.value("origin").toArray();
        const double size=layout.value("mmPerPixel").toDouble();
        if(layout["kind"]!="radius" && layout["kind"]!="arcSweep" && layout["kind"]!="angle") {
        Handle(PrsDim_LengthDimension) graphic=new PrsDim_LengthDimension(world(line[0]),world(line[1]),gp_Pln(gp_Ax3(gp_Pnt(origin[0].toDouble(),origin[1].toDouble(),origin[2].toDouble()),gp_Dir(n[0].toDouble(),n[1].toDouble(),n[2].toDouble()),gp_Dir(u[0].toDouble(),u[1].toDouble(),u[2].toDouble()))));
        Handle(Prs3d_DimensionAspect) aspect=new Prs3d_DimensionAspect;aspect->SetCommonColor(color);aspect->MakeText3d(false);aspect->MakeArrows3d(false);aspect->MakeUnitsDisplayed(false);
        aspect->LineAspect()->SetWidth(devicePixelRatioF());
aspect->ArrowAspect()->SetLength(7*size);aspect->SetArrowTailSize(8*size);aspect->SetExtensionSize(8*size);
        graphic->SetDimensionAspect(aspect);graphic->SetCustomValue(TCollection_ExtendedString(""));graphic->SetFlyout(0);graphic->SetZLayer(Graphic3d_ZLayerId_Topmost);
        context_->Display(graphic,0,-1,false);context_->Deactivate(graphic);sketchDecorations_.push_back(graphic);
        } else {
            TopoDS_Compound compound;BRep_Builder builder;builder.MakeCompound(compound);
            auto segment=[&](const gp_Pnt& a,const gp_Pnt& b){if(a.Distance(b)>1e-9)builder.Add(compound,BRepBuilderAPI_MakeEdge(a,b).Edge());};
            const gp_Vec normal(n[0].toDouble(),n[1].toDouble(),n[2].toDouble());
            auto arrow=[&](const gp_Pnt& tip,gp_Vec along){if(along.Magnitude()<1e-9)return;along.Normalize();const auto side=normal.Crossed(along);const auto back=tip.Translated(along*(7*size));segment(tip,back.Translated(side*(2.5*size)));segment(tip,back.Translated(side*(-2.5*size)));};
            if(layout["kind"]=="radius") {const auto leader=layout["leader"].toArray();const auto a=world(leader[0]),b=world(leader[1]);segment(a,b);arrow(a,gp_Vec(a,b));}
            else {
                for(const auto& value:layout["arcSegments"].toArray()){const auto arc=value.toObject();const auto centerId=layout["kind"]=="angle" ? QJsonValue("dimension-center") : dimension["entityId"];
                    QJsonObject e{{"type","arc"},{"centerPointId",centerId},{"radiusMm",layout["arcRadiusMm"]},{"startAngleDeg",arc["startAngleDeg"]},{"sweepAngleDeg",arc["sweepAngleDeg"]}};
                    auto p=parameters;const auto center=layout["center"].toObject();p["points"]=QJsonArray{QJsonObject{{"id",centerId},{"uMm",center["uMm"]},{"vMm",center["vMm"]}}};builder.Add(compound,SketchGeometry::analyticEdge(p,e,sketchFrame_));}
                const double start=layout["startAngleDeg"].toDouble()*std::numbers::pi/180,sweep=layout["sweepAngleDeg"].toDouble()*std::numbers::pi/180;
                const gp_Vec ua(u[0].toDouble(),u[1].toDouble(),u[2].toDouble()),va=normal.Crossed(ua);
                const double sense=sweep>0 ? 1 : -1;arrow(world(line[0]),(ua*(-std::sin(start))+va*std::cos(start))*sense);
                arrow(world(line[1]),(ua*(-std::sin(start+sweep))+va*std::cos(start+sweep))*(-sense));
            }
            const auto graphic=sketchPresentation(compound);graphic->SetColor(color);graphic->SetWidth(devicePixelRatioF());graphic->SetZLayer(Graphic3d_ZLayerId_Topmost);
            context_->Display(graphic,AIS_WireFrame,-1,false);context_->Deactivate(graphic);sketchDecorations_.push_back(graphic);
        }
        if(layout["kind"]=="diameter") {const auto leader=layout["leader"].toArray();const auto a=world(leader[0]),b=world(leader[1]);
            if(a.Distance(b)>1e-9){const auto graphic=sketchPresentation(BRepBuilderAPI_MakeEdge(a,b).Edge());graphic->SetColor(color);graphic->SetWidth(devicePixelRatioF());graphic->SetZLayer(Graphic3d_ZLayerId_Topmost);context_->Display(graphic,AIS_WireFrame,-1,false);context_->Deactivate(graphic);sketchDecorations_.push_back(graphic);}}
        for(const auto& witness:layout.value("witnesses").toArray()) {
            const auto pair=witness.toArray();const auto a=world(pair[0]),b=world(pair[1]);gp_Vec delta(a,b);if(delta.Magnitude()<1e-9)continue;delta.Normalize();
            const auto edge=BRepBuilderAPI_MakeEdge(a.Translated(delta*(3*size)),b.Translated(delta*(4*size))).Edge();const auto wire=sketchPresentation(edge);
            wire->SetColor(color);wire->SetWidth(devicePixelRatioF());wire->SetZLayer(Graphic3d_ZLayerId_Topmost);context_->Display(wire,AIS_WireFrame,-1,false);context_->Deactivate(wire);sketchDecorations_.push_back(wire);
        }
        const auto text=layout.value("label").toObject(),anchor=layout.value("annotation").toObject();
        Handle(AIS_TextLabel) label=new AIS_TextLabel;label->SetText(TCollection_ExtendedString(text.value("text").toString().toUtf8().constData(),true));
        label->SetPosition(sketchWorld(plane,anchor.value("uMm").toDouble(),anchor.value("vMm").toDouble()));label->SetHeight(13);label->SetFont("Arial");label->SetAngle(text.value("angleDegrees").toDouble()*std::acos(-1.0)/180);
        label->SetHJustification(Graphic3d_HTA_CENTER);label->SetVJustification(Graphic3d_VTA_CENTER);label->SetZoomable(false);label->SetDisplayType(Aspect_TODT_NORMAL);
        label->SetColor(color);label->SetZLayer(Graphic3d_ZLayerId_Topmost);context_->Display(label,0,-1,false);context_->Deactivate(label);sketchDecorations_.push_back(label);
    }
    context_->UpdateCurrentViewer();
}
QJsonObject ReferenceView::frameFor(const QString& plane) const {
    if(plane=="face") {
        if(sketchFrame_.isEmpty())throw std::invalid_argument("Face sketch frame is unresolved");
        return sketchFrame_;
    }
    return SketchGeometry::frame(plane);
}
gp_Pnt ReferenceView::sketchWorld(const QString& plane,double u,double v) const {
    const auto frame=frameFor(plane);const auto origin=frame.value("origin").toArray();
    const auto ua=frame.value("uAxis").toArray(),va=frame.value("vAxis").toArray();
    return gp_Pnt(origin[0].toDouble()+u*ua[0].toDouble()+v*va[0].toDouble(),
        origin[1].toDouble()+u*ua[1].toDouble()+v*va[1].toDouble(),
        origin[2].toDouble()+u*ua[2].toDouble()+v*va[2].toDouble());
}
void ReferenceView::setSketchPlane(const QString& plane,const QJsonObject& resolvedFrame) {
    sketchPlane_=plane;
    if(!plane.isEmpty() || orientation!="face")sketchFrame_=resolvedFrame;
}
void ReferenceView::lookAtSketch(const QString& plane,const QJsonObject& resolvedFrame) {
    setSketchPlane(plane,resolvedFrame);
    orientation=plane=="face" ? "face" : plane=="XY" ? "top" : plane=="XZ" ? "front" : "right";
    const auto o=frameFor(plane).value("origin").toArray();
    sceneCenter_=gp_Pnt(o[0].toDouble(),o[1].toDouble(),o[2].toDouble());sceneScale_=100;pan={};zoom=1;sync();
}
QJsonObject ReferenceView::sketchPoint(double x,double y,const QString& plane) const {
    if(view_.IsNull() || x<0 || y<0 || x>=width() || y>=height())throw std::invalid_argument("Point is outside the initialized viewport");
    const auto scale=windowScale();double px,py,pz,dx,dy,dz;
    view_->ConvertWithProj(qRound(x*scale.x()),qRound(y*scale.y()),px,py,pz,dx,dy,dz);
    const auto frame=frameFor(plane);const auto normal=frame["normal"].toArray(),origin=frame["origin"].toArray();
    const gp_Vec n(normal[0].toDouble(),normal[1].toDouble(),normal[2].toDouble()),ray(dx,dy,dz);
    const double denominator=n.Dot(ray);
    if(std::abs(denominator)<1e-9)throw std::invalid_argument("Look At the sketch plane before drawing");
    const gp_Vec base(origin[0].toDouble(),origin[1].toDouble(),origin[2].toDouble());
    const auto world=gp_Vec(px,py,pz)-ray*(n.Dot(gp_Vec(px,py,pz)-base)/denominator)-base;
    const auto u=frame["uAxis"].toArray(),v=frame["vAxis"].toArray();
    return {{"uMm",world.Dot(gp_Vec(u[0].toDouble(),u[1].toDouble(),u[2].toDouble()))},
        {"vMm",world.Dot(gp_Vec(v[0].toDouble(),v[1].toDouble(),v[2].toDouble()))}};
}
QJsonObject ReferenceView::projectSketchPoint(double uMm,double vMm,const QString& plane,const QJsonObject& resolvedFrame) const {
    if(view_.IsNull())return {};
    gp_Pnt world;
    if(plane=="face" && !resolvedFrame.isEmpty()) {
        const auto o=resolvedFrame.value("origin").toArray(),u=resolvedFrame.value("uAxis").toArray(),v=resolvedFrame.value("vAxis").toArray();
        world=gp_Pnt(o[0].toDouble()+uMm*u[0].toDouble()+vMm*v[0].toDouble(),
            o[1].toDouble()+uMm*u[1].toDouble()+vMm*v[1].toDouble(),
            o[2].toDouble()+uMm*u[2].toDouble()+vMm*v[2].toDouble());
    } else world=sketchWorld(plane,uMm,vMm);
    int x,y;view_->Convert(world.X(),world.Y(),world.Z(),x,y);
    const auto scale=windowScale();return {{"x",x/scale.x()},{"y",y/scale.y()},{"viewportLocal",true},{"units","logicalPixels"}};
}
void ReferenceView::showSketchCursor(const QString& plane,const QPointF& first,const QPointF& second,bool rectangle) {
    clearSketchCursor();if(context_.IsNull() || first==second)return;
    BRepBuilderAPI_MakePolygon polygon;polygon.Add(sketchWorld(plane,first.x(),first.y()));
    if(rectangle) {
        if(first.x()==second.x() || first.y()==second.y())return;
        polygon.Add(sketchWorld(plane,second.x(),first.y()));polygon.Add(sketchWorld(plane,second.x(),second.y()));
        polygon.Add(sketchWorld(plane,first.x(),second.y()));polygon.Close();
    } else polygon.Add(sketchWorld(plane,second.x(),second.y()));
    if(!polygon.IsDone())return;
    sketchCursor_=sketchPresentation(polygon.Wire());
    sketchCursor_->SetColor(Quantity_Color(0.30,0.58,0.87,Quantity_TOC_RGB));
    context_->Display(sketchCursor_,AIS_WireFrame,-1,false);context_->Deactivate(sketchCursor_);context_->UpdateCurrentViewer();
}
void ReferenceView::showSketchSnap(const QString& plane,const QJsonObject& snap) {
    if(!context_.IsNull() && !sketchSnapCue_.IsNull())context_->Remove(sketchSnapCue_,false);
    sketchSnapCue_.Nullify();sketchSnapState_=snap;
    const auto status=snap.value("status").toString();
    if(context_.IsNull() || (status!="acquired" && status!="ambiguous"))return;
    const double u=snap.value("uMm").toDouble(),v=snap.value("vMm").toDouble();
    const auto center=projectSketchPoint(u,v,plane),offset=projectSketchPoint(u+100,v,plane);
    const double pixels=std::hypot(offset["x"].toDouble()-center["x"].toDouble(),offset["y"].toDouble()-center["y"].toDouble());
    if(pixels<1e-7)return;
    const double size=600/pixels;
    TopoDS_Compound compound;BRep_Builder builder;builder.MakeCompound(compound);
    auto segment=[&](double x1,double y1,double x2,double y2){builder.Add(compound,BRepBuilderAPI_MakeEdge(sketchWorld(plane,u+x1*size,v+y1*size),sketchWorld(plane,u+x2*size,v+y2*size)).Edge());};
    if(status=="ambiguous"){
        segment(-1,-1,1,1);segment(-1,1,1,-1);
    } else if(snap.value("target").toObject().value("kind")=="tangent"){
        segment(-1,-1,1,-1);segment(1,-1,1,1);segment(1,1,-1,1);segment(-1,1,-1,-1);
    } else if(snap.value("target").toObject().value("kind")=="filletCorner"){
        segment(-1,-1,1,-1);segment(1,-1,1,1);segment(1,1,-1,1);segment(-1,1,-1,-1);
    } else if(snap.value("target").toObject().value("kind")=="midpoint"){
        segment(-1,0.7,1,0.7);segment(1,0.7,0,-1);segment(0,-1,-1,0.7);
    } else if(snap.value("supportPoint").toBool()){
        segment(-1,0,1,0);segment(0,-1,0,1);
    } else {
        segment(0,-1,1,0);segment(1,0,0,1);segment(0,1,-1,0);segment(-1,0,0,-1);
        if(snap.value("target").toObject().value("kind")=="origin"){
            segment(-0.5,0,0.5,0);segment(0,-0.5,0,0.5);
        }
    }
    sketchSnapCue_=sketchPresentation(compound);
    sketchSnapCue_->SetColor(status=="ambiguous" ? Quantity_Color(0.95,0.53,0.10,Quantity_TOC_RGB)
        : snap.value("supportPoint").toBool() ? Quantity_Color(0.73,0.34,0.91,Quantity_TOC_sRGB)
        : Quantity_Color(0.08,0.55,0.84,Quantity_TOC_RGB));
    sketchSnapCue_->SetWidth(2*devicePixelRatioF());sketchSnapCue_->SetZLayer(Graphic3d_ZLayerId_Topmost);
    context_->Display(sketchSnapCue_,AIS_WireFrame,-1,false);context_->Deactivate(sketchSnapCue_);context_->UpdateCurrentViewer();
}
void ReferenceView::showSketchCurveCursor(const QJsonObject& feature) {
    clearSketchCursor();if(context_.IsNull())return;
    const auto parameters=feature["parameters"].toObject(),entity=parameters["entities"].toArray().first().toObject();
    sketchCursor_=sketchPresentation(SketchGeometry::analyticEdge(parameters,entity,sketchFrame_));sketchCursor_->SetColor(Quantity_Color(0.30,0.58,0.87,Quantity_TOC_RGB));
    context_->Display(sketchCursor_,AIS_WireFrame,-1,false);context_->Deactivate(sketchCursor_);context_->UpdateCurrentViewer();
}
void ReferenceView::clearSketchCursor() {
    if(!context_.IsNull() && !sketchCursor_.IsNull())context_->Remove(sketchCursor_,false);
    if(!context_.IsNull() && !sketchSnapCue_.IsNull())context_->Remove(sketchSnapCue_,false);
    sketchCursor_.Nullify();
    sketchSnapCue_.Nullify();sketchSnapState_={};
}
void ReferenceView::fit(const std::vector<EvaluatedBody>& bodies,const std::vector<EvaluatedSketch>& sketches) {
    Bnd_Box bounds;
    for(const auto& body:bodies)if(body.visible)BRepBndLib::AddOptimal(body.shape,bounds,false,false);
    for(const auto& sketch:sketches)if(sketch.visible && !sketch.shape.IsNull())BRepBndLib::AddOptimal(sketch.shape,bounds,false,false);
    if(bounds.IsVoid()){sceneScale_=400;sceneCenter_=gp_Pnt();}
    else {
        double x0,y0,z0,x1,y1,z1;bounds.Get(x0,y0,z0,x1,y1,z1);
        sceneCenter_=gp_Pnt((x0+x1)/2,(y0+y1)/2,(z0+z1)/2);
        sceneScale_=std::max({x1-x0,y1-y0,z1-z0,1.0})*1.7;
    }
    zoom=1;pan={};sync();
}
void ReferenceView::selectBody(const QString& id) {
    if(context_.IsNull())return;
    context_->ClearSelected(false);
    if(models_.contains(id))context_->SetSelected(models_.value(id),false);
    context_->UpdateCurrentViewer();
}
void ReferenceView::highlightCombineInputs(const QString& targetBodyId,const QJsonArray& toolBodyIds) {
    combineTargetId_=targetBodyId;combineToolIds_=toolBodyIds;
    if(context_.IsNull())return;
    context_->ClearSelected(false);
    for(auto it=models_.begin();it!=models_.end();++it) {
        const auto color=it.key()==targetBodyId ? Quantity_Color(0.16,0.47,0.86,Quantity_TOC_sRGB)
            : toolBodyIds.contains(it.key()) ? Quantity_Color(0.91,0.45,0.13,Quantity_TOC_sRGB)
            : Quantity_Color(0.67,0.72,0.76,Quantity_TOC_RGB);
        it.value()->SetColor(color);context_->Redisplay(it.value(),false);
    }
    applyEditDisplay();
}
QPointF ReferenceView::windowScale() const {
    // OCCT input/projection uses native window units. Cocoa uses points;
    // other window backends may use pixels. Framebuffer DPR is separate.
    int w=0,h=0;view_->Window()->Size(w,h);
    return {double(w)/width(),double(h)/height()};
}
QString ReferenceView::pickBody(double x,double y) {
    if(context_.IsNull() || view_.IsNull())return {};
    if(x<0 || y<0 || x>=width() || y>=height())return {};
    sync();const auto scale=windowScale();
    context_->MoveTo(qRound(x*scale.x()),qRound(y*scale.y()),view_,true);
    if(!context_->HasDetected())return {};
    const auto detected=context_->DetectedInteractive();
    for(auto it=models_.begin();it!=models_.end();++it)if(it.value()==detected)return it.key();
    return {};
}
QJsonObject ReferenceView::projectBody(const EvaluatedBody& body) const {
    if(view_.IsNull())return {};
    Bnd_Box bounds;BRepBndLib::AddOptimal(body.shape,bounds,false,false);
    double x0,y0,z0,x1,y1,z1;bounds.Get(x0,y0,z0,x1,y1,z1);int x,y;
    view_->Convert((x0+x1)/2,(y0+y1)/2,(z0+z1)/2,x,y);
    const auto scale=windowScale();
    return {{"x",x/scale.x()},{"y",y/scale.y()},{"bodyId",body.bodyId},
        {"viewportLocal",true},{"units","logicalPixels"}};
}
void ReferenceView::replacePreview(const TopoDS_Shape& shape,const QString& replacedBody,bool neutral) {
    // Remove the previous presentation without exposing an intermediate empty
    // scene or the original body while the replacement is being installed.
    clearPreviewPresentation();if(context_.IsNull())return;
    if(models_.contains(replacedBody)) {
        previewReplacedBody_=replacedBody;context_->Erase(models_.value(replacedBody),false);
    }
    preview_=new AIS_Shape(shape);preview_->SetColor(neutral ? Quantity_Color(0.67,0.72,0.76,Quantity_TOC_RGB) : Quantity_Color(0.25,0.58,0.76,Quantity_TOC_RGB));
    preview_->SetOwnDeviationCoefficient(0.0001);
    preview_->SetOwnDeviationAngle(0.04);
    preview_->SetTransparency(neutral ? 0.0 : 0.55);
    preview_->Attributes()->SetFaceBoundaryDraw(true);
    if(neutral)preview_->Attributes()->SetFaceBoundaryAspect(new Prs3d_LineAspect(
        Quantity_Color(0.18,0.22,0.25,Quantity_TOC_sRGB),Aspect_TOL_SOLID,devicePixelRatioF()));
    context_->Display(preview_,AIS_Shaded,-1,false);context_->Deactivate(preview_);
    applyEditDisplay(false);
}
void ReferenceView::showPreview(const TopoDS_Shape& shape,const QString& replacedBody,bool neutral) {
    replacePreview(shape,replacedBody,neutral);
    if(!context_.IsNull())context_->UpdateCurrentViewer();
}
void ReferenceView::showExtrudePreview(const EvaluatedBody& body,const EvaluatedProfile& profile,double distanceMm,const QJsonArray& sourceProfileIds,const QString& units) {
    replacePreview(body.shape,body.bodyId,false);
    if(context_.IsNull())return;
    preview_->SetColor(Quantity_Color(0.16,0.47,0.86,Quantity_TOC_sRGB));
    preview_->SetTransparency(0.20);
    context_->Redisplay(preview_,false);
    previewProfile_=profile.profileId;previewProfileIds_=sourceProfileIds.isEmpty() ? QJsonArray{profile.profileId} : sourceProfileIds;
    applyEditDisplay(false);
    GProp_GProps properties;BRepGProp::SurfaceProperties(profile.face,properties);const auto start=properties.CentreOfMass();
    const auto frame=profile.summary.value("frame").toObject().isEmpty() ? SketchGeometry::frame(profile.plane) : profile.summary.value("frame").toObject();
    const auto normal=frame.value("normal").toArray();
    gp_Vec direction(normal[0].toDouble(),normal[1].toDouble(),normal[2].toDouble());direction*=distanceMm<0 ? -1 : 1;
    const auto end=start.Translated(direction*std::abs(distanceMm));
    const double displayDistance=units=="in" ? distanceMm/25.4 : distanceMm;
    const QString label=QString::number(displayDistance,'f',2)+" "+units;
    extrusionCueState_={{"sketchId",profile.sketchId},{"profileId",profile.profileId},{"profileIds",previewProfileIds_},{"distanceMm",distanceMm},
        {"units",units},{"label",label},{"plane",profile.plane},{"frame",frame},
        {"startMm",QJsonArray{start.X(),start.Y(),start.Z()}},{"endMm",QJsonArray{end.X(),end.Y(),end.Z()}}};
    refreshExtrusionCue();
    context_->UpdateCurrentViewer();
}
void ReferenceView::refreshExtrusionCue() {
    if(context_.IsNull() || view_.IsNull() || extrusionCueState_.isEmpty())return;
    for(const auto& cue:extrusionCue_)context_->Remove(cue,false);
    extrusionCue_.clear();
    const auto startArray=extrusionCueState_.value("startMm").toArray(),endArray=extrusionCueState_.value("endMm").toArray();
    if(startArray.size()!=3 || endArray.size()!=3)return;
    const gp_Pnt start(startArray[0].toDouble(),startArray[1].toDouble(),startArray[2].toDouble());
    const gp_Pnt end(endArray[0].toDouble(),endArray[1].toDouble(),endArray[2].toDouble());
    gp_Vec direction(start,end);const double length=direction.Magnitude();if(length<1e-9)return;
    direction.Normalize();
    const auto frame=extrusionCueState_.value("frame").toObject();const auto u=frame.value("uAxis").toArray();
    gp_Vec side(u[0].toDouble(),u[1].toDouble(),u[2].toDouble());
    int startX,startY,endX,endY,sideX,sideY;
    view_->Convert(start.X(),start.Y(),start.Z(),startX,startY);
    view_->Convert(end.X(),end.Y(),end.Z(),endX,endY);
    const auto sidePoint=end.Translated(side*100.0);view_->Convert(sidePoint.X(),sidePoint.Y(),sidePoint.Z(),sideX,sideY);
    const auto scale=windowScale();
    const double alongPixels=std::hypot((endX-startX)/scale.x(),(endY-startY)/scale.y())/length;
    const double sidePixels=std::hypot((sideX-endX)/scale.x(),(sideY-endY)/scale.y())/100.0;
    if(alongPixels<1e-5 || sidePixels<1e-5)return;
    const double head=std::min(length*0.36,14.0/alongPixels),wing=6.0/sidePixels;
    const Quantity_Color color(0.04,0.29,0.72,Quantity_TOC_sRGB);
    for(const auto& edge:std::vector<TopoDS_Edge>{BRepBuilderAPI_MakeEdge(start,end).Edge(),
        BRepBuilderAPI_MakeEdge(end,end.Translated(direction*(-head)+side*wing)).Edge(),
        BRepBuilderAPI_MakeEdge(end,end.Translated(direction*(-head)-side*wing)).Edge()}) {
        const auto cue=sketchPresentation(edge);cue->SetColor(color);cue->SetWidth(2.5*devicePixelRatioF());cue->SetZLayer(Graphic3d_ZLayerId_Topmost);
        context_->Display(cue,AIS_WireFrame,-1,false);context_->Deactivate(cue);extrusionCue_.push_back(cue);
    }
    Handle(AIS_TextLabel) label=new AIS_TextLabel;
    label->SetText(TCollection_ExtendedString(extrusionCueState_.value("label").toString().toUtf8().constData(),true));
    label->SetPosition(end.Translated(side*(21.0/sidePixels)));
    label->SetHeight(14);label->SetFont("Arial");label->SetHJustification(Graphic3d_HTA_LEFT);label->SetVJustification(Graphic3d_VTA_CENTER);
    label->SetZoomable(false);label->SetDisplayType(Aspect_TODT_SUBTITLE);label->SetColor(color);
    label->SetColorSubTitle(Quantity_Color(1.0,1.0,1.0,Quantity_TOC_sRGB));
    label->SetZLayer(Graphic3d_ZLayerId_Topmost);context_->Display(label,0,-1,false);context_->Deactivate(label);extrusionCue_.push_back(label);
    extrusionCueState_["arrowHeadPixels"]=head*alongPixels;extrusionCueState_["labelHeightPixels"]=14;
    extrusionCueState_["labelScreen"]=QJsonObject{{"x",endX/scale.x()+0.21*(sideX-endX)/scale.x()/sidePixels},{"y",endY/scale.y()+0.21*(sideY-endY)/scale.y()/sidePixels}};
}
void ReferenceView::clearPreviewPresentation() {
    if(!context_.IsNull()) {
        for(const auto& cue:extrusionCue_)context_->Remove(cue,false);
        if(!preview_.IsNull())context_->Remove(preview_,false);
    }
    preview_.Nullify();previewReplacedBody_.clear();previewProfile_.clear();previewProfileIds_={};extrusionCue_.clear();extrusionCueState_={};
}
void ReferenceView::clearPreview() {
    clearPreviewPresentation();
    applyEditDisplay();
}
QJsonObject ReferenceView::diagnostics() const {
    QJsonObject result{{"initialized", !view_.IsNull()}, {"error", error_}, {"hardwareAccelerated", false},
        {"qualification", "unverified"}, {"backend", "OCCT OpenGL"}};
    if (view_.IsNull()) return result;
    TColStd_IndexedDataMapOfStringString info;
    view_->DiagnosticInformation(info, Graphic3d_DiagnosticInfo_Basic);
    QJsonObject raw;
    for (int i=1; i<=info.Extent(); ++i) raw.insert(QString::fromUtf8(info.FindKey(i).ToCString()), QString::fromUtf8(info.FindFromIndex(i).ToCString()));
    const QString vendor = raw.value("GLvendor").toString();
    const QString renderer = raw.value("GLdevice").toString(raw.value("GLrenderer").toString());
    const QString version = raw.value("GLversion").toString();
    int nativeWidth = 0, nativeHeight = 0;
    view_->Window()->Size(nativeWidth, nativeHeight);
    result.insert("msaa",QJsonObject{{"requestedSamples",msaaRequested_},{"enabledSamples",msaaEnabled_},
        {"maximumSamples",msaaMaximum_},{"textureMultisampling",msaaTextures_}});
    result.insert("nativeWidth", nativeWidth); result.insert("nativeHeight", nativeHeight);
    result.insert("framebufferWidth", qRound(width()*devicePixelRatioF()));
    result.insert("framebufferHeight", qRound(height()*devicePixelRatioF()));
    const QString identity = (vendor + " " + renderer).toLower();
    bool software = false;
    for (const auto* token : {"llvmpipe", "softpipe", "software", "swrast", "swiftshader"}) software |= identity.contains(token);
    const bool knownGpu = identity.contains("apple") || identity.contains("nvidia") || identity.contains("amd") || identity.contains("ati ") || identity.contains("intel") || identity.contains("qualcomm");
    result.insert("vendor", vendor); result.insert("renderer", renderer); result.insert("version", version);
    result.insert("hardwareAccelerated", !software && knownGpu && !version.isEmpty());
    result.insert("qualification", software ? "software-rejected" : knownGpu && !version.isEmpty() ? "hardware-driver-identified" : "unverified");
    result.insert("details", raw);
    int displayedBodies=0;for(const auto& model:models_)if(context_->IsDisplayed(model))++displayedBodies;
    result.insert("displayedBodies",displayedBodies);
    int displayedSketches=sketchModels_.size();
    for(auto it=constructionSketchModels_.cbegin();it!=constructionSketchModels_.cend();++it)
        if(!sketchModels_.contains(it.key()))++displayedSketches;
    result.insert("displayedSketches",displayedSketches);
    result.insert("draftSketchVisible",!sketchDraft_.IsNull() || !sketchConstructionDraft_.IsNull());
    result.insert("constructionDraftVisible",!sketchConstructionDraft_.IsNull());
    result.insert("constructionDraftLineIds",constructionDraftLineIds_);
    QJsonArray projectedLineIds,projectedPointIds,selectedProjectedPointIds,projectedEndpoints;
    for(const auto& value:projectedLines_) {
        const auto line=value.toObject();projectedLineIds.append(line.value("id"));
        for(const bool end:{false,true}) {
            const auto id=line.value(end ? "endPointId" : "startPointId");
            if(!projectedPointIds.contains(id))projectedPointIds.append(id);
            if(decorationSelection_.value("pointIds").toArray().contains(id) && !selectedProjectedPointIds.contains(id))selectedProjectedPointIds.append(id);
            projectedEndpoints.append(QJsonObject{{"pointId",id},{"projectionId",line.value("id")},{"source",line.value("source")},
                {"endpoint",end ? "end" : "start"},{"uMm",line.value(end ? "u2Mm" : "u1Mm")},{"vMm",line.value(end ? "v2Mm" : "v1Mm")}});
        }
    }
    result.insert("projectedDraftVisible",!projectedDraft_.IsNull());
    result.insert("projectedLineIds",projectedLineIds);
    result.insert("projectedLines",projectedLines_);
    result.insert("projectedPointIds",projectedPointIds);
    result.insert("selectedProjectedPointIds",selectedProjectedPointIds);
    result.insert("projectedEndpoints",projectedEndpoints);
    result.insert("availableSupportPoints",supportPoints_);
    QJsonArray selectedSupportPointIds;
    for(const auto& value:supportPoints_)if(decorationSelection_.value("pointIds").toArray().contains(value.toObject().value("id")))
        selectedSupportPointIds.append(value.toObject().value("id"));
    result.insert("selectedSupportPointIds",selectedSupportPointIds);
    result.insert("constructionSketches",constructionSketchModels_.size());
    result.insert("rightMousePressCount",rightMousePressCount_);
    result.insert("contextMenuEventCount",contextMenuEventCount_);
    result.insert("contextMenuShownCount",contextMenuShownCount_);
    result.insert("lastContextMenu",lastContextMenu_);
    result.insert("contextMenuVisible",qobject_cast<QMenu*>(QApplication::activePopupWidget())!=nullptr);
    result.insert("draftSketchId",draftSketchId_);
    result.insert("sketchPlane",sketchPlane_);
    result.insert("sketchGrid",QJsonObject{{"adaptive",adaptiveGrid_},{"centerUMm",gridCenterU_},
        {"centerVMm",gridCenterV_},{"stepMm",gridStep_},{"graphicHalfWidthMm",gridWidth_},
        {"graphicHalfHeightMm",gridHeight_}});
    result.insert("cursorPreviewVisible",!sketchCursor_.IsNull());
    result.insert("sketchDecorations",static_cast<int>(sketchDecorations_.size()));
    result.insert("sketchAnnotations",annotationLayouts_);
    auto snapCue=sketchSnapState_;snapCue["visible"]=!sketchSnapCue_.IsNull();
    snapCue["marker"]=snapCue.value("status")=="ambiguous" ? "cross" : snapCue.value("status")=="acquired" ?
        (snapCue.value("target").toObject().value("kind")=="filletCorner" ? "fillet-corner-square" :
         snapCue.value("target").toObject().value("kind")=="tangent" ? "tangent-square" :
         snapCue.value("target").toObject().value("kind")=="midpoint" ? "midpoint-triangle" :
         snapCue.value("supportPoint").toBool() ? "support-center-cross" :
         snapCue.value("target").toObject().value("kind")=="origin" ? "origin-diamond" : "endpoint-diamond") : "none";
    result.insert("sketchSnapCue",snapCue);
    result.insert("sketchConstraintGlyphs",constraintGlyphs_);
    result.insert("sketchConstraintVisibility",QJsonObject{{"visible",!decorationFeature_.isEmpty()},{"editable",false}});
    QJsonArray profiles;for(const auto& profile:draftProfiles_) {
        auto summary=profile.summary;TopLoc_Location location;const auto triangles=BRep_Tool::Triangulation(profile.face,location);
        summary["displayTriangleCount"]=triangles.IsNull() ? 0 : triangles->NbTriangles();profiles.append(summary);
    }
    result.insert("sketchProfiles",profiles);
    QJsonArray finishedProfiles;for(const auto& sketch:finishedSketches_)for(const auto& profile:sketch.profiles) {
        auto summary=profile.summary;TopLoc_Location location;const auto triangles=BRep_Tool::Triangulation(profile.face,location);
        summary["displayTriangleCount"]=triangles.IsNull() ? 0 : triangles->NbTriangles();
        summary["visible"]=(sketch.visible || sketch.featureId==editSourceSketch_) && sketch.featureId!=draftSketchId_;
        summary["selected"]=selectedProfileIds_.contains(profile.profileId) && sketch.featureId==selectedProfileSketch_;finishedProfiles.append(summary);
    }
    result.insert("finishedProfiles",finishedProfiles);
    result.insert("editDisplay",QJsonObject{{"sourceSketchId",editSourceSketch_},{"hiddenBodyIds",hiddenBodyIds_}});
    result.insert("combineInputs",QJsonObject{{"targetBodyId",combineTargetId_},{"toolBodyIds",combineToolIds_},
        {"targetColor","blue"},{"toolColor","orange"},{"hiddenSelectedInputsTemporarilyRevealed",true}});
    result.insert("extrusionCue",extrusionCueState_);
    result.insert("selectedSupportFace",selectedSupportFace_);
    result.insert("supportFaceHighlightVisible",!selectedSupportFaceOverlay_.IsNull());
    result.insert("hoveredSupportFace",hoveredSupportFace_);
    result.insert("previewVisible",!preview_.IsNull());
    result.insert("previewReplacedBodyId",previewReplacedBody_);
    result.insert("previewProfileId",previewProfile_);
    result.insert("previewProfileIds",previewProfileIds_);
    result.insert("bodyStageKey",bodyStageKey_);
    QJsonArray displayIds;for(auto it=models_.begin();it!=models_.end();++it)if(context_->IsDisplayed(it.value()))displayIds.append(it.key());result.insert("displayedBodyIds",displayIds);
    result.insert("documentRevision",static_cast<qint64>(modelRevision_));
    const auto camera=view_->Camera();
    auto vector=[](const auto& value) {return QJsonArray{value.X(),value.Y(),value.Z()};};
    result.insert("camera",QJsonObject{{"eye",vector(camera->Eye())},{"center",vector(camera->Center())},
        {"direction",vector(camera->Direction())},{"up",vector(camera->Up())},
        {"scale",camera->Scale()},{"aspect",camera->Aspect()}});
    return result;
}
QImage ReferenceView::framebuffer() {
    if (view_.IsNull()) return {};
    view_->MustBeResized();
    sync();
    Image_PixMap pixels;
    const int w = qRound(width()*devicePixelRatioF()), h = qRound(height()*devicePixelRatioF());
    const bool captured = view_->ToPixMap(pixels, w, h, Graphic3d_BT_RGB);
    view_->Redraw(); // Restore native drawable after the offscreen dump.
    if (!captured) return {};
    QImage image(w, h, QImage::Format_RGB32);
    for (int y=0; y<h; ++y) {
        auto* row = reinterpret_cast<QRgb*>(image.scanLine(y));
        for (int x=0; x<w; ++x) {
            const auto color = pixels.PixelColor(x,y).GetRGB();
            row[x] = qRgb(qRound(color.Red()*255), qRound(color.Green()*255), qRound(color.Blue()*255));
        }
    }
    image.setDevicePixelRatio(devicePixelRatioF());
    return image;
}
void ReferenceView::mousePressEvent(QMouseEvent* e) {
    if(e->button()==Qt::RightButton)++rightMousePressCount_;
    if (e->button() == Qt::MiddleButton || (e->button()==Qt::LeftButton && e->modifiers().testFlag(Qt::AltModifier))) {
        panButton_=e->button();last_=e->position();setCursor(Qt::ClosedHandCursor);e->accept();
    }
    else if(e->button()==Qt::LeftButton){
        if(!sketchPlane_.isEmpty() && decorationSelection_.value("tool")!="line" && decorationSelection_.value("tool")!="rectangle" && decorationSelection_.value("tool")!="circle" && decorationSelection_.value("tool")!="centerArc" && decorationSelection_.value("tool")!="threePointArc" && decorationPending_.value("phase")!="placing") {
            const auto picked=pickAnnotation(e->position().x(),e->position().y());
            if(!picked.isEmpty()) {
                draggedAnnotation_=picked.value("dimensionId").toString();annotationGestureId_=QString::number(++nextAnnotationGesture_);annotationPress_=e->position();annotationDragged_=false;
                dispatch("sketch.annotationSelect",{{"dimensionId",draggedAnnotation_}});e->accept();return;
            }
        }
        QJsonObject args{{"x",e->position().x()},{"y",e->position().y()}};
        if(!sketchPlane_.isEmpty())args["additive"]=e->modifiers().testFlag(Qt::ShiftModifier);
        else {
            args["additive"]=e->modifiers().testFlag(Qt::ShiftModifier);
            args["toggle"]=e->modifiers().testFlag(Qt::ControlModifier) || e->modifiers().testFlag(Qt::MetaModifier);
        }
        dispatch(sketchPlane_.isEmpty() ? "selection.pick" : "sketch.placePoint",args);e->accept();
    }
    else QWidget::mousePressEvent(e);
}
void ReferenceView::mouseMoveEvent(QMouseEvent* e) {
    if (panButton_!=Qt::NoButton && e->buttons().testFlag(panButton_)) {
        const auto delta = e->position() - last_; last_ = e->position();
        dispatch("view.pan", {{"x",delta.x()},{"y",delta.y()}}); e->accept();
    } else if(!draggedAnnotation_.isEmpty() && e->buttons().testFlag(Qt::LeftButton)) {
        if(QLineF(annotationPress_,e->position()).length()>3)annotationDragged_=true;
        if(annotationDragged_) {const auto p=sketchPoint(std::clamp(e->position().x(),0.0,double(width()-1)),std::clamp(e->position().y(),0.0,double(height()-1)),sketchPlane_);dispatch("sketch.annotationMove",{{"dimensionId",draggedAnnotation_},{"uMm",p.value("uMm")},{"vMm",p.value("vMm")},{"gestureId",annotationGestureId_}});}
        e->accept();
    } else if(!sketchPlane_.isEmpty()) {dispatch("sketch.pointerMove",{{"x",e->position().x()},{"y",e->position().y()}});e->accept();}
    else {
        hoverSupportFace(pickSupportFace(e->position().x(),e->position().y(),documentBodies_));
        QWidget::mouseMoveEvent(e);
    }
}
void ReferenceView::mouseReleaseEvent(QMouseEvent* e) {
    if(e->button()==Qt::LeftButton && !draggedAnnotation_.isEmpty()){
        if(annotationDragged_)dispatch("sketch.annotationEndGesture",{{"dimensionId",draggedAnnotation_},{"gestureId",annotationGestureId_}});
        draggedAnnotation_.clear();annotationGestureId_.clear();annotationDragged_=false;e->accept();return;
    }
    if(e->button()==panButton_) {panButton_=Qt::NoButton;unsetCursor();e->accept();}
    else QWidget::mouseReleaseEvent(e);
}
void ReferenceView::mouseDoubleClickEvent(QMouseEvent* e) {
    if(e->button()==Qt::LeftButton && !sketchPlane_.isEmpty()) {
        const auto picked=pickAnnotation(e->position().x(),e->position().y());
        if(!picked.isEmpty()){draggedAnnotation_.clear();dispatch("sketch.dimensionEdit",{{"dimensionId",picked.value("dimensionId")}});e->accept();return;}
    }
    QWidget::mouseDoubleClickEvent(e);
}
void ReferenceView::contextMenuEvent(QContextMenuEvent* e) {
    ++contextMenuEventCount_;
    const bool sketchActive=!sketchPlane_.isEmpty();
    const bool selectTool=decorationSelection_.value("tool")=="select";
    const int selectedEntities=decorationSelection_["entityIds"].toArray().size();
    lastContextMenu_=QJsonObject{{"reason",static_cast<int>(e->reason())},{"sketchActive",sketchActive},
        {"selectTool",selectTool},{"selectedEntities",selectedEntities},
        {"x",e->pos().x()},{"y",e->pos().y()}};
    if(sketchActive && selectTool && selectedEntities>0) {
        ++contextMenuShownCount_;
        QMenu menu(this);auto* toggle=menu.addAction("Normal/Construction  X");
        lastContextMenu_["phase"]="open";
        const auto selected=menu.exec(e->globalPos());
        lastContextMenu_["phase"]="closed";
        lastContextMenu_["result"]=selected==toggle ? "selected" : "cancelled";
        if(selected==toggle)dispatch("sketch.constructionToggle",{});
        e->accept();return;
    }
    const auto menuFace=sketchActive ? QJsonObject{} : pickSupportFace(e->pos().x(),e->pos().y(),documentBodies_);
    if(!sketchActive && (!menuFace.isEmpty() || !selectedSupportFace_.isEmpty())) {
        QMenu menu(this);auto* create=menu.addAction("Create Sketch");
        if(menu.exec(e->globalPos())==create)dispatch("sketch.create",{{"support",menuFace.isEmpty() ? selectedSupportFace_ : menuFace}});
        e->accept();return;
    }
    QWidget::contextMenuEvent(e);
}
void ReferenceView::wheelEvent(QWheelEvent* e) {
    dispatch("view.zoom", {{"factor",std::clamp(std::pow(1.0015,e->angleDelta().y()),0.1,10.0)}}); e->accept();
}
