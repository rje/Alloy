#include "ReferenceView.h"
#include <QMouseEvent>
#include <QWheelEvent>
#include <QTimer>
#include <QJsonArray>
#include <Aspect_DisplayConnection.hxx>
#include <Aspect_Grid.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <OpenGl_Caps.hxx>
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
#include <cmath>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <Prs3d_Drawer.hxx>
#include <Prs3d_LineAspect.hxx>

ReferenceView::ReferenceView(QWidget* parent) : QWidget(parent) {
    setMinimumSize(400, 300);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_NativeWindow);
    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_NoSystemBackground);
}
ReferenceView::~ReferenceView() {
    if (!view_.IsNull()) view_->Remove();
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
        viewer_->SetDefaultLights(); viewer_->SetLightOn();
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
    view_->SetProj(orientation == "top" ? V3d_Zpos : orientation == "front" ? V3d_Yneg : orientation == "right" ? V3d_Xpos : V3d_XposYnegZpos);
    // Move Eye/Center together: SetCenter alone changes the view direction,
    // while SetProj preserves the origin's screen offset rather than Center.
    const auto camera=view_->Camera();
    camera->MoveEyeTo(gp_Pnt(sceneCenter_.XYZ()-camera->Direction().XYZ()*camera->Distance()));
    // Fit uses the smaller viewport dimension; V3d::SetSize treats its input
    // as horizontal on a wide window and can crop a tall model vertically.
    camera->SetScale(sceneScale_ / (zoom * std::min(camera->Aspect(), 1.0)));
    const auto scale=windowScale();
    view_->Pan(qRound(pan.x()*scale.x()),-qRound(pan.y()*scale.y()));
    if (grid) viewer_->ActivateGrid(Aspect_GT_Rectangular, Aspect_GDM_Lines);
    else viewer_->DeactivateGrid();
    if (origin) context_->Display(axes_, false); else context_->Erase(axes_, false);
    context_->Deactivate(axes_);
    view_->Redraw();
}
void ReferenceView::setBodies(const std::vector<EvaluatedBody>& bodies,quint64 revision) {
    if(context_.IsNull() || modelRevision_==revision)return;
    for(const auto& model:models_)context_->Remove(model,false);
    models_.clear();
    for(const auto& body:bodies) {
        if(!body.visible)continue;
        Handle(AIS_Shape) model=new AIS_Shape(body.shape);
        model->SetColor(Quantity_Color(0.67,0.72,0.76,Quantity_TOC_RGB));
        model->Attributes()->SetFaceBoundaryDraw(true);
        model->Attributes()->SetFaceBoundaryAspect(new Prs3d_LineAspect(Quantity_Color(0.20,0.25,0.29,Quantity_TOC_RGB),Aspect_TOL_SOLID,1));
        context_->Display(model,AIS_Shaded,0,false);models_[body.bodyId]=model;
    }
    modelRevision_=revision;context_->UpdateCurrentViewer();
}
void ReferenceView::fit(const std::vector<EvaluatedBody>& bodies) {
    Bnd_Box bounds;
    for(const auto& body:bodies)if(body.visible)BRepBndLib::AddOptimal(body.shape,bounds,false,false);
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
void ReferenceView::showPreview(const TopoDS_Shape& shape,const QString& replacedBody) {
    clearPreview();if(context_.IsNull())return;
    if(models_.contains(replacedBody)) {
        previewReplacedBody_=replacedBody;context_->Erase(models_.value(replacedBody),false);
    }
    preview_=new AIS_Shape(shape);preview_->SetColor(Quantity_Color(0.25,0.58,0.76,Quantity_TOC_RGB));
    preview_->SetTransparency(0.55);context_->Display(preview_,AIS_Shaded,-1,false);context_->Deactivate(preview_);
    context_->UpdateCurrentViewer();
}
void ReferenceView::clearPreview() {
    if(!context_.IsNull()) {
        if(!preview_.IsNull())context_->Remove(preview_,false);
        if(models_.contains(previewReplacedBody_))context_->Display(models_.value(previewReplacedBody_),false);
        if(!preview_.IsNull() || !previewReplacedBody_.isEmpty())context_->UpdateCurrentViewer();
    }
    preview_.Nullify();
    previewReplacedBody_.clear();
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
    result.insert("displayedBodies",models_.size());
    result.insert("previewVisible",!preview_.IsNull());
    result.insert("previewReplacedBodyId",previewReplacedBody_);
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
    if (e->button() == Qt::MiddleButton || (e->button()==Qt::LeftButton && e->modifiers().testFlag(Qt::AltModifier))) {
        panButton_=e->button();last_=e->position();setCursor(Qt::ClosedHandCursor);e->accept();
    }
    else if(e->button()==Qt::LeftButton){dispatch("selection.pick",{{"x",e->position().x()},{"y",e->position().y()}});e->accept();}
    else QWidget::mousePressEvent(e);
}
void ReferenceView::mouseMoveEvent(QMouseEvent* e) {
    if (panButton_!=Qt::NoButton && e->buttons().testFlag(panButton_)) {
        const auto delta = e->position() - last_; last_ = e->position();
        dispatch("view.pan", {{"x",delta.x()},{"y",delta.y()}}); e->accept();
    } else QWidget::mouseMoveEvent(e);
}
void ReferenceView::mouseReleaseEvent(QMouseEvent* e) {
    if(e->button()==panButton_) {panButton_=Qt::NoButton;unsetCursor();e->accept();}
    else QWidget::mouseReleaseEvent(e);
}
void ReferenceView::wheelEvent(QWheelEvent* e) {
    dispatch("view.zoom", {{"factor",std::clamp(std::pow(1.0015,e->angleDelta().y()),0.1,10.0)}}); e->accept();
}
