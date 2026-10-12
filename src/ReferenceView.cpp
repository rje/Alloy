#include "ReferenceView.h"
#include "SketchGeometry.h"
#include <QMouseEvent>
#include <QWheelEvent>
#include <QTimer>
#include <QJsonArray>
#include <QFontMetricsF>
#include <QFont>
#include <QLineF>
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
#include <gp_Pln.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRep_Tool.hxx>
#include <Poly_Triangulation.hxx>

namespace {
gp_Pnt sketchWorld(const QString& plane,double u,double v) {
    const auto frame=SketchGeometry::frame(plane);
    const auto ua=frame["uAxis"].toArray(),va=frame["vAxis"].toArray();
    return gp_Pnt(u*ua[0].toDouble()+v*va[0].toDouble(),u*ua[1].toDouble()+v*va[1].toDouble(),u*ua[2].toDouble()+v*va[2].toDouble());
}
Handle(AIS_Shape) sketchPresentation(const TopoDS_Shape& shape) {
    Handle(AIS_Shape) presentation=new AIS_Shape(shape);
    presentation->SetColor(Quantity_Color(0.10,0.34,0.84,Quantity_TOC_RGB));
    presentation->SetWidth(2);
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
    const auto frame=SketchGeometry::frame(sketchPlane_.isEmpty() ? "XY" : sketchPlane_);
    const auto normal=frame["normal"].toArray(),u=frame["uAxis"].toArray();
    viewer_->SetPrivilegedPlane(gp_Ax3(gp_Pnt(),gp_Dir(normal[0].toDouble(),normal[1].toDouble(),normal[2].toDouble()),gp_Dir(u[0].toDouble(),u[1].toDouble(),u[2].toDouble())));
    if (grid) viewer_->ActivateGrid(Aspect_GT_Rectangular, Aspect_GDM_Lines);
    else viewer_->DeactivateGrid();
    if (origin) context_->Display(axes_, false); else context_->Erase(axes_, false);
    context_->Deactivate(axes_);
    if(!decorationFeature_.isEmpty())decorateSketch(decorationFeature_,decorationSelection_,decorationDiagnostics_,decorationUnits_,decorationPending_);
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
void ReferenceView::setSketches(const std::vector<EvaluatedSketch>& sketches,quint64 revision) {
    if(context_.IsNull() || sketchRevision_==revision)return;
    for(const auto& model:sketchModels_)context_->Remove(model,false);
    sketchModels_.clear();
    for(const auto& sketch:sketches) {
        if(!sketch.visible || sketch.shape.IsNull())continue;
        const auto model=sketchPresentation(sketch.shape);
        if(sketch.summary["solver"].toObject()["dof"].toInt(-1)==0)model->SetColor(Quantity_Color(0.12,0.14,0.16,Quantity_TOC_RGB));
        context_->Display(model,AIS_WireFrame,-1,false);context_->Deactivate(model);
        if(sketch.featureId==draftSketchId_)context_->Erase(model,false);
        sketchModels_[sketch.featureId]=model;
    }
    sketchRevision_=revision;context_->UpdateCurrentViewer();
}
void ReferenceView::showSketchDraft(const EvaluatedSketch& sketch) {
    if(context_.IsNull())return;
    for(const auto& model:profileModels_)context_->Remove(model,false);profileModels_.clear();draftProfiles_=sketch.profiles;
    for(const auto& profile:sketch.profiles) {
        BRepMesh_IncrementalMesh mesh(profile.face,0.1,false,0.5,false);
        Handle(AIS_Shape) fill=new AIS_Shape(profile.face);fill->SetDisplayMode(AIS_Shaded);fill->Attributes()->SetAutoTriangulation(false);
        fill->Attributes()->SetShadingModel(Graphic3d_TypeOfShadingModel_Unlit,true);
        fill->SetColor(Quantity_Color(0.39,0.68,0.91,Quantity_TOC_sRGB));fill->SetTransparency(0.58);
        context_->Display(fill,AIS_Shaded,-1,false);context_->Deactivate(fill);profileModels_[profile.profileId]=fill;
    }
    if(!sketchDraft_.IsNull())context_->Remove(sketchDraft_,false);
    if(draftSketchId_!=sketch.featureId && sketchModels_.contains(draftSketchId_))context_->Display(sketchModels_.value(draftSketchId_),false);
    draftSketchId_=sketch.featureId;sketchDraft_.Nullify();
    if(sketchModels_.contains(draftSketchId_))context_->Erase(sketchModels_.value(draftSketchId_),false);
    if(!sketch.shape.IsNull()) {
        sketchDraft_=sketchPresentation(sketch.shape);
        if(sketch.summary["solver"].toObject()["dof"].toInt(-1)==0)sketchDraft_->SetColor(Quantity_Color(0.12,0.14,0.16,Quantity_TOC_sRGB));
        context_->Display(sketchDraft_,AIS_WireFrame,-1,false);context_->Deactivate(sketchDraft_);
    }
    context_->UpdateCurrentViewer();
}
void ReferenceView::clearSketchDraft() {
    clearSketchDecorations();
    decorationFeature_={};decorationPending_={};annotationLayouts_={};draggedAnnotation_.clear();
    if(!context_.IsNull()) {
        for(const auto& model:profileModels_)context_->Remove(model,false);
        if(!sketchDraft_.IsNull())context_->Remove(sketchDraft_,false);
        if(sketchModels_.contains(draftSketchId_))context_->Display(sketchModels_.value(draftSketchId_),false);
        context_->UpdateCurrentViewer();
    }
    profileModels_.clear();draftProfiles_.clear();
    sketchDraft_.Nullify();draftSketchId_.clear();clearSketchCursor();
}
QJsonObject ReferenceView::pickSketch(const QJsonObject& feature,double x,double y) const {
    if(view_.IsNull() || x<0 || y<0 || x>=width() || y>=height())return {};
    const auto parameters=feature["parameters"].toObject();const auto plane=parameters["plane"].toString();
    QMap<QString,QPointF> points;
    double best=8;QString picked;
    for(const auto& entry:parameters["points"].toArray()) {
        const auto point=entry.toObject();const auto projection=projectSketchPoint(point["uMm"].toDouble(),point["vMm"].toDouble(),plane);
        const QPointF p(projection["x"].toDouble(),projection["y"].toDouble());points[point["id"].toString()]=p;
        const double distance=std::hypot(p.x()-x,p.y()-y);
        if(distance<=best){best=distance;picked=point["id"].toString();}
    }
    if(!picked.isEmpty())return {{"kind","point"},{"pointId",picked},{"distancePixels",best}};
    best=6;
    for(const auto& entry:parameters["entities"].toArray()) {
        const auto entity=entry.toObject();const auto start=points.value(entity["startPointId"].toString()),end=points.value(entity["endPointId"].toString());
        const auto delta=end-start;const double length2=delta.x()*delta.x()+delta.y()*delta.y();if(length2<1e-12)continue;
        const double t=std::clamp(((x-start.x())*delta.x()+(y-start.y())*delta.y())/length2,0.0,1.0);
        const auto nearest=start+delta*t;const double distance=std::hypot(nearest.x()-x,nearest.y()-y);
        if(distance<=best){best=distance;picked=entity["id"].toString();}
    }
    if(!picked.isEmpty())return {{"kind","entity"},{"entityId",picked},{"distancePixels",best}};
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
}
QJsonObject ReferenceView::dimensionLayout(const QJsonObject& feature,const QJsonObject& dimension,double valueMm,const QString& units) const {
    if(view_.IsNull())return {};
    const auto parameters=feature.value("parameters").toObject();const auto plane=parameters.value("plane").toString();
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
    for(const auto& entry:parameters["entities"].toArray()) {
        const auto entity=entry.toObject();if(!selection["entityIds"].toArray().contains(entity["id"]))continue;
        const auto a=points.value(entity["startPointId"].toString()),b=points.value(entity["endPointId"].toString());
        const auto edge=BRepBuilderAPI_MakeEdge(sketchWorld(plane,a["uMm"].toDouble(),a["vMm"].toDouble()),sketchWorld(plane,b["uMm"].toDouble(),b["vMm"].toDouble())).Edge();
        const auto highlight=sketchPresentation(edge);highlight->SetColor(Quantity_Color(0.98,0.43,0.08,Quantity_TOC_RGB));highlight->SetWidth(4);highlight->SetZLayer(Graphic3d_ZLayerId_Topmost);
        context_->Display(highlight,AIS_WireFrame,-1,false);context_->Deactivate(highlight);sketchDecorations_.push_back(highlight);
    }
    QMap<QString,QJsonObject> measurements;
    for(const auto& entry:diagnostics["dimensionValues"].toArray()){const auto value=entry.toObject();measurements[value["id"].toString()]=value;}
    auto dimensions=parameters.value("dimensions").toArray();
    if(!pending.isEmpty() && !pending.contains("dimensionId")){auto preview=pending;preview["id"]="pending";dimensions.append(preview);}
    for(const auto& entry:dimensions) {
        const auto dimension=entry.toObject();const auto measured=measurements.value(dimension.value("id").toString());
        const double value=dimension.value("id")=="pending" ? pending.value("measuredValueMm").toDouble() : measured.value("valueMm").toDouble(dimension.value("valueMm").toDouble());
        const auto layout=dimensionLayout(feature,dimension,value,units);if(layout.isEmpty())continue;annotationLayouts_.append(layout);
        const bool selected=selection.value("dimensionId")==dimension.value("id");
        const Quantity_Color color=selected || dimension.value("id")=="pending" ? Quantity_Color(0.10,0.34,0.84,Quantity_TOC_RGB) : Quantity_Color(0.10,0.11,0.12,Quantity_TOC_sRGB);
        const auto line=layout.value("line").toArray();auto world=[&](const QJsonValue& v){const auto p=v.toObject();return sketchWorld(plane,p.value("uMm").toDouble(),p.value("vMm").toDouble());};
        const auto frame=SketchGeometry::frame(plane);const auto n=frame.value("normal").toArray(),u=frame.value("uAxis").toArray();
        Handle(PrsDim_LengthDimension) graphic=new PrsDim_LengthDimension(world(line[0]),world(line[1]),gp_Pln(gp_Ax3(gp_Pnt(),gp_Dir(n[0].toDouble(),n[1].toDouble(),n[2].toDouble()),gp_Dir(u[0].toDouble(),u[1].toDouble(),u[2].toDouble()))));
        Handle(Prs3d_DimensionAspect) aspect=new Prs3d_DimensionAspect;aspect->SetCommonColor(color);aspect->MakeText3d(false);aspect->MakeArrows3d(false);aspect->MakeUnitsDisplayed(false);
        aspect->LineAspect()->SetWidth(devicePixelRatioF());
        const double size=layout.value("mmPerPixel").toDouble();aspect->ArrowAspect()->SetLength(7*size);aspect->SetArrowTailSize(8*size);aspect->SetExtensionSize(8*size);
        graphic->SetDimensionAspect(aspect);graphic->SetCustomValue(TCollection_ExtendedString(""));graphic->SetFlyout(0);graphic->SetZLayer(Graphic3d_ZLayerId_Topmost);
        context_->Display(graphic,0,-1,false);context_->Deactivate(graphic);sketchDecorations_.push_back(graphic);
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
void ReferenceView::setSketchPlane(const QString& plane) { sketchPlane_=plane; }
void ReferenceView::lookAtSketch(const QString& plane) {
    sketchPlane_=plane;orientation=plane=="XY" ? "top" : plane=="XZ" ? "front" : "right";
    sceneCenter_=gp_Pnt();sceneScale_=100;pan={};zoom=1;sync();
}
QJsonObject ReferenceView::sketchPoint(double x,double y,const QString& plane) const {
    if(view_.IsNull() || x<0 || y<0 || x>=width() || y>=height())throw std::invalid_argument("Point is outside the initialized viewport");
    const auto scale=windowScale();double px,py,pz,dx,dy,dz;
    view_->ConvertWithProj(qRound(x*scale.x()),qRound(y*scale.y()),px,py,pz,dx,dy,dz);
    const auto frame=SketchGeometry::frame(plane);const auto normal=frame["normal"].toArray();
    const gp_Vec n(normal[0].toDouble(),normal[1].toDouble(),normal[2].toDouble()),ray(dx,dy,dz);
    const double denominator=n.Dot(ray);
    if(std::abs(denominator)<1e-9)throw std::invalid_argument("Look At the sketch plane before drawing");
    const auto world=gp_Vec(px,py,pz)-ray*(n.Dot(gp_Vec(px,py,pz))/denominator);
    const auto u=frame["uAxis"].toArray(),v=frame["vAxis"].toArray();
    return {{"uMm",world.Dot(gp_Vec(u[0].toDouble(),u[1].toDouble(),u[2].toDouble()))},
        {"vMm",world.Dot(gp_Vec(v[0].toDouble(),v[1].toDouble(),v[2].toDouble()))}};
}
QJsonObject ReferenceView::projectSketchPoint(double uMm,double vMm,const QString& plane) const {
    if(view_.IsNull())return {};
    const auto world=sketchWorld(plane,uMm,vMm);int x,y;view_->Convert(world.X(),world.Y(),world.Z(),x,y);
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
void ReferenceView::clearSketchCursor() {
    if(!context_.IsNull() && !sketchCursor_.IsNull())context_->Remove(sketchCursor_,false);
    sketchCursor_.Nullify();
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
    result.insert("displayedSketches",sketchModels_.size());
    result.insert("draftSketchVisible",!sketchDraft_.IsNull());
    result.insert("draftSketchId",draftSketchId_);
    result.insert("sketchPlane",sketchPlane_);
    result.insert("cursorPreviewVisible",!sketchCursor_.IsNull());
    result.insert("sketchDecorations",static_cast<int>(sketchDecorations_.size()));
    result.insert("sketchAnnotations",annotationLayouts_);
    QJsonArray profiles;for(const auto& profile:draftProfiles_) {
        auto summary=profile.summary;TopLoc_Location location;const auto triangles=BRep_Tool::Triangulation(profile.face,location);
        summary["displayTriangleCount"]=triangles.IsNull() ? 0 : triangles->NbTriangles();profiles.append(summary);
    }
    result.insert("sketchProfiles",profiles);
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
    else if(e->button()==Qt::LeftButton){
        if(!sketchPlane_.isEmpty() && decorationSelection_.value("tool")!="line" && decorationSelection_.value("tool")!="rectangle" && decorationPending_.value("phase")!="placing") {
            const auto picked=pickAnnotation(e->position().x(),e->position().y());
            if(!picked.isEmpty()) {
                draggedAnnotation_=picked.value("dimensionId").toString();annotationPress_=e->position();annotationDragged_=false;
                dispatch("sketch.annotationSelect",{{"dimensionId",draggedAnnotation_}});e->accept();return;
            }
        }
        QJsonObject args{{"x",e->position().x()},{"y",e->position().y()}};
        if(!sketchPlane_.isEmpty())args["additive"]=e->modifiers().testFlag(Qt::ShiftModifier);
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
        if(annotationDragged_) {const auto p=sketchPoint(std::clamp(e->position().x(),0.0,double(width()-1)),std::clamp(e->position().y(),0.0,double(height()-1)),sketchPlane_);dispatch("sketch.annotationMove",{{"dimensionId",draggedAnnotation_},{"uMm",p.value("uMm")},{"vMm",p.value("vMm")}});}
        e->accept();
    } else if(!sketchPlane_.isEmpty()) {dispatch("sketch.pointerMove",{{"x",e->position().x()},{"y",e->position().y()}});e->accept();}
    else QWidget::mouseMoveEvent(e);
}
void ReferenceView::mouseReleaseEvent(QMouseEvent* e) {
    if(e->button()==Qt::LeftButton && !draggedAnnotation_.isEmpty()){draggedAnnotation_.clear();annotationDragged_=false;e->accept();return;}
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
void ReferenceView::wheelEvent(QWheelEvent* e) {
    dispatch("view.zoom", {{"factor",std::clamp(std::pow(1.0015,e->angleDelta().y()),0.1,10.0)}}); e->accept();
}
