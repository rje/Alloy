#include <AIS_ColoredShape.hxx>
#include <Graphic3d_ArrayOfTriangles.hxx>
#include <Graphic3d_AspectFillArea3d.hxx>
#include <Graphic3d_Group.hxx>
#include <Prs3d_Presentation.hxx>
#include "ZebraPresentation.h"
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakeCone.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <BRepPrimAPI_MakeTorus.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRep_Builder.hxx>
#include <TopoDS_Compound.hxx>
#include "ReferenceView.h"
#include "CanvasMarkingMenu.h"
#include <BRepAdaptor_Surface.hxx>
#include "SketchGeometry.h"
#include "SketchSpline.h"
#include "GeometryKernel.h"
#include "JointOrigin.h"
#include <QMouseEvent>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QInputDevice>
#include <QNativeGestureEvent>
#include <QContextMenuEvent>
#include <QMenu>
#include <QApplication>
#include <QCursor>
#include <QTimer>
#include <QElapsedTimer>
#include <QDebug>
#include <QJsonArray>
#include <QFontMetricsF>
#include <QFont>
#include <QLineF>
#include <QRubberBand>
#include <QGridLayout>
#include <QToolButton>
#include <Aspect_DisplayConnection.hxx>
#include <Aspect_Grid.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <OpenGl_Caps.hxx>
#include <OpenGl_Context.hxx>
#include <V3d_DirectionalLight.hxx>
#include <V3d_AmbientLight.hxx>
#include <Geom_Axis2Placement.hxx>
#include <Graphic3d_DiagnosticInfo.hxx>
#include <Graphic3d_TransformPers.hxx>
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
#include <StdPrs_ToolTriangulatedShape.hxx>
#include <QJsonDocument>
#include <TopoDS.hxx>
#include <Prs3d_LineAspect.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <TopoDS_Wire.hxx>
#include <gp_Ax3.hxx>
#include <gp_Vec.hxx>
#include <AIS_Point.hxx>
#include <QLabel>
#include <gp_Trsf.hxx>
namespace {
class ExportFacetPresentation final:public AIS_InteractiveObject {
public:
    explicit ExportFacetPresentation(const std::vector<gp_Pnt>& facets):facets_(facets){}
private:
    std::vector<gp_Pnt> facets_;
    void Compute(const Handle(PrsMgr_PresentationManager)&,const Handle(Prs3d_Presentation)& presentation,const Standard_Integer) override {
        Handle(Graphic3d_ArrayOfTriangles) triangles=new Graphic3d_ArrayOfTriangles(int(facets_.size()));
        for(const auto& point:facets_)triangles->AddVertex(point);
        Handle(Graphic3d_AspectFillArea3d) aspect=new Graphic3d_AspectFillArea3d;
        aspect->SetInteriorStyle(Aspect_IS_SOLID);aspect->SetInteriorColor(Quantity_Color(.38,.65,.84,Quantity_TOC_sRGB));
        aspect->SetShadingModel(Graphic3d_TOSM_UNLIT);aspect->SetEdgeOn();
        aspect->SetEdgeColor(Quantity_Color(.09,.18,.24,Quantity_TOC_sRGB));aspect->SetEdgeWidth(1);
        aspect->SetPolygonOffsets(Aspect_POM_Fill,-1,-1);
        auto group=presentation->NewGroup();group->SetPrimitivesAspect(aspect);group->AddPrimitiveArray(triangles);
    }
    void ComputeSelection(const Handle(SelectMgr_Selection)&,const Standard_Integer) override {}
};
const QMap<QString,V3d_TypeOfOrientation>& cubeOrientations() {
    static const QMap<QString,V3d_TypeOfOrientation> rows{
        {"right",V3d_Xpos},{"left",V3d_Xneg},{"back",V3d_Ypos},{"front",V3d_Yneg},
        {"top",V3d_Zpos},{"bottom",V3d_Zneg},
        {"rightBack",V3d_XposYpos},{"rightFront",V3d_XposYneg},
        {"leftBack",V3d_XnegYpos},{"leftFront",V3d_XnegYneg},
        {"rightTop",V3d_XposZpos},{"rightBottom",V3d_XposZneg},
        {"leftTop",V3d_XnegZpos},{"leftBottom",V3d_XnegZneg},
        {"backTop",V3d_YposZpos},{"backBottom",V3d_YposZneg},
        {"frontTop",V3d_YnegZpos},{"frontBottom",V3d_YnegZneg},
        {"rightBackTop",V3d_XposYposZpos},{"rightBackBottom",V3d_XposYposZneg},
        {"rightFrontTop",V3d_XposYnegZpos},{"rightFrontBottom",V3d_XposYnegZneg},
        {"leftBackTop",V3d_XnegYposZpos},{"leftBackBottom",V3d_XnegYposZneg},
        {"leftFrontTop",V3d_XnegYnegZpos},{"leftFrontBottom",V3d_XnegYnegZneg}};
    return rows;
}
QString presentationBodyKey(const EvaluatedBody& body){
    return body.occurrenceId.isEmpty() ? body.bodyId : body.occurrenceId+"/"+body.bodyId;
}
void showOriginTriad(const Handle(AIS_InteractiveContext)& context,std::vector<Handle(AIS_Shape)>& cues,
    const gp_Pnt& center,const gp_Dir& x,const gp_Dir& y,const gp_Dir& z,double radius) {
    const auto add=[&](const TopoDS_Shape& shape,const Quantity_Color& color){
        Handle(AIS_Shape) cue=new AIS_Shape(shape);cue->SetColor(color);
        cue->Attributes()->SetShadingModel(Graphic3d_TOSM_UNLIT);cue->SetZLayer(Graphic3d_ZLayerId_Topmost);
        context->Display(cue,AIS_Shaded,-1,false);context->Deactivate(cue);cues.push_back(cue);
    };
    add(BRepPrimAPI_MakeSphere(center,radius*.7).Shape(),Quantity_Color(.05,.77,.86,Quantity_TOC_sRGB));
    const std::array<std::pair<gp_Dir,Quantity_Color>,4> directions{{
        {x,Quantity_Color(.90,.13,.12,Quantity_TOC_sRGB)},
        {y,Quantity_Color(.10,.65,.26,Quantity_TOC_sRGB)},
        {z,Quantity_Color(.94,.97,.99,Quantity_TOC_sRGB)},
        {z.Reversed(),Quantity_Color(.96,.50,.11,Quantity_TOC_sRGB)}}};
    for(const auto& [axis,color]:directions){
        add(BRepPrimAPI_MakeCylinder(gp_Ax2(center,axis),radius*.16,radius*3).Shape(),color);
        add(BRepPrimAPI_MakeCone(gp_Ax2(center.Translated(gp_Vec(axis)*radius*3),axis),radius*.45,0,radius*.9).Shape(),color);
    }
}
}
#include <AIS_TextLabel.hxx>
#include <Geom_CartesianPoint.hxx>
#include <Prs3d_PointAspect.hxx>
#include <Graphic3d_ZLayerId.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <TopoDS_Edge.hxx>
#include <TopExp_Explorer.hxx>
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
QJsonObject threadMeshEvidence(const TopoDS_Shape& shape) {
    int faces=0,meshedFaces=0,triangles=0;QStringList identities;
    for(TopExp_Explorer it(shape,TopAbs_FACE);it.More();it.Next()) {
        ++faces;TopLoc_Location location;const auto mesh=BRep_Tool::Triangulation(TopoDS::Face(it.Current()),location);
        if(mesh.IsNull())continue;
        ++meshedFaces;triangles+=mesh->NbTriangles();
        identities.append(QString::number(reinterpret_cast<quintptr>(mesh.get()),16));
    }
    return {{"faces",faces},{"meshedFaces",meshedFaces},{"triangles",triangles},
        {"identitySignature",QString::number(qHash(identities.join(":")),16)}};
}
void traceThreadMesh(const char* phase,const TopoDS_Shape& shape) {
    if(qEnvironmentVariableIsSet("ALLOY_TRACE_THREAD"))qInfo().noquote()<<"thread_mesh"<<phase
        <<QJsonDocument(threadMeshEvidence(shape)).toJson(QJsonDocument::Compact);
}
Handle(AIS_Shape) sketchPresentation(const TopoDS_Shape& shape) {
    Handle(AIS_Shape) presentation=new AIS_ColoredShape(shape);
    presentation->SetColor(Quantity_Color(0.10,0.34,0.84,Quantity_TOC_RGB));
    presentation->SetWidth(2);
    presentation->Attributes()->SetDeviationCoefficient(0.0001);
    presentation->Attributes()->SetDeviationAngle(0.1);
    return presentation;
}
Quantity_Color unselectedSketchProfileColor() {
    // The public tutorial's editable region is a pale warm fill. Keep blue
    // reserved for selected profiles and red for Cut previews.
    return Quantity_Color(0.98,0.84,0.71,Quantity_TOC_sRGB);
}
bool solvedEntity(const QJsonObject& diagnostics,const QString& id){return diagnostics["entityFreedom"].toObject()[id].toObject()["fullyConstrained"].toBool();}
bool solvedPoint(const QJsonObject& diagnostics,const QString& id){return diagnostics["pointFreedom"].toObject()[id].toObject()["fullyConstrained"].toBool();}
void colorSolvedEntities(const Handle(AIS_Shape)& presentation,const QJsonArray& entities,const QJsonObject& diagnostics,int kind=0,const QJsonArray& projectedLines={},const QSet<QString>& hidden={}) {
    const auto colored=Handle(AIS_ColoredShape)::DownCast(presentation);if(colored.IsNull())return;
    QSet<QString> projected;for(const auto& p:projectedLines)projected.insert(p.toObject()["id"].toString());
    QMap<QString,QString> roles;
    for(const auto& row:diagnostics.value("entityRoles").toArray()){
        const auto value=row.toObject();const auto id=value.value("entityId").toString(),role=value.value("role").toString();
        roles.insert(id,role);if(role=="projected")projected.insert(id);
    }
    TopoDS_Iterator child(presentation->Shape());
    for(const auto& v:entities){const auto e=v.toObject();if(e["type"]=="point"||projected.contains(e["id"].toString()))continue;
        const bool construction=e["construction"].toBool(),centerline=e["centerline"].toBool();
        if((kind==1)!=construction)continue;
        if(!construction&&((kind==2)!=centerline))continue;
        if(!child.More())break;
        const auto role=roles.value(e.value("id").toString());
        const Quantity_Color color=role=="fixed" ? Quantity_Color(.10,.52,.24,Quantity_TOC_RGB)
            : solvedEntity(diagnostics,e["id"].toString()) ? Quantity_Color(.12,.14,.16,Quantity_TOC_sRGB)
            : construction || centerline ? Quantity_Color(.91,.47,.11,Quantity_TOC_RGB) : Quantity_Color(.10,.34,.84,Quantity_TOC_RGB);
        colored->SetCustomColor(child.Value(),color);colored->CustomAspects(child.Value())->SetHidden(hidden.contains(e.value("id").toString()));child.Next();
    }
}
struct SketchStrokes { TopoDS_Shape normal, construction, centerline; int constructionCount = 0; };
SketchStrokes splitSketchStrokes(const EvaluatedSketch& sketch) {
    if(sketch.shape.IsNull())return {};
    const auto entities=sketch.summary.value("entities").toArray();
    QSet<QString> projectedIds;
    for(const auto& value:sketch.summary.value("projectedLines").toArray())projectedIds.insert(value.toObject()["id"].toString());
    for(const auto& value:sketch.summary.value("projectedCircles").toArray())projectedIds.insert(value.toObject()["id"].toString());
    TopoDS_Iterator child(sketch.shape);
    TopoDS_Compound normal,construction,centerline;BRep_Builder builder;builder.MakeCompound(normal);builder.MakeCompound(construction);builder.MakeCompound(centerline);
    int ordinaryCount=0,constructionCount=0,centerlineCount=0;
    for(const auto& entry:entities) {
        if(!child.More())return {sketch.shape,{},{},0};
        if(entry.toObject()["type"]=="point"){child.Next();continue;}
        if(projectedIds.contains(entry.toObject()["id"].toString())){child.Next();continue;}
        if(entry.toObject().value("construction").toBool()) {builder.Add(construction,child.Value());++constructionCount;}
        else if(entry.toObject().value("centerline").toBool()) {builder.Add(centerline,child.Value());++centerlineCount;}
        else {builder.Add(normal,child.Value());++ordinaryCount;}
        child.Next();
    }
    const auto frame=sketch.resolvedFrame;const auto origin=frame["origin"].toArray(),axisU=frame["uAxis"].toArray(),axisV=frame["vAxis"].toArray();
    auto world=[&](double u,double v){return gp_Pnt(origin[0].toDouble()+u*axisU[0].toDouble()+v*axisV[0].toDouble(),origin[1].toDouble()+u*axisU[1].toDouble()+v*axisV[1].toDouble(),origin[2].toDouble()+u*axisU[2].toDouble()+v*axisV[2].toDouble());};
    for(const auto& entry:entities){const auto comb=entry.toObject().value("curvatureComb").toObject();if(!comb.value("enabled").toBool())continue;bool previous=false,previousTooth=false;gp_Pnt priorTip;
        for(const auto& value:comb["samples"].toArray()){const auto sample=value.toObject();if(!sample["defined"].toBool()){previous=false;continue;}const auto base=world(sample["uMm"].toDouble(),sample["vMm"].toDouble()),tip=world(sample["tipUMm"].toDouble(),sample["tipVMm"].toDouble());const bool tooth=base.Distance(tip)>1e-9;
            if(tooth)builder.Add(normal,BRepBuilderAPI_MakeEdge(base,tip).Edge());if(previous&&(tooth||previousTooth)&&priorTip.Distance(tip)>1e-9)builder.Add(normal,BRepBuilderAPI_MakeEdge(priorTip,tip).Edge());priorTip=tip;previous=true;previousTooth=tooth;
        }
    }
    if(child.More())return {sketch.shape,{},{},0};
    return {ordinaryCount ? TopoDS_Shape(normal) : TopoDS_Shape{},
        constructionCount ? TopoDS_Shape(construction) : TopoDS_Shape{},
        centerlineCount ? TopoDS_Shape(centerline) : TopoDS_Shape{},constructionCount};
}
TopoDS_Shape visibleFinishedSketchShape(const EvaluatedSketch& sketch) {
    const auto strokes=splitSketchStrokes(sketch);
    TopoDS_Compound visible;BRep_Builder builder;builder.MakeCompound(visible);
    bool hasStrokes=false;
    for(const auto& shape:{strokes.normal,strokes.construction,strokes.centerline})
        if(!shape.IsNull()){builder.Add(visible,shape);hasStrokes=true;}
    return hasStrokes ? TopoDS_Shape(visible) : TopoDS_Shape{};
}
void styleConstruction(Handle(AIS_Shape) presentation,bool selected,bool fullyConstrained) {
    const Quantity_Color color=selected ? Quantity_Color(0.08,0.43,0.91,Quantity_TOC_sRGB)
        : fullyConstrained ? Quantity_Color(0.12,0.14,0.16,Quantity_TOC_sRGB)
        : Quantity_Color(0.91,0.47,0.11,Quantity_TOC_RGB);
    const Handle(Prs3d_LineAspect) dashed=new Prs3d_LineAspect(color,Aspect_TOL_DASH,selected ? 4 : 1.5);
    presentation->SetColor(color);
    presentation->Attributes()->SetWireAspect(dashed);presentation->Attributes()->SetLineAspect(dashed);
    presentation->Attributes()->SetFreeBoundaryAspect(dashed);presentation->Attributes()->SetUnFreeBoundaryAspect(dashed);
    presentation->SetDisplayMode(AIS_WireFrame);
}
Handle(AIS_Shape) constructionPresentation(const TopoDS_Shape& shape,bool selected=false,bool fullyConstrained=false) {
    Handle(AIS_Shape) presentation=new AIS_ColoredShape(shape);
    styleConstruction(presentation,selected,fullyConstrained);
    return presentation;
}
void styleCenterline(Handle(AIS_Shape) presentation,bool selected,bool fullyConstrained) {
    const Quantity_Color color=selected ? Quantity_Color(0.08,0.43,0.91,Quantity_TOC_sRGB)
        : fullyConstrained ? Quantity_Color(0.12,0.14,0.16,Quantity_TOC_sRGB)
        : Quantity_Color(0.91,0.47,0.11,Quantity_TOC_RGB);
    const Handle(Prs3d_LineAspect) dashed=new Prs3d_LineAspect(color,Aspect_TOL_DOTDASH,selected ? 4 : 1.5);
    presentation->SetColor(color);
    presentation->Attributes()->SetWireAspect(dashed);presentation->Attributes()->SetLineAspect(dashed);
    presentation->Attributes()->SetFreeBoundaryAspect(dashed);presentation->Attributes()->SetUnFreeBoundaryAspect(dashed);
    presentation->SetDisplayMode(AIS_WireFrame);
}
Handle(AIS_Shape) centerlinePresentation(const TopoDS_Shape& shape,bool selected=false,bool fullyConstrained=false) {
    Handle(AIS_Shape) presentation=new AIS_ColoredShape(shape);
    styleCenterline(presentation,selected,fullyConstrained);
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
    cubeAccessoryToggle_=new QToolButton(this);cubeAccessoryToggle_->setObjectName("cube.accessories.toggle");
    cubeAccessoryToggle_->setText("⋯");cubeAccessoryToggle_->setToolTip("Show Home, adjacent-view and roll controls");
    cubeAccessoryToggle_->setAccessibleName("Show orientation controls");cubeAccessoryToggle_->setFixedSize(27,27);
    cubeAccessoryToggle_->setFocusPolicy(Qt::StrongFocus);
    cubeAccessoryToggle_->setStyleSheet("QToolButton { background: rgba(247,250,252,235); color: #315369; border: 1px solid #afc3d0; border-radius: 4px; font-size: 14px; }"
        "QToolButton:hover, QToolButton:focus { background: #d9eaf5; border-color: #4587b7; }");
    cubeAccessoryToggle_->installEventFilter(this);
    connect(cubeAccessoryToggle_,&QToolButton::clicked,this,[this]{
        cubeAccessoriesDismissed_=false;updateCubeAccessoryVisibility();
        for(auto* control:cubeControls_->findChildren<QToolButton*>())
            if(control->isEnabled()){control->setFocus(Qt::OtherFocusReason);break;}
    });
    cubeControls_=new QWidget(this);cubeControls_->setObjectName("cube.accessories");
    cubeControls_->setStyleSheet("QToolButton { background: rgba(247,250,252,235); color: #315369; border: 1px solid #afc3d0; border-radius: 4px; font-size: 13px; }"
        "QToolButton:hover { background: #d9eaf5; border-color: #4587b7; }"
        "QToolButton:pressed { background: #b5d7ee; }"
        "QToolButton:disabled { color: #a5b0b8; background: rgba(246,248,249,180); border-color: #d8e0e4; }");
    auto* grid=new QGridLayout(cubeControls_);grid->setContentsMargins(0,0,0,0);grid->setSpacing(2);
    const auto button=[this,grid](const QString& name,const QString& glyph,const QString& title,int row,int col,
        const QString& command,const QJsonObject& args,bool faceOnly){
        auto* control=new QToolButton(cubeControls_);control->setObjectName("cube."+name);
        control->setText(glyph);control->setToolTip(title);control->setAccessibleName(title);
        control->setFixedSize(27,27);control->setFocusPolicy(Qt::StrongFocus);grid->addWidget(control,row,col);
        connect(control,&QToolButton::clicked,this,[this,command,args]{if(dispatch)dispatch(command,args);});
        if(faceOnly)cubeFaceControls_.push_back(control);
        return control;
    };
    button("rollLeft","↶","Roll view left 90°",0,0,"view.cubeRoll",{{"side","left"}},true);
    button("home","⌂","Home view",0,1,"view.home",{},false);
    button("rollRight","↷","Roll view right 90°",0,2,"view.cubeRoll",{{"side","right"}},true);
    button("adjacentUp","↑","Adjacent view above",1,1,"view.cubeAdjacent",{{"side","up"}},true);
    button("adjacentLeft","←","Adjacent view left",2,0,"view.cubeAdjacent",{{"side","left"}},true);
    button("adjacentDown","↓","Adjacent view below",2,1,"view.cubeAdjacent",{{"side","down"}},true);
    button("adjacentRight","→","Adjacent view right",2,2,"view.cubeAdjacent",{{"side","right"}},true);
    cubeControls_->setFixedSize(85,85);cubeControls_->move(width()-185,38);cubeControls_->hide();
    cubeControls_->installEventFilter(this);
    for(auto* control:cubeControls_->findChildren<QToolButton*>())control->installEventFilter(this);
    cubeUpLabel_=new QLabel("UP Z",this);cubeUpLabel_->setObjectName("cube.upAxis");
    cubeUpLabel_->setFixedSize(85,18);cubeUpLabel_->setAlignment(Qt::AlignCenter);
    cubeUpLabel_->setStyleSheet("background: rgba(247,250,252,225); color: #315369; border: 1px solid #afc3d0; border-radius: 3px; font-size: 10px; font-weight: 600;");
    cubeUpLabel_->move(width()-185,125);cubeUpLabel_->hide();
    cubeAccessoryToggle_->move(width()-105,6);cubeAccessoryToggle_->raise();
    updateCubeControls();
}
ReferenceView::~ReferenceView() {
    if (!view_.IsNull()) view_->Remove();
}
QJsonObject ReferenceView::cameraState() const {
    QJsonObject state{{"orientation",orientation},{"zoom",zoom},{"panX",pan.x()},{"panY",pan.y()},
        {"grid",grid},{"origin",origin},{"sketchPlane",sketchPlane_},{"sketchFrame",sketchFrame_},
        {"centerX",sceneCenter_.X()},{"centerY",sceneCenter_.Y()},{"centerZ",sceneCenter_.Z()},
        {"viewCenter",QJsonArray{viewCenter_.X(),viewCenter_.Y(),viewCenter_.Z()}},
        {"orbitMode",orbitMode_},{"navigationTool",navigationTool_},
        {"customDirection",QJsonArray{customDirection_.X(),customDirection_.Y(),customDirection_.Z()}},
        {"customUp",QJsonArray{customUp_.X(),customUp_.Y(),customUp_.Z()}},
        {"scale",sceneScale_},{"projectionPolicy",projectionPolicy_},
        {"resolvedProjection",view_.IsNull() || view_->Camera()->IsOrthographic() ? "orthographic" : "perspective"}};
    const auto pivot=customOrbitCenter_.value_or(sceneCenter_);
    state["orbitPivot"]=QJsonArray{pivot.X(),pivot.Y(),pivot.Z()};
    state["customOrbitCenter"]=customOrbitCenter_.has_value();
    state["lastInput"]=lastNavigationInput_;
    if(customOrbitCenter_ && !view_.IsNull()) {
        int x,y;view_->Convert(pivot.X(),pivot.Y(),pivot.Z(),x,y);
        const auto scale=windowScale();state["pivotScreen"]=QJsonObject{{"x",x/scale.x()},{"y",y/scale.y()}};
    }
    return state;
}
QJsonObject ReferenceView::viewConvention() const {
    const auto array=[](const gp_Dir& d){return QJsonArray{d.X(),d.Y(),d.Z()};};
    return {{"defaultUp",conventionDefaultUp_},{"right",array(conventionRight_)},
        {"back",array(conventionBack_)},{"up",array(conventionUp_)}};
}
void ReferenceView::freezeWorldCamera() {
    if(view_.IsNull())return;
    customDirection_=view_->Camera()->Direction();customUp_=view_->Camera()->Up();orientation="custom";
}
gp_Dir ReferenceView::conventionDirection(const gp_Dir& local) const {
    const gp_Vec world=gp_Vec(conventionRight_)*local.X()+gp_Vec(conventionBack_)*local.Y()+
        gp_Vec(conventionUp_)*local.Z();
    return gp_Dir(world);
}
void ReferenceView::setViewConvention(const QJsonObject& convention,bool preserveCamera) {
    if(preserveCamera)freezeWorldCamera();
    const auto effective=convention.isEmpty() ? QJsonObject{{"defaultUp","Z"},
        {"right",QJsonArray{1,0,0}},{"back",QJsonArray{0,1,0}},{"up",QJsonArray{0,0,1}}} : convention;
    const auto read=[&](const char* key){const auto a=effective.value(key).toArray();
        return gp_Dir(a[0].toDouble(),a[1].toDouble(),a[2].toDouble());};
    conventionRight_=read("right");conventionBack_=read("back");conventionUp_=read("up");
    conventionDefaultUp_=effective.value("defaultUp").toString("Z");
    if(cubeUpLabel_){
        const gp_Vec up(conventionUp_);
        const QString axis=(up-gp_Vec(0,0,1)).Magnitude()<1e-7 ? "Z" :
            (up-gp_Vec(0,1,0)).Magnitude()<1e-7 ? "Y" : "CUSTOM";
        cubeUpLabel_->setText("UP "+axis);
        cubeUpLabel_->setToolTip("Cube up direction; document default is "+conventionDefaultUp_+"-up");
    }
    if(!viewCube_.IsNull()){
        gp_Trsf transform;
        transform.SetValues(conventionRight_.X(),conventionBack_.X(),conventionUp_.X(),0,
            conventionRight_.Y(),conventionBack_.Y(),conventionUp_.Y(),0,
            conventionRight_.Z(),conventionBack_.Z(),conventionUp_.Z(),0);
        viewCube_->SetLocalTransformation(transform);
        context_->Redisplay(viewCube_,false);
    }
    sync();
}
QJsonObject ReferenceView::conventionFromCurrentView(const QString& face) const {
    if(view_.IsNull() || (face!="front" && face!="top"))return {};
    const gp_Dir direction=view_->Camera()->Direction(),cameraUp=view_->Camera()->Up();
    const gp_Dir back=face=="front" ? direction : cameraUp;
    const gp_Dir up=face=="front" ? cameraUp : gp_Dir(-direction.X(),-direction.Y(),-direction.Z());
    const gp_Dir right(gp_Vec(back).Crossed(gp_Vec(up)));
    const auto array=[](const gp_Dir& d){return QJsonArray{d.X(),d.Y(),d.Z()};};
    return {{"defaultUp",conventionDefaultUp_},{"right",array(right)},
        {"back",array(back)},{"up",array(up)}};
}
void ReferenceView::restoreCameraState(const QJsonObject& state) {
    orientation=state.value("orientation").toString("isometric");
    const auto policy=state.value("projectionPolicy").toString("orthographic");
    projectionPolicy_=policy=="perspective" || policy=="hybrid" ? policy : "orthographic";
    zoom=state.value("zoom").toDouble(1);pan={state.value("panX").toDouble(),state.value("panY").toDouble()};
    grid=state.value("grid").toBool(true);origin=state.value("origin").toBool(true);
    sketchPlane_=state.value("sketchPlane").toString();
    sketchFrame_=state.value("sketchFrame").toObject();
    sceneCenter_=gp_Pnt(state.value("centerX").toDouble(),state.value("centerY").toDouble(),state.value("centerZ").toDouble());
    const auto viewCenter=state.value("viewCenter").toArray();
    viewCenter_=viewCenter.size()==3 ? gp_Pnt(viewCenter[0].toDouble(),viewCenter[1].toDouble(),viewCenter[2].toDouble()) : sceneCenter_;
    orbitMode_=state.value("orbitMode")=="constrained" ? "constrained" : "free";
    setNavigationTool(state.value("navigationTool").toString("select"));
    lastNavigationInput_=state.value("lastInput").toObject();nativeGestureActive_=false;
    const auto pivot=state.value("orbitPivot").toArray();
    customOrbitCenter_=state.value("customOrbitCenter").toBool() && pivot.size()==3
        ? std::optional<gp_Pnt>(gp_Pnt(pivot[0].toDouble(),pivot[1].toDouble(),pivot[2].toDouble())) : std::nullopt;
    orbitCenterPicking_=false;
    sceneScale_=state.value("scale").toDouble(400);
    if(orientation=="custom") {
        const auto direction=state.value("customDirection").toArray(),up=state.value("customUp").toArray();
        if(direction.size()!=3 || up.size()!=3)orientation="isometric";
        else try {
            const gp_Vec d(direction[0].toDouble(),direction[1].toDouble(),direction[2].toDouble());
            const gp_Vec u(up[0].toDouble(),up[1].toDouble(),up[2].toDouble());
            if(d.Magnitude()<1e-9 || u.Magnitude()<1e-9 || d.Crossed(u).Magnitude()<1e-9)orientation="isometric";
            else {customDirection_=gp_Dir(d);customUp_=gp_Dir(u);}
        } catch(const Standard_Failure&) {orientation="isometric";}
    }
    sync();refreshOrbitCenterMarker();
}
void ReferenceView::setProjectionPolicy(const QString& policy) {
    if(policy!="orthographic" && policy!="perspective" && policy!="hybrid")return;
    projectionPolicy_=policy;sync();
}
void ReferenceView::orbitFree(double deltaX,double deltaY,bool useCustomPivot) {
    if(view_.IsNull() || !std::isfinite(deltaX) || !std::isfinite(deltaY))return;
    // Rotate the camera basis and the persistent view-center offset together.
    // This leaves the chosen pivot at the same screen position even after pan.
    constexpr double radiansPerPixel=std::numbers::pi/600;
    gp_Vec forward(view_->Camera()->Direction()),up(view_->Camera()->Up());
    const auto pivot=useCustomPivot ? customOrbitCenter_.value_or(sceneCenter_) : viewCenter_;
    gp_Vec centerOffset(pivot,viewCenter_);
    gp_Vec right=forward.Crossed(up);
    if(right.Magnitude()<1e-9)return;
    right.Normalize();
    const gp_Ax1 vertical{gp_Pnt{},gp_Dir{up}};
    forward.Rotate(vertical,-deltaX*radiansPerPixel);
    right.Rotate(vertical,-deltaX*radiansPerPixel);
    centerOffset.Rotate(vertical,-deltaX*radiansPerPixel);
    const gp_Ax1 horizontal{gp_Pnt{},gp_Dir{right}};
    forward.Rotate(horizontal,-deltaY*radiansPerPixel);
    up.Rotate(horizontal,-deltaY*radiansPerPixel);
    centerOffset.Rotate(horizontal,-deltaY*radiansPerPixel);
    if(forward.Crossed(up).Magnitude()<1e-9)return;
    viewCenter_=pivot.Translated(centerOffset);
    customDirection_=gp_Dir(forward);customUp_=gp_Dir(up);
    orientation="custom";sync();
}
void ReferenceView::orbitConstrained(double deltaX,double deltaY,bool useCustomPivot) {
    if(view_.IsNull() || !std::isfinite(deltaX) || !std::isfinite(deltaY))return;
    constexpr double radiansPerPixel=std::numbers::pi/600;
    constexpr double poleMargin=std::numbers::pi/36; // Alloy policy: 5° from either pole.
    const gp_Vec worldUp(conventionUp_);
    const auto pivot=useCustomPivot ? customOrbitCenter_.value_or(sceneCenter_) : viewCenter_;
    gp_Vec centerOffset(pivot,viewCenter_),forward(view_->Camera()->Direction());
    const gp_Ax1 yaw(gp_Pnt{},gp_Dir(worldUp));
    forward.Rotate(yaw,-deltaX*radiansPerPixel);
    centerOffset.Rotate(yaw,-deltaX*radiansPerPixel);
    gp_Vec right=forward.Crossed(worldUp);
    if(right.Magnitude()<1e-9)right=gp_Vec(view_->Camera()->Direction()).Crossed(gp_Vec(view_->Camera()->Up()));
    if(right.Magnitude()<1e-9)return;
    right.Normalize();
    const double current=std::acos(std::clamp(forward.Dot(worldUp),-1.0,1.0));
    const double desired=std::clamp(current+deltaY*radiansPerPixel,poleMargin,std::numbers::pi-poleMargin);
    const gp_Ax1 pitch(gp_Pnt{},gp_Dir(right));
    forward.Rotate(pitch,current-desired);
    centerOffset.Rotate(pitch,current-desired);
    gp_Vec levelUp=worldUp-forward*forward.Dot(worldUp);
    if(levelUp.Magnitude()<1e-9)return;
    levelUp.Normalize();
    viewCenter_=pivot.Translated(centerOffset);
    customDirection_=gp_Dir(forward);customUp_=gp_Dir(levelUp);
    orientation="custom";sync();
}
void ReferenceView::orbitByMode(double deltaX,double deltaY,bool useCustomPivot) {
    if(orbitMode_=="constrained")orbitConstrained(deltaX,deltaY,useCustomPivot);
    else orbitFree(deltaX,deltaY,useCustomPivot);
}
bool ReferenceView::zoomAt(double factor,std::optional<QPointF> cursor) {
    if(!std::isfinite(factor) || factor<=0)return false;
    if(cursor && (!std::isfinite(cursor->x()) || !std::isfinite(cursor->y()) ||
        cursor->x()<0 || cursor->y()<0 || cursor->x()>=width() || cursor->y()>=height()))return false;
    const auto previous=zoom;
    zoom=std::clamp(zoom*factor,0.1,10.0);
    const auto effective=zoom/previous;
    if(cursor && effective!=1) {
        // Orthographic camera policy: magnify about the pointer rather than
        // the canvas center. Use the effective clamped factor so zooming at a
        // bound cannot move the model without a scale change.
        const QPointF offset=*cursor-QPointF(width()/2.0,height()/2.0);
        pan=pan*effective+offset*(1-effective);
    }
    return true;
}
bool ReferenceView::zoomWindow(const QRectF& input) {
    if(!std::isfinite(input.left()) || !std::isfinite(input.top()) ||
        !std::isfinite(input.right()) || !std::isfinite(input.bottom()) || width()<=0 || height()<=0)return false;
    const QRectF rectangle=input.normalized().intersected(QRectF(0,0,width(),height()));
    if(rectangle.width()<8 || rectangle.height()<8)return false;
    const double factor=std::min(double(width())/rectangle.width(),double(height())/rectangle.height());
    if(!std::isfinite(factor) || factor<=0)return false;
    const double oldZoom=zoom;
    const double nextZoom=std::clamp(zoom*factor,0.1,10.0);
    const double effective=nextZoom/oldZoom;
    const QPointF center(width()/2.0,height()/2.0);
    pan=effective*(pan+center-rectangle.center());
    zoom=nextZoom;
    return true;
}
void ReferenceView::cancelNavigationGesture(bool suppressRelease) {
    if(suppressRelease && navigationPressActive_)navigationSuppressRelease_=true;
    navigationPressActive_=false;navigationMoved_=false;
    if(navigationWindowBand_)navigationWindowBand_->hide();
}
void ReferenceView::setNavigationTool(const QString& mode) {
    cancelNavigationGesture(navigationPressActive_);
    navigationTool_=mode;
    if(mode=="select")unsetCursor();
    else setCursor(mode=="zoomWindow" ? Qt::CrossCursor : Qt::OpenHandCursor);
}
void ReferenceView::setInputPreset(const QString& preset) {
    if(inputPreset_==preset)return;
    cancelInputGesture();
    inputPreset_=preset=="shiftAlt" ? "shiftAlt" : "default";
}
void ReferenceView::cancelInputGesture() {
    if(orbitButton_==Qt::LeftButton || panButton_==Qt::LeftButton)alternateOrbitSuppressRelease_=true;
    if(zoomButton_==Qt::RightButton)alternateRightContextSuppressed_=true;
    orbitButton_=Qt::NoButton;panButton_=Qt::NoButton;zoomButton_=Qt::NoButton;
    if(navigationTool_=="select" && !freeOrbitTool_)unsetCursor();
}
QStringList ReferenceView::cubeRegions() { return cubeOrientations().keys(); }
QJsonObject ReferenceView::cubeHit(double x,double y) {
    if(view_.IsNull() || viewCube_.IsNull() || !std::isfinite(x) || !std::isfinite(y) ||
        x<0 || y<0 || x>=width() || y>=height())return {};
    const auto scale=windowScale();
    context_->MoveTo(qRound(x*scale.x()),qRound(y*scale.y()),view_,true);
    if(!context_->HasDetected())return {};
    const auto owner=Handle(AIS_ViewCubeOwner)::DownCast(context_->DetectedOwner());
    if(owner.IsNull() || owner->Selectable()!=viewCube_)return {};
    const auto orientation=owner->MainOrientation();
    for(auto it=cubeOrientations().cbegin();it!=cubeOrientations().cend();++it)if(it.value()==orientation) {
        const auto kind=AIS_ViewCube::IsBoxSide(orientation) ? "face" :
            AIS_ViewCube::IsBoxEdge(orientation) ? "edge" :
            AIS_ViewCube::IsBoxCorner(orientation) ? "corner" : "unsupported";
        if(QString::fromUtf8(kind)=="unsupported")return {};
        return {{"region",it.key()},{"kind",kind}};
    }
    return {};
}
bool ReferenceView::setCubeOrientation(const QString& region) {
    if(view_.IsNull() || !cubeOrientations().contains(region))return false;
    // Ask OCCT for this standard region's basis on a disposable camera pose.
    // Restore the live pose before synchronizing from Alloy's authoritative
    // direction/up, framing, pan and zoom; never run AIS cube animation.
    const Handle(Graphic3d_Camera) previous=new Graphic3d_Camera(view_->Camera());
    view_->SetProj(cubeOrientations().value(region));
    const auto direction=conventionDirection(view_->Camera()->Direction()),up=conventionDirection(view_->Camera()->Up());
    view_->SetCamera(previous);
    customDirection_=direction;customUp_=up;orientation="custom";
    sync();return true;
}
QJsonObject ReferenceView::cubeFaceState() const {
    if(view_.IsNull())return {{"enabled",false}};
    const gp_Vec direction(view_->Camera()->Direction()),up(view_->Camera()->Up());
    const std::array<double,3> d{direction.Dot(gp_Vec(conventionRight_)),
        direction.Dot(gp_Vec(conventionBack_)),direction.Dot(gp_Vec(conventionUp_))};
    const std::array<double,3> u{up.Dot(gp_Vec(conventionRight_)),
        up.Dot(gp_Vec(conventionBack_)),up.Dot(gp_Vec(conventionUp_))};
    int normalAxis=-1,upAxis=-1;
    for(int i=0;i<3;++i) {
        if(std::abs(std::abs(d[i])-1)<1e-7)normalAxis=i;
        if(std::abs(std::abs(u[i])-1)<1e-7)upAxis=i;
    }
    if(normalAxis<0 || upAxis<0 || normalAxis==upAxis)return {{"enabled",false}};
    for(int i=0;i<3;++i)if((i!=normalAxis && std::abs(d[i])>1e-7) || (i!=upAxis && std::abs(u[i])>1e-7))
        return {{"enabled",false}};
    const QString face=normalAxis==0 ? (d[0]<0 ? "right" : "left") :
        normalAxis==1 ? (d[1]>0 ? "front" : "back") : (d[2]<0 ? "top" : "bottom");
    return {{"enabled",true},{"face",face}};
}
void ReferenceView::updateCubeControls() {
    const bool enabled=cubeFaceState().value("enabled").toBool();
    for(auto* control:cubeFaceControls_)control->setEnabled(enabled);
}
void ReferenceView::updateCubeAccessoryVisibility() {
    if(!cubeControls_ || !cubeAccessoryToggle_)return;
    const auto* hovered=QApplication::widgetAt(QCursor::pos());
    const auto belongsTo=[](const QWidget* widget,const QWidget* root) {
        for(auto* current=widget;current;current=current->parentWidget())if(current==root)return true;
        return false;
    };
    const bool pointerOnToggle=belongsTo(hovered,cubeAccessoryToggle_);
    const bool pointerOnAccessories=cubeControls_->isVisible() && belongsTo(hovered,cubeControls_);
    const QPoint cursorLocal=mapFromGlobal(QCursor::pos());
    const QRect controlsRect=cubeControls_->geometry();
    const QRect cubeNeighborhood(width()-85,0,85,125);
    const QRect openZone=controlsRect.united(cubeNeighborhood).united(cubeUpLabel_->geometry());
    const bool pointerWithinCubeNeighborhood=hovered==this && cubeNeighborhood.contains(cursorLocal);
    const bool cubeHoverAtPointer=cubeHoverActive_ && pointerWithinCubeNeighborhood;
    const bool pointerInOpenZone=cubeControls_->isVisible() && openZone.contains(cursorLocal);
    const auto* focused=QApplication::focusWidget();
    const bool focusOnToggle=belongsTo(focused,cubeAccessoryToggle_);
    const bool focusOnAccessories=belongsTo(focused,cubeControls_);
    if(cubeAccessoriesDismissed_ && !cubeHoverAtPointer && !pointerOnToggle && !pointerOnAccessories &&
       !pointerInOpenZone &&
       !focusOnToggle && !focusOnAccessories)cubeAccessoriesDismissed_=false;
    const bool visible=!cubeAccessoriesDismissed_ &&
        (cubeHoverAtPointer || pointerOnToggle || pointerOnAccessories || pointerInOpenZone ||
         focusOnToggle || focusOnAccessories);
    cubeControls_->setVisible(visible);
    cubeUpLabel_->setVisible(visible);
    cubeAccessoryToggle_->setAccessibleDescription(visible ?
        "Orientation controls are visible. Escape closes them." :
        "Activate or focus to open orientation controls; they also appear while pointing at the view cube.");
    cubeAccessoryToggle_->raise();
    if(visible){cubeControls_->raise();cubeUpLabel_->raise();cubeAccessoryToggle_->raise();}
}
bool ReferenceView::eventFilter(QObject* watched,QEvent* e) {
    if(watched->objectName()=="sketch.selectOther") {
        if(e->type()==QEvent::Leave)previewSketchSelectionCandidate();
        if(e->type()==QEvent::MouseMove) {
            auto* menu=qobject_cast<QMenu*>(watched);
            auto* action=menu ? menu->actionAt(static_cast<QMouseEvent*>(e)->position().toPoint()) : nullptr;
            previewSketchSelectionCandidate(action ? action->data().toJsonObject() : QJsonObject{});
        }
    }
    if(watched==cubeAccessoryToggle_ || watched==cubeControls_ ||
       (cubeControls_ && cubeControls_->isAncestorOf(qobject_cast<QWidget*>(watched)))) {
        if(e->type()==QEvent::ShortcutOverride && static_cast<QKeyEvent*>(e)->key()==Qt::Key_Escape) {
            static_cast<QKeyEvent*>(e)->accept();return true;
        }
        if(e->type()==QEvent::KeyPress && static_cast<QKeyEvent*>(e)->key()==Qt::Key_Escape) {
            cubeAccessoriesDismissed_=true;cubeControls_->hide();cubeUpLabel_->hide();
            cubeAccessoryToggle_->setFocus(Qt::OtherFocusReason);
            static_cast<QKeyEvent*>(e)->accept();return true;
        }
        if(e->type()==QEvent::Enter || e->type()==QEvent::Leave ||
           e->type()==QEvent::FocusIn || e->type()==QEvent::FocusOut) {
            QTimer::singleShot(0,this,[this]{updateCubeAccessoryVisibility();});
        }
    }
    return QWidget::eventFilter(watched,e);
}
bool ReferenceView::cubeAdjacent(const QString& side) {
    if(!cubeFaceState().value("enabled").toBool())return false;
    if(side!="up" && side!="down" && side!="left" && side!="right")return false;
    gp_Vec direction(view_->Camera()->Direction()),up(view_->Camera()->Up());
    gp_Vec right=direction.Crossed(up);if(right.Magnitude()<1e-9)return false;
    right.Normalize();
    const bool vertical=side=="up" || side=="down";
    const gp_Ax1 axis(gp_Pnt{},gp_Dir(vertical ? right : up));
    const double angle=(side=="up" || side=="right" ? 1 : -1)*std::numbers::pi/2;
    direction.Rotate(axis,angle);up.Rotate(axis,angle);
    customDirection_=gp_Dir(direction);customUp_=gp_Dir(up);orientation="custom";sync();return true;
}
bool ReferenceView::cubeRoll(const QString& side) {
    if(!cubeFaceState().value("enabled").toBool() || (side!="left" && side!="right"))return false;
    const gp_Vec direction(view_->Camera()->Direction());
    gp_Vec up(view_->Camera()->Up());
    up.Rotate(gp_Ax1(gp_Pnt{},gp_Dir(direction)),(side=="left" ? 1 : -1)*std::numbers::pi/2);
    customDirection_=gp_Dir(direction);customUp_=gp_Dir(up);orientation="custom";sync();return true;
}
void ReferenceView::cancelCubeGesture(bool suppressRelease) {
    cubePressHandled_=false;cubeDragging_=false;cubePressedRegion_.clear();
    cubeSuppressRelease_=cubeSuppressRelease_ || suppressRelease;
    if(!cubeHoverActive_)unsetCursor();
}
void ReferenceView::setOrbitMode(const QString& mode) {
    const QString next=mode=="constrained" ? "constrained" : "free";
    cancelOrbitCenterPick();
    if(next==orbitMode_)return;
    orbitMode_=next;
    if(next!="constrained" || view_.IsNull())return;
    // Level a rolled Free view about its forward axis. Rotate the offset from
    // the chosen pivot as well, so switching modes does not move that pivot on
    // screen even when the camera was panned before the switch.
    const gp_Vec forward(view_->Camera()->Direction()),oldUp(view_->Camera()->Up());
    const gp_Vec worldUp(conventionUp_);
    gp_Vec levelUp=worldUp-forward*forward.Dot(worldUp);
    if(levelUp.Magnitude()>1e-9) {
        levelUp.Normalize();
        const double roll=std::atan2(forward.Dot(oldUp.Crossed(levelUp)),oldUp.Dot(levelUp));
        const auto pivot=customOrbitCenter_.value_or(sceneCenter_);
        gp_Vec offset(pivot,viewCenter_);
        offset.Rotate(gp_Ax1(gp_Pnt{},gp_Dir(forward)),roll);
        viewCenter_=pivot.Translated(offset);
    }
    orbitConstrained(0,0);
}
void ReferenceView::beginOrbitCenterPick() {
    orbitCenterPicking_=true;setCursor(Qt::CrossCursor);
}
void ReferenceView::cancelOrbitCenterPick() {
    if(!orbitCenterPicking_)return;
    orbitCenterPicking_=false;unsetCursor();
}
QJsonObject ReferenceView::pickOrbitCenter(double x,double y) const {
    // A transient body stage can replace the displayed shapes. This first
    // pivot slice admits only the retained, visibly displayed body set.
    if(view_.IsNull() || !bodyStageKey_.isEmpty() || x<0 || y<0 || x>=width() || y>=height())return {};
    const auto scale=windowScale();double px,py,pz,dx,dy,dz;
    view_->ConvertWithProj(qRound(x*scale.x()),qRound(y*scale.y()),px,py,pz,dx,dy,dz);
    const gp_Vec direction(dx,dy,dz);
    if(direction.Magnitude()<1e-9)return {};
    const auto rayDirection=gp_Dir(direction);
    const gp_Pnt projected(px,py,pz);
    // ConvertWithProj lies on the projection plane. Start toward the eye so
    // the nearest hit includes visible surfaces in front of that plane.
    const gp_Pnt origin=projected.Translated(-gp_Vec(rayDirection)*(view_->Camera()->Distance()+sceneScale_*2));
    const gp_Lin ray(origin,rayDirection);
    double nearest=std::numeric_limits<double>::infinity();QJsonObject hit;
    try {
        for(const auto& body:documentBodies_)if(body.visible && !body.shape.IsNull()) {
            IntCurvesFace_ShapeIntersector intersections;intersections.Load(body.shape,1e-6);
            intersections.PerformNearest(ray,0,1e9);
            if(!intersections.IsDone() || intersections.NbPnt()<1 || intersections.WParameter(1)>=nearest)continue;
            nearest=intersections.WParameter(1);
            const auto point=intersections.Pnt(1);
            hit={{"bodyId",body.bodyId},{"point",QJsonArray{point.X(),point.Y(),point.Z()}}};
            if(!body.occurrenceId.isEmpty())hit["occurrenceId"]=body.occurrenceId;
        }
    } catch(const Standard_Failure&) {return {};}
    return hit;
}
void ReferenceView::refreshOrbitCenterMarker() {
    if(context_.IsNull())return;
    if(!orbitCenterMarker_.IsNull())context_->Remove(orbitCenterMarker_,false);
    orbitCenterMarker_.Nullify();
    if(!customOrbitCenter_) {context_->UpdateCurrentViewer();return;}
    orbitCenterMarker_=new AIS_Point(new Geom_CartesianPoint(*customOrbitCenter_));
    orbitCenterMarker_->Attributes()->SetPointAspect(new Prs3d_PointAspect(Aspect_TOM_O_POINT,
        Quantity_Color(0.07,0.78,0.22,Quantity_TOC_sRGB),11*devicePixelRatioF()));
    orbitCenterMarker_->SetZLayer(Graphic3d_ZLayerId_Topmost);
    context_->Display(orbitCenterMarker_,false);context_->Deactivate(orbitCenterMarker_);
    context_->UpdateCurrentViewer();
}
void ReferenceView::setOrbitCenter(const QJsonObject& hit) {
    const auto point=hit.value("point").toArray();
    if(point.size()!=3)return;
    customOrbitCenter_=gp_Pnt(point[0].toDouble(),point[1].toDouble(),point[2].toDouble());
    cancelOrbitCenterPick();refreshOrbitCenterMarker();
}
void ReferenceView::resetOrbitCenter() {
    customOrbitCenter_.reset();cancelOrbitCenterPick();refreshOrbitCenterMarker();
}
void ReferenceView::rebindDocument() {
    setInterferenceFeedback({},{});
    setZebra({});
    setFaceNormals({});
    setMeshExportPreview({});
    cancelNavigationGesture(navigationPressActive_);
    cancelOrbitCenterPick();
    nativeGestureActive_=false;
    if(!context_.IsNull() && !orbitCenterMarker_.IsNull())context_->Remove(orbitCenterMarker_,false);
    orbitCenterMarker_.Nullify();customOrbitCenter_.reset();
    setCanvases({},EvaluatedDocument{});canvasStamp_={};
    clearPatternAxes();
    setSliderCue({});
    setJointOriginPicking(false);setJointOrigins({},false);
    if(context_.IsNull())return;
    clearPreview();clearSketchDraft();clearBodyStage();clearSketchCursor();
    selectSupportFace({},{});hoverSupportFace({});sketchFrame_={};
    clearSweepPath();commandSketches_.clear();
    setEditDisplay({},{});selectProfile({},{});highlightCombineInputs({},{});
    for(const auto& model:models_)context_->Remove(model,false);
    for(const auto& model:sketchModels_)context_->Remove(model,false);
    for(const auto& model:constructionSketchModels_)context_->Remove(model,false);
    for(const auto& model:centerlineSketchModels_)context_->Remove(model,false);
    for(const auto& model:constructionPointModels_)context_->Remove(model,false);
    constructionPointModels_.clear();constructionPointRevision_=std::numeric_limits<quint64>::max();
    for(const auto& model:constructionAxisModels_)context_->Remove(model,false);
    constructionAxisModels_.clear();constructionAxisRevision_=std::numeric_limits<quint64>::max();
    for(const auto& model:constructionPlaneModels_)context_->Remove(model,false);
    for(const auto& model:finishedProfileModels_)context_->Remove(model,false);
    for(const auto& model:finishedPointModels_)context_->Remove(model,false);
    finishedPointModels_.clear();selectedFinishedPoint_={};
    models_.clear();modelReferences_.clear();bodyVisibility_.clear();sketchModels_.clear();constructionSketchModels_.clear();centerlineSketchModels_.clear();constructionPlaneModels_.clear();constructionPlanes_.clear();finishedProfileModels_.clear();finishedSketches_.clear();
    modelRevision_=std::numeric_limits<quint64>::max();sketchRevision_=modelRevision_;constructionPlaneRevision_=modelRevision_;
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
        // A weaker world-space underside fill keeps internal roofs and tube
        // rims readable when inspecting a bottom-facing sketch or solid.
        Handle(V3d_DirectionalLight) fill=new V3d_DirectionalLight(gp_Dir(0.5,-0.6,1),Quantity_NOC_WHITE,false);
        fill->SetIntensity(0.55f);
        viewer_->AddLight(key);viewer_->AddLight(fill);viewer_->AddLight(ambient);viewer_->SetLightOn();
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
        // End-on arrow cones obscure small planar details in orthographic views.
        axes_->SetDrawArrows(false);
        axes_->SetDatumPartColor(Prs3d_DatumParts_XAxis, Quantity_Color(0.60,0.18,0.16,Quantity_TOC_RGB));
        axes_->SetDatumPartColor(Prs3d_DatumParts_YAxis, Quantity_Color(0.18,0.42,0.24,Quantity_TOC_RGB));
        axes_->SetDatumPartColor(Prs3d_DatumParts_ZAxis, Quantity_Color(0.18,0.33,0.62,Quantity_TOC_RGB));
        axes_->SetTextColor(Quantity_Color(0.18,0.22,0.26,Quantity_TOC_RGB));
        context_->Display(axes_, false);
        context_->Deactivate(axes_); // Decoration, not selectable document geometry.
        view_->SetProj(V3d_XposYnegZpos);
        view_->SetSize(400);
        baseCamera_ = new Graphic3d_Camera(view_->Camera());
        viewCube_=new AIS_ViewCube();
        viewCube_->SetAutoStartAnimation(false);
        viewCube_->SetYup(false);
        viewCube_->SetDrawAxes(false);
        viewCube_->SetSize(80);
        viewCube_->SetBoxColor(Quantity_Color(.72,.82,.90,Quantity_TOC_sRGB));
        viewCube_->SetTextColor(Quantity_Color(.10,.22,.31,Quantity_TOC_sRGB));
        const auto hoverStyle=viewCube_->DynamicHilightAttributes();
        if(!hoverStyle.IsNull() && !hoverStyle->ShadingAspect().IsNull())
            hoverStyle->ShadingAspect()->SetColor(Quantity_Color(.19,.51,.87,Quantity_TOC_sRGB));
        viewCube_->SetFontHeight(11);
        viewCube_->SetBoxSideLabel(V3d_Xpos,"RIGHT");
        viewCube_->SetBoxSideLabel(V3d_Xneg,"LEFT");
        viewCube_->SetBoxSideLabel(V3d_Yneg,"FRONT");
        viewCube_->SetBoxSideLabel(V3d_Ypos,"BACK");
        viewCube_->SetBoxSideLabel(V3d_Zpos,"TOP");
        viewCube_->SetBoxSideLabel(V3d_Zneg,"BOTTOM");
        // Keep the cube fixed to the viewport and orthographic even when the
        // model camera uses perspective projection.
        viewCube_->SetTransformPersistence(new Graphic3d_TransformPers(
            Graphic3d_TMF_TriedronPers | Graphic3d_TMF_OrthoPers,
            Aspect_TOTP_RIGHT_UPPER,Graphic3d_Vec2i(85,85)));
        viewCube_->SetZLayer(Graphic3d_ZLayerId_Topmost);
        context_->Display(viewCube_,false);
        context_->Activate(viewCube_);
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
    if(cubeControls_){cubeControls_->move(width()-185,38);cubeControls_->raise();}
    if(cubeUpLabel_){cubeUpLabel_->move(width()-185,125);cubeUpLabel_->raise();}
    if(cubeAccessoryToggle_){cubeAccessoryToggle_->move(width()-105,6);cubeAccessoryToggle_->raise();}
    if (!view_.IsNull()) { sync(); QTimer::singleShot(0, this, [this] { sync(); }); }
}
void ReferenceView::sync() {
    if (view_.IsNull()) return;
    view_->ChangeRenderingParams().Resolution = qRound(72 * devicePixelRatioF());
    view_->SetCamera(new Graphic3d_Camera(baseCamera_));
    // Resize after installing the camera so its aspect matches this drawable.
    view_->MustBeResized();
    const auto camera=view_->Camera();
    if(orientation=="custom") {
        view_->Camera()->SetDirection(customDirection_);
        view_->Camera()->SetUp(customUp_);
        view_->Camera()->OrthogonalizeUp();
    } else if(orientation=="face" && !sketchFrame_.isEmpty()) {
        const auto n=sketchFrame_.value("normal").toArray(),v=sketchFrame_.value("vAxis").toArray();
        view_->Camera()->SetDirection(gp_Dir(-n[0].toDouble(),-n[1].toDouble(),-n[2].toDouble()));
        view_->Camera()->SetUp(gp_Dir(v[0].toDouble(),v[1].toDouble(),v[2].toDouble()));
        view_->Camera()->OrthogonalizeUp();
    } else {
        view_->SetProj(orientation == "top" ? V3d_Zpos : orientation == "bottom" ? V3d_Zneg : orientation == "front" ? V3d_Yneg : orientation == "back" ? V3d_Ypos : orientation == "right" ? V3d_Xpos : orientation == "left" ? V3d_Xneg : orientation == "rearIsometric" ? V3d_XposYposZpos : V3d_XposYnegZpos);
        camera->SetDirection(conventionDirection(camera->Direction()));
        camera->SetUp(conventionDirection(camera->Up()));
        camera->OrthogonalizeUp();
    }
    // Move Eye/Center together: SetCenter alone changes the view direction,
    // while SetProj preserves the origin's screen offset rather than Center.
    // The hybrid mode is orthographic only when the actual view direction is
    // within 0.5 degrees of a principal face, including rolled/custom poses.
    const auto direction=camera->Direction();
    const double faceAlignment=std::max({std::abs(direction.Dot(conventionRight_)),
        std::abs(direction.Dot(conventionBack_)),std::abs(direction.Dot(conventionUp_))});
    const bool faceView=faceAlignment>=std::cos(0.5*std::numbers::pi/180.0);
    const bool perspective=projectionPolicy_=="perspective" || (projectionPolicy_=="hybrid" && !faceView);
    camera->SetProjectionType(perspective ? Graphic3d_Camera::Projection_Perspective : Graphic3d_Camera::Projection_Orthographic);
    if(perspective)camera->SetFOVy(45.0);
    camera->MoveEyeTo(gp_Pnt(viewCenter_.XYZ()-camera->Direction().XYZ()*camera->Distance()));
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
    if(!moveCueState_.isEmpty())showMoveCue(moveCueState_["inputs"].toObject());
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
    if(!faceNormalsState_.isEmpty())refreshFaceNormals();
    if(!extrusionCueState_.isEmpty())refreshExtrusionCue();
    if(!revolveCueState_.isEmpty())refreshRevolveCue();
    updateCubeControls();
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
void ReferenceView::setPreparedThreadMeshes(const std::vector<TopoDS_Shape>& shapes) {
    preparedThreadMeshes_=shapes;
    for(const auto& shape:preparedThreadMeshes_)traceThreadMesh("registered",shape);
}
bool ReferenceView::preservePreparedThreadMesh(const Handle(AIS_Shape)& presentation) const {
    const auto& shape=presentation->Shape();
    for(const auto& prepared:preparedThreadMeshes_)if(shape.IsPartner(prepared)) {
        auto drawer=presentation->Attributes();
        StdPrs_ToolTriangulatedShape::GetDeflection(shape,drawer);
        if(!StdPrs_ToolTriangulatedShape::IsTessellated(shape,drawer))return false;
        drawer->SetAutoTriangulation(false);
        return true;
    }
    return false;
}
void ReferenceView::displayBodies(const std::vector<EvaluatedBody>& bodies) {
    if(context_.IsNull())return;
    const bool trace=qEnvironmentVariableIsSet("ALLOY_TRACE_ASSEMBLY_SOLVES");
    QElapsedTimer timer;if(trace)timer.start();
    clearZebraPresentation();
    for(const auto& model:models_)context_->Remove(model,false);
    models_.clear();modelReferences_.clear();bodyVisibility_.clear();
    for(const auto& body:bodies) {
        Handle(AIS_Shape) model=new AIS_ColoredShape(body.shape);model->SetDisplayMode(AIS_Shaded);
        // Bound screen presentation detail separately from export accuracy.
        // The finer legacy angular mesh stalled small multi-fillet parts.
        model->SetOwnDeviationCoefficient(0.0005);
        model->SetOwnDeviationAngle(0.10);
        const bool preparedThread=preservePreparedThreadMesh(model);
        if(preparedThread)traceThreadMesh("committed_before_display",body.shape);
        const bool surface=body.summary.value("type")=="surface";
        model->SetColor(surface ? Quantity_Color(0.90,0.70,0.24,Quantity_TOC_RGB)
                                : Quantity_Color(0.67,0.72,0.76,Quantity_TOC_RGB));
        const double opacity=body.summary.value("opacity").toDouble(1.0);
        // Honor authored opacity. Forced surface alpha produces triangle-order
        // artifacts on curved sheets even when their mesh normals are correct.
        if(opacity<1.0)model->SetTransparency(1.0-opacity);
        model->Attributes()->SetFaceBoundaryDraw(true);
        model->Attributes()->SetFaceBoundaryAspect(new Prs3d_LineAspect(surface ? Quantity_Color(0.39,0.27,0.07,Quantity_TOC_RGB)
            : Quantity_Color(0.20,0.25,0.29,Quantity_TOC_RGB),Aspect_TOL_SOLID,1));
        // OCCT 7.9.3 whole-cylinder selection can invert prism bounds.
        // Face selection uses actual domains and maps to the same body ID.
        // Keep the inactive global owner for whole-body SetSelected cues.
        context_->Display(model,AIS_Shaded,0,false);
        context_->Deactivate(model,0);
        context_->Activate(model,AIS_Shape::SelectionMode(TopAbs_FACE));
        if(preparedThread)traceThreadMesh("committed_after_display",body.shape);
        if(!sectionClip_.IsNull())model->AddClipPlane(sectionClip_);
        const auto key=presentationBodyKey(body);
        models_[key]=model;bodyVisibility_[key]=body.visible;
        modelReferences_[key]={{"bodyId",body.bodyId},{"occurrenceId",body.occurrenceId},{"definitionId",body.definitionId}};
    }
    applyEditDisplay();
    if(trace)qInfo().noquote()<<"assembly_phase_ms displayBodies_inclusive"<<timer.nsecsElapsed()/1e6
        <<"bodies"<<bodies.size();
}
void ReferenceView::setSketches(const std::vector<EvaluatedSketch>& sketches,quint64 revision) {
    if(context_.IsNull() || sketchRevision_==revision)return;
    for(const auto& model:sketchModels_)context_->Remove(model,false);
    for(const auto& model:constructionSketchModels_)context_->Remove(model,false);
    for(const auto& model:centerlineSketchModels_)context_->Remove(model,false);
    for(const auto& model:finishedProfileModels_)context_->Remove(model,false);
    for(const auto& model:finishedPointModels_)context_->Remove(model,false);
    finishedPointModels_.clear();selectedFinishedPoint_={};
    sketchModels_.clear();constructionSketchModels_.clear();centerlineSketchModels_.clear();finishedProfileModels_.clear();finishedSketches_=sketches;
    for(const auto& sketch:sketches) {
        const auto strokes=splitSketchStrokes(sketch);
        if(!strokes.normal.IsNull()) {
            const auto model=sketchPresentation(strokes.normal);model->SetDisplayMode(AIS_WireFrame);
            if(sketch.plane=="face")model->SetZLayer(Graphic3d_ZLayerId_Topmost);
            if(sketch.summary["solver"].toObject()["dof"].toInt(-1)==0)model->SetColor(Quantity_Color(0.12,0.14,0.16,Quantity_TOC_sRGB));
            colorSolvedEntities(model,sketch.summary["entities"].toArray(),sketch.summary["solver"].toObject(),0,sketch.summary["projectedLines"].toArray());context_->Display(model,AIS_WireFrame,-1,false);context_->Deactivate(model);sketchModels_[sketch.featureId]=model;
        }
        if(!strokes.construction.IsNull()) {
            const auto model=constructionPresentation(strokes.construction,false,sketch.summary["solver"].toObject()["dof"].toInt(-1)==0);
            if(sketch.plane=="face")model->SetZLayer(Graphic3d_ZLayerId_Topmost);
            colorSolvedEntities(model,sketch.summary["entities"].toArray(),sketch.summary["solver"].toObject(),1,sketch.summary["projectedLines"].toArray());context_->Display(model,AIS_WireFrame,-1,false);context_->Deactivate(model);constructionSketchModels_[sketch.featureId]=model;
        }
        if(!strokes.centerline.IsNull()) {
            const auto model=centerlinePresentation(strokes.centerline,false,sketch.summary["solver"].toObject()["dof"].toInt(-1)==0);
            if(sketch.plane=="face")model->SetZLayer(Graphic3d_ZLayerId_Topmost);
            colorSolvedEntities(model,sketch.summary["entities"].toArray(),sketch.summary["solver"].toObject(),2,sketch.summary["projectedLines"].toArray());context_->Display(model,AIS_WireFrame,-1,false);context_->Deactivate(model);centerlineSketchModels_[sketch.featureId]=model;
        }
        for(const auto& entry:sketch.summary["standalonePoints"].toArray()) {
            const auto point=entry.toObject();const auto p=point["positionMm"].toArray();
            Handle(AIS_Point) marker=new AIS_Point(new Geom_CartesianPoint(gp_Pnt(p[0].toDouble(),p[1].toDouble(),p[2].toDouble())));
            marker->SetZLayer(Graphic3d_ZLayerId_Topmost);
            context_->Display(marker,0,-1,false);context_->Deactivate(marker);
            finishedPointModels_[point["id"].toString()]=marker;
        }
        for(const auto& profile:sketch.profiles) {
            BRepMesh_IncrementalMesh mesh(profile.face,0.005,false,0.1,false);
            Handle(AIS_Shape) fill=new AIS_Shape(profile.face);fill->SetDisplayMode(AIS_Shaded);fill->Attributes()->SetAutoTriangulation(false);
            fill->Attributes()->SetShadingModel(Graphic3d_TypeOfShadingModel_Unlit,true);
            if(sketch.plane=="face")fill->SetZLayer(Graphic3d_ZLayerId_Topmost);
            fill->SetTransparency(0.58);context_->Display(fill,AIS_Shaded,-1,false);context_->Deactivate(fill);
            finishedProfileModels_[profile.profileId]=fill;
        }
    }
    sketchRevision_=revision;applyEditDisplay();
}
void ReferenceView::setConstructionPoints(const std::vector<EvaluatedConstructionPoint>& points,quint64 revision) {
    if(context_.IsNull() || constructionPointRevision_==revision)return;
    for(const auto& model:constructionPointModels_)context_->Remove(model,false);constructionPointModels_.clear();
    for(const auto& point:points){if(!point.visible)continue;const auto xyz=point.point.value("positionMm").toArray();if(xyz.size()!=3)continue;
        Handle(AIS_Point) model=new AIS_Point(new Geom_CartesianPoint(gp_Pnt(xyz[0].toDouble(),xyz[1].toDouble(),xyz[2].toDouble())));
        const auto color=point.featureId=="point-preview"?Quantity_Color(0.10,0.34,0.84,Quantity_TOC_sRGB):Quantity_Color(0.90,0.48,0.08,Quantity_TOC_sRGB);
        model->Attributes()->SetPointAspect(new Prs3d_PointAspect(Aspect_TOM_PLUS,color,5.0*devicePixelRatioF()));model->SetZLayer(Graphic3d_ZLayerId_Topmost);
        context_->Display(model,0,-1,false);context_->Deactivate(model);constructionPointModels_[point.featureId]=model;
    }
    constructionPointRevision_=revision;context_->UpdateCurrentViewer();
}
void ReferenceView::setConstructionAxes(const std::vector<EvaluatedConstructionAxis>& axes,quint64 revision) {
    if(context_.IsNull() || constructionAxisRevision_==revision)return;
    for(const auto& model:constructionAxisModels_)context_->Remove(model,false);constructionAxisModels_.clear();
    for(const auto& axis:axes){if(!axis.visible)continue;const auto origin=axis.line.value("originMm").toArray(),direction=axis.line.value("direction").toArray();
        if(origin.size()!=3 || direction.size()!=3)continue;
        const gp_Pnt p(origin[0].toDouble(),origin[1].toDouble(),origin[2].toDouble());const gp_Vec d(direction[0].toDouble(),direction[1].toDouble(),direction[2].toDouble());
        Handle(AIS_Shape) model=new AIS_Shape(BRepBuilderAPI_MakeEdge(p.Translated(d.Multiplied(-15)),p.Translated(d.Multiplied(axis.line.value("sourceSpanMm").toDouble()+15))).Edge());
        model->SetColor(axis.featureId=="axis-preview"?Quantity_Color(0.10,0.34,0.84,Quantity_TOC_sRGB):Quantity_Color(0.90,0.48,0.08,Quantity_TOC_sRGB));
        model->SetWidth(2.5);context_->Display(model,AIS_WireFrame,-1,false);context_->Deactivate(model);constructionAxisModels_[axis.featureId]=model;
    }
    constructionAxisRevision_=revision;context_->UpdateCurrentViewer();
}
void ReferenceView::setConstructionPlanes(const std::vector<EvaluatedConstructionPlane>& planes,quint64 revision) {
    if(context_.IsNull() || constructionPlaneRevision_==revision)return;
    for(const auto& model:constructionPlaneModels_)context_->Remove(model,false);
    constructionPlaneModels_.clear();constructionPlanes_=planes;
    for(const auto& plane:planes) {
        const auto frame=plane.frame;const auto origin=frame["origin"].toArray(),u=frame["uAxis"].toArray(),v=frame["vAxis"].toArray();
        if(origin.size()!=3 || u.size()!=3 || v.size()!=3)continue;
        auto point=[&](double x,double y){return gp_Pnt(origin[0].toDouble()+x*u[0].toDouble()+y*v[0].toDouble(),
            origin[1].toDouble()+x*u[1].toDouble()+y*v[1].toDouble(),origin[2].toDouble()+x*u[2].toDouble()+y*v[2].toDouble());};
        const auto fit=plane.displayBoundsMm;
        const double left=fit.size()==4 ? fit[0].toDouble() : -30,right=fit.size()==4 ? fit[1].toDouble() : 30;
        const double lower=fit.size()==4 ? fit[2].toDouble() : -30,upper=fit.size()==4 ? fit[3].toDouble() : 30;
        BRepBuilderAPI_MakePolygon border;border.Add(point(left,lower));border.Add(point(right,lower));border.Add(point(right,upper));border.Add(point(left,upper));border.Close();
        Handle(AIS_Shape) model=new AIS_Shape(BRepBuilderAPI_MakeFace(border.Wire()).Face());
        model->SetColor(Quantity_Color(0.94,0.63,0.16,Quantity_TOC_sRGB));model->SetTransparency(0.84);
        model->Attributes()->SetFaceBoundaryDraw(true);
        model->Attributes()->SetFaceBoundaryAspect(new Prs3d_LineAspect(Quantity_Color(0.72,0.43,0.08,Quantity_TOC_sRGB),Aspect_TOL_SOLID,1.5));
        context_->Display(model,AIS_Shaded,plane.visible ? AIS_Shape::SelectionMode(TopAbs_FACE) : -1,false);
        if(!plane.visible)context_->Erase(model,false);
        constructionPlaneModels_[plane.featureId]=model;
    }
    constructionPlaneRevision_=revision;context_->UpdateCurrentViewer();
}
void ReferenceView::selectConstructionPlane(const QString& planeId) {
    if(context_.IsNull())return;
    for(auto it=constructionPlaneModels_.begin();it!=constructionPlaneModels_.end();++it) {
        it.value()->SetColor(it.key()==planeId ? Quantity_Color(0.10,0.34,0.84,Quantity_TOC_sRGB) : Quantity_Color(0.94,0.63,0.16,Quantity_TOC_sRGB));
        context_->Redisplay(it.value(),false);
    }
    context_->UpdateCurrentViewer();
}
QString ReferenceView::pickConstructionPlane(double x,double y) const {
    if(view_.IsNull() || x<0 || y<0 || x>=width() || y>=height())return {};
    const auto scale=windowScale();double px,py,pz,dx,dy,dz;
    view_->ConvertWithProj(qRound(x*scale.x()),qRound(y*scale.y()),px,py,pz,dx,dy,dz);
    const gp_Vec eye(px,py,pz),ray(dx,dy,dz);double nearest=std::numeric_limits<double>::infinity();QString hit;
    for(const auto& plane:constructionPlanes_)if(plane.visible) {
        const auto frame=plane.frame;const auto origin=frame["origin"].toArray(),normal=frame["normal"].toArray(),u=frame["uAxis"].toArray(),v=frame["vAxis"].toArray();
        const gp_Vec o(origin[0].toDouble(),origin[1].toDouble(),origin[2].toDouble());
        const gp_Vec n(normal[0].toDouble(),normal[1].toDouble(),normal[2].toDouble());
        const auto denominator=n.Dot(ray);if(std::abs(denominator)<1e-9)continue;
        const auto distance=n.Dot(o-eye)/denominator;if(distance<0 || distance>=nearest)continue;
        const auto local=eye+ray*distance-o;
        const gp_Vec xAxis(u[0].toDouble(),u[1].toDouble(),u[2].toDouble()),yAxis(v[0].toDouble(),v[1].toDouble(),v[2].toDouble());
        const auto fit=plane.displayBoundsMm;
        const double left=fit.size()==4 ? fit[0].toDouble() : -30,right=fit.size()==4 ? fit[1].toDouble() : 30;
        const double lower=fit.size()==4 ? fit[2].toDouble() : -30,upper=fit.size()==4 ? fit[3].toDouble() : 30;
        if(local.Dot(xAxis)<left || local.Dot(xAxis)>right || local.Dot(yAxis)<lower || local.Dot(yAxis)>upper)continue;
        nearest=distance;hit=plane.featureId;
    }
    return hit;
}
void ReferenceView::applyEditDisplay(bool redraw) {
    if(context_.IsNull())return;
    // Datum fills may be coplanar with Loft caps. Hide only their transient
    // presentation during a valid preview; authored visibility stays intact.
    for(const auto& plane:constructionPlanes_)if(constructionPlaneModels_.contains(plane.featureId)) {
        const auto model=constructionPlaneModels_.value(plane.featureId);
        if(plane.visible && (selectedLoftProfileIds_.isEmpty() || preview_.IsNull()))context_->Display(model,false);
        else context_->Erase(model,false);
    }
    for(const auto& sketch:finishedSketches_) {
        const bool visible=(sketch.visible || sketch.featureId==editSourceSketch_ || commandSketches_.contains(sketch.featureId)) && sketch.featureId!=draftSketchId_;
        if(sketchModels_.contains(sketch.featureId)) {
            if(visible)context_->Display(sketchModels_.value(sketch.featureId),false);
            else context_->Erase(sketchModels_.value(sketch.featureId),false);
        }
        if(constructionSketchModels_.contains(sketch.featureId)) {
            if(visible)context_->Display(constructionSketchModels_.value(sketch.featureId),false);
            else context_->Erase(constructionSketchModels_.value(sketch.featureId),false);
        }
        if(centerlineSketchModels_.contains(sketch.featureId)) {
            if(visible)context_->Display(centerlineSketchModels_.value(sketch.featureId),false);
            else context_->Erase(centerlineSketchModels_.value(sketch.featureId),false);
        }
        for(const auto& entry:sketch.summary["standalonePoints"].toArray()) {
            const auto point=entry.toObject();const auto marker=finishedPointModels_.value(point["id"].toString());if(marker.IsNull())continue;
            const bool selected=selectedFinishedPoint_["sketchId"]==sketch.featureId && selectedFinishedPoint_["pointId"]==point["pointId"];
            const auto color=selected ? Quantity_Color(0.98,0.43,0.08,Quantity_TOC_RGB)
                : solvedPoint(sketch.summary["solver"].toObject(),point["pointId"].toString()) ? Quantity_Color(0.12,0.14,0.16,Quantity_TOC_sRGB) : Quantity_Color(0.10,0.34,0.84,Quantity_TOC_RGB);
            marker->Attributes()->SetPointAspect(new Prs3d_PointAspect(Aspect_TOM_O_POINT,color,selected ? 3 : 2.5));
            if(visible)context_->Display(marker,false);else context_->Erase(marker,false);
            context_->Redisplay(marker,false);
        }
        for(const auto& profile:sketch.profiles)if(finishedProfileModels_.contains(profile.profileId)) {
            const auto fill=finishedProfileModels_.value(profile.profileId);
            fill->SetColor((selectedProfileIds_.contains(profile.profileId) && sketch.featureId==selectedProfileSketch_) || selectedLoftProfileIds_.contains(profile.profileId)
                ? Quantity_Color(0.17,0.48,0.90,Quantity_TOC_sRGB) : unselectedSketchProfileColor());
            // The extrusion base cap occupies this same plane. Keep its sketch
            // outline, but suppress the coplanar transparent profile fill.
            // Loft caps occupy the selected section planes. Keep blue section
            // outlines during preview without competing coplanar filled faces.
            context_->SetDisplayMode(fill,selectedLoftProfileIds_.contains(profile.profileId) && !preview_.IsNull() ? AIS_WireFrame : AIS_Shaded,false);
            // An extrusion handle at zero distance has no preview solid to
            // cover the selected cells. Keep their blue planar fill visible
            // until a real solid preview occupies the coplanar source face.
            const bool coveredBySolidPreview=!preview_.IsNull() && previewProfileIds_.contains(profile.profileId);
            // Diagnostic face shading must not compete with a coplanar source
            // profile fill. Keep sketch outlines; restore profile fills on Off.
            const bool coveredByZebra=!zebraModel_.IsNull() && profile.face.IsSame(zebraFace_);
            if(visible && !coveredBySolidPreview && !coveredByZebra)context_->Display(fill,false);else context_->Erase(fill,false);
        }
    }
    for(auto i=models_.begin();i!=models_.end();++i) {
        const auto bodyId=modelReferences_.value(i.key())["bodyId"].toString();
        const bool visible=bodyVisibility_.value(i.key()) || bodyId==combineTargetId_ || combineToolIds_.contains(bodyId);
        const auto reference=modelReferences_.value(i.key());
        const bool exportReplaced=!meshExportPreview_.IsNull() && reference.value("occurrenceId").toString()+":"+bodyId==meshExportPreviewInstance_;
        if(!visible || exportReplaced || hiddenBodyIds_.contains(bodyId) || bodyId==previewReplacedBody_)context_->Erase(i.value(),false);
        else context_->Display(i.value(),AIS_Shaded,AIS_Shape::SelectionMode(TopAbs_FACE),false);
    }
    if(redraw)context_->UpdateCurrentViewer();
}
void ReferenceView::setEditDisplay(const QString& sourceSketchId,const QJsonArray& hiddenBodyIds) {
    editSourceSketch_=sourceSketchId;hiddenBodyIds_=hiddenBodyIds;applyEditDisplay();
}
void ReferenceView::selectProfile(const QString& sketchId,const QString& profileId) {
    selectProfiles(sketchId,profileId.isEmpty() ? QJsonArray{} : QJsonArray{profileId});
}
void ReferenceView::selectLoftProfiles(const QJsonArray& references) {
    selectedLoftProfileIds_.clear();for(const auto& value:references)selectedLoftProfileIds_.insert(value.toObject()["profileId"].toString());applyEditDisplay();
}
void ReferenceView::selectProfiles(const QString& sketchId,const QJsonArray& profileIds) {
    selectedFinishedPoint_={};
    selectedProfileSketch_=sketchId;selectedProfileIds_=profileIds;
    selectedProfile_=profileIds.isEmpty() ? QString{} : profileIds.first().toString();applyEditDisplay();
}
void ReferenceView::selectFinishedPoint(const QJsonObject& reference) {
    selectedFinishedPoint_=reference;applyEditDisplay();
}
QJsonObject ReferenceView::pickMeasureSketchEdge(double x,double y) const {
    if(view_.IsNull())return {};const auto scale=windowScale();double best=7;
    struct Candidate {double distance;QJsonObject source;};std::vector<Candidate> candidates;
    for(const auto& sketch:finishedSketches_)if(sketch.visible)for(auto it=sketch.entityEdges.cbegin();it!=sketch.entityEdges.cend();++it){
        BRepAdaptor_Curve curve(it.value());if(curve.GetType()!=GeomAbs_Line)continue;
        const auto a=curve.Value(curve.FirstParameter()),b=curve.Value(curve.LastParameter());int ax,ay,bx,by;
        view_->Convert(a.X(),a.Y(),a.Z(),ax,ay);view_->Convert(b.X(),b.Y(),b.Z(),bx,by);
        const double px=ax/scale.x(),py=ay/scale.y(),dx=bx/scale.x()-px,dy=by/scale.y()-py;
        const double square=dx*dx+dy*dy;if(square<1)continue;
        const double t=std::clamp(((x-px)*dx+(y-py)*dy)/square,0.,1.);
        const double distance=std::hypot(x-px-t*dx,y-py-t*dy);
        if(distance<7){best=std::min(best,distance);candidates.push_back({distance,{{"kind","sketchEdge"},{"sketchId",sketch.featureId},{"entityId",it.key()}}});}
    }
    // Pixel projection/rounding makes coincident hits numerically unequal.
    // Refuse ties within half a logical pixel rather than depending on QHash order.
    QJsonObject hit;int matches=0;
    for(const auto& candidate:candidates)if(candidate.distance<=best+0.5){hit=candidate.source;++matches;}
    if(matches>1)return {{"ok",false},{"error",QJsonObject{{"code","measure_ambiguous"},{"message","Multiple sketch edges under the pointer; hide an overlapping source or choose exact source IDs"}}}};
    return hit;
}
QJsonObject ReferenceView::pickFinishedPoint(double x,double y) const {
    if(view_.IsNull() || x<0 || y<0 || x>=width() || y>=height())return {};
    const auto scale=windowScale();double best=8;QJsonObject hit;
    for(const auto& sketch:finishedSketches_) {
        if((!sketch.visible && sketch.featureId!=editSourceSketch_ && !commandSketches_.contains(sketch.featureId)) || sketch.featureId==draftSketchId_)continue;
        for(const auto& item:sketch.summary["standalonePoints"].toArray()) {
            const auto point=item.toObject();const auto p=point["positionMm"].toArray();int px,py;
            view_->Convert(p[0].toDouble(),p[1].toDouble(),p[2].toDouble(),px,py);
            const double distance=std::hypot(px/scale.x()-x,py/scale.y()-y);if(distance>best)continue;
            best=distance;hit={{"kind","sketchPoint"},{"sketchId",sketch.featureId},{"entityId",point["id"]},{"pointId",point["pointId"]},{"positionMm",p}};
        }
    }
    return hit;
}
QJsonObject ReferenceView::pickFinishedProfile(double x,double y) const {
    if(view_.IsNull() || x<0 || y<0 || x>=width() || y>=height())return {};
    const auto scale=windowScale();double px,py,pz,dx,dy,dz;
    view_->ConvertWithProj(qRound(x*scale.x()),qRound(y*scale.y()),px,py,pz,dx,dy,dz);
    const gp_Vec eye(px,py,pz),ray(dx,dy,dz);
    double nearest=std::numeric_limits<double>::infinity();QJsonObject hit;
    for(const auto& sketch:finishedSketches_) {
        if((!sketch.visible && sketch.featureId!=editSourceSketch_ && !commandSketches_.contains(sketch.featureId)) || sketch.featureId==draftSketchId_)continue;
        const auto frame=sketch.resolvedFrame.isEmpty() ? SketchGeometry::frame(sketch.plane) : sketch.resolvedFrame;
        const auto o=frame.value("origin").toArray(),n=frame.value("normal").toArray();
        const gp_Vec origin(o[0].toDouble(),o[1].toDouble(),o[2].toDouble());
        const gp_Vec normal(n[0].toDouble(),n[1].toDouble(),n[2].toDouble());
        const double denominator=normal.Dot(ray);if(std::abs(denominator)<1e-10)continue;
        const double distance=normal.Dot(origin-eye)/denominator;if(distance<0 || distance>=nearest)continue;
        const auto world=gp_Pnt((eye+ray*distance).XYZ());
        for(const auto& profile:sketch.profiles) {
            BRepClass_FaceClassifier classifier(profile.face,world,1e-7);
            if(classifier.State()==TopAbs_IN){nearest=distance;hit={{"kind","profile"},{"sketchId",sketch.featureId},{"profileId",profile.profileId}};}
        }
    }
    return hit;
}
gp_Lin ViewPickSnapshot::ray(double x,double y) const {
    const double nx=2.0*qRound(x*deviceScale.x())/physicalWidth-1.0;
    const double ny=2.0*(physicalHeight-1-qRound(y*deviceScale.y()))/physicalHeight-1.0;
    auto unproject=[&](double z){
        auto p=inverseProjection*Graphic3d_Vec4d(nx,ny,z,1.0);p=inverseOrientation*p;
        const auto inverseW=1.0/p.w();return gp_Pnt(p.x()*inverseW,p.y()*inverseW,p.z()*inverseW);
    };
    const auto origin=unproject(-1.0);return gp_Lin(origin,gp_Dir(gp_Vec(unproject(-11.0),origin)));
}
QPointF ViewPickSnapshot::project(const gp_Pnt& world) const {
    // Match the kernel camera's finite conversion guard as well as its order.
    const auto safe=[](double v){const double limit=1e15f;return std::abs(v)>limit?std::copysign(limit*.1f,v):v;};
    auto p=orientation*Graphic3d_Vec4d(safe(world.X()),safe(world.Y()),safe(world.Z()),1.0);p=projection*p;
    const auto inverseW=1.0/p.w();
    return {RealToInt((p.x()*inverseW+1)*.5*physicalWidth)/deviceScale.x(),
        RealToInt(physicalHeight-1-(p.y()*inverseW+1)*.5*physicalHeight)/deviceScale.y()};
}
std::shared_ptr<const ViewPickSnapshot> ReferenceView::capturePick(double x,double y) const {
    if(view_.IsNull() || x<0 || y<0 || x>=width() || y>=height())return {};
    auto pick=std::make_shared<ViewPickSnapshot>();pick->pixel={x,y};pick->deviceScale=windowScale();
    view_->Window()->Size(pick->physicalWidth,pick->physicalHeight);pick->logicalHeight=height();
    pick->orientation=view_->Camera()->OrientationMatrix();pick->projection=view_->Camera()->ProjectionMatrix();
    if(!pick->orientation.Inverted(pick->inverseOrientation) || !pick->projection.Inverted(pick->inverseProjection))return {};
    pick->cameraScale=view_->Camera()->Scale();return pick;
}
QJsonObject ReferenceView::pickSupportFace(double x,double y,const std::vector<EvaluatedBody>& bodies) const {
    const auto pick=capturePick(x,y);return pick ? pickSupportFace(*pick,bodies) : QJsonObject{};
}
QJsonObject ReferenceView::pickSupportFace(const ViewPickSnapshot& pick,const std::vector<EvaluatedBody>& bodies) {
    const auto ray=pick.ray(pick.pixel.x(),pick.pixel.y());
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
    for(const auto& face:hitBody->supportFaces)if(hitFace.IsSame(face.face)){
        QJsonObject result{{"producerFeatureId",face.producerFeatureId},{"bodyId",face.bodyId},{"faceId",face.faceId}};
        if(!hitBody->occurrenceId.isEmpty())result["occurrenceId"]=hitBody->occurrenceId;
        return result;
    }
    return {};
}
void ReferenceView::selectSupportFace(const QJsonObject& support,const TopoDS_Face& face,bool outline) {
    selectedSupportShape_=face;selectedSupportOutline_=outline;
    outline=outline || (zebraState_.value("visible").toBool() && zebraState_.value("source").toObject()==support);
    hoverSupportFace({});
    if(!context_.IsNull() && !selectedSupportFaceOverlay_.IsNull())context_->Remove(selectedSupportFaceOverlay_,false);
    selectedSupportFaceOverlay_.Nullify();selectedSupportFace_=support;
    if(!context_.IsNull() && !face.IsNull()) {
        selectedSupportFaceOverlay_=new AIS_Shape(face);
        selectedSupportFaceOverlay_->SetDisplayMode(outline ? AIS_WireFrame : AIS_Shaded);
        selectedSupportFaceOverlay_->SetColor(Quantity_Color(0.16,0.47,0.86,Quantity_TOC_sRGB));
        const bool curved=BRepAdaptor_Surface(face).GetType()!=GeomAbs_Plane;
        selectedSupportFaceOverlay_->SetTransparency(outline ? 0.0 : curved ? 0.12 : 0.48);
        selectedSupportFaceOverlay_->SetZLayer(Graphic3d_ZLayerId_Topmost);
        context_->Display(selectedSupportFaceOverlay_,outline ? AIS_WireFrame : AIS_Shaded,-1,false);
        context_->Deactivate(selectedSupportFaceOverlay_);
        context_->UpdateCurrentViewer();
    }
}
void ReferenceView::clearZebraPresentation() {
    if(zebraModel_.IsNull())return;
    ZebraPresentation::remove(zebraModel_,zebraFace_);
    if(!context_.IsNull() && context_->IsDisplayed(zebraModel_))context_->Redisplay(zebraModel_,false);
    zebraModel_.Nullify();zebraFace_.Nullify();
}
void ReferenceView::setZebra(const QJsonObject& state,const TopoDS_Face& face) {
    Handle(AIS_ColoredShape) target;
    if(state.value("enabled").toBool() && state.value("status")=="ready" && !face.IsNull()) {
        const auto source=state.value("source").toObject();
        for(auto it=models_.cbegin();it!=models_.cend();++it) {
            const auto ref=modelReferences_.value(it.key());
            if(ref.value("bodyId")!=source.value("bodyId") ||
                (source.contains("occurrenceId") && ref.value("occurrenceId").toString()!=source.value("occurrenceId").toString()))continue;
            if(!context_->IsDisplayed(it.value()))continue;
            target=Handle(AIS_ColoredShape)::DownCast(it.value());break;
        }
    }
    const bool same=target==zebraModel_ && (face.IsNull() ? zebraFace_.IsNull() : face.IsSame(zebraFace_));
    if(!same) {
        clearZebraPresentation();
        if(!target.IsNull()) {
            ZebraPresentation::apply(target,face);zebraModel_=target;zebraFace_=face;
            context_->Redisplay(target,false);
        }
    }
    auto next=state;next["visible"]=!zebraModel_.IsNull();
    const bool selectionChanged=zebraState_.value("visible")!=next.value("visible") || zebraState_.value("source")!=next.value("source");
    const bool displayChanged=zebraState_.value("visible")!=next.value("visible");
    zebraState_=next;
    if(displayChanged || !same)applyEditDisplay(false);
    if(selectionChanged && !selectedSupportShape_.IsNull())selectSupportFace(selectedSupportFace_,selectedSupportShape_,selectedSupportOutline_);
    if(!context_.IsNull())context_->UpdateCurrentViewer();
}
void ReferenceView::setFaceNormals(const QJsonObject& state) {
    auto prior=faceNormalsState_;prior.remove("glyph");prior["visible"]=false;
    if(prior==state)return;
    faceNormalsState_=state;faceNormalsRenderKey_={};refreshFaceNormals();
    if(!context_.IsNull())context_->UpdateCurrentViewer();
}
void ReferenceView::refreshFaceNormals() {
    if(context_.IsNull() || view_.IsNull())return;
    const auto clear=[&]{for(const auto& glyph:faceNormalGlyphs_)context_->Remove(glyph,false);faceNormalGlyphs_.clear();faceNormalsRenderKey_={};faceNormalsState_["visible"]=false;faceNormalsState_.remove("glyph");};
    if(!faceNormalsState_.value("enabled").toBool() || faceNormalsState_.value("status")!="ready"){clear();return;}
    const auto normal=faceNormalsState_.value("normal").toObject();
    const auto point=normal.value("pointMm").toArray(),vector=normal.value("unitNormal").toArray();
    const gp_Pnt tail(point[0].toDouble(),point[1].toDouble(),point[2].toDouble());
    const gp_Dir direction(vector[0].toDouble(),vector[1].toDouble(),vector[2].toDouble());
    const gp_Vec right=view_->Camera()->Direction().Crossed(view_->Camera()->Up());
    int x0,y0,x1,y1;view_->Convert(tail.X(),tail.Y(),tail.Z(),x0,y0);
    const auto tangent=tail.Translated(right);view_->Convert(tangent.X(),tangent.Y(),tangent.Z(),x1,y1);
    const auto scale=windowScale();
    const double pxPerMm=std::hypot((x1-x0)/scale.x(),(y1-y0)/scale.y());
    if(pxPerMm<1e-8){clear();return;}
    const auto along=tail.Translated(gp_Vec(direction));view_->Convert(along.X(),along.Y(),along.Z(),x1,y1);
    const double normalPxPerMm=std::hypot((x1-x0)/scale.x(),(y1-y0)/scale.y());
    const double cap=normal.value("faceDiagonalMm").toDouble()*.35;
    const double length=std::min(cap,48.0/std::max(normalPxPerMm,pxPerMm*.15));
    if(length<1e-8){clear();return;}
    const QJsonObject renderKey{{"normal",normal},{"length",length},{"pxPerMm",pxPerMm},{"x",x0},{"y",y0},{"nx",x1},{"ny",y1},{"dpr",devicePixelRatioF()},{"scaleX",scale.x()},{"scaleY",scale.y()}};
    if(renderKey==faceNormalsRenderKey_ && !faceNormalGlyphs_.empty())return;
    clear();faceNormalsRenderKey_=renderKey;
    const double head=length*.25,radius=std::min(length*.045,1.7/pxPerMm);
    const double headRadius=std::min(length*.13,5.0/pxPerMm);
    const auto tip=tail.Translated(gp_Vec(direction)*length);
    BRep_Builder builder;TopoDS_Compound arrow;builder.MakeCompound(arrow);
    builder.Add(arrow,BRepPrimAPI_MakeCylinder(gp_Ax2(tail,direction),radius,length-head).Shape());
    builder.Add(arrow,BRepPrimAPI_MakeCone(gp_Ax2(tip.Translated(gp_Vec(direction)*(-head)),direction),headRadius,0,head).Shape());
    Handle(AIS_Shape) glyph=new AIS_Shape(arrow);
    glyph->SetColor(Quantity_Color(1.0,.77,.08,Quantity_TOC_sRGB));
    glyph->Attributes()->SetShadingModel(Graphic3d_TOSM_UNLIT);
    glyph->Attributes()->SetFaceBoundaryDraw(true);
    glyph->Attributes()->SetFaceBoundaryAspect(new Prs3d_LineAspect(Quantity_Color(.12,.14,.18,Quantity_TOC_sRGB),Aspect_TOL_SOLID,1.5*devicePixelRatioF()));
    glyph->SetOwnDeviationCoefficient(.0001);glyph->SetZLayer(Graphic3d_ZLayerId_Topmost);
    context_->Display(glyph,AIS_Shaded,-1,false);context_->Deactivate(glyph);faceNormalGlyphs_.push_back(glyph);
    Handle(AIS_Point) anchor=new AIS_Point(new Geom_CartesianPoint(tail));
    anchor->Attributes()->SetPointAspect(new Prs3d_PointAspect(Aspect_TOM_O_POINT,Quantity_Color(.12,.14,.18,Quantity_TOC_sRGB),4*devicePixelRatioF()));
    anchor->SetZLayer(Graphic3d_ZLayerId_Topmost);context_->Display(anchor,0,-1,false);context_->Deactivate(anchor);faceNormalGlyphs_.push_back(anchor);
    view_->Convert(tip.X(),tip.Y(),tip.Z(),x1,y1);
    const double projected=std::hypot((x1-x0)/scale.x(),(y1-y0)/scale.y());
    faceNormalsState_["visible"]=true;
    faceNormalsState_["glyph"]=QJsonObject{{"tailMm",point},{"tipMm",QJsonArray{tip.X(),tip.Y(),tip.Z()}},{"lengthMm",length},
        {"projectedLengthLogicalPx",projected},{"targetLengthLogicalPx",48},{"worldCapMm",cap},{"foreshortened",projected<12},
        {"tailScreen",QJsonObject{{"x",x0/scale.x()},{"y",y0/scale.y()}}},{"tipScreen",QJsonObject{{"x",x1/scale.x()},{"y",y1/scale.y()}}},
        {"pickable",false},{"color","#ffc414"},{"outline","#1f242e"}};
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
    cancelSlotPointDrag();
    if(context_.IsNull())return;
    const bool insetPresentation=sketch.summary.value("loopOffsets").toArray().size()==1
        && sketch.summary.value("loopOffsets").toArray().first().toObject().value("kind")=="planarInsetV1";
    for(const auto& model:profileModels_)context_->Remove(model,false);profileModels_.clear();draftProfiles_=sketch.profiles;
    for(const auto& profile:sketch.profiles) {
        BRepMesh_IncrementalMesh mesh(profile.face,0.005,false,0.1,false);
        Handle(AIS_Shape) fill=new AIS_Shape(profile.face);fill->SetDisplayMode(AIS_Shaded);fill->Attributes()->SetAutoTriangulation(false);
        fill->Attributes()->SetShadingModel(Graphic3d_TypeOfShadingModel_Unlit,true);
        if(sketch.plane=="face")fill->SetZLayer(Graphic3d_ZLayerId_Topmost);
        fill->SetColor(insetPresentation ? Quantity_Color(0.30,0.66,0.94,Quantity_TOC_sRGB)
            : unselectedSketchProfileColor());fill->SetTransparency(insetPresentation ? 0.48 : 0.58);
        context_->Display(fill,AIS_Shaded,-1,false);context_->Deactivate(fill);profileModels_[profile.profileId]=fill;
    }
    if(!sketchDraft_.IsNull())context_->Remove(sketchDraft_,false);
    if(!sketchConstructionDraft_.IsNull())context_->Remove(sketchConstructionDraft_,false);
    if(!sketchCenterlineDraft_.IsNull())context_->Remove(sketchCenterlineDraft_,false);
    if(!projectedDraft_.IsNull())context_->Remove(projectedDraft_,false);
    if(draftSketchId_!=sketch.featureId && sketchModels_.contains(draftSketchId_))context_->Display(sketchModels_.value(draftSketchId_),false);
    if(draftSketchId_!=sketch.featureId && constructionSketchModels_.contains(draftSketchId_))context_->Display(constructionSketchModels_.value(draftSketchId_),false);
    if(draftSketchId_!=sketch.featureId && centerlineSketchModels_.contains(draftSketchId_))context_->Display(centerlineSketchModels_.value(draftSketchId_),false);
    draftSketchId_=sketch.featureId;sketchDraft_.Nullify();sketchConstructionDraft_.Nullify();sketchCenterlineDraft_.Nullify();projectedDraft_.Nullify();
    projectedLines_=sketch.summary.value("projectedLines").toArray();
    projectedCircles_=sketch.summary.value("projectedCircles").toArray();
    supportPoints_=sketch.summary.value("availableSupportPoints").toArray();constructionDraftLineIds_={};centerlineDraftLineIds_={};
    if(sketchModels_.contains(draftSketchId_))context_->Erase(sketchModels_.value(draftSketchId_),false);
    if(constructionSketchModels_.contains(draftSketchId_))context_->Erase(constructionSketchModels_.value(draftSketchId_),false);
    if(centerlineSketchModels_.contains(draftSketchId_))context_->Erase(centerlineSketchModels_.value(draftSketchId_),false);
    draftEntityShapes_.clear();
    if(!sketch.shape.IsNull()){
        TopoDS_Iterator child(sketch.shape);
        for(const auto& row:sketch.summary.value("entities").toArray()){
            if(!child.More())break;draftEntityShapes_.insert(row.toObject().value("id").toString(),child.Value());child.Next();
        }
    }
    const auto strokes=splitSketchStrokes(sketch);
    draftNormalEdgeCount_=0;
    if(!strokes.normal.IsNull())for(TopExp_Explorer edge(strokes.normal,TopAbs_EDGE);edge.More();edge.Next())++draftNormalEdgeCount_;
    if(!strokes.normal.IsNull()) {
        sketchDraft_=sketchPresentation(strokes.normal);
        if(sketch.plane=="face")sketchDraft_->SetZLayer(Graphic3d_ZLayerId_Topmost);
        if(insetPresentation){sketchDraft_->SetColor(Quantity_Color(0.08,0.35,0.86,Quantity_TOC_sRGB));sketchDraft_->SetWidth(3.2*devicePixelRatioF());}
        else if(sketch.summary["solver"].toObject()["dof"].toInt(-1)==0)sketchDraft_->SetColor(Quantity_Color(0.12,0.14,0.16,Quantity_TOC_sRGB));
        context_->Display(sketchDraft_,AIS_WireFrame,-1,false);context_->Deactivate(sketchDraft_);
    }
    if(!strokes.construction.IsNull()) {
        sketchConstructionDraft_=constructionPresentation(strokes.construction,false,sketch.summary["solver"].toObject()["dof"].toInt(-1)==0);
        if(sketch.plane=="face")sketchConstructionDraft_->SetZLayer(Graphic3d_ZLayerId_Topmost);
        context_->Display(sketchConstructionDraft_,AIS_WireFrame,-1,false);context_->Deactivate(sketchConstructionDraft_);
        for(const auto& entry:sketch.summary.value("entities").toArray()) {
            const auto entity=entry.toObject();
            if(entity.value("construction").toBool() && entity.value("type")=="line"
                && !std::any_of(projectedLines_.begin(),projectedLines_.end(),[&](const QJsonValue& value){return value.toObject().value("id")==entity.value("id");}))
                constructionDraftLineIds_.append(entity.value("id"));
        }
    }
    if(!strokes.centerline.IsNull()) {
        sketchCenterlineDraft_=centerlinePresentation(strokes.centerline,false,sketch.summary["solver"].toObject()["dof"].toInt(-1)==0);
        if(sketch.plane=="face")sketchCenterlineDraft_->SetZLayer(Graphic3d_ZLayerId_Topmost);
        context_->Display(sketchCenterlineDraft_,AIS_WireFrame,-1,false);context_->Deactivate(sketchCenterlineDraft_);
        for(const auto& entry:sketch.summary.value("entities").toArray()) {
            const auto entity=entry.toObject();
            if(entity.value("centerline").toBool() && entity.value("type")=="line"
                && !std::any_of(projectedLines_.begin(),projectedLines_.end(),[&](const QJsonValue& value){return value.toObject().value("id")==entity.value("id");}))
                centerlineDraftLineIds_.append(entity.value("id"));
        }
    }
    if(!projectedLines_.isEmpty() || !projectedCircles_.isEmpty()) {
        TopoDS_Compound compound;BRep_Builder builder;builder.MakeCompound(compound);
        for(const auto& value:projectedLines_) {
            const auto line=value.toObject();const auto a=sketchWorld(sketch.plane,line["u1Mm"].toDouble(),line["v1Mm"].toDouble());
            const auto b=sketchWorld(sketch.plane,line["u2Mm"].toDouble(),line["v2Mm"].toDouble());
            if(a.Distance(b)>1e-9)builder.Add(compound,BRepBuilderAPI_MakeEdge(a,b).Edge());
        }
        for(const auto& value:projectedCircles_) {
            const auto circle=value.toObject();const auto axis=sketch.resolvedFrame["normal"].toArray();
            const auto center=sketchWorld(sketch.plane,circle["uMm"].toDouble(),circle["vMm"].toDouble());
            builder.Add(compound,BRepBuilderAPI_MakeEdge(gp_Circ(gp_Ax2(center,gp_Dir(axis[0].toDouble(),axis[1].toDouble(),axis[2].toDouble())),circle["radiusMm"].toDouble())).Edge());
        }
        projectedDraft_=sketchPresentation(compound);
        projectedDraft_->SetColor(Quantity_Color(0.55,0.22,0.72,Quantity_TOC_sRGB));projectedDraft_->SetWidth(2.2*devicePixelRatioF());
        projectedDraft_->SetZLayer(Graphic3d_ZLayerId_Topmost);
        context_->Display(projectedDraft_,AIS_WireFrame,-1,false);context_->Deactivate(projectedDraft_);
    }
    applyEditDisplay();
}
void ReferenceView::clearSketchDraft() {
    if(sketchSelectOtherTimer_)sketchSelectOtherTimer_->stop();
    sketchSelectOtherPending_=false;sketchSelectOtherOpened_=false;
    draftEntityShapes_.clear();
    cancelSlotPointDrag();
    clearSketchDecorations();
    decorationFeature_={};decorationPending_={};annotationLayouts_={};draggedAnnotation_.clear();annotationAutoDrag_=false;
    if(!context_.IsNull()) {
        for(const auto& model:profileModels_)context_->Remove(model,false);
        if(!sketchDraft_.IsNull())context_->Remove(sketchDraft_,false);
        if(!sketchConstructionDraft_.IsNull())context_->Remove(sketchConstructionDraft_,false);
        if(!sketchCenterlineDraft_.IsNull())context_->Remove(sketchCenterlineDraft_,false);
        if(!projectedDraft_.IsNull())context_->Remove(projectedDraft_,false);
        if(sketchModels_.contains(draftSketchId_))context_->Display(sketchModels_.value(draftSketchId_),false);
        if(constructionSketchModels_.contains(draftSketchId_))context_->Display(constructionSketchModels_.value(draftSketchId_),false);
        if(centerlineSketchModels_.contains(draftSketchId_))context_->Display(centerlineSketchModels_.value(draftSketchId_),false);
        context_->UpdateCurrentViewer();
    }
    previewSketchSelectionCandidate();
    profileModels_.clear();draftProfiles_.clear();
    sketchDraft_.Nullify();sketchConstructionDraft_.Nullify();sketchCenterlineDraft_.Nullify();projectedDraft_.Nullify();draftNormalEdgeCount_=0;projectedLines_={};projectedCircles_={};supportPoints_={};constructionDraftLineIds_={};centerlineDraftLineIds_={};draftSketchId_.clear();clearSketchCursor();applyEditDisplay();
}
QJsonObject ReferenceView::pickSketch(const QJsonObject& sourceFeature,double x,double y,const QString& filter,const QJsonObject& resolvedFrame,QJsonArray* candidates) const {
    if(view_.IsNull() || x<0 || y<0 || x>=width() || y>=height())return {};
    const auto feature=SketchGeometry::chamferPresentation(sourceFeature);
    const auto parameters=feature["parameters"].toObject();const auto plane=parameters["plane"].toString();
    const auto frame=resolvedFrame.isEmpty() ? frameFor(plane) : resolvedFrame;
    auto world=[&](double u,double v){const auto o=frame["origin"].toArray(),a=frame["uAxis"].toArray(),b=frame["vAxis"].toArray();return gp_Pnt(o[0].toDouble()+u*a[0].toDouble()+v*b[0].toDouble(),o[1].toDouble()+u*a[1].toDouble()+v*b[1].toDouble(),o[2].toDouble()+u*a[2].toDouble()+v*b[2].toDouble());};
    QMap<QString,QPointF> points;
    double best=8;QString picked;QJsonObject pickedProjectedPoint;
    for(const auto& entry:parameters["points"].toArray()) {
        const auto point=entry.toObject();const auto projection=projectSketchPoint(point["uMm"].toDouble(),point["vMm"].toDouble(),plane,resolvedFrame);
        const QPointF p(projection["x"].toDouble(),projection["y"].toDouble());points[point["id"].toString()]=p;
        const double distance=std::hypot(p.x()-x,p.y()-y);
        if(sketchPointVisible(point.value("id").toString()) && filter!="entity" && filter!="line" && distance<=best){best=distance;picked=point["id"].toString();pickedProjectedPoint={};}
    }
    if(filter!="entity" && filter!="line")if(resolvedFrame.isEmpty())for(const auto& value:projectedLines_) {
        const auto line=value.toObject();if(!sketchEntityVisible(line.value("id").toString()))continue;
        for(const bool end:{false,true}) {
            const auto id=line[end ? "endPointId" : "startPointId"].toString();if(!sketchPointVisible(id))continue;
            const auto projection=projectSketchPoint(line[end ? "u2Mm" : "u1Mm"].toDouble(),line[end ? "v2Mm" : "v1Mm"].toDouble(),plane,resolvedFrame);
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
    if(filter!="entity" && filter!="line")if(resolvedFrame.isEmpty())for(const auto& value:supportPoints_) {
        const auto point=value.toObject();const auto id=point["id"].toString();if(!sketchPointVisible(id))continue;
        const auto projection=projectSketchPoint(point["uMm"].toDouble(),point["vMm"].toDouble(),plane,resolvedFrame);
        const double distance=std::hypot(projection["x"].toDouble()-x,projection["y"].toDouble()-y);
        if(distance<best-1e-6 || (picked.isEmpty() && distance<=best)
            || (!pickedProjectedPoint.isEmpty() && std::abs(distance-best)<=1e-6 && id<picked)) {
            best=distance;picked=id;
            pickedProjectedPoint={{"source",point["source"]},{"supportPoint",true},{"retained",point["retained"]}};
        }
    }
    if(!picked.isEmpty()){
        QJsonObject result{{"kind","point"},{"pointId",picked},{"distancePixels",best}};
        for(const auto& entry:parameters["entities"].toArray())if(entry.toObject()["type"]=="point" && entry.toObject()["pointId"]==picked)result["entityId"]=entry.toObject()["id"];
        if(!pickedProjectedPoint.isEmpty()){
            result["projected"]=true;
            for(auto it=pickedProjectedPoint.begin();it!=pickedProjectedPoint.end();++it)result[it.key()]=it.value();
        }
        return result;
    }
    if(filter=="point")return {};
    best=6;QJsonObject pickedProjection;
    const auto planePoint=sketchPoint(x,y,plane,resolvedFrame);const auto hitWorld=world(planePoint["uMm"].toDouble(),planePoint["vMm"].toDouble());
    for(const auto& entry:parameters["entities"].toArray()) {
        const auto entity=entry.toObject();if(!sketchEntityVisible(entity.value("id").toString()))continue;if(entity["type"]=="point")continue;
        if(filter=="line" && entity["type"]!="line")continue;QPointF nearest;
        if(entity["type"]=="line") {
            const auto start=points.value(entity["startPointId"].toString()),end=points.value(entity["endPointId"].toString());
            const auto delta=end-start;const double length2=delta.x()*delta.x()+delta.y()*delta.y();if(length2<1e-12)continue;
            const double t=std::clamp(((x-start.x())*delta.x()+(y-start.y())*delta.y())/length2,0.0,1.0);nearest=start+delta*t;
        } else {
            const auto edge=SketchGeometry::analyticEdge(parameters,entity,frame);double first,last;const auto curve=BRep_Tool::Curve(edge,first,last);
            GeomAPI_ProjectPointOnCurve projection(hitWorld,curve,first,last);if(projection.NbPoints()==0)continue;
            const auto p=projection.NearestPoint();int px,py;view_->Convert(p.X(),p.Y(),p.Z(),px,py);const auto scale=windowScale();nearest={px/scale.x(),py/scale.y()};
        }
        const double distance=std::hypot(nearest.x()-x,nearest.y()-y);
        if(candidates && distance<=6)candidates->append(QJsonObject{{"kind","entity"},{"entityId",entity["id"]},{"type",entity["type"]},{"projected",false},{"distancePixels",distance}});
        if(distance<=best){best=distance;picked=entity["id"].toString();pickedProjection={};}
    }
    if(resolvedFrame.isEmpty())for(const auto& value:projectedLines_) {
        const auto line=value.toObject();if(!sketchEntityVisible(line.value("id").toString()))continue;
        const auto first=projectSketchPoint(line["u1Mm"].toDouble(),line["v1Mm"].toDouble(),plane,resolvedFrame);
        const auto last=projectSketchPoint(line["u2Mm"].toDouble(),line["v2Mm"].toDouble(),plane,resolvedFrame);
        const QPointF start(first["x"].toDouble(),first["y"].toDouble()),end(last["x"].toDouble(),last["y"].toDouble());
        const auto delta=end-start;const double length2=delta.x()*delta.x()+delta.y()*delta.y();if(length2<1e-12)continue;
        const double t=std::clamp(((x-start.x())*delta.x()+(y-start.y())*delta.y())/length2,0.0,1.0);
        const auto nearest=start+delta*t;const double distance=std::hypot(nearest.x()-x,nearest.y()-y);
        if(candidates && distance<=6)candidates->append(QJsonObject{{"kind","entity"},{"entityId",line["id"]},{"type","line"},{"projected",true},{"source",line["source"]},{"distancePixels",distance}});
        if(distance<=best){best=distance;picked=line["id"].toString();pickedProjection=line;}
    }
    if(filter!="line")for(const auto& value:projectedCircles_) {
        const auto circle=value.toObject();if(!sketchEntityVisible(circle.value("id").toString()))continue;const auto center=projectSketchPoint(circle["uMm"].toDouble(),circle["vMm"].toDouble(),plane,resolvedFrame);
        const auto rim=projectSketchPoint(circle["uMm"].toDouble()+circle["radiusMm"].toDouble(),circle["vMm"].toDouble(),plane,resolvedFrame);
        const double radius=std::hypot(rim["x"].toDouble()-center["x"].toDouble(),rim["y"].toDouble()-center["y"].toDouble());
        const double distance=std::abs(std::hypot(x-center["x"].toDouble(),y-center["y"].toDouble())-radius);
        if(candidates && distance<=6)candidates->append(QJsonObject{{"kind","entity"},{"entityId",circle["id"]},{"type","circle"},{"projected",true},{"source",circle["source"]},{"distancePixels",distance}});
        if(distance<=best){best=distance;picked=circle["id"].toString();pickedProjection=circle;}
    }
    if(!picked.isEmpty()){
        QJsonObject result{{"kind","entity"},{"entityId",picked},{"distancePixels",best}};
        if(!pickedProjection.isEmpty()){result["projected"]=true;result["source"]=pickedProjection["source"];}
        return result;
    }
    if(!filter.isEmpty())return {};
    const auto hit=sketchPoint(x,y,parameters.value("plane").toString(),resolvedFrame);
    const auto profileWorld=world(hit.value("uMm").toDouble(),hit.value("vMm").toDouble());
    const EvaluatedProfile* selected=nullptr;
    if(sketchDisplayOptions_.value("showProfile").toBool(true) && !sketchDisplayOptions_.value("hideFullyConstrained").toBool())for(const auto& profile:draftProfiles_) {
        BRepClass_FaceClassifier classifier(profile.face,profileWorld,1e-7);
        if(classifier.State()!=TopAbs_IN)continue;
        if(!parameters.contains("loopOffsets"))return {{"kind","profile"},{"profileId",profile.profileId}};
        // Nested generated owners retain independent profiles. Prefer the smaller
        // containing region so every distinct band remains accessible in any row order.
        const double area=profile.summary.value("areaMm2").toDouble();
        const double selectedArea=selected ? selected->summary.value("areaMm2").toDouble() : 0;
        if(!selected || area<selectedArea || (area==selectedArea && profile.profileId<selected->profileId))selected=&profile;
    }
    return selected ? QJsonObject{{"kind","profile"},{"profileId",selected->profileId}} : QJsonObject{};
}
QJsonArray ReferenceView::sketchSelectionCandidates(const QJsonObject& feature,double x,double y) const {
    QJsonArray candidates;
    pickSketch(feature,x,y,"entity",{},&candidates);
    return candidates;
}
void ReferenceView::previewSketchSelectionCandidate(const QJsonObject& candidate) {
    if(candidate==sketchSelectOtherPreview_)return;
    if(!context_.IsNull() && !sketchSelectOtherOverlay_.IsNull())context_->Remove(sketchSelectOtherOverlay_,false);
    sketchSelectOtherOverlay_.Nullify();sketchSelectOtherPreview_={};
    const auto id=candidate.value("entityId").toString();
    if(!context_.IsNull() && !id.isEmpty() && sketchEntityVisible(id)) {
        const auto feature=SketchGeometry::expandedProjectionFeature(
            SketchGeometry::chamferPresentation(decorationFeature_),
            {{"lines",projectedLines_},{"circles",projectedCircles_},{"points",supportPoints_}});
        const auto parameters=feature.value("parameters").toObject();
        for(const auto& value:parameters.value("entities").toArray()) {
            const auto entity=value.toObject();if(entity.value("id")!=id)continue;
            const auto edge=SketchGeometry::analyticEdge(parameters,entity,sketchFrame_);
            if(edge.IsNull())break;
            sketchSelectOtherOverlay_=sketchPresentation(edge);
            sketchSelectOtherOverlay_->SetColor(Quantity_Color(0.12,0.65,0.95,Quantity_TOC_sRGB));
            sketchSelectOtherOverlay_->SetWidth(5.0*devicePixelRatioF());
            sketchSelectOtherOverlay_->SetZLayer(Graphic3d_ZLayerId_Topmost);
            context_->Display(sketchSelectOtherOverlay_,AIS_WireFrame,-1,false);
            context_->SetDisplayPriority(sketchSelectOtherOverlay_,Graphic3d_DisplayPriority_Topmost);
            context_->Deactivate(sketchSelectOtherOverlay_);sketchSelectOtherPreview_=candidate;break;
        }
    }
    if(!view_.IsNull())view_->Redraw();
}
QJsonObject ReferenceView::selectSketchWindow(const QJsonObject& feature,double x1,double y1,double x2,double y2,const QJsonObject& resolvedFrame) const {
    if(view_.IsNull() || std::abs(x2-x1)<4 || std::abs(y2-y1)<4)return {};
    const QRectF bounds(QPointF(std::min(x1,x2),std::min(y1,y2)),QPointF(std::max(x1,x2),std::max(y1,y2)));
    const auto parameters=feature["parameters"].toObject();const auto plane=parameters["plane"].toString();
    QMap<QString,QJsonObject> points;QJsonArray pointIds,entityIds;
    auto project=[&](double u,double v){const auto p=projectSketchPoint(u,v,plane,resolvedFrame);return QPointF(p["x"].toDouble(),p["y"].toDouble());};
    for(const auto& value:parameters["points"].toArray()){
        const auto p=value.toObject();points[p["id"].toString()]=p;
        if(sketchPointVisible(p.value("id").toString()) && bounds.contains(project(p["uMm"].toDouble(),p["vMm"].toDouble())))pointIds.append(p["id"]);
    }
    for(const auto& value:parameters["entities"].toArray()){
        const auto entity=value.toObject();if(!sketchEntityVisible(entity.value("id").toString()))continue;const auto type=entity["type"].toString();
        if(type=="line"){
            const auto a=points.value(entity["startPointId"].toString()),b=points.value(entity["endPointId"].toString());
            if(bounds.contains(project(a["uMm"].toDouble(),a["vMm"].toDouble())) &&
                bounds.contains(project(b["uMm"].toDouble(),b["vMm"].toDouble())))entityIds.append(entity["id"]);
        }else if(type=="fitSpline"){
            // Use the evaluated curve's kernel bounds, including handle
            // overshoot; enclosing only its fit points can select a crossing.
            try {
                const auto curve=SketchSpline::evaluate(parameters,entity).curve;
                Bnd_Box curveBounds;BRepBndLib::AddOptimal(BRepBuilderAPI_MakeEdge(curve).Edge(),curveBounds,false,false);
                if(curveBounds.IsVoid())continue;
                double xmin,ymin,zmin,xmax,ymax,zmax;curveBounds.Get(xmin,ymin,zmin,xmax,ymax,zmax);
                bool enclosed=true;for(double u:{xmin,xmax})for(double v:{ymin,ymax})if(!bounds.contains(project(u,v)))enclosed=false;
                if(enclosed)entityIds.append(entity["id"]);
            } catch(const Standard_Failure&) {} catch(const std::exception&) {}
        }else if(type=="arc" || type=="circle"){
            const auto c=points.value(entity["centerPointId"].toString());const double u=c["uMm"].toDouble(),v=c["vMm"].toDouble(),r=entity["radiusMm"].toDouble();
            const QPointF center=project(u,v),a=project(u+r,v)-center,b=project(u,v+r)-center;
            const double start=type=="circle" ? 0 : entity["startAngleDeg"].toDouble()*std::numbers::pi/180;
            const double sweep=type=="circle" ? 2*std::numbers::pi : entity["sweepAngleDeg"].toDouble()*std::numbers::pi/180;
            auto onSweep=[&](double angle){if(type=="circle")return true;
                const double travel=std::remainder(angle-start,2*std::numbers::pi);
                const double directed=sweep>=0 ? std::fmod(travel+2*std::numbers::pi,2*std::numbers::pi) : std::fmod(-travel+2*std::numbers::pi,2*std::numbers::pi);
                return directed<=std::abs(sweep)+1e-8;};
            auto inside=[&](double angle){return bounds.contains(center+a*std::cos(angle)+b*std::sin(angle));};
            bool enclosed=inside(start) && inside(start+sweep);
            for(const auto angle:{std::atan2(b.x(),a.x()),std::atan2(b.x(),a.x())+std::numbers::pi,
                    std::atan2(b.y(),a.y()),std::atan2(b.y(),a.y())+std::numbers::pi})
                if(onSweep(angle) && !inside(angle))enclosed=false;
            if(enclosed)entityIds.append(entity["id"]);
        }
    }
    return {{"pointIds",pointIds},{"entityIds",entityIds},{"pointCount",pointIds.size()},
        {"curveCount",entityIds.size()},{"objectCount",pointIds.size()+entityIds.size()}};
}
void ReferenceView::clearSketchDecorations() {
    sketchAxis3dMarkers_={};
    if(!context_.IsNull())for(const auto& decoration:sketchDecorations_)context_->Remove(decoration,false);
    sketchDecorations_.clear();splineHandleState_={};
    annotationLayouts_={};chamferHandles_={};
    constraintGlyphs_={};patternGlyphs_={};patternHandles_={};
}
QJsonObject ReferenceView::dimensionLayout(const QJsonObject& feature,const QJsonObject& inputDimension,double valueMm,const QString& units) const {
    auto dimension=inputDimension;
    if(view_.IsNull())return {};
    const auto parameters=feature.value("parameters").toObject();const auto plane=parameters.value("plane").toString();
    // Rotate keeps source annotation coordinates authored; only presentation follows its owner.
    if(dimension["annotation"].isObject())for(const auto& value:parameters["rotateGroups"].toArray()){
        const auto g=value.toObject();bool owned=false;for(const auto& d:g["dimensions"].toArray())owned |= d.toObject()["id"]==dimension["id"];if(!owned)continue;
        for(const auto& value:parameters["constructionLines3d"].toArray()){const auto a=value.toObject();if(a["id"]!=g["axisLineId"])continue;const auto pivot=a["start"].toObject(),annotation=dimension["annotation"].toObject();const double x=pivot["uMm"].toDouble(),y=pivot["vMm"].toDouble(),u=annotation["uMm"].toDouble()-x,v=annotation["vMm"].toDouble()-y,t=g["angleDeg"].toDouble()*std::numbers::pi/180;dimension["annotation"]=QJsonObject{{"uMm",x+u*std::cos(t)-v*std::sin(t)},{"vMm",y+u*std::sin(t)+v*std::cos(t)}};}
    }
    if(dimension["kind"]=="polygonSize") {
        QJsonObject group;for(const auto& row:parameters["polygonGroups"].toArray())
            if(row.toObject()["id"]==dimension["id"]){group=row.toObject();break;}
        const auto ids=group["pointIds"].toArray();if(ids.size()<3)return {};
        QMap<QString,QPointF> vertices;for(const auto& row:parameters["points"].toArray()){
            const auto p=row.toObject();vertices[p["id"].toString()]={p["uMm"].toDouble(),p["vMm"].toDouble()};}
        const auto mode=group["mode"].toString();const bool edge=mode=="edge";
        const QPointF first=vertices.value(ids[0].toString()),second=vertices.value(ids[1].toString());
        const QPointF anchor=edge ? first : QPointF(group["uMm"].toDouble(),group["vMm"].toDouble());
        const QPointF endpoint=edge ? second : mode=="circumscribed" ? (first+second)/2 : first;
        const QPointF delta=endpoint-anchor;const double length=std::hypot(delta.x(),delta.y());if(length<1e-9)return {};
        auto derived=feature,pp=parameters;auto points=pp["points"].toArray();
        const QString startId=group["id"].toString()+"-annotation-start",endId=group["id"].toString()+"-annotation-end";
        points.append(QJsonObject{{"id",startId},{"uMm",anchor.x()},{"vMm",anchor.y()}});
        points.append(QJsonObject{{"id",endId},{"uMm",endpoint.x()},{"vMm",endpoint.y()}});pp["points"]=points;derived["parameters"]=pp;
        QJsonObject span{{"id",group["id"]},{"kind","linear"},{"pointIds",QJsonArray{startId,endId}},{"driving",true}};
        auto layout=dimensionLayout(derived,span,group["sizeMm"].toDouble(),units);if(layout.isEmpty())return {};
        const QPointF perpendicular(-delta.y()/length,delta.x()/length);
        const double offset=edge ? -group["side"].toInt(1)*36 : layout["lineClearancePixels"].toDouble();
        const auto textAnchor=(anchor+endpoint)/2+perpendicular*(offset*layout["mmPerPixel"].toDouble());
        span["annotation"]=QJsonObject{{"uMm",textAnchor.x()},{"vMm",textAnchor.y()}};
        layout=dimensionLayout(derived,span,group["sizeMm"].toDouble(),units);
        const auto displayedUnits=units=="in" ? QString("in") : QString("mm");
        const QString content=(edge ? QString{} : QString("R"))+QString::number(group["sizeMm"].toDouble()/(units=="in" ? 25.4 : 1),'g',6)+" "+displayedUnits;
        const QString text=group.contains("driverPointId") ? "("+content+")" : content;
        auto label=layout["label"].toObject();QFont font("Arial");font.setPixelSize(13);const QFontMetricsF metrics(font);label["text"]=text;label["width"]=metrics.horizontalAdvance(text)+12;layout["label"]=label;
        layout["driving"]=!group.contains("driverPointId");layout["sizeOwner"]=group.contains("driverPointId") ? "secondDriver" : "polygon";
        layout["polygonId"]=group["id"];layout["sizeBasis"]=edge ? "edgeLength" : mode=="circumscribed" ? "inradius" : "circumradius";
        layout["anchor"]=QJsonObject{{"uMm",anchor.x()},{"vMm",anchor.y()}};
        layout["sizeEndpoint"]=QJsonObject{{"uMm",endpoint.x()},{"vMm",endpoint.y()}};
        layout["anchorKind"]=edge ? "baseEndpoint" : "center";layout["units"]=displayedUnits;layout["editable"]=true;
        return layout;
    }
    if(dimension["kind"]=="angle")return angleDimensionLayout(feature,dimension,valueMm);
    if(dimension["kind"]=="overallSlotAngle") {
        auto derived=feature,pp=parameters;const auto ids=dimension["pointIds"].toArray();if(ids.size()!=2)return {};
        QJsonObject start,end;for(const auto& row:pp["points"].toArray()){const auto p=row.toObject();if(p["id"]==ids[0])start=p;if(p["id"]==ids[1])end=p;}
        if(start.isEmpty() || end.isEmpty())return {};
        auto points=pp["points"].toArray(),entities=pp["entities"].toArray();
        const double span=std::hypot(end["uMm"].toDouble()-start["uMm"].toDouble(),end["vMm"].toDouble()-start["vMm"].toDouble());
        points.append(QJsonObject{{"id","overall-angle-axis-end"},{"uMm",start["uMm"].toDouble()+span},{"vMm",start["vMm"]}});
        entities.append(QJsonObject{{"id","overall-angle-axis"},{"type","line"},{"startPointId",ids[0]},{"endPointId","overall-angle-axis-end"}});
        entities.append(QJsonObject{{"id","overall-angle-guide"},{"type","line"},{"startPointId",ids[0]},{"endPointId",ids[1]}});
        pp["points"]=points;pp["entities"]=entities;derived["parameters"]=pp;
        auto angle=dimension;angle["kind"]="angle";angle.remove("pointIds");angle["entityIds"]=QJsonArray{"overall-angle-axis","overall-angle-guide"};
        auto layout=angleDimensionLayout(derived,angle,valueMm);layout["kind"]="overallSlotAngle";return layout;
    }
    if(dimension["kind"]=="overallSlotLength") {
        const auto ids=dimension["pointIds"].toArray();if(ids.size()!=2)return {};
        QJsonObject first,last,arc;for(const auto& row:parameters["points"].toArray()){const auto p=row.toObject();if(p["id"]==ids[0])first=p;if(p["id"]==ids[1])last=p;}
        for(const auto& row:parameters["entities"].toArray())if(row.toObject()["id"]==dimension["entityId"]){arc=row.toObject();break;}
        if(first.isEmpty() || last.isEmpty() || arc.isEmpty())return {};
        const double dx=last["uMm"].toDouble()-first["uMm"].toDouble(),dy=last["vMm"].toDouble()-first["vMm"].toDouble();
        const double length=std::hypot(dx,dy),radius=arc["radiusMm"].toDouble();if(length<1e-6)return {};
        auto derived=feature,pp=parameters;auto rows=pp["points"].toArray();
        rows.append(QJsonObject{{"id","overall-first-witness"},{"uMm",first["uMm"].toDouble()-radius*dx/length},{"vMm",first["vMm"].toDouble()-radius*dy/length}});
        rows.append(QJsonObject{{"id","overall-last-witness"},{"uMm",last["uMm"].toDouble()+radius*dx/length},{"vMm",last["vMm"].toDouble()+radius*dy/length}});
        pp["points"]=rows;derived["parameters"]=pp;auto span=dimension;span["kind"]="linear";span.remove("entityId");span["pointIds"]=QJsonArray{"overall-first-witness","overall-last-witness"};
        auto layout=dimensionLayout(derived,span,valueMm,units);layout["kind"]="overallSlotLength";return layout;
    }
    if(dimension["kind"]=="radius" || dimension["kind"]=="diameter" || dimension["kind"]=="arcSweep")return curveDimensionLayout(feature,dimension,valueMm,units);
    if(dimension["kind"]=="centerlineDiameter") {
        const auto ids=dimension["entityIds"].toArray();if(ids.size()!=2)return {};
        QMap<QString,QPointF> points;for(const auto& row:parameters["points"].toArray()){const auto p=row.toObject();points[p["id"].toString()]={p["uMm"].toDouble(),p["vMm"].toDouble()};}
        QJsonObject axis,edge;for(const auto& row:parameters["entities"].toArray()){const auto e=row.toObject();if(e["id"]==ids[0])axis=e;if(e["id"]==ids[1])edge=e;}
        if(axis["type"]!="line" || edge["type"]!="line")return {};
        const auto a=points.value(axis["startPointId"].toString()),b=points.value(axis["endPointId"].toString()),q=points.value(edge["startPointId"].toString());
        const auto d=b-a;const double length2=d.x()*d.x()+d.y()*d.y();if(length2<1e-6)return {};
        const auto foot=a+d*((q.x()-a.x())*d.x()+(q.y()-a.y())*d.y())/length2,mirror=foot*2-q;
        auto derived=feature;auto pp=parameters;auto rows=pp["points"].toArray();
        rows.append(QJsonObject{{"id","diameter-profile-witness"},{"uMm",q.x()},{"vMm",q.y()}});
        rows.append(QJsonObject{{"id","diameter-mirrored-witness"},{"uMm",mirror.x()},{"vMm",mirror.y()}});pp["points"]=rows;derived["parameters"]=pp;
        auto span=dimension;span["kind"]="linear";span["pointIds"]=QJsonArray{"diameter-profile-witness","diameter-mirrored-witness"};span["diameterPresentation"]=true;
        auto layout=dimensionLayout(derived,span,valueMm,units);layout["kind"]="centerlineDiameter";layout["entityIds"]=ids;layout["side"]=dimension["side"];return layout;
    }
    if(dimension["kind"]=="parallelLineDistance") {
        const auto ids=dimension["entityIds"].toArray();if(ids.size()!=2)return {};
        QMap<QString,QPointF> points;for(const auto& row:parameters["points"].toArray()){const auto p=row.toObject();points[p["id"].toString()]={p["uMm"].toDouble(),p["vMm"].toDouble()};}
        QJsonObject first,second;for(const auto& row:parameters["entities"].toArray()){const auto line=row.toObject();if(line["id"]==ids[0])first=line;if(line["id"]==ids[1])second=line;}
        if(first["type"]!="line" || second["type"]!="line")return {};
        const auto a=points.value(first["startPointId"].toString()),b=points.value(first["endPointId"].toString());
        const auto c=points.value(second["startPointId"].toString()),d=points.value(second["endPointId"].toString());
        const auto axis=b-a;const double length2=axis.x()*axis.x()+axis.y()*axis.y();if(length2<1e-6)return {};
        const auto secondWitness=(c+d)/2,firstWitness=a+axis*((secondWitness.x()-a.x())*axis.x()+(secondWitness.y()-a.y())*axis.y())/length2;
        auto derived=feature;auto pp=parameters;auto rows=pp["points"].toArray();
        rows.append(QJsonObject{{"id","parallel-distance-first-witness"},{"uMm",firstWitness.x()},{"vMm",firstWitness.y()}});
        rows.append(QJsonObject{{"id","parallel-distance-second-witness"},{"uMm",secondWitness.x()},{"vMm",secondWitness.y()}});
        pp["points"]=rows;derived["parameters"]=pp;
        auto span=dimension;span["kind"]="linear";span["pointIds"]=QJsonArray{"parallel-distance-first-witness","parallel-distance-second-witness"};
        auto layout=dimensionLayout(derived,span,valueMm,units);layout["kind"]="parallelLineDistance";layout["entityIds"]=ids;layout["side"]=dimension["side"];return layout;
    }
    if(dimension["kind"]=="pointLineDistance") {
        const auto ids=dimension["pointIds"].toArray();if(ids.size()!=1)return {};QMap<QString,QPointF> points;for(const auto& value:parameters["points"].toArray()){const auto p=value.toObject();points[p["id"].toString()]={p["uMm"].toDouble(),p["vMm"].toDouble()};}QJsonObject edge;for(const auto& value:parameters["entities"].toArray())if(value.toObject()["id"]==dimension["entityId"])edge=value.toObject();if(edge["type"]!="line" || !points.contains(ids[0].toString()))return {};const auto a=points.value(edge["startPointId"].toString()),b=points.value(edge["endPointId"].toString()),q=points.value(ids[0].toString());const auto d=b-a;const double length2=d.x()*d.x()+d.y()*d.y();if(length2<1e-6)return {};const auto foot=a+d*((q.x()-a.x())*d.x()+(q.y()-a.y())*d.y())/length2;
        auto derived=feature;auto pp=parameters;auto rows=parameters["points"].toArray();rows.append(QJsonObject{{"id","dimension-perpendicular-foot"},{"uMm",foot.x()},{"vMm",foot.y()}});pp["points"]=rows;derived["parameters"]=pp;auto linear=dimension;linear["kind"]="linear";linear["pointIds"]=QJsonArray{ids[0],"dimension-perpendicular-foot"};auto layout=dimensionLayout(derived,linear,valueMm,units);layout["kind"]="pointLineDistance";layout["pointIds"]=ids;layout["entityId"]=dimension["entityId"];layout["measurementFoot"]=QJsonObject{{"uMm",foot.x()},{"vMm",foot.y()}};return layout;
    }
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
    const QString text=(driving ? "" : "(")+(dimension["diameterPresentation"].toBool() ? QString::fromUtf8("Ø") : QString{})+QString::number(valueMm/scale,'g',6)+(driving ? "" : ")");
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
QJsonArray ReferenceView::resolveDimensionLabels(QJsonArray layouts,const QString& plane) const {
    // Pure presentation: authored anchors/strokes stay unchanged. Logical-pixel
    // footprints are also the authoritative pointer targets below.
    const auto bounds=[](const QJsonObject& layout){const auto l=layout.value("label").toObject();const double t=l.value("angleDegrees").toDouble()*std::numbers::pi/180,c=std::abs(std::cos(t)),s=std::abs(std::sin(t)),w=l.value("width").toDouble(),h=l.value("height").toDouble();return QRectF(l.value("x").toDouble()-(w*c+h*s)/2,l.value("y").toDouble()-(w*s+h*c)/2,w*c+h*s,w*s+h*c).adjusted(-3,-3,3,3);};
    const auto moveLabel=[&](QJsonObject layout,QPointF center){
        auto label=layout.value("label").toObject();const auto anchor=sketchPoint(center.x(),center.y(),plane);const auto projected=projectSketchPoint(anchor.value("uMm").toDouble(),anchor.value("vMm").toDouble(),plane);
        label["x"]=projected.value("x");label["y"]=projected.value("y");label["angleDegrees"]=0;
        layout["label"]=label;layout["displayAnchor"]=anchor;layout["automaticPlacement"]=true;
        const auto original=layout.value("annotation").toObject();const auto raw=projectSketchPoint(original.value("uMm").toDouble(),original.value("vMm").toDouble(),plane);
        const double side=projected.value("x").toDouble()>=raw.value("x").toDouble()?1:-1;
        const double edge=std::clamp(projected.value("x").toDouble()-side*(label.value("width").toDouble()/2+3),1.0,double(width()-2));
        layout["displayLeader"]=QJsonArray{original,sketchPoint(edge,projected.value("y").toDouble(),plane)};
        return layout;
    };
    const int count=layouts.size();QVector<QRectF> raw(count);QVector<bool> fixed(count,false);QVector<int> component(count,-1);
    for(int i=0;i<count;++i){auto layout=layouts[i].toObject();fixed[i]=layout.value("dimensionId")=="pending";
        if(annotationAutoDrag_ && layout.value("dimensionId")==draggedAnnotation_){const auto label=layout.value("label").toObject();const QPointF now(label.value("x").toDouble(),label.value("y").toDouble());
            // A refused owner edit leaves the old raw anchor: keep its displayed
            // label still. A successful explicit drag adopts the visible target.
            const auto center=QLineF(now,annotationRawPress_).length()<1 ? annotationDisplayPress_ : now;
            if(center.x()>0 && center.y()>0 && center.x()<width()-1 && center.y()<height()-1)layouts[i]=moveLabel(layout,center);fixed[i]=true;
        }raw[i]=bounds(layouts[i].toObject());
    }
    QVector<QRectF> placed;
    for(int first=0;first<count;++first){if(component[first]>=0 || fixed[first])continue;QVector<int> group{first};component[first]=first;
        for(int cursor=0;cursor<group.size();++cursor)for(int j=0;j<count;++j)if(!fixed[j] && component[j]<0 && raw[group[cursor]].intersects(raw[j])){component[j]=first;group.append(j);}
        bool collision=group.size()>1;for(int i:group)for(int j=0;j<count;++j)if(fixed[j] && raw[i].intersects(raw[j]))collision=true;
        if(!collision)continue;
        QRectF cluster;double columnWidth=0,totalHeight=0;for(int i:group){cluster=cluster.isNull()?raw[i]:cluster.united(raw[i]);const auto l=layouts[i].toObject().value("label").toObject();columnWidth=std::max(columnWidth,l.value("width").toDouble());totalHeight+=l.value("height").toDouble()+6;}totalHeight-=6;
        std::sort(group.begin(),group.end(),[&](int a,int b){const auto x=layouts[a].toObject(),y=layouts[b].toObject();const auto l=x.value("label").toObject(),r=y.value("label").toObject();if(l.value("y")!=r.value("y"))return l.value("y").toDouble()<r.value("y").toDouble();if(l.value("x")!=r.value("x"))return l.value("x").toDouble()<r.value("x").toDouble();return x.value("dimensionId").toString()<y.value("dimensionId").toString();});
        const QRectF available(8,8,std::max(0,width()-16),std::max(0,height()-16)),cube(width()-190,0,190,180);
        const double top=std::clamp(cluster.center().y()-totalHeight/2,8.0,std::max(8.0,height()-8.0-totalHeight));
        const double middle=std::clamp(cluster.center().x()-columnWidth/2,8.0,std::max(8.0,width()-8.0-columnWidth));
        const QVector<QPointF> candidates{{cluster.right()+24,top},{cluster.left()-24-columnWidth,top},{middle,cluster.top()-24-totalHeight},{middle,cluster.bottom()+24}};
        bool done=false;
        for(const auto& origin:candidates){QVector<QRectF> boxes;double y=origin.y();bool fits=true;
            for(int i:group){const auto l=layouts[i].toObject().value("label").toObject();const QRectF box(origin.x()+(columnWidth-l.value("width").toDouble())/2,y,l.value("width").toDouble(),l.value("height").toDouble());const auto padded=box.adjusted(-3,-3,3,3);if(!available.contains(padded) || cube.intersects(padded))fits=false;
                for(int j=0;j<count;++j)if(!group.contains(j) && raw[j].intersects(padded))fits=false;for(const auto& p:placed)if(p.intersects(padded))fits=false;boxes.append(box);y+=l.value("height").toDouble()+6;
            }
            if(!fits)continue;for(int n=0;n<group.size();++n){layouts[group[n]]=moveLabel(layouts[group[n]].toObject(),boxes[n].center());placed.append(bounds(layouts[group[n]].toObject()));}done=true;break;
        }
        if(!done)for(int i:group){auto l=layouts[i].toObject();l["crowdedUnresolved"]=true;layouts[i]=l;} // Never hide a dimension to force a fit.
    }
    return layouts;
}
QJsonObject ReferenceView::pickAnnotation(double x,double y) const {
    if(!showDimensions_)return {};
    for(auto i=annotationLayouts_.size()-1;i>=0;--i) {
        const auto layout=annotationLayouts_[i].toObject();if(layout.value("dimensionId")=="pending")continue;
        const auto label=layout.value("label").toObject();const double angle=label.value("angleDegrees").toDouble()*std::acos(-1.0)/180;
        const double dx=x-label.value("x").toDouble(),dy=y-label.value("y").toDouble();
        if(std::abs(dx*std::cos(angle)-dy*std::sin(angle))<=label.value("width").toDouble()/2
            && std::abs(dx*std::sin(angle)+dy*std::cos(angle))<=label.value("height").toDouble()/2)
            return {{"kind","dimension"},{"dimensionId",layout.value("dimensionId")},{"part","label"},{"annotation",layout.value("annotation")},
                {"polygonId",layout.value("polygonId")},{"sizeBasis",layout.value("sizeBasis")},{"chamferId",layout.value("chamferId")},{"automaticPlacement",layout.value("automaticPlacement")},{"displayLabel",label}};
    }
    return {};
}
QJsonObject ReferenceView::pickConstraint(double x,double y) const {
    if(sketchPlane_.isEmpty())return {};
    QJsonObject best;double bestDistance=std::numeric_limits<double>::infinity();
    for(const auto& value:constraintGlyphs_) {
        const auto glyph=value.toObject(),bounds=glyph.value("bounds").toObject();
        const double left=bounds.value("x").toDouble(),top=bounds.value("y").toDouble();
        const double width=bounds.value("width").toDouble(),height=bounds.value("height").toDouble();
        if(x<left || y<top || x>left+width || y>top+height)continue;
        const double distance=std::hypot(x-left-width/2,y-top-height/2);
        if(distance>=bestDistance)continue;
        bestDistance=distance;
        best={{"kind","constraint"},{"constraintId",glyph.value("constraintId")},
            {"entityIds",glyph.value("entityIds")},{"pointIds",glyph.value("pointIds")},
            {"targets",QJsonObject{{"entityIds",glyph.value("entityIds")},{"pointIds",glyph.value("pointIds")}}},
            {"bounds",bounds},{"anchorEntityId",glyph.value("anchorEntityId")}};
    }
    return best;
}
bool ReferenceView::constraintBadgeVisible(const QString& id) const {
    for(const auto& value:constraintGlyphs_)if(value.toObject().value("constraintId")==id)return true;
    return false;
}
void ReferenceView::setSketchDisplayOptions(const QJsonObject& options) {
    sketchDisplayOptions_=options;
    setSketchAnnotationVisibility(options.value("showConstraints").toBool(true),options.value("showDimensions").toBool(true));
}
bool ReferenceView::sketchEntityVisible(const QString& id) const {
    for(const auto& row:decorationDiagnostics_.value("entityRoles").toArray()){
        const auto role=row.toObject();if(role.value("entityId")!=id)continue;
        if(sketchDisplayOptions_.value("hideFullyConstrained").toBool() && role.value("fullyConstrained").toBool())return false;
        if(role.value("role")=="projected")return sketchDisplayOptions_.value("showProjectedGeometries").toBool(true);
    }
    for(const auto& row:decorationFeature_.value("parameters").toObject().value("entities").toArray()){
        const auto e=row.toObject();if(e.value("id")!=id)continue;
        if(e.value("type")=="point")return sketchDisplayOptions_.value("showPoints").toBool(true);
        if(e.value("construction").toBool() || e.value("centerline").toBool())return sketchDisplayOptions_.value("showConstructionGeometries").toBool(true);
    }
    return true;
}
bool ReferenceView::sketchPointVisible(const QString& id) const {
    if(!sketchDisplayOptions_.value("showPoints").toBool(true))return false;
    bool owned=false,visible=false;
    for(const auto& row:decorationFeature_.value("parameters").toObject().value("entities").toArray()){
        const auto e=row.toObject();bool owns=false;
        for(const auto& key:QStringList{"startPointId","endPointId","centerPointId","pointId","majorPointId","minorPointId"})owns |= e.value(key)==id;
        owns |= e.value("pointIds").toArray().contains(id);
        if(owns){owned=true;visible |= sketchEntityVisible(e.value("id").toString());}
    }
    for(const auto& row:projectedLines_){const auto e=row.toObject();if(e.value("startPointId")==id || e.value("endPointId")==id){owned=true;visible |= sketchEntityVisible(e.value("id").toString());}}
    if(owned)return visible;
    if(sketchDisplayOptions_.value("hideFullyConstrained").toBool() && solvedPoint(decorationDiagnostics_,id))return false;
    for(const auto& row:supportPoints_)if(row.toObject().value("id")==id)return sketchDisplayOptions_.value("showProjectedGeometries").toBool(true) && !sketchDisplayOptions_.value("hideFullyConstrained").toBool();
    return true;
}
void ReferenceView::setSketchAnnotationVisibility(bool showConstraints,bool showDimensions) {
    showConstraints_=showConstraints;showDimensions_=showDimensions;
}
void ReferenceView::decorateSketch(const QJsonObject& sourceFeature,const QJsonObject& selection,const QJsonObject& diagnostics,const QString& units,const QJsonObject& pending) {
    const auto feature=SketchGeometry::chamferPresentation(sourceFeature);
    if(sourceFeature!=decorationFeature_ || selection!=decorationSelection_
        || (!sketchSelectOtherPreview_.isEmpty() && !sketchEntityVisible(sketchSelectOtherPreview_.value("entityId").toString())))
        previewSketchSelectionCandidate();
    if(!selection["patternInteraction"].toObject()["active"].toBool())patternHandleDrag_={};
    decorationFeature_=sourceFeature;decorationSelection_=selection;decorationDiagnostics_=diagnostics;decorationUnits_=units;decorationPending_=pending;
    clearSketchDecorations();if(context_.IsNull())return;
    const bool insetPresentation=std::any_of(draftProfiles_.begin(),draftProfiles_.end(),[](const auto& profile){
        return profile.summary.contains("signedDistanceMm") && profile.summary.contains("sourceLoopId");});
    QSet<QString> hidden;
    for(const auto& row:feature.value("parameters").toObject().value("entities").toArray())if(!sketchEntityVisible(row.toObject().value("id").toString()))hidden.insert(row.toObject().value("id").toString());
    const bool showProfile=sketchDisplayOptions_.value("showProfile").toBool(true) && !sketchDisplayOptions_.value("hideFullyConstrained").toBool();
    for(auto i=profileModels_.begin();i!=profileModels_.end();++i) {
        if(showProfile)context_->Display(i.value(),false);else context_->Erase(i.value(),false);
        i.value()->SetColor(selection.value("profileIds").toArray().contains(i.key()) ? Quantity_Color(0.17,0.48,0.90,Quantity_TOC_sRGB)
            : insetPresentation ? Quantity_Color(0.30,0.66,0.94,Quantity_TOC_sRGB) : unselectedSketchProfileColor());
        context_->Redisplay(i.value(),false);
    }
    const auto parameters=feature["parameters"].toObject();const auto plane=parameters["plane"].toString();
    for(const auto& row:parameters["constructionLines3d"].toArray()){
        const auto axis=row.toObject(),start=axis["start"].toObject(),end=axis["end"].toObject();const auto a=sketchWorld(plane,start["uMm"].toDouble(),start["vMm"].toDouble());const auto b=gp_Pnt(a.X(),a.Y(),a.Z()+end["wMm"].toDouble());const bool selected=selection.value("axis3dId")==axis["id"];const auto color=selected ? Quantity_Color(.86,.32,.04,Quantity_TOC_sRGB) : Quantity_Color(.55,.24,.72,Quantity_TOC_sRGB);
        auto segment=sketchPresentation(BRepBuilderAPI_MakeEdge(a,b).Edge());segment->SetColor(color);segment->SetWidth(2.5*devicePixelRatioF());segment->SetZLayer(Graphic3d_ZLayerId_Topmost);context_->Display(segment,AIS_WireFrame,-1,false);context_->Deactivate(segment);sketchDecorations_.push_back(segment);
        const double size=view_->Camera()->Scale()/qMax(1,height()),r=7*size;TopoDS_Compound strokes;BRep_Builder builder;builder.MakeCompound(strokes);builder.Add(strokes,BRepBuilderAPI_MakeEdge(gp_Circ(gp_Ax2(a,gp_Dir(0,0,1)),r)).Edge());builder.Add(strokes,BRepBuilderAPI_MakeEdge(gp_Pnt(a.X()-r,a.Y(),a.Z()),gp_Pnt(a.X()+r,a.Y(),a.Z())).Edge());builder.Add(strokes,BRepBuilderAPI_MakeEdge(gp_Pnt(a.X(),a.Y()-r,a.Z()),gp_Pnt(a.X(),a.Y()+r,a.Z())).Edge());auto marker=sketchPresentation(strokes);marker->SetColor(color);marker->SetWidth(2*devicePixelRatioF());marker->SetZLayer(Graphic3d_ZLayerId_Topmost);context_->Display(marker,AIS_WireFrame,-1,false);context_->Deactivate(marker);sketchDecorations_.push_back(marker);
        Handle(AIS_TextLabel) label=new AIS_TextLabel;label->SetText(TCollection_ExtendedString("3D +Z"));label->SetPosition(gp_Pnt(a.X()+12*size,a.Y()+12*size,a.Z()));label->SetHeight(12);label->SetColor(color);label->SetZLayer(Graphic3d_ZLayerId_Topmost);context_->Display(label,false);context_->Deactivate(label);sketchDecorations_.push_back(label);
        const auto screen=projectSketchPoint(start["uMm"].toDouble(),start["vMm"].toDouble(),plane);int ex,ey;view_->Convert(b.X(),b.Y(),b.Z(),ex,ey);const auto scale=windowScale();sketchAxis3dMarkers_.append(QJsonObject{{"id",axis["id"]},{"startPointId",start["id"]},{"endPointId",end["id"]},{"x",screen["x"]},{"y",screen["y"]},{"endX",ex/scale.x()},{"endY",ey/scale.y()},{"hitRadius",10},{"selected",selected},{"referenceOnly",true},{"role","normal construction Line"},{"endOn",std::hypot(ex/scale.x()-screen["x"].toDouble(),ey/scale.y()-screen["y"].toDouble())<3}});
    }

    const bool fullyConstrained=diagnostics["dof"].toInt(-1)==0;
    if(!sketchDraft_.IsNull()){sketchDraft_->SetColor(insetPresentation ? Quantity_Color(0.08,0.35,0.86,Quantity_TOC_sRGB)
        : fullyConstrained ? Quantity_Color(0.12,0.14,0.16,Quantity_TOC_sRGB) : Quantity_Color(0.10,0.34,0.84,Quantity_TOC_RGB));
        colorSolvedEntities(sketchDraft_,parameters["entities"].toArray(),diagnostics,0,projectedLines_,hidden);context_->Redisplay(sketchDraft_,false);}
    if(!sketchConstructionDraft_.IsNull()){
        styleConstruction(sketchConstructionDraft_,false,fullyConstrained);
        colorSolvedEntities(sketchConstructionDraft_,parameters["entities"].toArray(),diagnostics,1,projectedLines_,hidden);context_->Redisplay(sketchConstructionDraft_,false);
    }
    if(!sketchCenterlineDraft_.IsNull()){
        styleCenterline(sketchCenterlineDraft_,false,fullyConstrained);
        colorSolvedEntities(sketchCenterlineDraft_,parameters["entities"].toArray(),diagnostics,2,projectedLines_,hidden);context_->Redisplay(sketchCenterlineDraft_,false);
    }
    if(!projectedDraft_.IsNull()){
        if(sketchDisplayOptions_.value("showProjectedGeometries").toBool(true) && !sketchDisplayOptions_.value("hideFullyConstrained").toBool())context_->Display(projectedDraft_,false);else context_->Erase(projectedDraft_,false);
    }
    QMap<QString,QJsonObject> points;QSet<QString> fixed,coincidentPoints;
    for(const auto& entry:parameters["constraints"].toArray()) {
        const auto constraint=entry.toObject();if(constraint["type"]=="fixed" || constraint["type"]=="fixedLine" || constraint["type"]=="fixedCircle" || constraint["type"]=="fixedArc")for(const auto& id:constraint["pointIds"].toArray())fixed.insert(id.toString());
        if(constraint["type"]=="coincident")for(const auto& id:constraint["pointIds"].toArray())coincidentPoints.insert(id.toString());
    }
    for(const auto& entry:parameters["points"].toArray()) {
        const auto point=entry.toObject();const auto id=point["id"].toString();points[id]=point;
        if(!sketchPointVisible(id))continue;
        bool selected=selection["pointIds"].toArray().contains(id);
        for(const auto& e:parameters["entities"].toArray())if(e.toObject()["type"]=="point" && e.toObject()["pointId"]==id && selection["entityIds"].toArray().contains(e.toObject()["id"]))selected=true;
        if(coincidentPoints.contains(id) && !selected)continue;
        const bool patternHover=selection["patternInteraction"].toObject()["hoveredPointId"]==id;
        const Quantity_Color color=patternHover ? Quantity_Color(.98,.62,.08,Quantity_TOC_sRGB) : selected ? Quantity_Color(0.98,0.43,0.08,Quantity_TOC_RGB)
            : fixed.contains(id) ? Quantity_Color(0.10,0.52,0.24,Quantity_TOC_RGB)
            : solvedPoint(diagnostics,id) ? Quantity_Color(0.12,0.14,0.16,Quantity_TOC_sRGB) : Quantity_Color(0.10,0.34,0.84,Quantity_TOC_RGB);
        Handle(AIS_Point) marker=new AIS_Point(new Geom_CartesianPoint(sketchWorld(plane,point["uMm"].toDouble(),point["vMm"].toDouble())));
        marker->Attributes()->SetPointAspect(new Prs3d_PointAspect(Aspect_TOM_O_POINT,color,patternHover ? 5 : selected ? 3 : 2));
        marker->SetZLayer(Graphic3d_ZLayerId_Topmost);context_->Display(marker,0,-1,false);context_->Deactivate(marker);sketchDecorations_.push_back(marker);
    }
    QSet<QString> markedProjectedPoints;
    for(const auto& value:projectedLines_) {
        const auto line=value.toObject();
        for(const bool end:{false,true}) {
            const auto id=line[end ? "endPointId" : "startPointId"].toString();
            if(!sketchPointVisible(id) || markedProjectedPoints.contains(id) || !selection["pointIds"].toArray().contains(id))continue;
            markedProjectedPoints.insert(id);
            const auto u=line[end ? "u2Mm" : "u1Mm"].toDouble(),v=line[end ? "v2Mm" : "v1Mm"].toDouble();
            Handle(AIS_Point) marker=new AIS_Point(new Geom_CartesianPoint(sketchWorld(plane,u,v)));
            marker->Attributes()->SetPointAspect(new Prs3d_PointAspect(Aspect_TOM_O_POINT,
                Quantity_Color(0.81,0.40,0.98,Quantity_TOC_sRGB),4));
            marker->SetZLayer(Graphic3d_ZLayerId_Topmost);context_->Display(marker,0,-1,false);context_->Deactivate(marker);sketchDecorations_.push_back(marker);
        }
    }
    for(const auto& value:supportPoints_) {
        const auto point=value.toObject();if(!sketchPointVisible(point.value("id").toString()))continue;const bool selected=selection["pointIds"].toArray().contains(point["id"]);
        const bool retained=point.value("retained").toBool();
        const auto color=Quantity_Color(selected ? 0.84 : retained ? 0.66 : 0.55,
            selected ? 0.44 : retained ? 0.31 : 0.27,selected ? 0.98 : retained ? 0.85 : 0.70,Quantity_TOC_sRGB);
        Handle(AIS_Point) marker=new AIS_Point(new Geom_CartesianPoint(sketchWorld(plane,point["uMm"].toDouble(),point["vMm"].toDouble())));
        marker->Attributes()->SetPointAspect(new Prs3d_PointAspect(Aspect_TOM_PLUS,color,(selected ? 7.0 : 5.0)*devicePixelRatioF()));
        marker->SetZLayer(Graphic3d_ZLayerId_Topmost);context_->Display(marker,0,-1,false);context_->Deactivate(marker);sketchDecorations_.push_back(marker);
    }
    for(const auto& entry:parameters["entities"].toArray()) {
        const auto entity=entry.toObject();if(!sketchEntityVisible(entity.value("id").toString()) || entity["type"]=="point" || (!selection["entityIds"].toArray().contains(entity["id"]) && selection["patternInteraction"].toObject()["hoveredEntityId"]!=entity["id"]))continue;
        const auto edge=SketchGeometry::analyticEdge(parameters,entity,sketchFrame_);
        const auto highlight=entity.value("construction").toBool() ? constructionPresentation(edge,true) : entity.value("centerline").toBool() ? centerlinePresentation(edge,true) : sketchPresentation(edge);
        if(!entity.value("construction").toBool() && !entity.value("centerline").toBool()){highlight->SetColor(selection.value("tool")=="chamfer" ? Quantity_Color(0.88,0.16,0.12,Quantity_TOC_RGB) : Quantity_Color(0.08,0.43,0.91,Quantity_TOC_sRGB));highlight->SetWidth(4);}
        if(selection.value("pasteInteraction").toObject().value("active").toBool() || selection.value("createCopyPreviewEntityIds").toArray().contains(entity["id"])){highlight->SetColor(Quantity_Color(.98,.43,.08,Quantity_TOC_RGB));highlight->SetWidth(4*devicePixelRatioF());}
        if(selection["patternInteraction"].toObject()["hoveredEntityId"]==entity["id"]){highlight->SetColor(Quantity_Color(.98,.62,.08,Quantity_TOC_sRGB));highlight->SetWidth(4*devicePixelRatioF());}
        highlight->SetZLayer(Graphic3d_ZLayerId_Topmost);
        context_->Display(highlight,AIS_WireFrame,-1,false);context_->Deactivate(highlight);sketchDecorations_.push_back(highlight);
    }
    decorateSplineHandles(feature,selection);
    decorateConicHandle(feature,selection);
    QJsonArray projectedUsedIds;QSet<QString> projectedIds;
    for(const auto& value:projectedLines_)projectedIds.insert(value.toObject()["id"].toString());
    for(const auto& value:projectedCircles_)projectedIds.insert(value.toObject()["id"].toString());
    for(const auto& entry:parameters["constraints"].toArray())
        for(const auto& id:entry.toObject()["entityIds"].toArray())projectedUsedIds.append(id);
    for(const auto& value:projectedLines_) {
        const auto line=value.toObject();const auto id=line["id"];if(!sketchEntityVisible(id.toString()))continue;
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
    for(const auto& value:projectedCircles_) {
        const auto circle=value.toObject();const auto id=circle.value("id");
        if(!sketchEntityVisible(id.toString()) || !selection.value("entityIds").toArray().contains(id))continue;
        const auto normal=sketchFrame_.value("normal").toArray();
        const auto center=sketchWorld(plane,circle.value("uMm").toDouble(),circle.value("vMm").toDouble());
        auto highlight=sketchPresentation(BRepBuilderAPI_MakeEdge(gp_Circ(gp_Ax2(center,gp_Dir(normal[0].toDouble(),normal[1].toDouble(),normal[2].toDouble())),circle.value("radiusMm").toDouble())).Edge());
        highlight->SetColor(Quantity_Color(0.81,0.40,0.98,Quantity_TOC_sRGB));
        highlight->SetWidth(4.0*devicePixelRatioF());highlight->SetZLayer(Graphic3d_ZLayerId_Topmost);
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
    QSet<QString> highlightedRelations;
    auto highlightRelation=[&](const QJsonObject& relation,bool selected,bool hovered) {
        if((!selected && !hovered) || highlightedRelations.contains(relation["id"].toString()))return;
        highlightedRelations.insert(relation["id"].toString());
        const Quantity_Color color=selected ? Quantity_Color(0.08,0.43,0.91,Quantity_TOC_sRGB)
            : Quantity_Color(0.96,0.59,0.09,Quantity_TOC_sRGB);
        QSet<QString> highlightedEntities;
        for(const auto& idValue:relation["entityIds"].toArray()) {
            const auto id=idValue.toString();if(!sketchEntityVisible(id) || highlightedEntities.contains(id))continue;highlightedEntities.insert(id);
            TopoDS_Shape edge;
            if(projectedIds.contains(id)) {
                for(const auto& value:projectedLines_)if(value.toObject()["id"]==id) {
                    const auto line=value.toObject();
                    const auto a=sketchWorld(plane,line["u1Mm"].toDouble(),line["v1Mm"].toDouble());
                    const auto b=sketchWorld(plane,line["u2Mm"].toDouble(),line["v2Mm"].toDouble());
                    if(a.Distance(b)>1e-9)edge=BRepBuilderAPI_MakeEdge(a,b).Edge();
                    break;
                }
            } else if(entities.contains(id))edge=SketchGeometry::analyticEdge(parameters,entities.value(id),sketchFrame_);
            if(edge.IsNull())continue;
            const auto overlay=sketchPresentation(edge);overlay->SetColor(color);overlay->SetWidth(3*devicePixelRatioF());
            overlay->SetZLayer(Graphic3d_ZLayerId_Topmost);
            context_->Display(overlay,AIS_WireFrame,-1,false);context_->Deactivate(overlay);sketchDecorations_.push_back(overlay);
        }
        for(const auto& idValue:relation["pointIds"].toArray()) {
            const auto point=points.value(idValue.toString());if(point.isEmpty() || !sketchPointVisible(idValue.toString()))continue;
            Handle(AIS_Point) marker=new AIS_Point(new Geom_CartesianPoint(sketchWorld(plane,point["uMm"].toDouble(),point["vMm"].toDouble())));
            marker->Attributes()->SetPointAspect(new Prs3d_PointAspect(Aspect_TOM_O_POINT,color,4*devicePixelRatioF()));
            marker->SetZLayer(Graphic3d_ZLayerId_Topmost);
            context_->Display(marker,0,-1,false);context_->Deactivate(marker);sketchDecorations_.push_back(marker);
        }
    };
    // The visible origin trihedron labels occupy a small screen-space area.
    // Move only a badge that would overlap that fixed decoration; its later
    // anchor and hit bounds use the shifted position.
    auto avoidOriginLabel=[&](double& u,double& v,double mmPerPixel,bool lateral=false) {
        if(!origin)return false;
        const auto center=projectSketchPoint(0,0,plane);
        const double cx=center["x"].toDouble(),cy=center["y"].toDouble();
        if(cx<0 || cy<0 || cx>=width() || cy>=height())return false;
        const auto badge=projectSketchPoint(u,v,plane);
        const double x=badge["x"].toDouble(),y=badge["y"].toDouble();
        if(x+6<cx-19 || x-6>cx+19 || y+6<cy-22 || y-6>cy+11)return false;
        if(lateral)u-=std::max(18.0,x-(cx-28))*mmPerPixel;
        else v+=std::max(28.0,y-(cy-28))*mmPerPixel;
        return true;
    };
    QMap<QString,int> badgeSlots;
    QMap<QString,double> originShiftPixels;
    for(const auto& entry:parameters["constraints"].toArray()) {
        const auto relation=entry.toObject();const auto kind=relation["type"].toString();
        const bool selectedBadge=selection["constraintIds"].toArray().contains(relation["id"]);
        const bool hoveredBadge=selection.value("hoveredConstraintId")==relation.value("id");
        bool selectedPoint=false,relatedEntity=false;
        const auto relationPoints=relation.value("pointIds").toArray();
        for(const auto& id:selection.value("pointIds").toArray())selectedPoint |= relationPoints.contains(id);
        for(const auto& id:selection.value("entityIds").toArray()) {
            relatedEntity |= relation.value("entityIds").toArray().contains(id);
            const auto entity=entities.value(id.toString());
            for(const auto& key:QStringList{"startPointId","endPointId","centerPointId","pointId","majorPointId","minorPointId"})
                relatedEntity |= entity.contains(key) && relationPoints.contains(entity.value(key));
            for(const auto& point:entity.value("pointIds").toArray())relatedEntity |= relationPoints.contains(point);
        }
        // Point-to-point Coincident has no persistent glyph, even with Show
        // Constraints on. A selected point reveals its retained relation only.
        if(kind=="coincident" && !selectedPoint && !selectedBadge)continue;
        if(!showConstraints_ && !selectedPoint && !relatedEntity && !selectedBadge)continue;

        if(kind=="tangent" || kind=="arcTangent" || kind=="circleTangent" || kind=="arcEndpointArcTangent" || kind=="arcEndpointCircleTangent" || kind=="arcCircleTangent" || kind=="arcArcTangent") {
            const auto ids=relation["entityIds"].toArray();if(ids.size()!=2)continue;
            const auto circle=entities.value(ids[0].toString()),line=entities.value(ids[1].toString());
            if(kind!="circleTangent" && kind!="arcEndpointArcTangent" && kind!="arcEndpointCircleTangent" && kind!="arcCircleTangent" && kind!="arcArcTangent" && line["type"]!="line")continue;
            QPointF contact;
            if(kind=="circleTangent" || kind=="arcEndpointArcTangent" || kind=="arcEndpointCircleTangent" || kind=="arcCircleTangent" || kind=="arcArcTangent") {
                const auto a=points.value(circle["centerPointId"].toString()),b=points.value(line["centerPointId"].toString());
                const QPointF center(a["uMm"].toDouble(),a["vMm"].toDouble()),delta(b["uMm"].toDouble()-center.x(),b["vMm"].toDouble()-center.y());
                const double distance=std::hypot(delta.x(),delta.y());if(distance<1e-9)continue;
                contact=center+delta*(circle["radiusMm"].toDouble()/distance);
            }else if(kind=="arcTangent" && !relation.contains("contact")) {
                if(circle["type"]!="arc")continue;
                const auto id=relation.contains("endpoints") ? relation["endpoints"].toArray()[0].toObject()["pointId"] : relation["pointIds"].toArray().first();
                const auto shared=points.value(id.toString());
                if(shared.isEmpty())continue;
                contact=QPointF(shared["uMm"].toDouble(),shared["vMm"].toDouble());
            } else {
                if(circle["type"]!="circle" && circle["type"]!="arc")continue;
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
            const bool localCircleTangent=kind=="tangent" && circle.value("type")=="circle";
            double u=contact.x()-(localCircleTangent ? 10 : 0)*size;
            const double v=contact.y()+(localCircleTangent ? 10 : 10+15*badgeSlots[ids[1].toString()]++)*size;
            const auto anchorId=ids[1].toString();
            double shiftedV=v;bool originAvoidanceApplied=!localCircleTangent && originShiftPixels.contains(anchorId);
            if(originAvoidanceApplied)shiftedV+=originShiftPixels.value(anchorId)*size;
            else if(avoidOriginLabel(u,shiftedV,size,localCircleTangent)) {
                originShiftPixels[anchorId]=(shiftedV-v)/size;originAvoidanceApplied=true;
            }
            TopoDS_Compound strokes;BRep_Builder builder;builder.MakeCompound(strokes);
            auto stroke=[&](double x1,double y1,double x2,double y2){builder.Add(strokes,BRepBuilderAPI_MakeEdge(sketchWorld(plane,u+x1*size,shiftedV+y1*size),sketchWorld(plane,u+x2*size,shiftedV+y2*size)).Edge());};
            // A tangent line touches the low point of a compact curved trace.
            // Keep both strokes inside the existing 12-pixel pick bounds.
            stroke(-5,-4,5,-4);
            const auto curveY=[](int px){return -4.0+0.32*px*px;};
            for(int x=-5;x<5;++x) {
                stroke(x,curveY(x),x+1,curveY(x+1));
            }
            const bool selectedGeometry=selection["entityIds"].toArray().contains(ids[0]) || selection["entityIds"].toArray().contains(ids[1]);
            highlightRelation(relation,selectedBadge,hoveredBadge);
            auto badge=sketchPresentation(strokes);badge->SetColor(selectedBadge ? Quantity_Color(0.08,0.43,0.91,Quantity_TOC_sRGB)
                : hoveredBadge ? Quantity_Color(0.96,0.59,0.09,Quantity_TOC_sRGB)
                : selectedGeometry ? Quantity_Color(0.08,0.43,0.91,Quantity_TOC_sRGB) : Quantity_Color(0.36,0.39,0.42,Quantity_TOC_sRGB));
            badge->SetWidth((selectedBadge ? 3.5 : hoveredBadge ? 3.0 : 2.0)*devicePixelRatioF());badge->SetZLayer(Graphic3d_ZLayerId_Topmost);
            context_->Display(badge,AIS_WireFrame,-1,false);context_->Deactivate(badge);sketchDecorations_.push_back(badge);
            double left=1e100,top=1e100,right=-1e100,bottom=-1e100;
            for(const double dx:{-6.0,6.0})for(const double dy:{-6.0,6.0}) {
                const auto corner=projectSketchPoint(u+dx*size,shiftedV+dy*size,plane);
                left=std::min(left,corner["x"].toDouble());right=std::max(right,corner["x"].toDouble());
                top=std::min(top,corner["y"].toDouble());bottom=std::max(bottom,corner["y"].toDouble());
            }
            constraintGlyphs_.append(QJsonObject{{"constraintId",relation["id"]},{"entityIds",ids},{"pointIds",relation["pointIds"]},{"anchorEntityId",ids[1]},
                {"kind",kind},{"contextual",!showConstraints_},{"selected",selectedBadge},{"hovered",hoveredBadge},{"contactMm",QJsonObject{{"uMm",contact.x()},{"vMm",contact.y()}}},
                {"anchorMm",QJsonObject{{"uMm",u},{"vMm",shiftedV}}},{"originAvoidanceApplied",originAvoidanceApplied},{"bounds",QJsonObject{{"x",left},{"y",top},
                    {"width",right-left},{"height",bottom-top}}},
                {"viewportLocal",true},{"units","logicalPixels"}});
            continue;
        }
        if(kind!="coincident" && kind!="equal" && kind!="horizontal" && kind!="vertical" && kind!="concentric" && kind!="perpendicular" && kind!="parallel" && kind!="collinear" && kind!="midpoint" && kind!="intersection" && kind!="pointOnCurve" && kind!="circleOffset" && kind!="symmetry")continue;
        const bool pointAlignment=(kind=="coincident" || kind=="horizontal" || kind=="vertical") && relation.contains("pointIds");
        // Concentric has one semantic center shared by the pair. One badge
        // near that center is clearer than a badge on each distant arc rim.
        const auto targets=pointAlignment ? QJsonArray{relation["pointIds"].toArray().at(1)}
            : kind=="concentric" ? QJsonArray{relation["entityIds"].toArray().at(0)}
            : kind=="symmetry" ? QJsonArray{relation["entityIds"].toArray().at(2)}
            : kind=="intersection" ? QJsonArray{relation["entityIds"].toArray().at(0)}
            : kind=="collinear" ? QJsonArray{relation["entityIds"].toArray().at(0)} : relation["entityIds"].toArray();
        for(const auto& target:targets) {
            const auto entity=entities.value(target.toString());if(!pointAlignment && entity.isEmpty())continue;
            double u,v;
            if(pointAlignment) {
                const auto point=points.value(target.toString());if(point.isEmpty())continue;
                u=point["uMm"].toDouble();v=point["vMm"].toDouble();
            } else if(kind=="concentric") {
                const auto center=points.value(entity["centerPointId"].toString());if(center.isEmpty())continue;
                u=center["uMm"].toDouble();v=center["vMm"].toDouble();
            } else if(kind=="midpoint" || kind=="intersection" || kind=="pointOnCurve") {
                const auto pointId=relation["pointIds"].toArray().at(0).toString();
                const auto point=points.value(pointId);if((kind=="midpoint" && entity["type"]!="line" && entity["type"]!="arc") || point.isEmpty())continue;
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
            const auto anchorId=target.toString();
            const double size=100/pixels;
            const bool localMidpoint=kind=="midpoint" && entity.value("construction").toBool();
            if(localMidpoint){u-=10*size;v+=10*size;}
            else v+=(10+15*badgeSlots[anchorId]++)*size;
            // A Vertical stroke must clear its own vertical target line. Keep
            // this lateral offset in the shared stroke/anchor/hit-bound frame.
            if(kind=="vertical")u+=12*size;
            if(kind=="concentric")u-=20*size;
            bool originAvoidanceApplied=!localMidpoint && originShiftPixels.contains(anchorId);
            if(originAvoidanceApplied)v+=originShiftPixels.value(anchorId)*size;
            else {
                const double originalV=v;
                if(avoidOriginLabel(u,v,size,localMidpoint)) {
                    originShiftPixels[anchorId]=(v-originalV)/size;originAvoidanceApplied=true;
                }
            }
            TopoDS_Compound strokes;BRep_Builder builder;builder.MakeCompound(strokes);
            auto stroke=[&](double x1,double y1,double x2,double y2){builder.Add(strokes,BRepBuilderAPI_MakeEdge(sketchWorld(plane,u+x1*size,v+y1*size),sketchWorld(plane,u+x2*size,v+y2*size)).Edge());};
            if(kind=="pointOnCurve" || kind=="coincident"){stroke(-3,-3,3,-3);stroke(3,-3,3,3);stroke(3,3,-3,3);stroke(-3,3,-3,-3);stroke(-1,0,1,0);}
            else if(kind=="intersection"){stroke(-4,-4,4,4);stroke(-4,4,4,-4);stroke(-5,-5,5,-5);stroke(5,-5,5,5);stroke(5,5,-5,5);stroke(-5,5,-5,-5);}
            else if(kind=="midpoint"){stroke(-5,-4,5,-4);stroke(5,-4,0,5);stroke(0,5,-5,-4);}
            else if(kind=="symmetry"){stroke(0,-5,0,5);stroke(-5,-3,-2,0);stroke(-2,0,-5,3);stroke(5,-3,2,0);stroke(2,0,5,3);}
            else if(kind=="collinear"){stroke(-6,0,-1,0);stroke(1,0,6,0);stroke(-1,-3,1,-3);stroke(-1,3,1,3);}
            else if(kind=="circleOffset"){stroke(-5,-4,5,-4);stroke(-5,4,5,4);stroke(0,-2,0,2);stroke(-2,0,0,2);stroke(2,0,0,2);}
            else if(kind=="equal"){stroke(-5,-2,5,-2);stroke(-5,2,5,2);}
            else if(kind=="concentric"){
                for(const double radius:{5.0,2.0})for(int i=0;i<16;++i){
                    const double a=2*std::numbers::pi*i/16,b=2*std::numbers::pi*(i+1)/16;
                    stroke(radius*std::cos(a),radius*std::sin(a),radius*std::cos(b),radius*std::sin(b));
                }
            }
            else if(kind=="horizontal")stroke(-5,0,5,0);
            else if(kind=="vertical")stroke(0,-5,0,5);
            else if(kind=="perpendicular"){stroke(-4,4,-4,-4);stroke(-4,-4,4,-4);}
            else {stroke(-4,-4,0,4);stroke(0,-4,4,4);}
            const bool selectedGeometry=selectedPoint || selection[pointAlignment ? "pointIds" : "entityIds"].toArray().contains(target) ||
                (kind=="midpoint" && selection["pointIds"].toArray().contains(relation["pointIds"].toArray().at(0)));
            highlightRelation(relation,selectedBadge,hoveredBadge);
            auto badge=sketchPresentation(strokes);badge->SetColor(selectedBadge ? Quantity_Color(0.08,0.43,0.91,Quantity_TOC_sRGB)
                : hoveredBadge ? Quantity_Color(0.96,0.59,0.09,Quantity_TOC_sRGB)
                : selectedGeometry ? Quantity_Color(0.08,0.43,0.91,Quantity_TOC_sRGB)
                : projectedIds.contains(target.toString()) ? Quantity_Color(0.73,0.34,0.91,Quantity_TOC_sRGB)
                : Quantity_Color(0.36,0.39,0.42,Quantity_TOC_sRGB));
            badge->SetWidth((selectedBadge ? 3.5 : hoveredBadge ? 3.0 : 2.0)*devicePixelRatioF());badge->SetZLayer(Graphic3d_ZLayerId_Topmost);
            context_->Display(badge,AIS_WireFrame,-1,false);context_->Deactivate(badge);sketchDecorations_.push_back(badge);
            double left=1e100,top=1e100,right=-1e100,bottom=-1e100;
            for(const double x:{-6.0,6.0})for(const double y:{-6.0,6.0}) {
                const auto corner=projectSketchPoint(u+x*size,v+y*size,plane);left=std::min(left,corner["x"].toDouble());right=std::max(right,corner["x"].toDouble());top=std::min(top,corner["y"].toDouble());bottom=std::max(bottom,corner["y"].toDouble());
            }
            constraintGlyphs_.append(QJsonObject{{"constraintId",relation["id"]},{"pointIds",relation["pointIds"]},{"entityIds",relation["entityIds"]},{"anchorEntityId",target},{"kind",kind},{"contextual",kind=="coincident" || !showConstraints_},{"selected",selectedBadge},{"hovered",hoveredBadge},{"originAvoidanceApplied",originAvoidanceApplied},{"anchorMm",QJsonObject{{"uMm",u},{"vMm",v}}},{"bounds",QJsonObject{{"x",left},{"y",top},{"width",right-left},{"height",bottom-top}}},{"viewportLocal",true},{"units","logicalPixels"}});
        }
    }
    // Independently authored grid/ring glyph. Its identity and hit bounds
    // belong to the retained association, never to a synthetic constraint.
    for(const auto& row:parameters["patternGroups"].toArray()){
        const auto group=row.toObject(),seed=entities.value(group["sourceEntityId"].toString());
        const bool circular=group["type"]=="circular";
        const auto center=points.value((circular ? group["centerPointId"] : seed["centerPointId"]).toString());if(center.isEmpty())continue;
        const double cu=center["uMm"].toDouble(),cv=center["vMm"].toDouble();
        const auto screen=projectSketchPoint(cu,cv,plane),unit=projectSketchPoint(cu+100,cv,plane);
        const double pixels=std::hypot(unit["x"].toDouble()-screen["x"].toDouble(),unit["y"].toDouble()-screen["y"].toDouble());if(pixels<1e-6)continue;
        const double size=100/pixels,u=circular ? cu : cu+seed["radiusMm"].toDouble()+22*size,v=circular ? cv : cv+18*size;
        TopoDS_Compound strokes;BRep_Builder builder;builder.MakeCompound(strokes);
        auto stroke=[&](double x1,double y1,double x2,double y2){builder.Add(strokes,BRepBuilderAPI_MakeEdge(sketchWorld(plane,u+x1*size,v+y1*size),sketchWorld(plane,u+x2*size,v+y2*size)).Edge());};
        if(circular){for(int k=0;k<6;++k){const double angle=k*std::numbers::pi/3,x=5*std::cos(angle)-1,y=5*std::sin(angle)-1;stroke(x,y,x+2,y);stroke(x+2,y,x+2,y+2);stroke(x+2,y+2,x,y+2);stroke(x,y+2,x,y);}}
        else for(int i=0;i<3;++i)for(int j=0;j<2;++j){const double x=-8+i*6,y=-5+j*6;stroke(x,y,x+4,y);stroke(x+4,y,x+4,y+4);stroke(x+4,y+4,x,y+4);stroke(x,y+4,x,y);}
        auto glyph=sketchPresentation(strokes);glyph->SetColor(Quantity_Color(.36,.39,.42,Quantity_TOC_sRGB));glyph->SetWidth(1.6*devicePixelRatioF());glyph->SetZLayer(Graphic3d_ZLayerId_Topmost);context_->Display(glyph,AIS_WireFrame,-1,false);context_->Deactivate(glyph);sketchDecorations_.push_back(glyph);
        double left=1e100,top=1e100,right=-1e100,bottom=-1e100;for(double x:{-10.,10.})for(double y:{-8.,8.}){const auto p=projectSketchPoint(u+x*size,v+y*size,plane);left=std::min(left,p["x"].toDouble());right=std::max(right,p["x"].toDouble());top=std::min(top,p["y"].toDouble());bottom=std::max(bottom,p["y"].toDouble());}
        patternGlyphs_.append(QJsonObject{{"groupId",group["id"]},{"sourceEntityId",group["sourceEntityId"]},{"bounds",QJsonObject{{"x",left},{"y",top},{"width",right-left},{"height",bottom-top}}},{"viewportLocal",true},{"units","logicalPixels"}});
    }
    if(selection["patternInteraction"].toObject()["active"].toBool() && !selection["patternInteraction"].toObject()["invalid"].toBool() && selection["patternInteraction"].toObject()["role"]=="values")for(const auto& row:parameters["patternGroups"].toArray()){
        const auto group=row.toObject();if(group["type"]!="rectangular")continue;const auto seed=entities.value(group["sourceEntityId"].toString()),center=points.value(seed["centerPointId"].toString());
        const QPointF origin(center["uMm"].toDouble(),center["vMm"].toDouble());
        for(int axis=1;axis<=2;++axis){const auto line=entities.value(group[QString("direction%1EntityId").arg(axis)].toString()),first=points.value(line["startPointId"].toString()),last=points.value(line["endPointId"].toString());
            auto direction=QPointF(last["uMm"].toDouble()-first["uMm"].toDouble(),last["vMm"].toDouble()-first["vMm"].toDouble());const double length=std::hypot(direction.x(),direction.y());if(length<.001)continue;direction/=length;const QPointF normal(-direction.y(),direction.x());
            const auto a=projectSketchPoint(origin.x(),origin.y(),plane),b=projectSketchPoint(origin.x()+100,origin.y(),plane);const double pixels=std::hypot(b["x"].toDouble()-a["x"].toDouble(),b["y"].toDouble()-a["y"].toDouble());if(pixels<1e-6)continue;const double size=100/pixels;
            const double spacing=group[QString("spacing%1Mm").arg(axis)].toDouble();const int quantity=group[QString("quantity%1").arg(axis)].toInt();
            for(const bool count:{false,true}){const auto anchor=origin+direction*(spacing*(count ? quantity-1 : 1))+normal*((count ? -1 : 1)*28*size);const auto screen=projectSketchPoint(anchor.x(),anchor.y(),plane);
                TopoDS_Compound strokes;BRep_Builder builder;builder.MakeCompound(strokes);auto stroke=[&](double x1,double y1,double x2,double y2){const auto p=anchor+(direction*x1+normal*y1)*size,q=anchor+(direction*x2+normal*y2)*size;builder.Add(strokes,BRepBuilderAPI_MakeEdge(sketchWorld(plane,p.x(),p.y()),sketchWorld(plane,q.x(),q.y())).Edge());};
                if(count){stroke(-5,-5,5,-5);stroke(5,-5,5,5);stroke(5,5,-5,5);stroke(-5,5,-5,-5);stroke(-2,0,2,0);stroke(0,-2,0,2);}else{stroke(-9,0,9,0);stroke(9,0,4,4);stroke(9,0,4,-4);stroke(-9,0,-4,4);stroke(-9,0,-4,-4);}
                auto graphic=sketchPresentation(strokes);graphic->SetColor(axis==1 ? Quantity_Color(.1,.43,.81,Quantity_TOC_sRGB) : Quantity_Color(.77,.3,.1,Quantity_TOC_sRGB));graphic->SetWidth(2.2*devicePixelRatioF());graphic->SetZLayer(Graphic3d_ZLayerId_Topmost);context_->Display(graphic,AIS_WireFrame,-1,false);context_->Deactivate(graphic);sketchDecorations_.push_back(graphic);
                Handle(AIS_TextLabel) label=new AIS_TextLabel;label->SetText(TCollection_ExtendedString(QString("%1%2 · %3").arg(count ? "N" : "D").arg(axis).arg(count ? QString::number(quantity) : QString::number(spacing,'g',6)+" mm").toUtf8().constData(),true));const auto labelPoint=anchor+normal*((count ? -1 : 1)*(axis==2 && !count ? 60 : 12)*size);if(axis==2 && !count){const auto a=anchor+normal*(9*size),b=anchor+normal*(54*size);auto leader=sketchPresentation(BRepBuilderAPI_MakeEdge(sketchWorld(plane,a.x(),a.y()),sketchWorld(plane,b.x(),b.y())).Edge());leader->SetColor(Quantity_Color(.42,.47,.52,Quantity_TOC_sRGB));leader->SetWidth(devicePixelRatioF());leader->SetZLayer(Graphic3d_ZLayerId_Topmost);context_->Display(leader,AIS_WireFrame,-1,false);context_->Deactivate(leader);sketchDecorations_.push_back(leader);}const auto labelScreen=projectSketchPoint(labelPoint.x(),labelPoint.y(),plane);const double labelDx=labelScreen["x"].toDouble()-screen["x"].toDouble();label->SetHJustification(std::abs(labelDx)<2 ? Graphic3d_HTA_CENTER : labelDx<0 ? Graphic3d_HTA_RIGHT : Graphic3d_HTA_LEFT);label->SetPosition(sketchWorld(plane,labelPoint.x(),labelPoint.y()));label->SetHeight(12);label->SetColor(Quantity_Color(.2,.25,.3,Quantity_TOC_sRGB));label->SetZLayer(Graphic3d_ZLayerId_Topmost);context_->Display(label,false);context_->Deactivate(label);sketchDecorations_.push_back(label);
                patternHandles_.append(QJsonObject{{"key",QString(count ? "quantity%1" : "spacing%1Mm").arg(axis)},{"kind",count ? "quantity" : "distance"},{"axis",axis},{"value",count ? double(quantity) : spacing},{"spacingMm",spacing},{"uMm",anchor.x()},{"vMm",anchor.y()},{"unitU",direction.x()},{"unitV",direction.y()},{"x",screen["x"]},{"y",screen["y"]},{"hitRadius",12},{"viewportLocal",true}});
            }
        }
    }
    if(selection["patternInteraction"].toObject()["active"].toBool() && !selection["patternInteraction"].toObject()["invalid"].toBool() && selection["patternInteraction"].toObject()["role"]=="values")for(const auto& row:parameters["patternGroups"].toArray()){
        const auto group=row.toObject();if(group["type"]!="circular")continue;
        const auto seed=entities.value(group["sourceEntityId"].toString()),seedPoint=points.value(seed["centerPointId"].toString()),pivot=points.value(group["centerPointId"].toString());
        const QPointF center(pivot["uMm"].toDouble(),pivot["vMm"].toDouble()),source(seedPoint["uMm"].toDouble(),seedPoint["vMm"].toDouble());
        const auto offset=source-center;const double radius=std::hypot(offset.x(),offset.y()),start=std::atan2(offset.y(),offset.x()),sweep=group["totalAngleDeg"].toDouble()*std::numbers::pi/180.;if(radius<.001)continue;
        const auto a=projectSketchPoint(center.x(),center.y(),plane),b=projectSketchPoint(center.x()+100,center.y(),plane);const double pixels=std::hypot(b["x"].toDouble()-a["x"].toDouble(),b["y"].toDouble()-a["y"].toDouble());if(pixels<1e-6)continue;const double size=100/pixels,ringRadius=radius+28*size;
        auto at=[&](double angle,double r){return center+QPointF(std::cos(angle),std::sin(angle))*r;};
        TopoDS_Compound strokes;BRep_Builder builder;builder.MakeCompound(strokes);
        auto edge=[&](const QPointF& p,const QPointF& q){builder.Add(strokes,BRepBuilderAPI_MakeEdge(sketchWorld(plane,p.x(),p.y()),sketchWorld(plane,q.x(),q.y())).Edge());};
        for(int i=0;i<96;++i)edge(at(start+sweep*i/96,ringRadius),at(start+sweep*(i+1)/96,ringRadius));
        // The two radial witnesses make positive sketch-UV sweep explicit.
        edge(center,at(start,ringRadius));if(group["mode"]=="angle")edge(center,at(start+sweep,ringRadius));
        auto graphic=sketchPresentation(strokes);graphic->SetColor(Quantity_Color(.32,.48,.65,Quantity_TOC_sRGB));graphic->SetWidth(1.2*devicePixelRatioF());graphic->SetZLayer(Graphic3d_ZLayerId_Topmost);context_->Display(graphic,AIS_WireFrame,-1,false);context_->Deactivate(graphic);sketchDecorations_.push_back(graphic);
        for(const bool count:{false,true}){
            const double angle=count ? (group["mode"]=="angle" ? start+sweep*.5 : start-std::numbers::pi/4) : start+sweep;const auto anchor=at(angle,ringRadius),radial=QPointF(std::cos(angle),std::sin(angle)),tangent=QPointF(-radial.y(),radial.x());const auto screen=projectSketchPoint(anchor.x(),anchor.y(),plane);
            const bool draggable=count || group["mode"]=="angle";
            if(draggable){TopoDS_Compound grip;builder.MakeCompound(grip);auto stroke=[&](double x1,double y1,double x2,double y2){const auto p=anchor+(tangent*x1+radial*y1)*size,q=anchor+(tangent*x2+radial*y2)*size;builder.Add(grip,BRepBuilderAPI_MakeEdge(sketchWorld(plane,p.x(),p.y()),sketchWorld(plane,q.x(),q.y())).Edge());};
                if(count){stroke(-5,-5,5,-5);stroke(5,-5,5,5);stroke(5,5,-5,5);stroke(-5,5,-5,-5);stroke(-2,0,2,0);stroke(0,-2,0,2);}else{stroke(-8,0,8,0);stroke(8,0,3,4);stroke(8,0,3,-4);}
                auto handle=sketchPresentation(grip);handle->SetColor(count ? Quantity_Color(.77,.3,.1,Quantity_TOC_sRGB) : Quantity_Color(.1,.43,.81,Quantity_TOC_sRGB));handle->SetWidth(2.2*devicePixelRatioF());handle->SetZLayer(Graphic3d_ZLayerId_Topmost);context_->Display(handle,AIS_WireFrame,-1,false);context_->Deactivate(handle);sketchDecorations_.push_back(handle);
                patternHandles_.append(QJsonObject{{"key",count ? "quantity" : "totalAngleDeg"},{"kind",count ? "quantity" : "angle"},{"value",count ? group["quantity"].toDouble() : group["totalAngleDeg"].toDouble()},{"spacingMm",radius*.3},{"uMm",anchor.x()},{"vMm",anchor.y()},{"centerU",center.x()},{"centerV",center.y()},{"unitU",tangent.x()},{"unitV",tangent.y()},{"x",screen["x"]},{"y",screen["y"]},{"hitRadius",12},{"viewportLocal",true}});
            }
            const auto labelPoint=at(angle,ringRadius+14*size);const auto labelScreen=projectSketchPoint(labelPoint.x(),labelPoint.y(),plane);const double dx=labelScreen["x"].toDouble()-screen["x"].toDouble();
            Handle(AIS_TextLabel) label=new AIS_TextLabel;const auto text=count ? QString("N · %1").arg(group["quantity"].toInt()) : group["mode"]=="full" ? QString("Full · 360°") : QString("A · %1°").arg(group["totalAngleDeg"].toDouble());label->SetText(TCollection_ExtendedString(text.toUtf8().constData(),true));label->SetHJustification(std::abs(dx)<2 ? Graphic3d_HTA_CENTER : dx<0 ? Graphic3d_HTA_RIGHT : Graphic3d_HTA_LEFT);label->SetPosition(sketchWorld(plane,labelPoint.x(),labelPoint.y()));label->SetHeight(12);label->SetColor(Quantity_Color(.2,.25,.3,Quantity_TOC_sRGB));label->SetZLayer(Graphic3d_ZLayerId_Topmost);context_->Display(label,false);context_->Deactivate(label);sketchDecorations_.push_back(label);
        }
    }
    QMap<QString,QJsonObject> measurements;
    for(const auto& entry:diagnostics["dimensionValues"].toArray()){const auto value=entry.toObject();measurements[value["id"].toString()]=value;}
    auto layoutFeature=SketchGeometry::expandedProjectionFeature(sourceFeature,{{"lines",projectedLines_},{"circles",projectedCircles_},{"points",supportPoints_}});
    auto layoutParameters=layoutFeature["parameters"].toObject();
    layoutParameters["dimensions"]=feature["parameters"].toObject()["dimensions"].toArray();
    layoutFeature["parameters"]=layoutParameters;
    auto dimensions=parameters.value("dimensions").toArray();
    // Polygon drivers are retained group parameters, not extra solver dimensions.
    for(const auto& row:parameters["polygonGroups"].toArray()){
        const auto group=row.toObject();dimensions.append(QJsonObject{{"id",group["id"]},{"kind","polygonSize"},{"valueMm",group["sizeMm"]},{"driving",true}});
    }
    if(!pending.isEmpty() && !pending.contains("dimensionId")){auto preview=pending;preview["id"]="pending";dimensions.append(preview);}
    QJsonArray dimensionLayouts;
    if(showDimensions_)for(const auto& entry:dimensions) {
        const auto dimension=entry.toObject();const auto measured=measurements.value(dimension.value("id").toString());
        const bool angular=dimension["kind"]=="arcSweep" || dimension["kind"]=="angle" || dimension["kind"]=="overallSlotAngle";const auto valueKey=angular ? "valueDeg" : "valueMm";
        const double value=dimension.value("id")=="pending" ? pending.value(angular ? "measuredValueDeg" : "measuredValueMm").toDouble() : measured.value(valueKey).toDouble(dimension.value(valueKey).toDouble());
        const auto layout=dimensionLayout(layoutFeature,dimension,value,units);if(!layout.isEmpty())dimensionLayouts.append(layout);
    }
    dimensionLayouts=resolveDimensionLabels(dimensionLayouts,plane);
    for(const auto& entry:dimensionLayouts){
        const auto layout=entry.toObject();QJsonObject dimension;for(const auto& d:dimensions)if(d.toObject().value("id")==layout.value("dimensionId")){dimension=d.toObject();break;}annotationLayouts_.append(layout);
        if(layout["anchorKind"]=="center"){
            const auto anchor=layout["anchor"].toObject();
            Handle(AIS_Point) marker=new AIS_Point(new Geom_CartesianPoint(sketchWorld(plane,anchor["uMm"].toDouble(),anchor["vMm"].toDouble())));
            marker->Attributes()->SetPointAspect(new Prs3d_PointAspect(Aspect_TOM_PLUS,Quantity_Color(0.12,0.14,0.16,Quantity_TOC_sRGB),3*devicePixelRatioF()));
            marker->SetZLayer(Graphic3d_ZLayerId_Topmost);context_->Display(marker,0,-1,false);context_->Deactivate(marker);sketchDecorations_.push_back(marker);
        }
        const bool selected=selection.value("dimensionId")==dimension.value("id");
        const Quantity_Color color=selected || dimension.value("id")=="pending" ? Quantity_Color(0.10,0.34,0.84,Quantity_TOC_RGB) : Quantity_Color(0.10,0.11,0.12,Quantity_TOC_sRGB);
        const auto line=layout.value("line").toArray();auto world=[&](const QJsonValue& v){const auto p=v.toObject();return sketchWorld(plane,p.value("uMm").toDouble(),p.value("vMm").toDouble());};
        const auto frame=frameFor(plane);const auto n=frame.value("normal").toArray(),u=frame.value("uAxis").toArray(),origin=frame.value("origin").toArray();
        const double size=layout.value("mmPerPixel").toDouble();
        if(layout["kind"]!="radius" && layout["kind"]!="arcSweep" && layout["kind"]!="angle" && layout["kind"]!="overallSlotAngle") {
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
                for(const auto& value:layout["arcSegments"].toArray()){const auto arc=value.toObject();const auto centerId=layout["kind"]=="angle" || layout["kind"]=="overallSlotAngle" ? QJsonValue("dimension-center") : dimension["entityId"];
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
        const auto leader=layout.value("displayLeader").toArray();if(leader.size()==2){const auto a=world(leader[0]),b=world(leader[1]);if(a.Distance(b)>1e-9){const auto wire=sketchPresentation(BRepBuilderAPI_MakeEdge(a,b).Edge());wire->SetColor(color);wire->SetWidth(devicePixelRatioF());wire->SetZLayer(Graphic3d_ZLayerId_Topmost);context_->Display(wire,AIS_WireFrame,-1,false);context_->Deactivate(wire);sketchDecorations_.push_back(wire);}}
        const auto text=layout.value("label").toObject(),anchor=layout.value("displayAnchor").toObject(layout.value("annotation").toObject());
        Handle(AIS_TextLabel) label=new AIS_TextLabel;label->SetText(TCollection_ExtendedString(text.value("text").toString().toUtf8().constData(),true));
        label->SetPosition(sketchWorld(plane,anchor.value("uMm").toDouble(),anchor.value("vMm").toDouble()));label->SetHeight(13);label->SetFont("Arial");label->SetAngle(text.value("angleDegrees").toDouble()*std::acos(-1.0)/180);
        label->SetHJustification(Graphic3d_HTA_CENTER);label->SetVJustification(Graphic3d_VTA_CENTER);label->SetZoomable(false);label->SetDisplayType(layout.value("automaticPlacement").toBool()?Aspect_TODT_SUBTITLE:Aspect_TODT_NORMAL);
        if(layout.value("automaticPlacement").toBool())label->SetColorSubTitle(Quantity_Color(1,1,1,Quantity_TOC_RGB));
        label->SetColor(color);label->SetZLayer(Graphic3d_ZLayerId_Topmost);context_->Display(label,0,-1,false);context_->Deactivate(label);sketchDecorations_.push_back(label);
    }
    if(showDimensions_)for(const auto& row:parameters.value("chamferGroups").toArray()){
        const auto group=row.toObject();const auto contacts=group.value("pointIds").toArray();
        const bool two=group.value("type")=="twoDistance",angular=group.value("type")=="distanceAngle";
        const auto source=sourceFeature.value("parameters").toObject();QMap<QString,QPointF> authoredPoints;
        for(const auto& entry:source.value("points").toArray()){const auto p=entry.toObject();authoredPoints[p.value("id").toString()]={p.value("uMm").toDouble(),p.value("vMm").toDouble()};}
        QJsonObject lines[2];const auto ids=group.value("sourceEntityIds").toArray();
        for(const auto& entry:source.value("entities").toArray())for(int j=0;j<2;++j)if(entry.toObject().value("id")==ids[j])lines[j]=entry.toObject();
        QPointF vertex;bool found=false;
        for(const auto* first:{"startPointId","endPointId"})for(const auto* second:{"startPointId","endPointId"}){
            const auto a=authoredPoints.value(lines[0].value(first).toString()),b=authoredPoints.value(lines[1].value(second).toString());
            if(QLineF(a,b).length()<1e-6){vertex=(a+b)*0.5;found=true;}
        }
        const auto a=points.value(contacts[0].toString()),b=points.value(contacts[1].toString());
        const QPointF endpoints[]{QPointF(a.value("uMm").toDouble(),a.value("vMm").toDouble()),QPointF(b.value("uMm").toDouble(),b.value("vMm").toDouble())};
        const double d=group.value("distanceMm").toDouble(),theta=group.value("angleDeg").toDouble()*std::acos(-1.0)/180;
        const QPointF toward=(vertex-endpoints[0])/d,segment=endpoints[1]-endpoints[0];
        const double turn=toward.x()*segment.y()-toward.y()*segment.x()>=0?1:-1;
        const auto rotate=[](const QPointF& p,double radians){return QPointF(p.x()*std::cos(radians)-p.y()*std::sin(radians),p.x()*std::sin(radians)+p.y()*std::cos(radians));};
        const auto screen=projectSketchPoint(endpoints[0].x(),endpoints[0].y(),plane),offset=projectSketchPoint(endpoints[0].x()+100,endpoints[0].y(),plane);
        const double pixelsPerMm=std::max(0.001,std::hypot(offset.value("x").toDouble()-screen.value("x").toDouble(),offset.value("y").toDouble()-screen.value("y").toDouble())/100);
        const double radius=28/pixelsPerMm;
        for(int side=0;side<2;++side){
            const bool angle=angular && side==1,driving=two || angular || side==0;
            const auto at=angle?endpoints[0]+rotate(toward,turn*theta/2)*(radius+18/pixelsPerMm):endpoints[side]+QPointF(two || angular?7:2,two || angular?7:3);
            const auto quantity=group.value(angle?"angleDeg":two && side==1?"distance2Mm":"distanceMm");
            const auto text=angle?QString::number(quantity.toDouble(),'g',6)+QString::fromUtf8("°")
                :QString(driving?"%1 %2":"(%1 %2)").arg(quantity.toDouble()/(units=="in"?25.4:1.0),0,'g',6).arg(units);
            const auto pixel=projectSketchPoint(at.x(),at.y(),plane);QFont font("Arial");font.setPixelSize(13);const QFontMetricsF metrics(font);
            annotationLayouts_.append(QJsonObject{{"dimensionId",side==0?group.value("id"):contacts[side]},{"chamferId",group.value("id")},
                {"kind",angle?"chamferAngle":"chamferDistance"},{"driving",driving},{"side",side},{"independentSecond",two && side==1},
                {angle?"valueDeg":"valueMm",quantity},{"annotation",QJsonObject{{"uMm",at.x()},{"vMm",at.y()}}},
                {"label",QJsonObject{{"x",pixel.value("x")},{"y",pixel.value("y")},{"width",metrics.horizontalAdvance(text)+8},{"height",metrics.height()+6},{"angleDegrees",0},{"text",text}}}});
            Handle(AIS_TextLabel) label=new AIS_TextLabel;label->SetText(TCollection_ExtendedString(text.toUtf8().constData(),true));label->SetPosition(sketchWorld(plane,at.x(),at.y()));label->SetHeight(13);label->SetFont("Arial");label->SetZoomable(false);
            label->SetHJustification(Graphic3d_HTA_CENTER);label->SetVJustification(Graphic3d_VTA_CENTER);label->SetColor(driving?Quantity_Color(0.10,0.34,0.84,Quantity_TOC_RGB):Quantity_Color(0.42,0.46,0.51,Quantity_TOC_RGB));label->SetZLayer(Graphic3d_ZLayerId_Topmost);
            context_->Display(label,0,-1,false);context_->Deactivate(label);sketchDecorations_.push_back(label);
        }
        const bool handles=found && (selection.value("tool")=="select" || (selection.value("tool")=="chamfer" && selection.value("entityIds").toArray().contains(group.value("lineId"))));
        if(angular && found){
            // Angle is visibly based at the first contact, toward the old corner.
            BRepBuilderAPI_MakePolygon arc;for(int step=0;step<=24;++step){const auto at=endpoints[0]+rotate(toward,turn*theta*step/24)*radius;arc.Add(sketchWorld(plane,at.x(),at.y()));}
            if(arc.IsDone()){Handle(AIS_Shape) graphic=new AIS_Shape(arc.Wire());graphic->SetColor(Quantity_Color(0.12,0.55,0.72,Quantity_TOC_RGB));graphic->SetZLayer(Graphic3d_ZLayerId_Topmost);context_->Display(graphic,0,-1,false);context_->Deactivate(graphic);sketchDecorations_.push_back(graphic);}
        }
        if(!handles)continue;
        for(int side=0;side<(two || angular?2:1);++side){
            const bool angle=angular && side==1;
            const QPointF handle=angle?endpoints[0]+rotate(toward,turn*theta)*radius:two || angular?endpoints[side]:(endpoints[0]+endpoints[1])/2;
            const double distance=group.value(two && side==1?"distance2Mm":"distanceMm").toDouble();
            const auto axis=angle?toward:(handle-vertex)/distance;const auto origin=angle?endpoints[0]:vertex;const auto pixel=projectSketchPoint(handle.x(),handle.y(),plane);
            chamferHandles_.append(QJsonObject{{"chamferId",group.value("id")},{"side",side},{"kind",angle?"angle":"distance"},{"distanceMm",distance},{"angleDeg",group.value("angleDeg")},
                {"x",pixel.value("x")},{"y",pixel.value("y")},{"hitRadius",11},{"originUMm",origin.x()},{"originVMm",origin.y()},{"axisU",axis.x()},{"axisV",axis.y()},{"turn",turn}});
            if(!angle){Handle(AIS_Shape) guide=new AIS_Shape(BRepBuilderAPI_MakeEdge(sketchWorld(plane,vertex.x(),vertex.y()),sketchWorld(plane,handle.x(),handle.y())).Edge());guide->SetColor(Quantity_Color(0.18,0.66,0.77,Quantity_TOC_RGB));guide->Attributes()->LineAspect()->SetTypeOfLine(Aspect_TOL_DASH);guide->SetZLayer(Graphic3d_ZLayerId_Topmost);context_->Display(guide,0,-1,false);context_->Deactivate(guide);sketchDecorations_.push_back(guide);}
            Handle(AIS_Point) grip=new AIS_Point(new Geom_CartesianPoint(sketchWorld(plane,handle.x(),handle.y())));grip->Attributes()->SetPointAspect(new Prs3d_PointAspect(Aspect_TOM_O_POINT,Quantity_Color(0.08,0.70,0.82,Quantity_TOC_RGB),6*devicePixelRatioF()));grip->SetZLayer(Graphic3d_ZLayerId_Topmost);context_->Display(grip,0,-1,false);context_->Deactivate(grip);sketchDecorations_.push_back(grip);
        }
    }
    context_->UpdateCurrentViewer();
}
QJsonObject ReferenceView::frameFor(const QString& plane) const {
    if(plane=="face" || plane=="construction") {
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
    if(plane!=sketchPlane_)cancelSlotPointDrag();
    sketchPlane_=plane;
    if(!plane.isEmpty() || orientation!="face")sketchFrame_=resolvedFrame;
}
void ReferenceView::lookAtSketch(const QString& plane,const QJsonObject& resolvedFrame,const TopoDS_Shape& content) {
    setSketchPlane(plane,resolvedFrame);
    orientation=(plane=="face" || plane=="construction") ? "face" : plane=="XY" ? "top" : plane=="XZ" ? "front" : "right";
    const auto o=frameFor(plane).value("origin").toArray();
    sceneCenter_=gp_Pnt(o[0].toDouble(),o[1].toDouble(),o[2].toDouble());sceneScale_=100;
    if(!content.IsNull()) {
        Bnd_Box bounds;BRepBndLib::AddOptimal(content,bounds,false,false);
        if(!bounds.IsVoid()) {
            double x0,y0,z0,x1,y1,z1;bounds.Get(x0,y0,z0,x1,y1,z1);
            sceneCenter_=gp_Pnt((x0+x1)/2,(y0+y1)/2,(z0+z1)/2);
            sceneScale_=std::max({x1-x0,y1-y0,z1-z0,1.0})*1.7;
        }
    }
    viewCenter_=sceneCenter_;pan={};zoom=1;sync();
}
bool ReferenceView::fitSelection(const TopoDS_Shape& shape) {
    if(shape.IsNull())return false;
    Bnd_Box bounds;BRepBndLib::AddOptimal(shape,bounds,false,false);
    if(bounds.IsVoid())return false;
    double x0,y0,z0,x1,y1,z1;bounds.Get(x0,y0,z0,x1,y1,z1);
    if(!std::isfinite(x0)||!std::isfinite(y0)||!std::isfinite(z0)||
        !std::isfinite(x1)||!std::isfinite(y1)||!std::isfinite(z1))return false;
    sceneCenter_=gp_Pnt((x0+x1)/2,(y0+y1)/2,(z0+z1)/2);
    sceneScale_=std::max({x1-x0,y1-y0,z1-z0,1.0})*1.7;
    viewCenter_=sceneCenter_;pan={};zoom=1;sync();return true;
}
bool ReferenceView::fitDraftEntity(const QString& id) {
    return fitSelection(draftEntityShapes_.value(id));
}
bool ReferenceView::fitFinishedSketch(const EvaluatedSketch& sketch) {
    return fitSelection(visibleFinishedSketchShape(sketch));
}
bool ReferenceView::lookAtPlane(const QJsonObject& frame,const TopoDS_Shape& shape) {
    const auto normal=frame.value("normal").toArray(),up=frame.value("vAxis").toArray();
    if(normal.size()!=3 || up.size()!=3 || shape.IsNull())return false;
    for(const auto& value:normal)if(!value.isDouble() || !std::isfinite(value.toDouble()))return false;
    for(const auto& value:up)if(!value.isDouble() || !std::isfinite(value.toDouble()))return false;
    const gp_Vec n(normal[0].toDouble(),normal[1].toDouble(),normal[2].toDouble());
    const gp_Vec v(up[0].toDouble(),up[1].toDouble(),up[2].toDouble());
    if(n.Magnitude()<1e-9 || v.Magnitude()<1e-9 || n.Crossed(v).Magnitude()<1e-9)return false;
    const gp_Vec levelUp=v-n*(v.Dot(n)/n.SquareMagnitude());
    if(levelUp.Magnitude()<1e-9)return false;
    Bnd_Box bounds;BRepBndLib::AddOptimal(shape,bounds,false,false);
    if(bounds.IsVoid())return false;
    double x0,y0,z0,x1,y1,z1;bounds.Get(x0,y0,z0,x1,y1,z1);
    if(!std::isfinite(x0)||!std::isfinite(y0)||!std::isfinite(z0)||
        !std::isfinite(x1)||!std::isfinite(y1)||!std::isfinite(z1))return false;
    customDirection_=gp_Dir(-n);customUp_=gp_Dir(levelUp);
    orientation="custom";
    sceneCenter_=gp_Pnt((x0+x1)/2,(y0+y1)/2,(z0+z1)/2);
    sceneScale_=std::max({x1-x0,y1-y0,z1-z0,1.0})*1.7;
    viewCenter_=sceneCenter_;pan={};zoom=1;sync();return true;
}
bool ReferenceView::lookAtFinishedSketch(const EvaluatedSketch& sketch) {
    if(sketch.plane!="XY" && sketch.plane!="XZ" && sketch.plane!="YZ" && sketch.plane!="face" && sketch.plane!="construction")return false;
    return lookAtPlane((sketch.plane=="face" || sketch.plane=="construction") ? sketch.resolvedFrame : frameFor(sketch.plane),
        visibleFinishedSketchShape(sketch));
}
QJsonObject ReferenceView::sketchPoint(double x,double y,const QString& plane,const QJsonObject& resolvedFrame) const {
    if(view_.IsNull() || x<0 || y<0 || x>=width() || y>=height())throw std::invalid_argument("Point is outside the initialized viewport");
    const auto scale=windowScale();double px,py,pz,dx,dy,dz;
    view_->ConvertWithProj(qRound(x*scale.x()),qRound(y*scale.y()),px,py,pz,dx,dy,dz);
    const auto frame=resolvedFrame.isEmpty() ? frameFor(plane) : resolvedFrame;const auto normal=frame["normal"].toArray(),origin=frame["origin"].toArray();
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
    if((plane=="face" || plane=="construction") && !resolvedFrame.isEmpty()) {
        const auto o=resolvedFrame.value("origin").toArray(),u=resolvedFrame.value("uAxis").toArray(),v=resolvedFrame.value("vAxis").toArray();
        world=gp_Pnt(o[0].toDouble()+uMm*u[0].toDouble()+vMm*v[0].toDouble(),
            o[1].toDouble()+uMm*u[1].toDouble()+vMm*v[1].toDouble(),
            o[2].toDouble()+uMm*u[2].toDouble()+vMm*v[2].toDouble());
    } else world=sketchWorld(plane,uMm,vMm);
    int x,y;view_->Convert(world.X(),world.Y(),world.Z(),x,y);
    const auto scale=windowScale();return {{"x",x/scale.x()},{"y",y/scale.y()},{"viewportLocal",true},{"units","logicalPixels"}};
}
void ReferenceView::showBreakPreview(const QString& plane,const QJsonArray& points) {
    clearSketchCursor();if(context_.IsNull() || points.isEmpty())return;
    TopoDS_Compound compound;BRep_Builder builder;builder.MakeCompound(compound);
    const double size=view_->Camera()->Scale()*5.0/std::max(height(),1);
    for(auto value:points){const auto p=value.toObject();const double u=p.value("uMm").toDouble(),v=p.value("vMm").toDouble();
        for(double sign:{-1.0,1.0})builder.Add(compound,BRepBuilderAPI_MakeEdge(sketchWorld(plane,u-size,v-sign*size),sketchWorld(plane,u+size,v+sign*size)).Edge());}
    sketchCursor_=new AIS_Shape(compound);sketchCursor_->SetColor(Quantity_Color(.86,.12,.10,Quantity_TOC_sRGB));sketchCursor_->SetWidth(3);sketchCursor_->SetZLayer(Graphic3d_ZLayerId_Topmost);
    context_->Display(sketchCursor_,AIS_WireFrame,-1,false);context_->Deactivate(sketchCursor_);context_->UpdateCurrentViewer();
}
void ReferenceView::showTrimPreview(const QString& plane,const QJsonObject& segment) {
    clearSketchCursor();if(context_.IsNull() || segment.isEmpty())return;
    const auto a=sketchWorld(plane,segment["u1Mm"].toDouble(),segment["v1Mm"].toDouble()),b=sketchWorld(plane,segment["u2Mm"].toDouble(),segment["v2Mm"].toDouble());
    const gp_Vec delta(a,b);if(delta.Magnitude()<1e-9)return;
    const auto shape=BRepPrimAPI_MakeCylinder(gp_Ax2(a,gp_Dir(delta)),view_->Camera()->Scale()*2.0/std::max(height(),1),delta.Magnitude()).Shape();
    sketchCursor_=new AIS_Shape(shape);sketchCursor_->SetColor(Quantity_Color(.86,.12,.10,Quantity_TOC_sRGB));sketchCursor_->SetZLayer(Graphic3d_ZLayerId_Topmost);sketchCursor_->Attributes()->SetShadingModel(Graphic3d_TOSM_UNLIT);context_->Display(sketchCursor_,AIS_Shaded,-1,false);context_->Deactivate(sketchCursor_);context_->UpdateCurrentViewer();
}
void ReferenceView::showExtendPreview(const QString& plane,const QJsonObject& segment) {
    showTrimPreview(plane,segment);
    if(sketchCursor_.IsNull() || context_.IsNull())return;
    sketchCursor_->SetColor(Quantity_Color(.04,.55,.29,Quantity_TOC_sRGB));
    context_->Redisplay(sketchCursor_,false);context_->UpdateCurrentViewer();
}
void ReferenceView::showSketchCursor(const QString& plane,const QPointF& first,const QPointF& second,bool rectangle,bool centered) {
    clearSketchCursor();if(context_.IsNull() || first==second)return;
    if(centered && (first.x()==second.x() || first.y()==second.y()))return;
    const QPointF origin=centered ? QPointF(2*first.x()-second.x(),2*first.y()-second.y()) : first;
    BRepBuilderAPI_MakePolygon polygon;polygon.Add(sketchWorld(plane,origin.x(),origin.y()));
    if(rectangle) {
        if(first.x()==second.x() || first.y()==second.y())return;
        polygon.Add(sketchWorld(plane,second.x(),origin.y()));polygon.Add(sketchWorld(plane,second.x(),second.y()));
        polygon.Add(sketchWorld(plane,origin.x(),second.y()));polygon.Close();
    } else polygon.Add(sketchWorld(plane,second.x(),second.y()));
    if(!polygon.IsDone())return;
    sketchCursor_=sketchPresentation(polygon.Wire());
    sketchCursor_->SetColor(Quantity_Color(0.13,0.42,0.72,Quantity_TOC_sRGB));
    sketchCursor_->SetWidth(2*devicePixelRatioF());
    sketchCursor_->SetZLayer(Graphic3d_ZLayerId_Topmost);
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
    } else if(snap.value("target").toObject().value("kind")=="tangent" || snap.value("tangentCue").toBool()){
        segment(-1,-1,1,-1);segment(1,-1,1,1);segment(1,1,-1,1);segment(-1,1,-1,-1);
    } else if(snap.value("target").toObject().value("kind")=="filletCorner"){
        segment(-1,-1,1,-1);segment(1,-1,1,1);segment(1,1,-1,1);segment(-1,1,-1,-1);
    } else if(snap.value("target").toObject().value("kind")=="intersection"){
        segment(-1,-1,1,1);segment(-1,1,1,-1);segment(-1,-1,1,-1);segment(1,-1,1,1);segment(1,1,-1,1);segment(-1,1,-1,-1);
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
    context_->Display(sketchCursor_,AIS_WireFrame,-1,false);context_->Deactivate(sketchCursor_);
    if(entity["type"]=="controlSpline") {
        QMap<QString,QJsonObject> points;for(const auto& v:parameters["points"].toArray()){const auto p=v.toObject();points[p["id"].toString()]=p;}
        TopoDS_Compound cage;BRep_Builder builder;builder.MakeCompound(cage);const auto ids=entity["pointIds"].toArray();int legs=0;
        for(int i=1;i<ids.size();++i){const auto a=points[ids[i-1].toString()],b=points[ids[i].toString()];const auto start=sketchWorld(parameters["plane"].toString(),a["uMm"].toDouble(),a["vMm"].toDouble()),end=sketchWorld(parameters["plane"].toString(),b["uMm"].toDouble(),b["vMm"].toDouble());if(start.Distance(end)<1e-9)continue;builder.Add(cage,BRepBuilderAPI_MakeEdge(start,end).Edge());++legs;}
        if(legs){splineCageCursor_=new AIS_Shape(cage);const Quantity_Color color(.42,.24,.08,Quantity_TOC_RGB);Handle(Prs3d_LineAspect) dashed=new Prs3d_LineAspect(color,Aspect_TOL_DASH,1.5*devicePixelRatioF());splineCageCursor_->SetColor(color);splineCageCursor_->Attributes()->SetWireAspect(dashed);splineCageCursor_->Attributes()->SetLineAspect(dashed);splineCageCursor_->SetZLayer(Graphic3d_ZLayerId_Topmost);context_->Display(splineCageCursor_,AIS_WireFrame,-1,false);context_->Deactivate(splineCageCursor_);}
    }
    context_->UpdateCurrentViewer();
}
void ReferenceView::showSketchSlotCursor(const QJsonObject& feature,bool arcSlot) {
    clearSketchCursor();if(context_.IsNull())return;
    const auto parameters=feature["parameters"].toObject();
    const auto groups=parameters["slotGroups"].toArray();
    const auto arcGroups=parameters["arcSlotGroups"].toArray();
    if(arcSlot ? arcGroups.isEmpty() : groups.isEmpty())return;
    QSet<QString> ids;
    if(arcSlot){
        const auto group=arcGroups.last().toObject();
        for(const auto& key:{"pathArcId","outerArcId","innerArcId"})ids.insert(group[key].toString());
        for(const auto& cap:group["capIds"].toArray())ids.insert(cap.toString());
    } else {
        const auto group=groups.last().toObject();
        for(const auto& id:group["sideIds"].toArray())ids.insert(id.toString());
        for(const auto& id:group["arcIds"].toArray())ids.insert(id.toString());
        ids.insert(group["centerlineId"].toString());
    }
    TopoDS_Compound compound;BRep_Builder builder;builder.MakeCompound(compound);
    for(const auto& entry:parameters["entities"].toArray()){
        const auto entity=entry.toObject();if(ids.contains(entity["id"].toString()))
            builder.Add(compound,SketchGeometry::analyticEdge(parameters,entity,sketchFrame_));
    }
    sketchCursor_=sketchPresentation(compound);
    sketchCursor_->SetColor(Quantity_Color(0.30,0.58,0.87,Quantity_TOC_RGB));
    sketchCursor_->SetWidth(2*devicePixelRatioF());
    context_->Display(sketchCursor_,AIS_WireFrame,-1,false);context_->Deactivate(sketchCursor_);context_->UpdateCurrentViewer();
}
void ReferenceView::showSketchPolygonCursor(const QJsonObject& feature) {
    clearSketchCursor();if(context_.IsNull())return;
    const auto parameters=feature["parameters"].toObject();const auto groups=parameters["polygonGroups"].toArray();if(groups.isEmpty())return;
    showSketchEdgesCursor(feature,groups.last().toObject()["entityIds"].toArray());
}
void ReferenceView::showSketchEdgesCursor(const QJsonObject& feature,const QJsonArray& ids) {
    clearSketchCursor();if(context_.IsNull())return;
    const auto parameters=feature["parameters"].toObject();
    TopoDS_Compound compound;BRep_Builder builder;builder.MakeCompound(compound);
    for(const auto& entry:parameters["entities"].toArray()){
        const auto entity=entry.toObject();if(ids.contains(entity["id"]))builder.Add(compound,SketchGeometry::analyticEdge(parameters,entity,sketchFrame_));
    }
    sketchCursor_=sketchPresentation(compound);sketchCursor_->SetColor(Quantity_Color(0.30,0.58,0.87,Quantity_TOC_RGB));
    sketchCursor_->SetWidth(2*devicePixelRatioF());context_->Display(sketchCursor_,AIS_WireFrame,-1,false);context_->Deactivate(sketchCursor_);context_->UpdateCurrentViewer();
}
void ReferenceView::showSketchEllipseCursor(const QJsonObject& feature) {
    clearSketchCursor();if(context_.IsNull())return;
    const auto parameters=feature["parameters"].toObject();
    QJsonObject ellipse;
    for(const auto& value:parameters["entities"].toArray())if(value.toObject()["type"]=="ellipse")ellipse=value.toObject();
    if(ellipse.isEmpty())return;
    sketchCursor_=sketchPresentation(SketchGeometry::analyticEdge(parameters,ellipse,sketchFrame_));
    sketchCursor_->SetColor(Quantity_Color(0.30,0.58,0.87,Quantity_TOC_RGB));
    sketchCursor_->SetWidth(2*devicePixelRatioF());
    context_->Display(sketchCursor_,AIS_WireFrame,-1,false);context_->Deactivate(sketchCursor_);context_->UpdateCurrentViewer();
}
void ReferenceView::showSlotDragPreview(const EvaluatedSketch& evaluated,const QJsonObject& draft,const QJsonObject& selection,const QJsonObject& diagnostics,const QString& units) {
    const auto gesture=slotPointDrag_;showSketchDraft(evaluated);decorateSketch(draft,selection,diagnostics,units);slotPointDrag_=gesture;
}
void ReferenceView::cancelSlotPointDrag() {
    if(slotPointDrag_.isEmpty())return;
    slotPointDrag_={};clearSketchCursor();
}
void ReferenceView::clearSketchCursor() {
    if(!context_.IsNull() && !sketchCursor_.IsNull())context_->Remove(sketchCursor_,false);
    if(!context_.IsNull() && !sketchSnapCue_.IsNull())context_->Remove(sketchSnapCue_,false);
    if(!context_.IsNull() && !splineCageCursor_.IsNull())context_->Remove(splineCageCursor_,false);
    splineCageCursor_.Nullify();
    sketchCursor_.Nullify();
    sketchSnapCue_.Nullify();sketchSnapState_={};
}
void ReferenceView::fit(const std::vector<EvaluatedBody>& bodies,const std::vector<EvaluatedSketch>& sketches) {
    Bnd_Box bounds;
    for(const auto& canvas:canvasModels_)if(context_->IsDisplayed(canvas))BRepBndLib::AddOptimal(canvas->Shape(),bounds,false,false);
    for(const auto& body:bodies)if(body.visible){
        // Evaluation already computed these exact AddOptimal bounds. Repeating
        // that kernel traversal on the GUI thread stalls Fit on large designs.
        const auto box=body.summary.value("boundsMm").toObject();
        const auto low=box.value("min").toArray(),high=box.value("max").toArray();
        bool usable=low.size()==3 && high.size()==3;
        double lo[3]{},hi[3]{};
        if(usable)for(int axis=0;axis<3;++axis){
            usable=low[axis].isDouble() && high[axis].isDouble();
            if(!usable)break;
            lo[axis]=low[axis].toDouble();hi[axis]=high[axis].toDouble();
            usable=std::isfinite(lo[axis]) && std::isfinite(hi[axis]) && lo[axis]<=hi[axis];
            if(!usable)break;
        }
        if(usable)bounds.Update(lo[0],lo[1],lo[2],hi[0],hi[1],hi[2]);
        else BRepBndLib::AddOptimal(body.shape,bounds,false,false);
    }
    for(const auto& sketch:sketches)if(sketch.visible && !sketch.shape.IsNull())BRepBndLib::AddOptimal(sketch.shape,bounds,false,false);
    if(bounds.IsVoid()){sceneScale_=400;sceneCenter_=gp_Pnt();}
    else {
        double x0,y0,z0,x1,y1,z1;bounds.Get(x0,y0,z0,x1,y1,z1);
        sceneCenter_=gp_Pnt((x0+x1)/2,(y0+y1)/2,(z0+z1)/2);
        sceneScale_=std::max({x1-x0,y1-y0,z1-z0,1.0})*1.7;
    }
    viewCenter_=sceneCenter_;zoom=1;pan={};sync();
}
void ReferenceView::selectBody(const QString& id,const QString& occurrenceId) {
    // refreshUI reapplies an empty body cue while a finished point is selected.
    // Actual clear/sketch/profile picks clear through selectProfiles instead.
    if(!id.isEmpty() && !selectedFinishedPoint_.isEmpty()){selectedFinishedPoint_={};applyEditDisplay(false);}
    if(context_.IsNull())return;
    context_->ClearSelected(false);
    QString selected;for(auto it=modelReferences_.begin();it!=modelReferences_.end();++it){
        const auto ref=it.value();if(ref["bodyId"]!=id || (!occurrenceId.isEmpty() && ref["occurrenceId"]!=occurrenceId))continue;
        if(!selected.isEmpty()){selected.clear();break;}selected=it.key();
    }
    if(!selected.isEmpty())context_->SetSelected(models_.value(selected),false);
    context_->UpdateCurrentViewer();
}
void ReferenceView::highlightCombineInputs(const QString& targetBodyId,const QJsonArray& toolBodyIds) {
    combineTargetId_=targetBodyId;combineToolIds_=toolBodyIds;
    if(context_.IsNull())return;
    context_->ClearSelected(false);
    for(auto it=models_.begin();it!=models_.end();++it) {
        const auto bodyId=modelReferences_.value(it.key())["bodyId"].toString();
        const auto color=bodyId==targetBodyId ? Quantity_Color(0.16,0.47,0.86,Quantity_TOC_sRGB)
            : toolBodyIds.contains(bodyId) ? Quantity_Color(0.91,0.45,0.13,Quantity_TOC_sRGB)
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
    return pickBodyReference(x,y)["bodyId"].toString();
}
QJsonObject ReferenceView::pickBodyReference(double x,double y) {
    if(context_.IsNull() || view_.IsNull())return {};
    if(x<0 || y<0 || x>=width() || y>=height())return {};
    sync();const auto scale=windowScale();
    context_->MoveTo(qRound(x*scale.x()),qRound(y*scale.y()),view_,true);
    if(!context_->HasDetected())return {};
    const auto detected=context_->DetectedInteractive();
    for(auto it=models_.begin();it!=models_.end();++it)if(it.value()==detected)return modelReferences_.value(it.key());
    return {};
}
QJsonObject ReferenceView::projectSolidEdge(const EvaluatedSolidEdge& edge) const {
    if(view_.IsNull() || edge.edge.IsNull())return {};
    BRepAdaptor_Curve curve(edge.edge);const auto p=curve.Value((curve.FirstParameter()+curve.LastParameter())/2);int x,y;view_->Convert(p.X(),p.Y(),p.Z(),x,y);const auto scale=windowScale();
    return {{"x",x/scale.x()},{"y",y/scale.y()},{"edgeId",edge.edgeId},{"viewportLocal",true}};
}
QJsonObject ReferenceView::pickSolidEndpoint(double x,double y,const std::vector<EvaluatedBody>& bodies,const QJsonObject& face) const {
    if(view_.IsNull())return {};
    const auto scale=windowScale();double best=10;QJsonObject result;gp_Pnt selected;
    for(const auto& body:bodies)if(body.visible && body.bodyId==face["bodyId"].toString()) {
        const EvaluatedSupportFace* support=nullptr;
        for(const auto& f:body.supportFaces)if(f.faceId==face["faceId"].toString() && f.producerFeatureId==face["producerFeatureId"].toString())support=&f;
        if(!support)return {};
        TopTools_IndexedMapOfShape vertices;TopExp::MapShapes(support->face,TopAbs_VERTEX,vertices);
        for(const auto& edge:body.solidEdges)if(!edge.edge.IsNull())for(int endpoint=0;endpoint<2;++endpoint){
            const auto xyz=edge.provenance[endpoint==0?"startMm":"endMm"].toArray();if(xyz.size()!=3)continue;
            const gp_Pnt point(xyz[0].toDouble(),xyz[1].toDouble(),xyz[2].toDouble());
            bool onFace=false;for(int v=1;v<=vertices.Extent();++v)if(BRep_Tool::Pnt(TopoDS::Vertex(vertices(v))).Distance(point)<1e-6)onFace=true;
            if(!onFace)continue;
            int px,py;view_->Convert(point.X(),point.Y(),point.Z(),px,py);const double distance=QLineF(QPointF(x,y),QPointF(px/scale.x(),py/scale.y())).length();
            if(distance>best+1e-5)continue;
            // Shared vertices can be represented by more than one retained edge.
            // Stable ID order resolves equivalent references; screen-overlapping
            // distinct vertices are refused instead of guessing depth.
            if(!result.isEmpty() && std::abs(distance-best)<1e-5){
                if(selected.Distance(point)>1e-6)return {};
                if(edge.edgeId>=result["edgeId"].toString())continue;
            }
            best=distance;selected=point;result={{"bodyId",body.bodyId},{"edgeId",edge.edgeId},{"endpoint",endpoint}};
        }
    }
    return result;
}

QJsonObject ReferenceView::pickSolidEdge(double x,double y,const std::vector<EvaluatedBody>& bodies) const {
    const auto pick=capturePick(x,y);return pick ? pickSolidEdge(*pick,bodies) : QJsonObject{};
}
QJsonObject ReferenceView::pickSolidEdge(const ViewPickSnapshot& pick,const std::vector<EvaluatedBody>& bodies) {
    const auto x=pick.pixel.x(),y=pick.pixel.y();double best=10;QJsonObject result;
    for(const auto& body:bodies)if(body.visible)for(const auto& edge:body.solidEdges)if(!edge.edge.IsNull()) {
        BRepAdaptor_Curve curve(edge.edge);const int count=curve.GetType()==GeomAbs_Line ? 1 : 64;
        for(int i=0;i<count;++i) {
            const double t0=curve.FirstParameter()+(curve.LastParameter()-curve.FirstParameter())*i/count,t1=curve.FirstParameter()+(curve.LastParameter()-curve.FirstParameter())*(i+1)/count;
            const auto a=curve.Value(t0),b=curve.Value(t1);
            const QPointF start=pick.project(a),delta=pick.project(b)-start,point(x,y);
            const auto length=QPointF::dotProduct(delta,delta);if(length<1e-10)continue;
            const auto fraction=std::clamp(QPointF::dotProduct(point-start,delta)/length,0.0,1.0);const auto projected=start+delta*fraction;const double distance=QLineF(point,projected).length();if(distance>=best)continue;
            // Screen proximity selects a real edge; authored identity still comes
            // solely from provenance. Suppress back-side edges behind a face.
            const auto world=curve.Value(t0+(t1-t0)*fraction);const auto ray=pick.ray(projected.x(),projected.y());IntCurvesFace_ShapeIntersector hit;hit.Load(body.shape,1e-6);hit.PerformNearest(ray,0,1e9);
            const double depth=gp_Vec(ray.Location(),world).Dot(gp_Vec(ray.Direction()));
            if(hit.IsDone() && hit.NbPnt()>0 && hit.WParameter(1)<depth-pick.cameraScale/std::max(pick.logicalHeight,1)*2)continue;
            best=distance;result={{"bodyId",body.bodyId},{"edgeId",edge.edgeId},{"kind","edge"},{"distancePx",distance}};
            if(!body.occurrenceId.isEmpty()){result["occurrenceId"]=body.occurrenceId;result["definitionId"]=body.definitionId;}
            if(body.summary.contains("outerOccurrenceId"))result["outerOccurrenceId"]=body.summary["outerOccurrenceId"];
        }
    }
    return result;
}
QJsonObject ReferenceView::pickSupportBoundaryEdge(double x,double y,const std::vector<EvaluatedBody>& bodies) const {
    auto selectable=bodies;
    for(auto& body:selectable) {
        body.solidEdges.clear();QSet<QString> seen;
        for(const auto& face:body.supportFaces)for(const auto& edge:face.boundaryEdges)
            if(!edge.edge.IsNull() && !seen.contains(edge.edgeId)) {
                body.solidEdges.push_back({edge.edgeId,edge.edge,edge.provenance});seen.insert(edge.edgeId);
            }
    }
    return pickSolidEdge(x,y,selectable);
}
void ReferenceView::clearSolidEdgeHighlights() {
    if(!context_.IsNull())for(const auto& shape:solidEdgeHighlights_)context_->Remove(shape,false);solidEdgeHighlights_.clear();
}
void ReferenceView::setSliderCue(const QJsonObject& joint) {
    if(!context_.IsNull())for(const auto& cue:sliderCue_)context_->Remove(cue,false);sliderCue_.clear();
    if(context_.IsNull())return;
    const auto coordinates=joint["origin"].toObject()["pointMm"].toArray();if(coordinates.size()!=3){context_->UpdateCurrentViewer();return;}
    const gp_Pnt anchor(coordinates[0].toDouble(),coordinates[1].toDouble(),coordinates[2].toDouble());
    const double radius=std::clamp(view_->Camera()->Scale()*2/std::max(height(),1),0.5,2.5);
    const auto mark=[this](const TopoDS_Shape& shape,const Quantity_Color& color){
        Handle(AIS_Shape) cue=new AIS_Shape(shape);cue->SetColor(color);cue->Attributes()->SetShadingModel(Graphic3d_TOSM_UNLIT);
        cue->Attributes()->SetFaceBoundaryDraw(false);cue->SetZLayer(Graphic3d_ZLayerId_Topmost);
        context_->Display(cue,AIS_Shaded,-1,false);context_->Deactivate(cue);sliderCue_.push_back(cue);
    };
    mark(BRepPrimAPI_MakeSphere(anchor,radius*1.35).Shape(),Quantity_Color(.96,.48,.12,Quantity_TOC_sRGB));
    const gp_Dir axis(0,joint["reverseDirection"].toBool() ? -1 : 1,0);
    const double length=std::clamp(view_->Camera()->Scale()*.08,12.0,40.0);
    mark(BRepPrimAPI_MakeCylinder(gp_Ax2(anchor,axis),radius*.42,length-radius*2).Shape(),Quantity_Color(.08,.57,.18,Quantity_TOC_sRGB));
    mark(BRepPrimAPI_MakeCone(gp_Ax2(anchor.Translated(gp_Vec(axis)*(length-radius*2)),axis),radius*1.5,0,radius*2).Shape(),Quantity_Color(.08,.57,.18,Quantity_TOC_sRGB));
    context_->UpdateCurrentViewer();
}
void ReferenceView::setJointOriginPicking(bool active,const QString& occurrenceId,const QString& axis) {
    if(jointOriginPicking_==active && jointOriginOccurrence_==occurrenceId && jointOriginAxis_==axis)return;
    jointOriginPicking_=active;jointOriginOccurrence_=occurrenceId;jointOriginAxis_=axis;
    jointOriginHover_={};jointOriginLock_={};jointOriginLocked_=false;
    refreshJointOriginCue();
}
QJsonObject ReferenceView::jointOriginState() const {
    const auto candidate=jointOriginLocked_ ? jointOriginLock_ : jointOriginHover_;
    QJsonObject result{{"active",jointOriginPicking_},{"locked",jointOriginLocked_},
        {"targetOccurrenceId",jointOriginOccurrence_},{"candidate",candidate},
        {"lastPointer",QJsonObject{{"x",lastJointPointer_.x()},{"y",lastJointPointer_.y()}}}};
    if(const auto center=candidate["centerMm"].toArray();center.size()==3){
        const auto p=projectWorldPoint(center);result["glyph"]=QJsonObject{{"x",p.x()},{"y",p.y()},{"viewportLocal",true}};
    }
    return result;
}
void ReferenceView::updateJointOriginHover(const QPointF& point) {
    lastJointPointer_=point;
    if(!jointOriginPicking_ || jointOriginLocked_)return;
    QJsonObject candidate;bool otherBodyInFront=false;
    {
        const auto scale=windowScale();double px,py,pz,dx,dy,dz;
        view_->ConvertWithProj(qRound(point.x()*scale.x()),qRound(point.y()*scale.y()),px,py,pz,dx,dy,dz);
        const gp_Lin ray(gp_Pnt(px,py,pz),gp_Dir(dx,dy,dz));
        const EvaluatedBody* closest=nullptr;TopoDS_Face face;double depth=std::numeric_limits<double>::infinity();
        try {for(const auto& body:documentBodies_)if(body.visible && !body.shape.IsNull()){
            IntCurvesFace_ShapeIntersector intersections;intersections.Load(body.shape,1e-6);
            intersections.PerformNearest(ray,0,1e9);
            if(!intersections.IsDone() || intersections.NbPnt()<1 || intersections.WParameter(1)>=depth)continue;
            depth=intersections.WParameter(1);closest=&body;face=intersections.Face(1);
        }}catch(const Standard_Failure&) {closest=nullptr;}
        otherBodyInFront=closest && !jointOriginOccurrence_.isEmpty()
            && closest->occurrenceId!=jointOriginOccurrence_;
        if(closest && (jointOriginOccurrence_.isEmpty() || closest->occurrenceId==jointOriginOccurrence_))
            try {candidate=JointOrigin::cylindricalFaceCandidate(*closest,face);}
            catch(const std::exception&) {}
    }
    if(candidate.isEmpty() && !otherBodyInFront){
        const auto edge=pickSolidEdge(point.x(),point.y(),documentBodies_);
        if(!edge.isEmpty() && (jointOriginOccurrence_.isEmpty() || edge["occurrenceId"]==jointOriginOccurrence_))
            for(const auto& body:documentBodies_)if(body.bodyId==edge["bodyId"] && body.occurrenceId==edge["occurrenceId"])
                try {candidate=JointOrigin::circleCandidate(body,edge["edgeId"].toString());}catch(const std::exception&) {}
    }
    if(!candidate.isEmpty()){
        const auto axis=candidate["axis"].toArray();
        if(axis.size()!=3 || (jointOriginAxis_=="Y" && std::abs(axis[1].toDouble())<1-1e-6)
            || (jointOriginAxis_=="Z" && std::abs(axis[2].toDouble())<1-1e-6))candidate={};
    }
    if(candidate==jointOriginHover_)return;
    jointOriginHover_=candidate;refreshJointOriginCue();
    if(!candidate.isEmpty() && !hasFocus())setFocus(Qt::MouseFocusReason);
}
void ReferenceView::refreshJointOriginCue() {
    if(context_.IsNull())return;
    for(const auto& cue:jointOriginCue_)context_->Remove(cue,false);
    jointOriginCue_.clear();
    const auto candidate=jointOriginLocked_ ? jointOriginLock_ : jointOriginHover_;
    const auto p=candidate["centerMm"].toArray(),a=candidate["axis"].toArray();
    if(!jointOriginPicking_ || p.size()!=3 || a.size()!=3){context_->UpdateCurrentViewer();return;}
    const gp_Pnt center(p[0].toDouble(),p[1].toDouble(),p[2].toDouble());
    const gp_Dir axis(a[0].toDouble(),a[1].toDouble(),a[2].toDouble());
    const double radius=std::clamp(view_->Camera()->Scale()*2.2/std::max(height(),1),0.55,2.2)
        *(jointOriginLocked_ ? 1.25 : 1.0);
    const auto ref=candidate["position"].toObject();
    if(ref.contains("rimEdgeIds"))for(const auto& body:documentBodies_)
        if(body.bodyId==ref["bodyId"] && body.occurrenceId==ref["occurrenceId"])
            try {auto canonical=ref;canonical.remove("occurrenceId");const auto face=JointOrigin::resolve(body,canonical).face;
                Handle(AIS_Shape) highlight=new AIS_Shape(face);
                highlight->SetColor(Quantity_Color(.98,.49,.10,Quantity_TOC_sRGB));highlight->SetTransparency(.45);
                highlight->SetZLayer(Graphic3d_ZLayerId_Topmost);
                context_->Display(highlight,AIS_Shaded,-1,false);context_->Deactivate(highlight);jointOriginCue_.push_back(highlight);
            }catch(const std::exception&) {}
    const gp_Dir worldX(1,0,0);
    const gp_Dir x=worldX,z=axis,y(gp_Vec(z).Crossed(gp_Vec(x)));
    showOriginTriad(context_,jointOriginCue_,center,x,y,z,radius);
    context_->UpdateCurrentViewer();
}
void ReferenceView::setJointOrigins(const QJsonArray& origins,bool visible) {
    if(jointOrigins_==origins && jointOriginsVisible_==visible)return;
    jointOrigins_=origins;jointOriginsVisible_=visible;refreshJointOriginMarkers();
}
void ReferenceView::refreshJointOriginMarkers() {
    if(context_.IsNull())return;
    for(const auto& cue:jointOriginMarkers_)context_->Remove(cue,false);
    jointOriginMarkers_.clear();
    if(!jointOriginsVisible_){context_->UpdateCurrentViewer();return;}
    const double radius=std::clamp(view_->Camera()->Scale()*1.5/std::max(height(),1),0.4,1.5);
    for(const auto& value:jointOrigins_){const auto frame=value.toObject()["originFrameWorld"].toArray();if(frame.size()!=12)continue;
        const gp_Pnt center(frame[3].toDouble(),frame[7].toDouble(),frame[11].toDouble());
        const gp_Dir axis(frame[2].toDouble(),frame[6].toDouble(),frame[10].toDouble());
        const gp_Dir x(frame[0].toDouble(),frame[4].toDouble(),frame[8].toDouble());
        const gp_Dir y(frame[1].toDouble(),frame[5].toDouble(),frame[9].toDouble());
        showOriginTriad(context_,jointOriginMarkers_,center,x,y,axis,radius);
    }
    context_->UpdateCurrentViewer();
}
void ReferenceView::setAssemblyDragCue(const QJsonObject& cue) {
    if(cue.isEmpty() || cue["jointId"]!=assemblyDragCue_["jointId"] || cue["bodyId"]!=assemblyDragCue_["bodyId"])
        assemblyDrag_={};
    assemblyDragCue_=cue;
}
std::optional<double> ReferenceView::assemblyPointerAngle(const QPointF& position) const {
    const auto pivot=assemblyDragCue_["pivotMm"].toArray();
    if(view_.IsNull() || pivot.size()!=3)return std::nullopt;
    const auto scale=windowScale();double px,py,pz,dx,dy,dz;
    view_->ConvertWithProj(qRound(position.x()*scale.x()),qRound(position.y()*scale.y()),px,py,pz,dx,dy,dz);
    if(std::abs(dy)<1e-8)return std::nullopt;
    const double distance=(pivot[1].toDouble()-py)/dy;
    const double x=px+distance*dx-pivot[0].toDouble(),z=pz+distance*dz-pivot[2].toDouble();
    if(!std::isfinite(x) || !std::isfinite(z) || std::hypot(x,z)<1e-5)return std::nullopt;
    return std::atan2(-z,x);
}
void ReferenceView::clearPatternAxes() {
    if(!context_.IsNull())for(const auto& model:patternAxes_)context_->Remove(model,false);patternAxes_.clear();
}
void ReferenceView::showPatternAxes(const QJsonObject& summary) {
    clearPatternAxes();if(context_.IsNull() || view_.IsNull())return;
    const auto center=summary["seedCenterMm"].toArray();if(center.size()!=3)return;const gp_Pnt origin(center[0].toDouble(),center[1].toDouble(),center[2].toDouble());
    const double radius=view_->Camera()->Scale()*1.5/std::max(height(),1);int index=0;
    for(const auto& value:summary["patternAxes"].toArray()) {
        const auto axis=value.toObject();const auto d=axis["direction"].toArray();if(d.size()!=3)continue;const double extent=axis.contains("totalSpanMm") ? axis["totalSpanMm"].toDouble() : axis["extentMm"].toDouble();if(std::abs(extent)<1e-6)continue;
        const gp_Dir direction(d[0].toDouble()*(extent<0 ? -1 : 1),d[1].toDouble()*(extent<0 ? -1 : 1),d[2].toDouble()*(extent<0 ? -1 : 1));
        const auto length=std::abs(extent),head=std::min(radius*6,length*.25);BRep_Builder builder;TopoDS_Compound shape;builder.MakeCompound(shape);
        if(length>head)builder.Add(shape,BRepPrimAPI_MakeCylinder(gp_Ax2(origin,direction),radius,length-head).Shape());builder.Add(shape,BRepPrimAPI_MakeCone(gp_Ax2(origin.Translated(gp_Vec(direction)*(length-head)),direction),radius*3,0,head).Shape());
        Handle(AIS_Shape) cue=new AIS_Shape(shape);cue->SetColor(index++==0 ? Quantity_Color(.08,.43,.91,Quantity_TOC_sRGB) : Quantity_Color(.08,.57,.58,Quantity_TOC_sRGB));cue->Attributes()->SetShadingModel(Graphic3d_TOSM_UNLIT);cue->Attributes()->SetFaceBoundaryDraw(false);cue->SetZLayer(Graphic3d_ZLayerId_Topmost);context_->Display(cue,AIS_Shaded,-1,false);context_->Deactivate(cue);patternAxes_.push_back(cue);
    }
    context_->UpdateCurrentViewer();
}
void ReferenceView::highlightSolidEdges(const std::vector<EvaluatedBody>& bodies,const QString& bodyId,const QJsonArray& ids,
    const QString& occurrenceId) {
    clearSolidEdgeHighlights();if(context_.IsNull())return;
    for(const auto& body:bodies)if(body.bodyId==bodyId && (occurrenceId.isEmpty() || body.occurrenceId==occurrenceId))
        for(const auto& edge:body.solidEdges)if(ids.contains(edge.edgeId) && !edge.edge.IsNull()) {
        BRepAdaptor_Curve curve(edge.edge);BRep_Builder builder;TopoDS_Compound shape;builder.MakeCompound(shape);
        const int count=curve.GetType()==GeomAbs_Line ? 1 : 64;
        for(int i=0;i<count;++i){const auto a=curve.Value(curve.FirstParameter()+(curve.LastParameter()-curve.FirstParameter())*i/count),b=curve.Value(curve.FirstParameter()+(curve.LastParameter()-curve.FirstParameter())*(i+1)/count);const gp_Vec delta(a,b);if(delta.Magnitude()>1e-9)builder.Add(shape,BRepPrimAPI_MakeCylinder(gp_Ax2(a,gp_Dir(delta)),view_->Camera()->Scale()*1.6/std::max(height(),1),delta.Magnitude()).Shape());}
        Handle(AIS_Shape) cue=new AIS_Shape(shape);cue->SetColor(Quantity_Color(.08,.43,.91,Quantity_TOC_sRGB));cue->Attributes()->SetShadingModel(Graphic3d_TOSM_UNLIT);cue->Attributes()->SetFaceBoundaryDraw(false);cue->SetZLayer(Graphic3d_ZLayerId_Topmost);context_->Display(cue,AIS_Shaded,-1,false);context_->Deactivate(cue);solidEdgeHighlights_.push_back(cue);
    }
    context_->UpdateCurrentViewer();
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
QPointF ReferenceView::projectWorldPoint(const QJsonArray& pointMm) const {
    if(view_.IsNull() || pointMm.size()!=3)return {};
    int x,y;view_->Convert(pointMm[0].toDouble(),pointMm[1].toDouble(),pointMm[2].toDouble(),x,y);
    const auto scale=windowScale();return {x/scale.x(),y/scale.y()};
}
void ReferenceView::clearMoveCue(bool resetGesture) {
    if(resetGesture)moveDrag_={};
    if(!context_.IsNull())for(const auto& cue:moveCue_)context_->Remove(cue,false);
    moveCue_.clear();moveCueState_={};
}
void ReferenceView::showMovePreview(const TopoDS_Shape& shape,const QString& bodyId) {
    replacePreview(shape,bodyId,true);
    if(!preview_.IsNull()){preview_->SetColor(Quantity_Color(0.16,0.47,0.86,Quantity_TOC_sRGB));context_->Redisplay(preview_,false);}
}
void ReferenceView::showMoveCue(const QJsonObject& inputs) {
    clearMoveCue();if(view_.IsNull())return;
    auto parameters=inputs;auto steps=inputs["steps"].toArray();QJsonObject delta;for(const auto& key:{"xMm","yMm","zMm","xDeg","yDeg","zDeg"})delta[key]=inputs.value(key).toDouble();steps.append(delta);parameters["steps"]=steps;const auto transform=GeometryKernel::moveTransform(parameters);
    gp_Pnt center(inputs["pivotXMm"].toDouble(),inputs["pivotYMm"].toDouble(),inputs["pivotZMm"].toDouble());center.Transform(transform);
    const auto scale=windowScale();auto screen=[&](const gp_Pnt& p){int x,y;view_->Convert(p.X(),p.Y(),p.Z(),x,y);return QJsonObject{{"x",x/scale.x()},{"y",y/scale.y()}};};
    // Screen-sized solid local-triad handles avoid platform GL line-width clamps.
    const double radius=view_->Camera()->Scale()*0.065;
    gp_Vec axes[]={gp_Vec(1,0,0),gp_Vec(0,1,0),gp_Vec(0,0,1)};
    const auto frame=inputs["axisFrame"].toArray();if(frame.size()==12){gp_Trsf basis;basis.SetValues(frame[0].toDouble(),frame[1].toDouble(),frame[2].toDouble(),0,frame[4].toDouble(),frame[5].toDouble(),frame[6].toDouble(),0,frame[8].toDouble(),frame[9].toDouble(),frame[10].toDouble(),0);for(auto& axis:axes)axis.Transform(basis);}
    const bool translationOnly=inputs.value("translationOnly").toBool();
    QJsonArray handles;
    for(int axis=0;axis<3;++axis){
        const auto a=axes[axis].Transformed(transform),u=axes[(axis+1)%3].Transformed(transform),v=axes[(axis+2)%3].Transformed(transform);
        BRep_Builder builder;TopoDS_Compound shape;builder.MakeCompound(shape);
        const double shaftRadius=radius*.035,arrowLength=radius*.28;
        const auto end=center.Translated(a*radius*1.65);
        builder.Add(shape,BRepPrimAPI_MakeCylinder(gp_Ax2(center,gp_Dir(a)),shaftRadius,radius*1.65-arrowLength).Shape());
        builder.Add(shape,BRepPrimAPI_MakeCone(gp_Ax2(end.Translated(a*(-arrowLength)),gp_Dir(a)),radius*.12,0,arrowLength).Shape());
        if(!translationOnly)builder.Add(shape,BRepPrimAPI_MakeTorus(gp_Ax2(center,gp_Dir(a),gp_Dir(u)),radius,shaftRadius).Shape());
        QJsonArray ring;gp_Pnt previous=center.Translated(u*radius);ring.append(screen(previous));
        for(int i=1;!translationOnly&&i<=64;++i){const double angle=i*2*std::acos(-1.0)/64;const auto point=center.Translated(u*(radius*std::cos(angle))+v*(radius*std::sin(angle)));ring.append(screen(point));previous=point;}
        auto cue=new AIS_Shape(shape);cue->SetZLayer(Graphic3d_ZLayerId_Topmost);cue->SetColor(axis==0 ? Quantity_Color(.88,.12,.10,Quantity_TOC_sRGB) : axis==1 ? Quantity_Color(.08,.57,.18,Quantity_TOC_sRGB) : Quantity_Color(.08,.30,.88,Quantity_TOC_sRGB));cue->Attributes()->SetShadingModel(Graphic3d_TOSM_UNLIT);cue->Attributes()->SetFaceBoundaryDraw(false);cue->SetOwnDeviationCoefficient(.0001);context_->Display(cue,AIS_Shaded,-1,false);context_->Deactivate(cue);moveCue_.push_back(cue);
        handles.append(QJsonObject{{"axis",axis},{"start",screen(center)},{"end",screen(end)},{"mmLength",radius*1.65},{"ring",ring},{"basisU",screen(center.Translated(u*radius))},{"basisV",screen(center.Translated(v*radius))}});
    }
    moveCueState_={{"inputs",inputs},{"handles",handles},{"pivotScreen",screen(center)},{"viewportLocal",true},{"units","logicalPixels"}};view_->Redraw();
}
void ReferenceView::replacePreview(const TopoDS_Shape& shape,const QString& replacedBody,bool neutral) {
    // Remove the previous presentation without exposing an intermediate empty
    // scene or the original body while the replacement is being installed.
    clearPreviewPresentation();if(context_.IsNull())return;
    QString replacementKey;for(auto it=modelReferences_.begin();it!=modelReferences_.end();++it)
        if(it.value()["bodyId"]==replacedBody){if(!replacementKey.isEmpty()){replacementKey.clear();break;}replacementKey=it.key();}
    if(!replacementKey.isEmpty()) {
        previewReplacedBody_=replacedBody;context_->Erase(models_.value(replacementKey),false);
    }
    preview_=new AIS_Shape(shape);preview_->SetColor(neutral ? Quantity_Color(0.67,0.72,0.76,Quantity_TOC_RGB) : Quantity_Color(0.25,0.58,0.76,Quantity_TOC_RGB));
    preview_->SetOwnDeviationCoefficient(0.0005);
    preview_->SetOwnDeviationAngle(0.10);
    const bool preparedThread=preservePreparedThreadMesh(preview_);
    if(preparedThread)traceThreadMesh("preview_before_display",shape);
    preview_->SetTransparency(neutral ? 0.0 : 0.55);
    preview_->Attributes()->SetFaceBoundaryDraw(true);
    if(neutral)preview_->Attributes()->SetFaceBoundaryAspect(new Prs3d_LineAspect(
        Quantity_Color(0.18,0.22,0.25,Quantity_TOC_sRGB),Aspect_TOL_SOLID,devicePixelRatioF()));
    context_->Display(preview_,AIS_Shaded,-1,false);context_->Deactivate(preview_);
    if(preparedThread)traceThreadMesh("preview_after_display",shape);
    applyEditDisplay(false);
}
void ReferenceView::showHolePreview(const EvaluatedBody& result,const TopoDS_Shape& removedMaterial) {
    replacePreview(result.shape,result.bodyId,true);
    if(context_.IsNull())return;
    Handle(AIS_Shape) removed=new AIS_Shape(removedMaterial);
    removed->SetColor(Quantity_Color(.91,.24,.27,Quantity_TOC_sRGB));removed->SetTransparency(.48);
    removed->SetOwnDeviationCoefficient(.0001);removed->SetOwnDeviationAngle(.04);
    // Removed stock shares the exact cavity surface with the gray result.
    // Bias only this disposable presentation; retain actual cutter geometry.
    removed->SetPolygonOffsets(Aspect_POM_Fill,-2.f,-4.f);
    removed->Attributes()->SetFaceBoundaryDraw(true);
    removed->Attributes()->SetFaceBoundaryAspect(new Prs3d_LineAspect(Quantity_Color(.80,.12,.16,Quantity_TOC_sRGB),Aspect_TOL_SOLID,1.6*devicePixelRatioF()));
    context_->Display(removed,AIS_Shaded,-1,false);context_->Deactivate(removed);cutTools_.push_back(removed);
    context_->UpdateCurrentViewer();
}
void ReferenceView::showSpherePreview(const TopoDS_Shape& tool,bool cut) {
    replacePreview(tool,{},false);
    if(context_.IsNull())return;
    preview_->SetColor(Quantity_Color(cut ? .91 : .16,cut ? .24 : .47,cut ? .27 : .86,Quantity_TOC_sRGB));
    preview_->SetTransparency(cut ? .48 : .20);
    context_->Redisplay(preview_,false);context_->UpdateCurrentViewer();
}
void ReferenceView::showPreview(const TopoDS_Shape& shape,const QString& replacedBody,bool neutral) {
    replacePreview(shape,replacedBody,neutral);
    if(!context_.IsNull())context_->UpdateCurrentViewer();
}
void ReferenceView::showBoundaryFillPreview(const TopoDS_Shape& cells) {
    replacePreview(cells,{},true);if(context_.IsNull())return;
    preview_->SetColor(Quantity_Color(.32,.70,.42,Quantity_TOC_sRGB));
    preview_->SetTransparency(0.0);context_->Redisplay(preview_,false);context_->UpdateCurrentViewer();
}
void ReferenceView::showExtrudePreview(const EvaluatedBody& body,const EvaluatedProfile& profile,double distanceMm,const QJsonArray& sourceProfileIds,const QString& units,double startOffsetMm) {
    replacePreview(body.shape,body.bodyId,false);
    if(context_.IsNull())return;
    preview_->SetColor(Quantity_Color(0.16,0.47,0.86,Quantity_TOC_sRGB));
    preview_->SetTransparency(0.20);
    context_->Redisplay(preview_,false);
    setExtrusionCue(profile,distanceMm,sourceProfileIds,units,body.summary["operation"]=="join" || body.summary["operation"]=="intersect" ? body.summary["operation"].toString() : QString("newBody"),startOffsetMm);
    context_->UpdateCurrentViewer();
}
void ReferenceView::showCutPreview(const EvaluatedBody& body,const std::vector<EvaluatedProfile>& profiles,double distanceMm,
    const QJsonArray& sourceProfileIds,const QString& units,double startOffsetMm) {
    if(profiles.empty())return;
    replacePreview(body.shape,body.bodyId,true);
    if(context_.IsNull())return;
    for(const auto& profile:profiles) {
        Handle(AIS_Shape) tool=new AIS_Shape(GeometryKernel::extrusion(profile,distanceMm,nullptr,nullptr,{},nullptr,startOffsetMm));
        tool->SetColor(Quantity_Color(0.91,0.24,0.27,Quantity_TOC_sRGB));tool->SetTransparency(0.62);
        tool->SetOwnDeviationCoefficient(0.0001);tool->SetOwnDeviationAngle(0.04);
        tool->Attributes()->SetFaceBoundaryDraw(true);
        tool->Attributes()->SetFaceBoundaryAspect(new Prs3d_LineAspect(
            Quantity_Color(0.80,0.12,0.16,Quantity_TOC_sRGB),Aspect_TOL_SOLID,1.6*devicePixelRatioF()));
        context_->Display(tool,AIS_Shaded,-1,false);context_->Deactivate(tool);cutTools_.push_back(tool);
        // The selected source regions remain blue over the red disposable
        // cutter volumes. A dedicated top layer keeps coplanar face fills
        // stable while the normal finished-profile fills are suppressed.
        Handle(AIS_Shape) selected=new AIS_Shape(profile.face);
        selected->SetDisplayMode(AIS_Shaded);
        selected->SetColor(Quantity_Color(0.05,0.36,0.80,Quantity_TOC_sRGB));selected->SetTransparency(0.08);
        selected->Attributes()->SetFaceBoundaryDraw(true);
        selected->Attributes()->SetFaceBoundaryAspect(new Prs3d_LineAspect(
            Quantity_Color(0.88,0.20,0.24,Quantity_TOC_sRGB),Aspect_TOL_SOLID,2.0*devicePixelRatioF()));
        selected->SetZLayer(Graphic3d_ZLayerId_Topmost);
        context_->Display(selected,AIS_Shaded,-1,false);context_->Deactivate(selected);cutProfileOverlays_.push_back(selected);
    }
    setExtrusionCue(profiles.front(),distanceMm,sourceProfileIds,units,"cut",startOffsetMm);
    context_->UpdateCurrentViewer();
}
void ReferenceView::showSurfaceTrimPreview(const TopoDS_Shape& splitShape,const QString& replacedBody,
    const std::vector<TopoDS_Face>& removedRegions) {
    replacePreview(splitShape,replacedBody,true);
    if(context_.IsNull())return;
    for(const auto& face:removedRegions){
        Handle(AIS_Shape) overlay=new AIS_Shape(face);
        overlay->SetColor(Quantity_Color(.91,.24,.27,Quantity_TOC_sRGB));
        overlay->SetTransparency(.25);
        overlay->SetOwnDeviationCoefficient(.0001);overlay->SetOwnDeviationAngle(.04);
        overlay->SetPolygonOffsets(Aspect_POM_Fill,-2.f,-4.f);
        overlay->Attributes()->SetFaceBoundaryDraw(true);
        overlay->Attributes()->SetFaceBoundaryAspect(new Prs3d_LineAspect(
            Quantity_Color(.80,.12,.16,Quantity_TOC_sRGB),Aspect_TOL_SOLID,2.0*devicePixelRatioF()));
        overlay->SetZLayer(Graphic3d_ZLayerId_Topmost);
        context_->Display(overlay,AIS_Shaded,-1,false);context_->Deactivate(overlay);cutTools_.push_back(overlay);
    }
    context_->UpdateCurrentViewer();
}
void ReferenceView::clearExtrusionCue() {
    if(!context_.IsNull())for(const auto& cue:extrusionCue_)context_->Remove(cue,false);
    extrusionCue_.clear();extrusionCueState_={};
    if(!context_.IsNull())context_->UpdateCurrentViewer();
}
QJsonObject ReferenceView::faceOffsetCueState() const {
    if(extrusionCueState_.value("command")!="solid.faceOffset")return {};
    auto state=extrusionCueState_;state["dragActive"]=extrusionDragActive_;return state;
}
void ReferenceView::showFaceOffsetCue(const EvaluatedSupportFace& sourceFace,double distanceMm,const QString& units,const QString& command) {
    // This baseline always comes from the source history stage, never the moved
    // candidate face. Presentation replacement must not restart an active drag.
    // Curved normal manipulator geometry is outside the bounded numeric-input
    // continuation. Never place a planar centroid/normal cue on a curved face.
    if(BRepAdaptor_Surface(sourceFace.face).GetType()!=GeomAbs_Plane){clearExtrusionCue();return;}
    GProp_GProps properties;BRepGProp::SurfaceProperties(sourceFace.face,properties);
    const auto start=properties.CentreOfMass();
    const auto frame=sourceFace.frame;const auto n=frame.value("normal").toArray();
    if(n.size()!=3){clearExtrusionCue();return;}
    const gp_Vec normal(n[0].toDouble(),n[1].toDouble(),n[2].toDouble());
    const auto end=start.Translated(normal*(command=="surface.offset"?std::abs(distanceMm):distanceMm));
    extrusionCueState_={{"command",command},{"distanceMm",distanceMm},{"units",units},
        {"sourceFace",QJsonObject{{"producerFeatureId",sourceFace.producerFeatureId},{"bodyId",sourceFace.bodyId},{"faceId",sourceFace.faceId}}},
        {"frame",frame},{"label",(command=="surface.offset"?QString("+N · "):QString{})+QString::number(units=="in"?distanceMm/25.4:distanceMm,'f',2)+" "+units},
        {"startMm",QJsonArray{start.X(),start.Y(),start.Z()}},{"endMm",QJsonArray{end.X(),end.Y(),end.Z()}}};
    refreshExtrusionCue();if(!context_.IsNull())context_->UpdateCurrentViewer();
}
void ReferenceView::showExtrudeHandle(const EvaluatedProfile& profile,const QJsonArray& sourceProfileIds,const QString& units,const QString& operation,double distanceMm,double startOffsetMm) {
    clearPreviewPresentation();
    if(context_.IsNull())return;
    setExtrusionCue(profile,distanceMm,sourceProfileIds,units,operation,startOffsetMm);
    context_->UpdateCurrentViewer();
}
void ReferenceView::setExtrusionCue(const EvaluatedProfile& profile,double distanceMm,const QJsonArray& sourceProfileIds,
    const QString& units,const QString& operation,double startOffsetMm) {
    previewProfile_=profile.profileId;previewProfileIds_=sourceProfileIds.isEmpty() ? QJsonArray{profile.profileId} : sourceProfileIds;
    applyEditDisplay(false);
    GProp_GProps properties;BRepGProp::SurfaceProperties(profile.face,properties);auto start=properties.CentreOfMass();
    const auto frame=profile.summary.value("frame").toObject().isEmpty() ? SketchGeometry::frame(profile.plane) : profile.summary.value("frame").toObject();
    const auto normal=frame.value("normal").toArray();
    gp_Vec direction(normal[0].toDouble(),normal[1].toDouble(),normal[2].toDouble());start.Translate(direction*startOffsetMm);direction*=distanceMm<0 ? -1 : 1;
    const auto end=start.Translated(direction*std::abs(distanceMm));
    const double displayDistance=units=="in" ? distanceMm/25.4 : distanceMm;
    const QString label=QString::number(displayDistance,'f',2)+" "+units;
    extrusionCueState_={{"command","solid.extrude"},{"sketchId",profile.sketchId},{"profileId",profile.profileId},{"profileIds",previewProfileIds_},{"distanceMm",distanceMm},{"startOffsetMm",startOffsetMm},
        {"operation",operation},
        {"units",units},{"label",label},{"plane",profile.plane},{"frame",frame},
        {"startMm",QJsonArray{start.X(),start.Y(),start.Z()}},{"endMm",QJsonArray{end.X(),end.Y(),end.Z()}}};
    refreshExtrusionCue();
}
void ReferenceView::refreshExtrusionCue() {
    if(context_.IsNull() || view_.IsNull() || extrusionCueState_.isEmpty())return;
    for(const auto& cue:extrusionCue_)context_->Remove(cue,false);
    extrusionCue_.clear();
    // Clear all projected hit data before any projection failure. A normal
    // pointing into the camera has no meaningful normal-axis pointer mapping.
    for(const auto* key:{"startScreen","endScreen","axisPixelsPerMm","labelScreen","arrowHeadPixels"})extrusionCueState_.remove(key);
    extrusionCueState_["visible"]=false;extrusionCueState_["dragAvailable"]=false;
    extrusionCueState_["unavailableReason"]="projection_unavailable";
    const auto startArray=extrusionCueState_.value("startMm").toArray(),endArray=extrusionCueState_.value("endMm").toArray();
    if(startArray.size()!=3 || endArray.size()!=3)return;
    const gp_Pnt start(startArray[0].toDouble(),startArray[1].toDouble(),startArray[2].toDouble());
    const auto frame=extrusionCueState_.value("frame").toObject();const auto u=frame.value("uAxis").toArray(),n=frame.value("normal").toArray();
    if(n.size()!=3 || u.size()!=3)return;
    gp_Vec normal(n[0].toDouble(),n[1].toDouble(),n[2].toDouble());if(normal.Magnitude()<1e-9)return;normal.Normalize();
    const auto scale=windowScale();int startX,startY,axisX,axisY;
    view_->Convert(start.X(),start.Y(),start.Z(),startX,startY);
    const auto axisPoint=start.Translated(normal*100.0);view_->Convert(axisPoint.X(),axisPoint.Y(),axisPoint.Z(),axisX,axisY);
    const QPointF axisPixels((axisX-startX)/scale.x()/100.0,(axisY-startY)/scale.y()/100.0);
    const double axisPixelsPerMm=std::hypot(axisPixels.x(),axisPixels.y());
    if(axisPixelsPerMm<1e-5){
        extrusionCueState_["unavailableReason"]="normal_aligned_with_view";
        if(extrusionCueState_.value("command")=="solid.faceOffset" || extrusionCueState_.value("command")=="surface.offset"){
            // Exact numeric entry remains available; do not invent a screen-space
            // drag direction when the face normal projects to a point.
            Handle(AIS_TextLabel) label=new AIS_TextLabel;
            const auto text=extrusionCueState_.value("label").toString()+(extrusionCueState_.value("command")=="surface.offset" ? " · Normal along view":" · Rotate view to drag");
            label->SetText(TCollection_ExtendedString(text.toUtf8().constData(),true));label->SetPosition(start);
            label->SetHeight(14);label->SetFont("Arial");label->SetZoomable(false);
            label->SetDisplayType(Aspect_TODT_SUBTITLE);label->SetColor(Quantity_Color(.04,.29,.72,Quantity_TOC_sRGB));
            label->SetColorSubTitle(Quantity_Color(1,1,1,Quantity_TOC_sRGB));label->SetZLayer(Graphic3d_ZLayerId_Topmost);
            context_->Display(label,0,-1,false);context_->Deactivate(label);extrusionCue_.push_back(label);
            extrusionCueState_["visible"]=true;
            extrusionCueState_["labelScreen"]=QJsonObject{{"x",startX/scale.x()},{"y",startY/scale.y()}};
        }
        return;
    }
    const double actualDistance=extrusionCueState_.value("command")=="surface.offset" ? std::abs(extrusionCueState_.value("distanceMm").toDouble()) : extrusionCueState_.value("distanceMm").toDouble();
    const double displayDistance=std::abs(actualDistance)*axisPixelsPerMm<30.0
        ? (actualDistance<0 ? -30.0 : 30.0)/axisPixelsPerMm : actualDistance;
    const gp_Pnt end=start.Translated(normal*displayDistance);
    gp_Vec direction(start,end);const double length=direction.Magnitude();direction.Normalize();
    gp_Vec side(u[0].toDouble(),u[1].toDouble(),u[2].toDouble());
    int endX,endY,sideX,sideY;
    view_->Convert(end.X(),end.Y(),end.Z(),endX,endY);
    const auto sidePoint=end.Translated(side*100.0);view_->Convert(sidePoint.X(),sidePoint.Y(),sidePoint.Z(),sideX,sideY);
    const double alongPixels=std::hypot((endX-startX)/scale.x(),(endY-startY)/scale.y())/length;
    double sidePixels=std::hypot((sideX-endX)/scale.x(),(sideY-endY)/scale.y())/100.0;
    // Choose the tangent with greater projected width across the normal. A
    // long tangent projected along the arrow would still collapse its face.
    auto acrossPixels=[&](int x,int y){return std::abs(axisPixels.x()*(y-endY)/scale.y()/100.0-axisPixels.y()*(x-endX)/scale.x()/100.0)/axisPixelsPerMm;};
    auto alternate=normal.Crossed(side);
    if(alternate.Magnitude()>1e-9){
        alternate.Normalize();const auto point=end.Translated(alternate*100.0);int x,y;view_->Convert(point.X(),point.Y(),point.Z(),x,y);
        if(acrossPixels(x,y)>acrossPixels(sideX,sideY)){
            side=alternate;sideX=x;sideY=y;
            sidePixels=std::hypot((sideX-endX)/scale.x(),(sideY-endY)/scale.y())/100.0;
        }
    }
    if(alongPixels<1e-5 || sidePixels<1e-5 || acrossPixels(sideX,sideY)<1e-5)return;
    const double head=std::min(length*0.36,14.0/alongPixels),wing=6.0/sidePixels;
    const Quantity_Color color(0.04,0.29,0.72,Quantity_TOC_sRGB);
    // Filled geometry keeps the handle legible on drivers that clamp GL lines
    // to one physical pixel. The white ring separates blue from blue preview.
    auto arrowWire=[&](double shaft,double wings,double extension) {
        const auto tip=end.Translated(direction*extension);
        const auto back=end.Translated(direction*(-head));
        BRepBuilderAPI_MakePolygon polygon;
        for(const auto& point:std::vector<gp_Pnt>{start.Translated(side*shaft),back.Translated(side*shaft),
            back.Translated(side*wings),tip,back.Translated(side*(-wings)),back.Translated(side*(-shaft)),start.Translated(side*(-shaft))})polygon.Add(point);
        polygon.Close();return polygon.Wire();
    };
    auto inner=arrowWire(1.8/sidePixels,wing,0);
    auto outer=arrowWire(3.0/sidePixels,wing+1.5/sidePixels,1.5/alongPixels);
    BRepBuilderAPI_MakeFace border(outer);border.Add(TopoDS::Wire(inner.Reversed()));
    for(const auto& item:std::vector<std::pair<TopoDS_Shape,Quantity_Color>>{
        {border.Face(),Quantity_Color(1,1,1,Quantity_TOC_sRGB)}, {BRepBuilderAPI_MakeFace(inner).Face(),color}}) {
        Handle(AIS_Shape) cue=new AIS_Shape(item.first);cue->SetColor(item.second);cue->SetDisplayMode(AIS_Shaded);
        cue->Attributes()->SetShadingModel(Graphic3d_TypeOfShadingModel_Unlit,true);
        cue->SetZLayer(Graphic3d_ZLayerId_Topmost);
        context_->Display(cue,AIS_Shaded,-1,false);context_->Deactivate(cue);extrusionCue_.push_back(cue);
    }
    Handle(AIS_TextLabel) label=new AIS_TextLabel;
    label->SetText(TCollection_ExtendedString(extrusionCueState_.value("label").toString().toUtf8().constData(),true));
    label->SetPosition(end.Translated(side*(21.0/sidePixels)));
    label->SetHeight(14);label->SetFont("Arial");label->SetHJustification(Graphic3d_HTA_LEFT);label->SetVJustification(Graphic3d_VTA_CENTER);
    label->SetZoomable(false);label->SetDisplayType(Aspect_TODT_SUBTITLE);label->SetColor(color);
    label->SetColorSubTitle(Quantity_Color(1.0,1.0,1.0,Quantity_TOC_sRGB));
    label->SetZLayer(Graphic3d_ZLayerId_Topmost);context_->Display(label,0,-1,false);context_->Deactivate(label);extrusionCue_.push_back(label);
    extrusionCueState_["visible"]=true;extrusionCueState_["dragAvailable"]=extrusionCueState_.value("command")!="surface.offset";extrusionCueState_.remove("unavailableReason");
    extrusionCueState_["arrowHeadPixels"]=head*alongPixels;extrusionCueState_["labelHeightPixels"]=14;
    extrusionCueState_["startScreen"]=QJsonObject{{"x",startX/scale.x()},{"y",startY/scale.y()}};
    extrusionCueState_["endScreen"]=QJsonObject{{"x",endX/scale.x()},{"y",endY/scale.y()}};
    extrusionCueState_["axisPixelsPerMm"]=QJsonObject{{"x",axisPixels.x()},{"y",axisPixels.y()}};
    extrusionCueState_["labelScreen"]=QJsonObject{{"x",endX/scale.x()+21.0*(sideX-endX)/scale.x()/(100*sidePixels)},
        {"y",endY/scale.y()+21.0*(sideY-endY)/scale.y()/(100*sidePixels)}};
}
void ReferenceView::clearPreviewPresentation() {
    clearRevolveCue();
    if(!context_.IsNull()) {
        for(const auto& cue:extrusionCue_)context_->Remove(cue,false);
        for(const auto& tool:cutTools_)context_->Remove(tool,false);
        for(const auto& selected:cutProfileOverlays_)context_->Remove(selected,false);
        if(!preview_.IsNull())context_->Remove(preview_,false);
    }
    preview_.Nullify();previewReplacedBody_.clear();previewProfile_.clear();previewProfileIds_={};cutTools_.clear();cutProfileOverlays_.clear();extrusionCue_.clear();extrusionCueState_={};
}
void ReferenceView::clearPreview() {
    extrusionDragActive_=false;
    clearPreviewPresentation();
    applyEditDisplay();
}
void ReferenceView::updateExtrusionDrag(const QPointF& position,const QString& phase) {
    const QPointF pixelsPerMm=extrusionDragAxisPixelsPerMm_;
    const double length2=QPointF::dotProduct(pixelsPerMm,pixelsPerMm);if(length2<1e-10)return;
    const double deltaMm=QPointF::dotProduct(position-extrusionDragPress_,pixelsPerMm)/length2;
    const double distance=std::clamp(extrusionDragInitialMm_+deltaMm,-100000.0,100000.0);
    dispatch(extrusionDragCommand_,{{"distanceMm",distance},{"x",position.x()},{"y",position.y()},{"phase",phase}});
    if(!extrusionCueState_.isEmpty())extrusionCueState_["dragActive"]=phase!="end";
}
QJsonObject ReferenceView::diagnostics() const {
    QJsonObject result{{"initialized", !view_.IsNull()}, {"error", error_}, {"hardwareAccelerated", false},
        {"qualification", "unverified"}, {"backend", "OCCT OpenGL"}};
    result["sectionAnalysis"]=sectionAnalysis_;
    result["sectionClippedActors"]=sectionClip_.IsNull() ? 0 : models_.size();
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
    for(auto it=centerlineSketchModels_.cbegin();it!=centerlineSketchModels_.cend();++it)
        if(!sketchModels_.contains(it.key()) && !constructionSketchModels_.contains(it.key()))++displayedSketches;
    result.insert("displayedSketches",displayedSketches);
    result.insert("sketchDisplayOptions",sketchDisplayOptions_);
    result.insert("draftSketchVisible",!sketchDraft_.IsNull() || !sketchConstructionDraft_.IsNull() || !sketchCenterlineDraft_.IsNull());
    result.insert("draftNormalEdgeCount",draftNormalEdgeCount_);
    result.insert("constructionDraftVisible",!sketchConstructionDraft_.IsNull() && context_->IsDisplayed(sketchConstructionDraft_));
    result.insert("constructionDraftLineIds",constructionDraftLineIds_);
    result.insert("centerlineDraftVisible",!sketchCenterlineDraft_.IsNull());
    result.insert("centerlineDraftLineIds",centerlineDraftLineIds_);
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
    result.insert("projectedDraftVisible",!projectedDraft_.IsNull() && context_->IsDisplayed(projectedDraft_));
    result.insert("selectOtherPreview",sketchSelectOtherPreview_);
    result.insert("selectOtherPreviewVisible",!sketchSelectOtherOverlay_.IsNull());
    result.insert("projectedLineIds",projectedLineIds);
    result.insert("projectedLines",projectedLines_);
    result.insert("projectedCircles",projectedCircles_);
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
    auto context=lastContextMenu_;
    if(auto* marking=dynamic_cast<CanvasMarkingMenu*>(QApplication::activePopupWidget());marking && marking->parentWidget()==this)
        context.insert("radial",marking->inspection());
    result.insert("lastContextMenu",context);
    result.insert("contextMenuVisible",qobject_cast<QMenu*>(QApplication::activePopupWidget())!=nullptr);
    result.insert("draftSketchId",draftSketchId_);
    result.insert("sketchPlane",sketchPlane_);
    result.insert("sketchGrid",QJsonObject{{"adaptive",adaptiveGrid_},{"centerUMm",gridCenterU_},
        {"centerVMm",gridCenterV_},{"stepMm",gridStep_},{"graphicHalfWidthMm",gridWidth_},
        {"graphicHalfHeightMm",gridHeight_}});
    result.insert("cursorPreviewVisible",!sketchCursor_.IsNull());
    result.insert("slotDragPreviewVisible",slotPointDrag_["moved"].toBool());
    result.insert("splineCagePreviewVisible",!splineCageCursor_.IsNull());
    result.insert("sketchDecorations",static_cast<int>(sketchDecorations_.size()));
    result.insert("sketchAnnotations",annotationLayouts_);
    result.insert("chamferHandles",chamferHandles_);
    result.insert("chamferDrag",chamferDrag_);
    auto snapCue=sketchSnapState_;snapCue["visible"]=!sketchSnapCue_.IsNull();
    snapCue["marker"]=snapCue.value("status")=="ambiguous" ? "cross" : snapCue.value("status")=="acquired" ?
        (snapCue.value("target").toObject().value("kind")=="filletCorner" ? "fillet-corner-square" :
         snapCue.value("target").toObject().value("kind")=="tangent" ? "tangent-square" :
         snapCue.value("target").toObject().value("kind")=="intersection" ? "intersection-square-cross" :
         snapCue.value("target").toObject().value("kind")=="midpoint" ? "midpoint-triangle" :
         snapCue.value("supportPoint").toBool() ? "support-center-cross" :
         snapCue.value("target").toObject().value("kind")=="origin" ? "origin-diamond" : "endpoint-diamond") : "none";
    result.insert("sketchSnapCue",snapCue);
    result.insert("sketchConstraintGlyphs",constraintGlyphs_);
    result.insert("sketchPatternGlyphs",patternGlyphs_);
    result.insert("sketchPatternHandles",patternHandles_);
    result.insert("sketchPatternHandleDrag",patternHandleDrag_);
    result.insert("splineHandles",splineHandleState_);
    result.insert("conicHandle",conicHandleState_);
    result.insert("sketchConstraintVisibility",QJsonObject{{"visible",!decorationFeature_.isEmpty()},{"editable",false}});
    QJsonArray profiles;for(const auto& profile:draftProfiles_) {
        auto summary=profile.summary;TopLoc_Location location;const auto triangles=BRep_Tool::Triangulation(profile.face,location);
        summary["displayTriangleCount"]=triangles.IsNull() ? 0 : triangles->NbTriangles();profiles.append(summary);
    }
    result.insert("sketchProfiles",profiles);
    QJsonArray standaloneMarkers;
    for(const auto& sketch:finishedSketches_)for(const auto& entry:sketch.summary["standalonePoints"].toArray()) {
        const auto point=entry.toObject();const auto marker=finishedPointModels_.value(point["id"].toString());
        auto descriptor=point;descriptor["sketchId"]=sketch.featureId;
        descriptor["visible"]=!context_.IsNull() && !marker.IsNull() && context_->IsDisplayed(marker);
        descriptor["selected"]=selectedFinishedPoint_["sketchId"]==sketch.featureId && selectedFinishedPoint_["pointId"]==point["pointId"];
        standaloneMarkers.append(descriptor);
    }
    result["sketchAxes3d"]=sketchAxis3dMarkers_;
    result["standalonePointMarkers"]=standaloneMarkers;result["selectedFinishedPoint"]=selectedFinishedPoint_;
    QJsonArray finishedProfiles;for(const auto& sketch:finishedSketches_)for(const auto& profile:sketch.profiles) {
        auto summary=profile.summary;TopLoc_Location location;const auto triangles=BRep_Tool::Triangulation(profile.face,location);
        summary["displayTriangleCount"]=triangles.IsNull() ? 0 : triangles->NbTriangles();
        summary["visible"]=(sketch.visible || sketch.featureId==editSourceSketch_ || commandSketches_.contains(sketch.featureId)) && sketch.featureId!=draftSketchId_;
        summary["selected"]=selectedProfileIds_.contains(profile.profileId) && sketch.featureId==selectedProfileSketch_;finishedProfiles.append(summary);
    }
    result.insert("finishedProfiles",finishedProfiles);
    result.insert("sweepPath",sweepPathState_);
    QJsonArray commandSketches;for(const auto& id:commandSketches_)commandSketches.append(id);result.insert("commandSketches",commandSketches);
    result.insert("editDisplay",QJsonObject{{"sourceSketchId",editSourceSketch_},{"hiddenBodyIds",hiddenBodyIds_}});
    result.insert("combineInputs",QJsonObject{{"targetBodyId",combineTargetId_},{"toolBodyIds",combineToolIds_},
        {"targetColor","blue"},{"toolColor","orange"},{"hiddenSelectedInputsTemporarilyRevealed",true}});
    result.insert("extrusionCue",extrusionCueState_.value("command")=="solid.faceOffset" ? QJsonObject{} : extrusionCueState_);
    result.insert("faceOffsetCue",faceOffsetCueState());
    result.insert("surfaceOffsetCue",extrusionCueState_.value("command")=="surface.offset" ? extrusionCueState_ : QJsonObject{});
    result.insert("cutPreviewToolCount",static_cast<int>(cutTools_.size()));
    result.insert("cutPreviewSelectedProfileCount",static_cast<int>(cutProfileOverlays_.size()));
    result.insert("faceNormals",faceNormalsState_);
    result.insert("zebra",zebraState_);
    result.insert("selectedSupportFace",selectedSupportFace_);
    result.insert("supportFaceHighlightVisible",!selectedSupportFaceOverlay_.IsNull());
    result.insert("hoveredSupportFace",hoveredSupportFace_);
    result.insert("revolveCue",revolveCueState_);result.insert("revolvePointer",revolvePointerState_);
    result.insert("sketchTextHandles",sketchTextHandleState_);
    result.insert("previewVisible",!preview_.IsNull());
    if(!preview_.IsNull()){Quantity_Color color;preview_->Color(color);double red,green,blue;color.Values(red,green,blue,Quantity_TOC_sRGB);result.insert("previewAppearance",QJsonObject{{"red",red},{"green",green},{"blue",blue},{"transparency",preview_->Transparency()}});}
    result.insert("previewReplacedBodyId",previewReplacedBody_);
    result.insert("previewProfileId",previewProfile_);
    result.insert("previewProfileIds",previewProfileIds_);
    result.insert("bodyStageKey",bodyStageKey_);
    QJsonArray displayIds,displayRefs;for(auto it=models_.begin();it!=models_.end();++it)if(context_->IsDisplayed(it.value())){
        const auto ref=modelReferences_.value(it.key());displayIds.append(ref["bodyId"]);displayRefs.append(ref);
    }result.insert("displayedBodyIds",displayIds);result.insert("displayedBodyReferences",displayRefs);
    result.insert("documentRevision",static_cast<qint64>(modelRevision_));
    const auto camera=view_->Camera();
    auto vector=[](const auto& value) {return QJsonArray{value.X(),value.Y(),value.Z()};};
    result.insert("canvases",canvasState_);
    result.insert("camera",QJsonObject{{"eye",vector(camera->Eye())},{"center",vector(camera->Center())},
        {"direction",vector(camera->Direction())},{"up",vector(camera->Up())},
        {"scale",camera->Scale()},{"aspect",camera->Aspect()},
        {"projection",camera->IsOrthographic() ? "orthographic" : "perspective"},{"fovY",camera->FOVy()}});
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
    if(e->button()==Qt::LeftButton && e->modifiers()==Qt::NoModifier && decorationSelection_["scaleInteraction"].toObject()["active"].toBool()){axis3dSuppressRelease_=true;dispatch("sketch.scalePick",{{"role",decorationSelection_["scaleInteraction"].toObject()["role"]},{"x",e->position().x()},{"y",e->position().y()}});setFocus(Qt::MouseFocusReason);e->accept();return;}
    if(e->button()==Qt::LeftButton && e->modifiers()==Qt::NoModifier && decorationSelection_["moveInteraction"].toObject()["mode"]=="rotate"){axis3dSuppressRelease_=true;dispatch("sketch.rotatePick",{{"x",e->position().x()},{"y",e->position().y()}});setFocus(Qt::MouseFocusReason);e->accept();return;}
    if(e->button()==Qt::LeftButton && e->modifiers()==Qt::NoModifier && decorationSelection_.value("axis3dInteraction").toObject().value("active").toBool()){axis3dSuppressRelease_=true;dispatch("sketch.axis3dPivot",{{"x",e->position().x()},{"y",e->position().y()}});setFocus(Qt::MouseFocusReason);e->accept();return;}
    if(e->button()==Qt::LeftButton && e->modifiers()==Qt::NoModifier && decorationSelection_.value("sketch3dEnabled").toBool() && !pickSketchAxis3d(e->position().x(),e->position().y()).isEmpty()){axis3dSuppressRelease_=true;dispatch("sketch.axis3dPick",{{"x",e->position().x()},{"y",e->position().y()}});setFocus(Qt::MouseFocusReason);e->accept();return;}
    const bool alternate=inputPreset_=="shiftAlt";
    const bool chord=e->modifiers()==(Qt::ShiftModifier|Qt::AltModifier);
    if(e->button()==Qt::RightButton)alternateRightContextSuppressed_=false;
    if(e->button()==Qt::LeftButton && e->position().x()>=width()-180 && e->position().y()<=180) {
        const auto hit=cubeHit(e->position().x(),e->position().y());
        if(!hit.isEmpty()) {
            cubeSuppressRelease_=false;
            cubePressHandled_=true;cubeDragging_=false;
            cubePress_=cubeLast_=e->position();cubePressedRegion_=hit.value("region").toString();
            setFocus(Qt::MouseFocusReason);e->accept();return;
        }
    }
    if(e->button()==Qt::LeftButton && e->modifiers()==Qt::NoModifier && navigationTool_=="select" && decorationSelection_.value("pasteInteraction").toObject().value("active").toBool()){pasteSuppressRelease_=true;dispatch("sketch.pasteAccept",{});setFocus(Qt::MouseFocusReason);e->accept();return;}
    // Delay an ambiguous curve click until release so holding it can open
    // Select Other without transiently changing the authoritative selection.
    if(e->button()==Qt::LeftButton && e->modifiers()==Qt::NoModifier && navigationTool_=="select"
        && !sketchPlane_.isEmpty() && decorationSelection_.value("tool")=="select"
        && pickConstraint(e->position().x(),e->position().y()).isEmpty()
        && pickAnnotation(e->position().x(),e->position().y()).isEmpty()
        && sketchSelectionCandidates(decorationFeature_,e->position().x(),e->position().y()).size()>1) {
        sketchSelectOtherPress_=e->position();sketchSelectOtherPending_=true;sketchSelectOtherOpened_=false;
        if(!sketchSelectOtherTimer_) {
            sketchSelectOtherTimer_=new QTimer(this);sketchSelectOtherTimer_->setSingleShot(true);
            connect(sketchSelectOtherTimer_,&QTimer::timeout,this,[this]{
                if(!sketchSelectOtherPending_ || decorationSelection_.value("tool")!="select" || sketchPlane_.isEmpty())return;
                const auto candidates=sketchSelectionCandidates(decorationFeature_,sketchSelectOtherPress_.x(),sketchSelectOtherPress_.y());
                if(candidates.size()<2)return;
                sketchSelectOtherOpened_=true;
                auto* menu=new QMenu(this);menu->setObjectName("sketch.selectOther");menu->setAttribute(Qt::WA_DeleteOnClose);
                menu->installEventFilter(this);
                menu->addSection("Select Other");
                connect(menu,&QMenu::hovered,this,[this](QAction* action){previewSketchSelectionCandidate(action->data().toJsonObject());});
                for(const auto& value:candidates) {
                    const auto row=value.toObject();const auto id=row.value("entityId").toString();
                    const auto source=row.value("source").toObject();
                    const QString role=row.value("projected").toBool() ? "Projected" : "Authored";
                    auto* action=menu->addAction(QString("%1 %2 · %3").arg(role,row.value("type").toString(),id.first(8)));
                    action->setData(row);action->setToolTip(id+(source.isEmpty() ? QString{} : "\nSource: "+QString::fromUtf8(QJsonDocument(source).toJson(QJsonDocument::Compact))));
                    connect(action,&QAction::triggered,this,[this,id]{QTimer::singleShot(0,this,[this,id]{dispatch("sketch.selectionSet",{{"entityIds",QJsonArray{id}}});});});
                }
                connect(menu,&QMenu::aboutToHide,this,[this]{previewSketchSelectionCandidate();sketchSelectOtherPending_=false;sketchSelectOtherOpened_=false;});
                menu->popup(mapToGlobal(sketchSelectOtherPress_.toPoint()));
            });
        }
        setFocus(Qt::MouseFocusReason);sketchSelectOtherTimer_->start(500);e->accept();return;
    }
    if(e->button()==Qt::LeftButton && navigationTool_!="select") {
        navigationSuppressRelease_=false;navigationPressActive_=true;navigationMoved_=false;
        navigationPress_=navigationLast_=e->position();
        if(navigationTool_=="zoomWindow") {
            if(!navigationWindowBand_){navigationWindowBand_=new QRubberBand(QRubberBand::Rectangle,this);
                navigationWindowBand_->setObjectName("view.zoomWindowBand");
                navigationWindowBand_->setAttribute(Qt::WA_TransparentForMouseEvents);}
            navigationWindowBand_->setGeometry(QRect(e->position().toPoint(),QSize()));
            navigationWindowBand_->show();navigationWindowBand_->raise();
        } else setCursor(Qt::ClosedHandCursor);
        setFocus(Qt::MouseFocusReason);e->accept();return;
    }
    if(e->button()==Qt::LeftButton && e->modifiers()==Qt::NoModifier && decorationSelection_["moveInteraction"].toObject()["active"].toBool()){dispatch("sketch.movePick",{{"x",e->position().x()},{"y",e->position().y()}});setFocus(Qt::MouseFocusReason);e->accept();return;}
    if(e->button()==Qt::LeftButton && e->modifiers()==Qt::NoModifier && decorationSelection_["patternInteraction"].toObject()["active"].toBool()){
        const auto role=decorationSelection_["patternInteraction"].toObject()["role"].toString();
        if(role!="values"){dispatch("sketch.patternPick",{{"x",e->position().x()},{"y",e->position().y()}});setFocus(Qt::MouseFocusReason);e->accept();return;}
        for(const auto& row:patternHandles_){const auto handle=row.toObject();if(QLineF(e->position(),QPointF(handle["x"].toDouble(),handle["y"].toDouble())).length()>handle["hitRadius"].toDouble())continue;const auto point=sketchPoint(e->position().x(),e->position().y(),sketchPlane_,sketchFrame_);patternHandleDrag_=handle;patternHandleDrag_["pressU"]=point["uMm"];patternHandleDrag_["pressV"]=point["vMm"];dispatch("sketch.patternHandle",{{"phase","begin"},{"key",handle["key"]}});setFocus(Qt::MouseFocusReason);e->accept();return;}
        setFocus(Qt::MouseFocusReason);e->accept();return;
    }
    if(orbitCenterPicking_ && e->button()==Qt::LeftButton) {
        orbitCenterPressHandled_=true;
        if(dispatch)dispatch("view.orbitCenterPick",{{"x",e->position().x()},{"y",e->position().y()}});
        setFocus(Qt::MouseFocusReason);e->accept();return;
    }
    if(alternate && chord && e->button()==Qt::RightButton) {
        zoomButton_=Qt::RightButton;zoomAnchor_=zoomLast_=e->position();alternateRightContextSuppressed_=true;
        setCursor(Qt::ClosedHandCursor);setFocus(Qt::MouseFocusReason);e->accept();return;
    }
    if((alternate && chord && e->button()==Qt::LeftButton)
        || (!alternate && e->button()==Qt::MiddleButton && e->modifiers().testFlag(Qt::ShiftModifier))
        || (e->button()==Qt::LeftButton && freeOrbitTool_ && !e->modifiers().testFlag(Qt::AltModifier))) {
        orbitButton_=e->button();orbitLast_=e->position();setCursor(Qt::ClosedHandCursor);
        setFocus(Qt::MouseFocusReason);e->accept();return;
    }
    if((alternate && chord && e->button()==Qt::MiddleButton) ||
       (!alternate && (e->button()==Qt::MiddleButton || (e->button()==Qt::LeftButton && e->modifiers().testFlag(Qt::AltModifier))))) {
        panButton_=e->button();last_=e->position();setCursor(Qt::ClosedHandCursor);
        setFocus(Qt::MouseFocusReason);e->accept();return;
    }
    if(jointOriginPicking_ && jointOriginLocked_ && e->button()==Qt::LeftButton
        && e->modifiers().testFlag(Qt::ControlModifier)) {
        const auto glyph=jointOriginState()["glyph"].toObject();
        if(!glyph.isEmpty() && QLineF(e->position(),QPointF(glyph["x"].toDouble(),glyph["y"].toDouble())).length()<14){
            const auto position=jointOriginLock_["position"].toObject();
            jointOriginLock_={};jointOriginHover_={};jointOriginLocked_=false;refreshJointOriginCue();
            jointOriginPressHandled_=true;
            if(dispatch)dispatch("joint.originPick",{{"position",position}});
            setFocus(Qt::MouseFocusReason);e->accept();return;
        }
    }
    if(canvasPicking_ && e->button()==Qt::LeftButton && !e->modifiers().testFlag(Qt::AltModifier)){
        if(dispatch)dispatch("canvas.pick",{{"x",e->position().x()},{"y",e->position().y()}});e->accept();return;
    }
    if(e->button()==Qt::LeftButton && !e->modifiers().testFlag(Qt::AltModifier) && !assemblyDragCue_.isEmpty()
        && pickBody(e->position().x(),e->position().y())==assemblyDragCue_["bodyId"].toString()) {
        if(const auto angle=assemblyPointerAngle(e->position())){
            assemblyDrag_={{"jointId",assemblyDragCue_["jointId"]},{"pressX",e->position().x()},
                {"pressY",e->position().y()},{"startRadians",*angle},{"startDegrees",assemblyDragCue_["angleDeg"]}};
            setFocus(Qt::MouseFocusReason);e->accept();return;
        }
    }
    if(e->button()==Qt::LeftButton && e->modifiers()==Qt::NoModifier && !sketchPlane_.isEmpty()){
        for(const auto& row:chamferHandles_){const auto handle=row.toObject();
            if(QLineF(e->position(),QPointF(handle.value("x").toDouble(),handle.value("y").toDouble())).length()>handle.value("hitRadius").toDouble())continue;
            // Save the drag axis before dispatch: preview refresh rebuilds overlays.
            const auto captured=handle;setFocus(Qt::MouseFocusReason);
            dispatch("sketch.chamferHandle",{{"phase","begin"},{"chamferId",handle.value("chamferId")},{"side",handle.value("side")}});chamferDrag_=captured;
            e->accept();return;
        }
    }
    if(e->button()==Qt::LeftButton && !e->modifiers().testFlag(Qt::AltModifier) &&
       decorationSelection_.value("tool")=="conic" && !conicHandleState_.isEmpty() &&
       QLineF(e->position(),QPointF(conicHandleState_["x"].toDouble(),conicHandleState_["y"].toDouble())).length()<12) {
        conicDragging_=true;setFocus(Qt::MouseFocusReason);e->accept();return;
    }

    if(e->button()==Qt::LeftButton && !e->modifiers().testFlag(Qt::AltModifier) && !sketchPlane_.isEmpty() && decorationSelection_.value("tool")=="mirror"){
        mirrorWindowActive_=true;mirrorWindowPress_=e->position();setFocus(Qt::MouseFocusReason);
        if(!mirrorWindowBand_)mirrorWindowBand_=new QRubberBand(QRubberBand::Rectangle,this);
        mirrorWindowBand_->setGeometry(QRect(mirrorWindowPress_.toPoint(),QSize()));e->accept();return;
    }
    if(e->button()==Qt::LeftButton && !e->modifiers().testFlag(Qt::AltModifier) && decorationSelection_.value("tool")=="select"){
        QJsonObject picked;double best=9;for(const auto& value:splineHandleState_["grips"].toArray()){const auto grip=value.toObject();const double distance=QLineF(e->position(),QPointF(grip["x"].toDouble(),grip["y"].toDouble())).length();if(distance<best){best=distance;picked=grip;}}
        if(picked.isEmpty() && !decorationFeature_.isEmpty()){const auto hit=pickSketch(decorationFeature_,e->position().x(),e->position().y(),"point",sketchFrame_);if(!hit.isEmpty())for(const auto& v:decorationFeature_["parameters"].toObject()["entities"].toArray()){const auto spline=v.toObject();if((spline["type"]=="fitSpline"||spline["type"]=="controlSpline")&&spline["pointIds"].toArray().contains(hit["pointId"])){picked={{"kind","point"},{"entityId",spline["id"]},{"pointId",hit["pointId"]}};break;}}}
        if(!picked.isEmpty() && e->modifiers()==Qt::NoModifier){
            // Keep a deliberate grip drag available even when its generous hit
            // area covers the source's distinct Offset result. An unmoved click
            // on that result follows the ordinary picker on release instead.
            const auto hit=pickSketch(decorationFeature_,e->position().x(),e->position().y(),{},sketchFrame_);
            if(hit.value("kind")=="entity" && hit.value("distancePixels").toDouble()<=2)
                for(const auto& value:decorationFeature_.value("parameters").toObject().value("entities").toArray()){
                    const auto entity=value.toObject();
                    if(entity.value("id")==hit.value("entityId") && entity.value("type")=="fitSplineOffset"
                        && entity.value("sourceEntityId")==picked.value("entityId")){
                        picked["selectResultOnClick"]=true;break;
                    }
                }
        }
        if(!picked.isEmpty()){setFocus(Qt::MouseFocusReason);if(picked["kind"]=="point" && !picked.value("selectResultOnClick").toBool())dispatch("sketch.selectionPick",{{"x",e->position().x()},{"y",e->position().y()}});splineDrag_=picked;splineDrag_["pressX"]=e->position().x();splineDrag_["pressY"]=e->position().y();splineDrag_["moved"]=false;auto begin=picked;begin.remove("x");begin.remove("y");begin.remove("selectResultOnClick");begin["phase"]="begin";dispatch("sketch.splineDrag",begin);e->accept();return;}
    }
    if(e->button()==Qt::LeftButton && !sketchTextHandleState_.isEmpty()){
        const auto move=sketchTextHandleState_["move"].toObject(),rotate=sketchTextHandleState_["rotate"].toObject();
        const QPointF m(move["x"].toDouble(),move["y"].toDouble()),r(rotate["x"].toDouble(),rotate["y"].toDouble());
        const auto mode=QLineF(e->position(),m).length()<12 ? QString("move")
            : QLineF(e->position(),r).length()<12 ? QString("rotate") : QString{};
        if(!mode.isEmpty()){sketchTextDrag_={{"mode",mode},{"startX",e->position().x()},{"startY",e->position().y()}};
            setFocus(Qt::MouseFocusReason);dispatch("sketch.textHandle",{{"mode",mode},{"phase","begin"},
                {"startX",e->position().x()},{"startY",e->position().y()},{"x",e->position().x()},{"y",e->position().y()}});e->accept();return;}
    }
    if(!revolveCueState_.isEmpty())revolvePointerState_={{"event","press"},{"x",e->position().x()},{"y",e->position().y()},{"button",int(e->button())},{"modifiers",int(e->modifiers())}};
    if(e->button()==Qt::LeftButton && !e->modifiers().testFlag(Qt::AltModifier) && !revolveCueState_.isEmpty()) {
        auto point=[](const QJsonObject& p){return QPointF(p["x"].toDouble(),p["y"].toDouble());};
        bool picked=QLineF(e->position(),point(revolveCueState_["endScreen"].toObject())).length()<11;
        const auto ring=revolveCueState_["ringScreen"].toArray();
        for(int i=1;i<ring.size() && !picked;++i){const auto a=point(ring[i-1].toObject()),b=point(ring[i].toObject()),d=b-a;const auto length=QPointF::dotProduct(d,d);const auto t=length>1e-8 ? std::clamp(QPointF::dotProduct(e->position()-a,d)/length,0.0,1.0):0.;picked=QLineF(e->position(),a+d*t).length()<7;}
        revolvePointerState_["picked"]=picked;
        if(picked && revolveCueState_["dragAvailable"].toBool()){setFocus(Qt::MouseFocusReason);revolveDrag_=revolveCueState_;updateRevolveDrag(e->position(),"begin");e->accept();return;}
    }

    if(e->button()==Qt::LeftButton && !moveCueState_.isEmpty()) {
        auto point=[](const QJsonObject& p){return QPointF(p["x"].toDouble(),p["y"].toDouble());};
        auto distance=[](const QPointF& p,const QPointF& a,const QPointF& b){auto d=b-a;auto l=QPointF::dotProduct(d,d);return QLineF(p,a+d*(l>1e-9 ? std::clamp(QPointF::dotProduct(p-a,d)/l,0.0,1.0) : 0.0)).length();};
        double best=9;QJsonObject picked;QString mode;
        for(const auto& entry:moveCueState_["handles"].toArray()){const auto h=entry.toObject();const auto center=point(h["start"].toObject()),end=point(h["end"].toObject());const auto d=distance(e->position(),center+(end-center)*.25,end);
            if(d<best){best=d;picked=h;mode="translation";}
            auto ring=h["ring"].toArray();for(int i=1;i<ring.size();++i){auto d=distance(e->position(),point(ring[i-1].toObject()),point(ring[i].toObject()));if(d<best){best=d;picked=h;mode="rotation";}}
        }
        if(!picked.isEmpty()){setFocus(Qt::MouseFocusReason);moveDrag_={{"handle",picked},{"mode",mode},{"pressX",e->position().x()},{"pressY",e->position().y()},{"initialInputs",moveCueState_["inputs"]},{"handleCommand",moveCueState_["inputs"].toObject().value("handleCommand").toString("solid.moveHandle")},{"releaseCommand",moveCueState_["inputs"].toObject().value("releaseCommand").toString("solid.moveApplyGesture")}};e->accept();return;}
    }
    if(e->button()==Qt::RightButton) {
        ++rightMousePressCount_;
        if(!sketchPlane_.isEmpty() && decorationSelection_.value("tool")=="select" && e->modifiers()==Qt::NoModifier) {
            QContextMenuEvent context(QContextMenuEvent::Mouse,e->position().toPoint(),e->globalPosition().toPoint());
            contextMenuEvent(&context);e->accept();return;
        }
    }
    if(e->button()==Qt::LeftButton){
        if(!sketchPlane_.isEmpty() && decorationSelection_.value("tool")=="line"
            && !decorationSelection_.value("chainLineId").toString().isEmpty()
            && !e->modifiers().testFlag(Qt::AltModifier)){
            const auto pointId=decorationSelection_.value("chainPointId");
            for(const auto& value:decorationFeature_.value("parameters").toObject().value("points").toArray()){
                const auto point=value.toObject();if(point.value("id")!=pointId)continue;
                const auto screen=projectSketchPoint(point.value("uMm").toDouble(),point.value("vMm").toDouble(),sketchPlane_);
                if(QLineF(e->position(),QPointF(screen.value("x").toDouble(),screen.value("y").toDouble())).length()<=10){
                    sketchLineArcDrag_={{"pressX",e->position().x()},{"pressY",e->position().y()},{"moved",false}};
                    dispatch("sketch.lineArcDrag",{{"phase","begin"},{"x",e->position().x()},{"y",e->position().y()}});
                    setFocus(Qt::MouseFocusReason);e->accept();return;
                }
                break;
            }
        }
        if(!sketchPlane_.isEmpty() && decorationSelection_.value("tool")=="project") {
            dispatch("sketch.projectPick",{{"x",e->position().x()},{"y",e->position().y()}});
            e->accept();return;
        }
        if(sketchPlane_.isEmpty() && extrusionCueState_.value("dragAvailable").toBool() && !e->modifiers().testFlag(Qt::AltModifier)) {
            const auto start=extrusionCueState_.value("startScreen").toObject(),end=extrusionCueState_.value("endScreen").toObject();
            const QPointF a(start["x"].toDouble(),start["y"].toDouble()),b(end["x"].toDouble(),end["y"].toDouble());
            const auto direction=b-a;const double length2=QPointF::dotProduct(direction,direction);
            if(length2>1e-8) {
                const double t=std::clamp(QPointF::dotProduct(e->position()-a,direction)/length2,0.0,1.0);
                if(QLineF(e->position(),a+direction*t).length()<=10.0) {
                    extrusionDragActive_=true;extrusionDragPress_=e->position();
                    extrusionDragInitialMm_=extrusionCueState_.value("distanceMm").toDouble();
                    const auto axis=extrusionCueState_.value("axisPixelsPerMm").toObject();
                    extrusionDragAxisPixelsPerMm_=QPointF(axis.value("x").toDouble(),axis.value("y").toDouble());
                    extrusionDragCommand_=extrusionCueState_.value("command")=="solid.faceOffset" ? "solid.faceOffsetDrag" : "solid.extrudeDrag";
                    setFocus(Qt::MouseFocusReason);
                    extrusionCueState_["dragActive"]=true;e->accept();return;
                }
            }
        }
        if(!sketchPlane_.isEmpty() && decorationSelection_.value("tool")=="select") {
            const auto badge=pickConstraint(e->position().x(),e->position().y());
            if(!badge.isEmpty()) {
                dispatch("sketch.constraintSelect",{{"constraintId",badge.value("constraintId")},
                    {"additive",e->modifiers().testFlag(Qt::ShiftModifier)}});
                e->accept();return;
            }
            const auto groups=decorationFeature_["parameters"].toObject()["slotGroups"].toArray();
            if(!groups.isEmpty() && decorationDiagnostics_["dof"].toInt(-1)>0) {
                const auto hit=pickSketch(decorationFeature_,e->position().x(),e->position().y(),"point");
                for(const auto& value:groups) {
                    const auto group=value.toObject();QJsonObject guide;
                    for(const auto& entry:decorationFeature_["parameters"].toObject()["entities"].toArray())
                        if(entry.toObject()["id"]==group["centerlineId"]){guide=entry.toObject();break;}
                    if(hit["kind"]!="point" || (hit["pointId"]!=guide["startPointId"] && hit["pointId"]!=guide["endPointId"]))continue;
                    slotPointDrag_={{"pointId",hit["pointId"]},{"pressX",e->position().x()},{"pressY",e->position().y()}};
                    dispatch("sketch.slotDrag",{{"phase","begin"},{"pointId",hit["pointId"]}});
                    setFocus(Qt::MouseFocusReason);e->accept();return;
                }
            }
        }
        if(!sketchPlane_.isEmpty() && !decorationSelection_.value("tool").toString().endsWith("Polygon") && decorationSelection_.value("tool")!="point" && decorationSelection_.value("tool")!="line" && decorationSelection_.value("tool")!="rectangle" && decorationSelection_.value("tool")!="circle" && decorationSelection_.value("tool")!="twoPointCircle" && decorationSelection_.value("tool")!="threePointCircle" && decorationSelection_.value("tool")!="centerArc" && decorationSelection_.value("tool")!="threePointArc" && decorationPending_.value("phase")!="placing") {
        const auto picked=pickAnnotation(e->position().x(),e->position().y());
            if(!picked.isEmpty()) {
                if(!picked.value("chamferId").toString().isEmpty()){e->accept();return;}
                draggedAnnotation_=picked.value("dimensionId").toString();annotationGestureId_=QString::number(++nextAnnotationGesture_);annotationPress_=e->position();annotationDragged_=false;annotationAutoDrag_=picked.value("automaticPlacement").toBool();const auto display=picked.value("displayLabel").toObject();annotationDisplayPress_={display.value("x").toDouble(),display.value("y").toDouble()};annotationGrabOffset_=annotationAutoDrag_?e->position()-annotationDisplayPress_:QPointF{};const auto a=picked.value("annotation").toObject();const auto projected=projectSketchPoint(a.value("uMm").toDouble(),a.value("vMm").toDouble(),sketchPlane_);annotationRawPress_={projected.value("x").toDouble(),projected.value("y").toDouble()};
                dispatch("sketch.annotationSelect",{{"dimensionId",draggedAnnotation_}});e->accept();return;
            }
        }
        QJsonObject args{{"x",e->position().x()},{"y",e->position().y()}};
        if(!sketchPlane_.isEmpty()){
            args["additive"]=e->modifiers().testFlag(Qt::ShiftModifier);
            args["tangentModifier"]=e->modifiers().testFlag(Qt::ShiftModifier);
            args["suppressInference"]=e->modifiers().testFlag(Qt::ControlModifier) ||
                e->modifiers().testFlag(Qt::MetaModifier);
        }
        else {
            args["additive"]=e->modifiers().testFlag(Qt::ShiftModifier);
            args["toggle"]=e->modifiers().testFlag(Qt::ControlModifier) || e->modifiers().testFlag(Qt::MetaModifier);
        }
        dispatch(sketchPlane_.isEmpty() ? "selection.pick" : "sketch.placePoint",args);e->accept();
    }
    else QWidget::mousePressEvent(e);
}
void ReferenceView::mouseMoveEvent(QMouseEvent* e) {
    if(decorationSelection_["pasteInteraction"].toObject()["active"].toBool() && e->buttons()==Qt::NoButton){dispatch("sketch.pastePointer",{{"x",e->position().x()},{"y",e->position().y()}});e->accept();return;}
    if(sketchSelectOtherPending_ && !sketchSelectOtherOpened_) {
        if(QLineF(e->position(),sketchSelectOtherPress_).length()>QApplication::startDragDistance()) {
            sketchSelectOtherTimer_->stop();sketchSelectOtherPending_=false;
        } else {e->accept();return;}
    }

    if(!patternHandleDrag_.isEmpty() && e->buttons().testFlag(Qt::LeftButton)){
        const auto point=sketchPoint(e->position().x(),e->position().y(),sketchPlane_,sketchFrame_);const double delta=(point["uMm"].toDouble()-patternHandleDrag_["pressU"].toDouble())*patternHandleDrag_["unitU"].toDouble()+(point["vMm"].toDouble()-patternHandleDrag_["pressV"].toDouble())*patternHandleDrag_["unitV"].toDouble();
        double value=patternHandleDrag_["kind"]=="quantity" ? patternHandleDrag_["value"].toDouble()+std::round(delta/patternHandleDrag_["spacingMm"].toDouble()) : std::round((patternHandleDrag_["value"].toDouble()+delta)*10)/10;
        if(patternHandleDrag_["kind"]=="angle"){
            const double u=patternHandleDrag_["centerU"].toDouble(),v=patternHandleDrag_["centerV"].toDouble();const double start=std::atan2(patternHandleDrag_["pressV"].toDouble()-v,patternHandleDrag_["pressU"].toDouble()-u),now=std::atan2(point["vMm"].toDouble()-v,point["uMm"].toDouble()-u);const double accumulated=patternHandleDrag_["angleDeltaRad"].toDouble()+std::remainder(now-patternHandleDrag_.value("lastAngleRad").toDouble(start),2*std::numbers::pi);patternHandleDrag_["lastAngleRad"]=now;patternHandleDrag_["angleDeltaRad"]=accumulated;value=std::round(patternHandleDrag_["value"].toDouble()+accumulated*180/std::numbers::pi);
        }
        dispatch("sketch.patternHandle",{{"phase","move"},{"value",value}});e->accept();return;
    }
    if(decorationSelection_["patternInteraction"].toObject()["active"].toBool() && decorationSelection_["patternInteraction"].toObject()["role"]!="values"){dispatch("sketch.patternHover",{{"x",e->position().x()},{"y",e->position().y()}});e->accept();return;}
    if(!chamferDrag_.isEmpty() && e->buttons().testFlag(Qt::LeftButton)){
        const auto point=sketchPoint(e->position().x(),e->position().y(),sketchPlane_,sketchFrame_);
        const QPointF axis(chamferDrag_.value("axisU").toDouble(),chamferDrag_.value("axisV").toDouble());
        const QPointF offset(point.value("uMm").toDouble()-chamferDrag_.value("originUMm").toDouble(),point.value("vMm").toDouble()-chamferDrag_.value("originVMm").toDouble());
        const bool angle=chamferDrag_.value("kind")=="angle";
        const double distance=angle?std::atan2(chamferDrag_.value("turn").toDouble()*(axis.x()*offset.y()-axis.y()*offset.x()),QPointF::dotProduct(offset,axis))*180/std::acos(-1.0)
            :QPointF::dotProduct(offset,axis)/QPointF::dotProduct(axis,axis);
        chamferDrag_["requestedValue"]=distance;chamferDrag_[angle?"requestedAngleDeg":"requestedDistanceMm"]=distance;
        dispatch("sketch.chamferHandle",{{"phase","move"},{"value",distance}});e->accept();return;
    }
    if(cubePressHandled_) {
        if(!e->buttons().testFlag(Qt::LeftButton)){cancelCubeGesture();e->accept();return;}
        if(!cubeDragging_ && QLineF(cubePress_,e->position()).length()>=QApplication::startDragDistance()) {
            cubeDragging_=true;cubeHoverActive_=false;setCursor(Qt::ClosedHandCursor);
        }
        if(cubeDragging_) {
            const auto delta=e->position()-cubeLast_;
            if(dispatch && !delta.isNull())dispatch("view.orbit",{{"deltaX",delta.x()},{"deltaY",delta.y()},{"pivot","viewCenter"}});
            cubeLast_=e->position();
        }
        e->accept();return;
    }
    if(navigationPressActive_) {
        if(!e->buttons().testFlag(Qt::LeftButton)){cancelNavigationGesture(true);e->accept();return;}
        if(!navigationMoved_ && QLineF(navigationPress_,e->position()).length()>=QApplication::startDragDistance())navigationMoved_=true;
        if(navigationMoved_ && navigationTool_=="pan") {
            const auto delta=e->position()-navigationLast_;
            if(dispatch && !delta.isNull())dispatch("view.pan",{{"x",delta.x()},{"y",delta.y()}});
        } else if(navigationMoved_ && navigationTool_=="zoom") {
            const auto delta=e->position()-navigationLast_;
            const double factor=std::exp(-delta.y()*0.012);
            if(dispatch && std::isfinite(factor) && factor>0 && delta.y()!=0)
                dispatch("view.zoom",{{"factor",std::clamp(factor,0.1,10.0)},
                    {"x",navigationPress_.x()},{"y",navigationPress_.y()}});
        } else if(navigationTool_=="zoomWindow" && navigationWindowBand_) {
            navigationWindowBand_->setGeometry(QRect(navigationPress_.toPoint(),e->position().toPoint()).normalized());
        }
        navigationLast_=e->position();e->accept();return;
    }
    if(orbitCenterPicking_){e->accept();return;}
    if(orbitButton_!=Qt::NoButton) {
        if(!e->buttons().testFlag(orbitButton_)){orbitButton_=Qt::NoButton;unsetCursor();e->accept();return;}
        const auto delta=e->position()-orbitLast_;orbitLast_=e->position();
        if(dispatch && !delta.isNull())dispatch("view.orbit",{{"deltaX",delta.x()},{"deltaY",delta.y()}});
        e->accept();return;
    }
    if(zoomButton_!=Qt::NoButton) {
        if(!e->buttons().testFlag(zoomButton_)){zoomButton_=Qt::NoButton;unsetCursor();e->accept();return;}
        const auto delta=e->position()-zoomLast_;zoomLast_=e->position();
        const double factor=std::exp(-delta.y()*0.012);
        if(dispatch && std::isfinite(factor) && factor>0 && delta.y()!=0)
            dispatch("view.zoom",{{"factor",std::clamp(factor,0.1,10.0)},{"x",zoomAnchor_.x()},{"y",zoomAnchor_.y()}});
        e->accept();return;
    }
    if(panButton_!=Qt::NoButton) {
        if(!e->buttons().testFlag(panButton_)){panButton_=Qt::NoButton;unsetCursor();e->accept();return;}
        const auto delta=e->position()-last_;last_=e->position();
        if(dispatch && !delta.isNull())dispatch("view.pan",{{"x",delta.x()},{"y",delta.y()}});
        e->accept();return;
    }
    if(e->buttons()==Qt::NoButton) {
        const bool overCube=e->position().x()>=width()-180 && e->position().y()<=180 &&
            !cubeHit(e->position().x(),e->position().y()).isEmpty();
        if(overCube){cubeHoverActive_=true;setCursor(Qt::PointingHandCursor);updateCubeAccessoryVisibility();e->accept();return;}
        if(cubeHoverActive_){cubeHoverActive_=false;unsetCursor();context_->ClearDetected(true);}
        updateCubeAccessoryVisibility();
    }
    if(!sketchLineArcDrag_.isEmpty() && e->buttons().testFlag(Qt::LeftButton)){
        const QPointF press(sketchLineArcDrag_.value("pressX").toDouble(),sketchLineArcDrag_.value("pressY").toDouble());
        if(QLineF(press,e->position()).length()>=4)sketchLineArcDrag_["moved"]=true;
        if(sketchLineArcDrag_.value("moved").toBool())
            dispatch("sketch.lineArcDrag",{{"phase","move"},{"x",e->position().x()},{"y",e->position().y()}});
        e->accept();return;
    }
    if(jointOriginPicking_ && e->buttons()==Qt::NoButton) {
        updateJointOriginHover(e->position());e->accept();return;
    }
    if(!moveDrag_.isEmpty()){moveDrag_["lastPointerX"]=e->position().x();moveDrag_["lastPointerY"]=e->position().y();moveDrag_["leftButton"]=e->buttons().testFlag(Qt::LeftButton);}
    if(!assemblyDrag_.isEmpty() && e->buttons().testFlag(Qt::LeftButton)){
        const QPointF press(assemblyDrag_["pressX"].toDouble(),assemblyDrag_["pressY"].toDouble());
        if(QLineF(press,e->position()).length()>=4)if(const auto angle=assemblyPointerAngle(e->position())){
            const double degrees=assemblyDrag_["startDegrees"].toDouble()+
                std::remainder(*angle-assemblyDrag_["startRadians"].toDouble(),2*std::numbers::pi)*180/std::numbers::pi;
            if(degrees>=-180 && degrees<=180 && dispatch){
                assemblyDrag_["moved"]=true;
                dispatch("joint.previewAngle",{{"jointId",assemblyDrag_["jointId"]},{"angleDeg",degrees}});
            }
        }
        e->accept();return;
    }
    if(conicDragging_ && e->buttons().testFlag(Qt::LeftButton)) {
        dispatch("sketch.conicHandleDrag",{{"x",e->position().x()},{"y",e->position().y()}});e->accept();return;
    }

    if(!slotPointDrag_.isEmpty() && e->buttons().testFlag(Qt::LeftButton)) {
        const QPointF press(slotPointDrag_["pressX"].toDouble(),slotPointDrag_["pressY"].toDouble());
        if(QLineF(press,e->position()).length()<4&&!slotPointDrag_["moved"].toBool()){e->accept();return;}
        slotPointDrag_["moved"]=true;
        try {const auto end=sketchPoint(e->position().x(),e->position().y(),sketchPlane_,sketchFrame_);
            dispatch("sketch.slotDrag",{{"phase","move"},{"uMm",end["uMm"]},{"vMm",end["vMm"]}});
        } catch(const std::exception&) {dispatch("sketch.slotDrag",{{"phase","cancel"}});}
        e->accept();return;
    }
    if(!splineDrag_.isEmpty() && e->buttons().testFlag(Qt::LeftButton)){const QPointF press(splineDrag_["pressX"].toDouble(),splineDrag_["pressY"].toDouble());if(QLineF(press,e->position()).length()<4&&!splineDrag_["moved"].toBool()){e->accept();return;}splineDrag_["moved"]=true;const auto p=sketchPoint(e->position().x(),e->position().y(),sketchPlane_,sketchFrame_);dispatch("sketch.splineDrag",{{"phase","move"},{"uMm",p["uMm"]},{"vMm",p["vMm"]}});e->accept();return;}
    if(mirrorWindowActive_ && e->buttons().testFlag(Qt::LeftButton)){
        if(QLineF(mirrorWindowPress_,e->position()).length()>=4){mirrorWindowBand_->setGeometry(QRect(mirrorWindowPress_.toPoint(),e->position().toPoint()).normalized());mirrorWindowBand_->show();}
        e->accept();return;
    }
    if(!sketchTextDrag_.isEmpty() && e->buttons().testFlag(Qt::LeftButton)){
        dispatch("sketch.textHandle",{{"mode",sketchTextDrag_["mode"]},{"phase","move"},
            {"startX",sketchTextDrag_["startX"]},{"startY",sketchTextDrag_["startY"]},
            {"x",e->position().x()},{"y",e->position().y()}});e->accept();return;
    }
    if(!revolveDrag_.isEmpty() && e->buttons().testFlag(Qt::LeftButton)){updateRevolveDrag(e->position(),"move");e->accept();return;}
    if(!moveDrag_.isEmpty() && e->buttons().testFlag(Qt::LeftButton)) {
        const auto h=moveDrag_["handle"].toObject();auto point=[](const QJsonObject& p){return QPointF(p["x"].toDouble(),p["y"].toDouble());};
        const auto center=point(h["start"].toObject());const QPointF press(moveDrag_["pressX"].toDouble(),moveDrag_["pressY"].toDouble());
        const int axis=h["axis"].toInt();const QStringList distances{"xMm","yMm","zMm"},angles{"xDeg","yDeg","zDeg"};QString key;double delta=0;
        if(moveDrag_["mode"]=="translation") {auto direction=point(h["end"].toObject())-center;auto length2=QPointF::dotProduct(direction,direction);if(length2<1e-8)return;delta=QPointF::dotProduct(e->position()-press,direction)/length2*h["mmLength"].toDouble();key=distances[axis];}
        else {const auto u=point(h["basisU"].toObject())-center,v=point(h["basisV"].toObject())-center;const auto determinant=u.x()*v.y()-u.y()*v.x();if(std::abs(determinant)<1e-8)return;auto angle=[&](const QPointF& p){auto q=p-center;return std::atan2((u.x()*q.y()-u.y()*q.x())/determinant,(q.x()*v.y()-q.y()*v.x())/determinant);};delta=std::remainder(angle(e->position())-angle(press),2*std::acos(-1.0))*180/std::acos(-1.0);key=angles[axis];}
        moveDrag_["lastField"]=key;moveDrag_["lastValue"]=moveDrag_["initialInputs"].toObject()[key].toDouble()+delta;
        if(dispatch)dispatch(moveDrag_.value("handleCommand").toString("solid.moveHandle"),{{"field",key},{"value",moveDrag_["initialInputs"].toObject()[key].toDouble()+delta}});e->accept();return;
    }

    if(extrusionDragActive_ && e->buttons().testFlag(Qt::LeftButton)) {
        updateExtrusionDrag(e->position(),"move");e->accept();
    } else if(!draggedAnnotation_.isEmpty() && e->buttons().testFlag(Qt::LeftButton)) {
        if(QLineF(annotationPress_,e->position()).length()>3)annotationDragged_=true;
        if(annotationDragged_) {const auto target=e->position()-annotationGrabOffset_;const auto p=sketchPoint(std::clamp(target.x(),0.0,double(width()-1)),std::clamp(target.y(),0.0,double(height()-1)),sketchPlane_);dispatch("sketch.annotationMove",{{"dimensionId",draggedAnnotation_},{"uMm",p.value("uMm")},{"vMm",p.value("vMm")},{"gestureId",annotationGestureId_}});}
        e->accept();
    } else if(!sketchPlane_.isEmpty()) {
        if(decorationSelection_.value("tool")=="select") {
            const auto badge=pickConstraint(e->position().x(),e->position().y());
            const auto id=badge.value("constraintId").toString();
            if(id!=decorationSelection_.value("hoveredConstraintId").toString())
                dispatch("sketch.constraintHover",{{"constraintId",id}});
        }
        dispatch("sketch.pointerMove",{{"x",e->position().x()},{"y",e->position().y()},
            {"tangentModifier",e->modifiers().testFlag(Qt::ShiftModifier)},
            {"suppressInference",e->modifiers().testFlag(Qt::ControlModifier) ||
                e->modifiers().testFlag(Qt::MetaModifier)}});e->accept();
    }
    else {
        hoverSupportFace(pickSupportFace(e->position().x(),e->position().y(),documentBodies_));
        QWidget::mouseMoveEvent(e);
    }
}
void ReferenceView::leaveEvent(QEvent* e) {
    if(cubeHoverActive_){cubeHoverActive_=false;unsetCursor();if(!context_.IsNull())context_->ClearDetected(true);}
    QTimer::singleShot(0,this,[this]{updateCubeAccessoryVisibility();});
    if(jointOriginPicking_ && !jointOriginLocked_){jointOriginHover_={};refreshJointOriginCue();}
    if(!sketchPlane_.isEmpty() && !decorationSelection_.value("hoveredConstraintId").toString().isEmpty())
        dispatch("sketch.constraintHover",{{"constraintId",""}});
    QWidget::leaveEvent(e);
}
void ReferenceView::keyPressEvent(QKeyEvent* e) {
    if(e->key()==Qt::Key_Escape && sketchSelectOtherPending_) {
        sketchSelectOtherTimer_->stop();sketchSelectOtherPending_=false;sketchSelectOtherOpened_=false;e->accept();return;
    }

    if(e->key()==Qt::Key_Escape && !chamferDrag_.isEmpty()){
        chamferDrag_={};dispatch("sketch.chamferHandle",{{"phase","cancel"}});e->accept();return;
    }
    if(e->key()==Qt::Key_Escape && cubePressHandled_) {cancelCubeGesture(true);e->accept();return;}
    if(e->key()==Qt::Key_Escape && inputGestureActive()) {cancelInputGesture();e->accept();return;}
    if(e->key()==Qt::Key_Escape && navigationTool_!="select") {
        if(dispatch)dispatch("view.navigationTool",{{"mode","select"}});e->accept();return;
    }
    if(e->key()==Qt::Key_Escape && (freeOrbitTool_ || orbitButton_!=Qt::NoButton)) {
        orbitButton_=Qt::NoButton;unsetCursor();
        if(freeOrbitTool_ && dispatch)dispatch("view.orbitTool",{{"active",false}});
        e->accept();return;
    }
    if(jointOriginPicking_ && e->modifiers().testFlag(Qt::ControlModifier)
        && !jointOriginLocked_ && !jointOriginHover_.isEmpty()){
        jointOriginLock_=jointOriginHover_;jointOriginLocked_=true;refreshJointOriginCue();e->accept();return;
    }
    QWidget::keyPressEvent(e);
}
void ReferenceView::keyReleaseEvent(QKeyEvent* e) {
    if(jointOriginPicking_ && jointOriginLocked_ &&
        (e->key()==Qt::Key_Control || e->key()==Qt::Key_Meta || !e->modifiers().testFlag(Qt::ControlModifier))){
        jointOriginLock_={};jointOriginLocked_=false;updateJointOriginHover(lastJointPointer_);e->accept();return;
    }
    QWidget::keyReleaseEvent(e);
}
void ReferenceView::mouseReleaseEvent(QMouseEvent* e) {
    if(e->button()==Qt::LeftButton && axis3dSuppressRelease_){axis3dSuppressRelease_=false;e->accept();return;}
    if(e->button()==Qt::LeftButton && pasteSuppressRelease_){pasteSuppressRelease_=false;e->accept();return;}
    if(e->button()==Qt::LeftButton && !patternHandleDrag_.isEmpty()){patternHandleDrag_={};dispatch("sketch.patternHandle",{{"phase","end"}});e->accept();return;}
    if(e->button()==Qt::LeftButton && decorationSelection_["pasteInteraction"].toObject()["active"].toBool()){e->accept();return;}
    if(e->button()==Qt::LeftButton && decorationSelection_["moveInteraction"].toObject()["active"].toBool()){e->accept();return;}
    if(e->button()==Qt::LeftButton && decorationSelection_["patternInteraction"].toObject()["active"].toBool()){e->accept();return;}

    if(e->button()==Qt::LeftButton && sketchSelectOtherPending_) {
        sketchSelectOtherTimer_->stop();sketchSelectOtherPending_=false;
        if(!sketchSelectOtherOpened_)dispatch("sketch.selectionPick",{{"x",e->position().x()},{"y",e->position().y()}});
        e->accept();return;
    }

    if(e->button()==Qt::LeftButton && !chamferDrag_.isEmpty()){
        const auto drag=chamferDrag_;chamferDrag_={};
        dispatch("sketch.chamferHandle",drag.contains("requestedValue") ? QJsonObject{{"phase","end"},{"value",drag.value("requestedValue")}}:QJsonObject{{"phase","cancel"}});
        e->accept();return;
    }
    if(alternateOrbitSuppressRelease_ && e->button()==Qt::LeftButton){alternateOrbitSuppressRelease_=false;e->accept();return;}
    if(cubeSuppressRelease_ && e->button()==Qt::LeftButton){cubeSuppressRelease_=false;e->accept();return;}
    if(cubePressHandled_ && e->button()==Qt::LeftButton){
        const auto region=cubePressedRegion_;
        const bool click=!cubeDragging_;
        cancelCubeGesture(false);
        if(click && !region.isEmpty() && dispatch)dispatch("view.cubeOrient",{{"region",region}});
        e->accept();return;
    }
    if(navigationSuppressRelease_ && e->button()==Qt::LeftButton){navigationSuppressRelease_=false;e->accept();return;}
    if(navigationPressActive_ && e->button()==Qt::LeftButton) {
        const bool window=navigationTool_=="zoomWindow" && navigationMoved_;
        const auto press=navigationPress_;
        cancelNavigationGesture(false);
        if(window && dispatch)dispatch("view.zoomWindow",{{"x1",press.x()},{"y1",press.y()},
            {"x2",e->position().x()},{"y2",e->position().y()}});
        if(navigationTool_!="zoomWindow")setCursor(Qt::OpenHandCursor);
        e->accept();return;
    }
    if(orbitCenterPressHandled_ && e->button()==Qt::LeftButton){orbitCenterPressHandled_=false;e->accept();return;}
    if(e->button()==orbitButton_) {orbitButton_=Qt::NoButton;unsetCursor();e->accept();return;}
    if(e->button()==panButton_) {panButton_=Qt::NoButton;unsetCursor();e->accept();return;}
    if(e->button()==zoomButton_) {zoomButton_=Qt::NoButton;unsetCursor();e->accept();return;}
    if(e->button()==Qt::LeftButton && !sketchLineArcDrag_.isEmpty()){
        const bool moved=sketchLineArcDrag_.value("moved").toBool();sketchLineArcDrag_={};
        dispatch("sketch.lineArcDrag",moved ? QJsonObject{{"phase","end"},{"x",e->position().x()},{"y",e->position().y()}}
            : QJsonObject{{"phase","cancel"}});
        e->accept();return;
    }
    if(e->button()==Qt::LeftButton && jointOriginPressHandled_){jointOriginPressHandled_=false;e->accept();return;}
    if(e->button()==Qt::LeftButton && !assemblyDrag_.isEmpty()){
        const bool moved=assemblyDrag_["moved"].toBool();assemblyDrag_={};
        if(!moved && dispatch)dispatch("selection.pick",{{"x",e->position().x()},{"y",e->position().y()}});
        e->accept();return;
    }
    if(conicDragging_ && e->button()==Qt::LeftButton) {
        conicDragging_=false;
        dispatch("sketch.conicHandleDrag",{{"x",e->position().x()},{"y",e->position().y()}});e->accept();return;
    }

    if(e->button()==Qt::LeftButton && !slotPointDrag_.isEmpty()) {
        const auto drag=slotPointDrag_;slotPointDrag_={};clearSketchCursor();
        if(drag["moved"].toBool()) {
            try {const auto point=sketchPoint(e->position().x(),e->position().y(),sketchPlane_,sketchFrame_);
                dispatch("sketch.slotDrag",{{"phase","end"},{"uMm",point["uMm"]},{"vMm",point["vMm"]}});
            } catch(const std::exception&) {dispatch("sketch.slotDrag",{{"phase","cancel"}});}
        } else {dispatch("sketch.slotDrag",{{"phase","cancel"}});dispatch("sketch.selectionPick",{{"x",e->position().x()},{"y",e->position().y()}});}
        e->accept();return;
    }
    if(e->button()==Qt::LeftButton && !splineDrag_.isEmpty()){const auto drag=splineDrag_;const bool moved=drag.value("moved").toBool();splineDrag_={};if(!moved){dispatch("sketch.splineDrag",{{"phase","cancel"}});if(drag.value("selectResultOnClick").toBool())dispatch("sketch.selectionPick",{{"x",drag.value("pressX")},{"y",drag.value("pressY")}});e->accept();return;}const auto p=sketchPoint(e->position().x(),e->position().y(),sketchPlane_,sketchFrame_);dispatch("sketch.splineDrag",{{"phase","end"},{"uMm",p["uMm"]},{"vMm",p["vMm"]}});e->accept();return;}
    if(e->button()==Qt::LeftButton && mirrorWindowActive_){
        mirrorWindowActive_=false;if(mirrorWindowBand_)mirrorWindowBand_->hide();
        if(QLineF(mirrorWindowPress_,e->position()).length()>=4)
            dispatch("sketch.mirrorWindow",{{"x1",mirrorWindowPress_.x()},{"y1",mirrorWindowPress_.y()},
                {"x2",e->position().x()},{"y2",e->position().y()}});
        else dispatch("sketch.mirrorPick",{{"x",e->position().x()},{"y",e->position().y()}});
        e->accept();return;
    }
    if(e->button()==Qt::LeftButton && !sketchTextDrag_.isEmpty()){
        const auto drag=sketchTextDrag_;sketchTextDrag_={};dispatch("sketch.textHandle",{{"mode",drag["mode"]},{"phase","end"},
            {"startX",drag["startX"]},{"startY",drag["startY"]},{"x",e->position().x()},{"y",e->position().y()}});e->accept();return;
    }
    if(!revolveDrag_.isEmpty() && e->button()==Qt::LeftButton){updateRevolveDrag(e->position(),"end");revolveDrag_={};e->accept();return;}
    if(!moveDrag_.isEmpty() && e->button()==Qt::LeftButton){const auto command=moveDrag_.value("releaseCommand").toString("solid.moveApplyGesture");moveDrag_={};if(dispatch)dispatch(command,{});e->accept();return;}

    if(e->button()==Qt::LeftButton && extrusionDragActive_) {
        updateExtrusionDrag(e->position(),"end");extrusionDragActive_=false;e->accept();return;
    }
    if(e->button()==Qt::LeftButton && !draggedAnnotation_.isEmpty()){
        if(annotationDragged_)dispatch("sketch.annotationEndGesture",{{"dimensionId",draggedAnnotation_},{"gestureId",annotationGestureId_}});
        draggedAnnotation_.clear();annotationGestureId_.clear();annotationDragged_=false;annotationAutoDrag_=false;sync();e->accept();return;
    }
    QWidget::mouseReleaseEvent(e);
}
void ReferenceView::mouseDoubleClickEvent(QMouseEvent* e) {
    if(inputPreset_=="shiftAlt" && e->button()==Qt::LeftButton &&
       e->modifiers()==(Qt::ShiftModifier|Qt::AltModifier)){
        cancelInputGesture();alternateOrbitSuppressRelease_=true;e->accept();return;
    }
    if(e->button()==Qt::LeftButton &&
        (cubePressHandled_ || (e->position().x()>=width()-180 && e->position().y()<=180 &&
            !cubeHit(e->position().x(),e->position().y()).isEmpty()))) {
        cubePressHandled_=true;cubeDragging_=true;cubePressedRegion_.clear();
        cubePress_=cubeLast_=e->position();e->accept();return;
    }
    if(navigationTool_!="select" && e->button()==Qt::LeftButton) {
        cancelNavigationGesture(true);
        navigationSuppressRelease_=true; // Qt delivers one more release after double-click.
        e->accept();return;
    }
    if((freeOrbitTool_ || orbitCenterPicking_) && e->button()==Qt::LeftButton){e->accept();return;}
    if(e->button()==Qt::LeftButton && !sketchPlane_.isEmpty()) {
        for(const auto& row:patternGlyphs_){const auto glyph=row.toObject(),b=glyph["bounds"].toObject();const QRectF bounds(b["x"].toDouble(),b["y"].toDouble(),b["width"].toDouble(),b["height"].toDouble());if(bounds.contains(e->position())){dispatch("sketch.patternEdit",{{"groupId",glyph["groupId"]}});e->accept();return;}}
        const auto picked=pickAnnotation(e->position().x(),e->position().y());
        if(!picked.isEmpty()){draggedAnnotation_.clear();annotationAutoDrag_=false;dispatch("sketch.dimensionEdit",{{"dimensionId",picked.value("dimensionId")}});e->accept();return;}
    }
    QWidget::mouseDoubleClickEvent(e);
}
void ReferenceView::recordContextMenu(const QJsonObject& state,bool shown) {
    lastContextMenu_=state;if(shown)++contextMenuShownCount_;
}
void ReferenceView::contextMenuEvent(QContextMenuEvent* e) {
    // Some platforms deliver the native context request after the right press.
    // Do not open a second popup or dispatch a command twice.
    if(auto* popup=QApplication::activePopupWidget();popup && popup->parentWidget()==this){e->accept();return;}
    if(alternateRightContextSuppressed_){alternateRightContextSuppressed_=false;e->accept();return;}
    ++contextMenuEventCount_;
    const bool sketchActive=!sketchPlane_.isEmpty();
    const bool selectTool=decorationSelection_.value("tool")=="select";
    const int selectedEntities=decorationSelection_["entityIds"].toArray().size();
    lastContextMenu_=QJsonObject{{"reason",static_cast<int>(e->reason())},{"sketchActive",sketchActive},
        {"selectTool",selectTool},{"selectedEntities",selectedEntities},
        {"x",e->pos().x()},{"y",e->pos().y()}};
    if(sketchActive && selectTool) {
        const auto picked=pickAnnotation(e->pos().x(),e->pos().y());
        const auto id=picked.value("dimensionId").toString();
        for(const auto& entry:decorationFeature_.value("parameters").toObject().value("dimensions").toArray()) {
            const auto dimension=entry.toObject();if(id.isEmpty() || dimension.value("id")!=id)continue;
            dispatch("sketch.annotationSelect",{{"dimensionId",id}});
            auto* menu=new QMenu(this);menu->setObjectName("context.sketchDimension");menu->setAttribute(Qt::WA_DeleteOnClose);
            const bool driving=dimension.value("driving").toBool();
            auto* toggle=menu->addAction(driving ? "Toggle Driven" : "Toggle Driving");toggle->setObjectName("sketch.dimension.toggleDriving");
            toggle->setToolTip("Driven conversion removes the expression or user parameter binding; converting back uses the current measurement.");
            connect(toggle,&QAction::triggered,this,[this,id,driving]{dispatch("sketch.dimensionDriving",{{"dimensionId",id},{"driving",!driving}});});
            lastContextMenu_["dimensionId"]=id;lastContextMenu_["phase"]="open";++contextMenuShownCount_;
            connect(menu,&QMenu::aboutToHide,this,[this]{lastContextMenu_["phase"]="closed";});
            menu->popup(e->globalPos());e->accept();return;
        }
    }
    if(sketchActive && selectTool && selectedEntities==1) {
        const QString selected=decorationSelection_.value("entityIds").toArray().at(0).toString();
        for(const auto& value:decorationFeature_["parameters"].toObject()["entities"].toArray()) {
            const auto entity=value.toObject();if(entity["id"]!=selected || entity["type"]!="conic")continue;
            QMenu menu(this);auto* edit=menu.addAction("Edit Conic Rho");
            if(menu.exec(e->globalPos())==edit)dispatch("sketch.conicEdit",{{"entityId",selected}});
            e->accept();return;
        }
    }
    if(sketchActive && decorationSelection_.value("tool")=="conic") {
        QMenu menu(this);auto* accept=menu.addAction("OK");accept->setObjectName("sketch.conic.ok");
        auto* cancel=menu.addAction("Cancel");const auto* chosen=menu.exec(e->globalPos());
        if(chosen==accept)dispatch("sketch.conicAccept",{});
        else if(chosen==cancel)dispatch("sketch.conicCancel",{});
        e->accept();return;
    }
    // A retained Mirror result is edited through its source. Let its context
    // request reach the shared geometry menu (including whole-owner Delete).
    QSet<QString> mirroredEntities;
    if(sketchActive && selectTool)
        for(const auto& row:decorationFeature_.value("parameters").toObject().value("mirrorGroups").toArray())
            for(const auto& pair:row.toObject().value("entityPairs").toArray())
                mirroredEntities.insert(pair.toObject().value("resultEntityId").toString());
    if(sketchActive && selectTool && decorationSelection_["entityIds"].toArray().size()==1){const auto id=decorationSelection_.value("entityIds").toArray().at(0);for(const auto& value:decorationFeature_["parameters"].toObject()["entities"].toArray())if(value.toObject()["id"]==id&&value.toObject()["type"]=="fitSpline"&&!mirroredEntities.contains(id.toString())){QMenu menu(this);auto* insert=menu.addAction("Insert Spline Fit Point");insert->setObjectName("sketch.spline.insertFitPoint");if(menu.exec(e->globalPos())==insert)dispatch("sketch.splineInsertMode",{{"entityId",id}});e->accept();return;}}
    if(sketchActive && selectTool) {
        const auto badge=pickConstraint(e->pos().x(),e->pos().y());
        const auto selected=decorationSelection_.value("constraintIds").toArray();
        if(!badge.isEmpty() || !selected.isEmpty()) {
            ++contextMenuShownCount_;
            QMenu menu(this);auto* remove=menu.addAction("Delete Constraint");
            lastContextMenu_["constraintId"]=badge.value("constraintId");lastContextMenu_["phase"]="open";
            const auto action=menu.exec(e->globalPos());
            lastContextMenu_["phase"]="closed";lastContextMenu_["result"]=action==remove ? "selected" : "cancelled";
            if(action==remove) {
                const auto clickedId=badge.value("constraintId");
                const auto ids=badge.isEmpty() || selected.contains(clickedId) ? selected : QJsonArray{clickedId};
                dispatch("sketch.deleteSelection",{{"constraintIds",ids}});
            }
            e->accept();return;
        }
    }
    if(sketchActive && selectTool) {
        const auto hit=pickSketch(decorationFeature_,e->pos().x(),e->pos().y());
        if(hit["kind"]=="point" || hit["kind"]=="entity") {
            const auto field=hit["kind"]=="point" ? QString("pointIds") : QString("entityIds");
            const auto id=hit[hit["kind"]=="point" ? "pointId" : "entityId"];
            bool selectedCandidate=false;
            if(field=="entityIds")for(const auto& row:sketchSelectionCandidates(decorationFeature_,e->pos().x(),e->pos().y()))
                selectedCandidate |= decorationSelection_.value(field).toArray().contains(row.toObject().value("entityId"));
            if(!selectedCandidate && !decorationSelection_.value(field).toArray().contains(id))
                dispatch("sketch.selectionSet",{{field,QJsonArray{id}}});
        }
        const auto entities=decorationSelection_.value("entityIds").toArray();
        const auto points=decorationSelection_.value("pointIds").toArray();
        {
            auto* menu=new CanvasMarkingMenu(this,[this](const QString& command,const QJsonObject& args){dispatch(command,args);},!entities.isEmpty() || !points.isEmpty());
            menu->setObjectName("context.sketchGeometry");
            const bool hasSelection=!entities.isEmpty() || !points.isEmpty();
            auto* copyAction=menu->addAction("Copy");copyAction->setObjectName("sketch.action.copy");copyAction->setEnabled(!entities.isEmpty() && points.isEmpty());
            auto* pasteAction=menu->addAction("Paste");pasteAction->setObjectName("sketch.action.paste");
            connect(copyAction,&QAction::triggered,this,[this,entities]{QTimer::singleShot(0,this,[this,entities]{dispatch("sketch.copy",{{"entityIds",entities}});});});
            connect(pasteAction,&QAction::triggered,this,[this,at=e->pos()]{QTimer::singleShot(0,this,[this,at]{dispatch("sketch.paste",{{"x",at.x()},{"y",at.y()}});});});
            auto* remove=menu->addAction("Delete");remove->setObjectName("sketch.action.deleteGeometry");remove->setVisible(hasSelection);
            QAction* breakLink=nullptr;
            if(entities.size()==1&&points.isEmpty())for(const auto& value:decorationFeature_.value("parameters").toObject().value("entities").toArray()){
                const auto entity=value.toObject();if(entity.value("id")==entities[0]&&entity.value("type")=="fitSplineOffset"){
                    breakLink=menu->addAction("Break Link");breakLink->setObjectName("sketch.action.breakOffsetLink");
                    breakLink->setToolTip("Keep both curves; convert the result to editable control points (0.001 mm tolerance).");
                }
            }
            QMap<QAction*,QString> coincidentActions;
            for(const auto& value:decorationFeature_.value("parameters").toObject().value("constraints").toArray()) {
                const auto relation=value.toObject();const auto kind=relation.value("type").toString();
                if(kind!="coincident" && kind!="pointOnCurve")continue;
                bool attached=false;for(const auto& point:points)attached |= relation.value("pointIds").toArray().contains(point);
                if(!attached)continue;
                auto* action=menu->addAction("Delete Coincident");
                action->setObjectName("sketch.action.deleteCoincident");action->setData(relation.value("id").toString());
                // Multiple relations remain individually addressable by stable ID.
                action->setToolTip(relation.value("id").toString());
                coincidentActions.insert(action,relation.value("id").toString());
            }
            menu->addSeparator();
            auto* dimension=menu->addAction("Sketch Dimension\tD");dimension->setObjectName("sketch.tool.dimension");
            QAction* toggle=nullptr;if(!entities.isEmpty()) {
                toggle=menu->addAction("Normal/Construction\tX");toggle->setObjectName("sketch.action.constructionToggle");
            }
            menu->addSeparator();
            QMap<QAction*,QString> constraintActions;
            for(const auto& type:QStringList{"collinear","midpoint","fixed","parallel","perpendicular","horizontalVertical","equal"}) {
                const auto text=type=="fixed" ? QString("Fix/UnFix") : type=="horizontalVertical" ? QString("Horizontal/Vertical") : type=="midpoint" ? QString("MidPoint") : type.first(1).toUpper()+type.sliced(1);
                auto* action=menu->addAction(text);action->setObjectName("sketch.action.constraint."+type);
                action->setVisible(hasSelection);constraintActions.insert(action,type);
            }
            menu->addSeparator();
            auto* pan=menu->addAction("Pan");pan->setObjectName("context.navigation.pan");
            connect(pan,&QAction::triggered,this,[this]{dispatch("view.navigationTool",{{"mode","pan"}});});
            auto* zoom=menu->addAction("Zoom");zoom->setObjectName("context.navigation.zoom");
            connect(zoom,&QAction::triggered,this,[this]{dispatch("view.navigationTool",{{"mode","zoom"}});});
            lastContextMenu_["phase"]="open";lastContextMenu_["origin"]="sketch";
            // Expose actual visual order and shortcut columns through diagnostics.
            QJsonArray actions;
            for(auto* action:menu->actions())if(!action->isSeparator()) {
                QJsonObject row{{"id",action->objectName()},{"text",action->text()},{"enabled",action->isEnabled()}};
                if(coincidentActions.contains(action))row["constraintId"]=coincidentActions.value(action);
                actions.append(row);
            }
            lastContextMenu_["actions"]=actions;
            ++contextMenuShownCount_;
            connect(menu,&QMenu::triggered,this,[this,remove,toggle,dimension,breakLink,entities,points,coincidentActions,constraintActions](QAction* action){
                lastContextMenu_["result"]=action->objectName();
                if(action==remove)QTimer::singleShot(0,this,[this,entities,points]{dispatch("sketch.deleteSelection",{{"entityIds",entities},{"pointIds",points}});});
                else if(breakLink && action==breakLink)QTimer::singleShot(0,this,[this,entities]{dispatch("sketch.splineOffsetBreakLink",{{"offsetEntityId",entities[0]}});});
                else if(coincidentActions.contains(action)) {
                    const auto id=coincidentActions.value(action);lastContextMenu_["constraintId"]=id;
                    QTimer::singleShot(0,this,[this,id]{dispatch("sketch.deleteCoincident",{{"constraintId",id}});});
                }
                else if(action==dimension)QTimer::singleShot(0,this,[this]{dispatch("sketch.tool",{{"tool","dimension"}});});
                else if(constraintActions.contains(action)) {
                    const auto type=constraintActions.value(action);
                    QTimer::singleShot(0,this,[this,type]{dispatch("sketch.constraintTool",{{"type",type}});});
                }
                else if(action==toggle)QTimer::singleShot(0,this,[this]{dispatch("sketch.constructionToggle",{});});
            });
            connect(menu,&QMenu::aboutToHide,this,[this,menu]{lastContextMenu_["phase"]="closed";
                if(menu->property("radialResult").isValid())lastContextMenu_["result"]=menu->property("radialResult").toString();
                if(!lastContextMenu_.contains("result"))lastContextMenu_["result"]="cancelled";});
            menu->openAt(e->globalPos());e->accept();return;
        }
    }
    const auto menuProfile=sketchActive ? QJsonObject{} : pickFinishedProfile(e->pos().x(),e->pos().y());
    if(!menuProfile.isEmpty()) {
        QMenu menu(this);menu.setObjectName("context.viewportProfile");
        auto* offset=menu.addAction("Offset Plane");offset->setObjectName("context.offsetPlane");
        lastContextMenu_["origin"]="viewport";lastContextMenu_["sketchId"]=menuProfile["sketchId"];
        lastContextMenu_["actions"]=QJsonArray{QJsonObject{{"id",offset->objectName()},{"text",offset->text()},{"enabled",true}}};
        lastContextMenu_["phase"]="open";++contextMenuShownCount_;
        const auto action=menu.exec(e->globalPos());lastContextMenu_["phase"]="closed";
        lastContextMenu_["result"]=action==offset ? offset->objectName() : QString("cancelled");
        if(action==offset){dispatch("selection.sketchSet",{{"sketchId",menuProfile["sketchId"]}});dispatch("construction.plane",{});}
        e->accept();return;
    }
    const auto menuFace=sketchActive ? QJsonObject{} : pickSupportFace(e->pos().x(),e->pos().y(),documentBodies_);
    const auto menuBody=sketchActive ? QString{} : pickBody(e->pos().x(),e->pos().y());
    if(!sketchActive && (!menuBody.isEmpty() || !menuFace.isEmpty() || !selectedSupportFace_.isEmpty())) {
        auto* menu=new QMenu(this);menu->setObjectName("context.viewportBody");menu->setAttribute(Qt::WA_DeleteOnClose);
        QAction* create=nullptr;if(!menuFace.isEmpty() || !selectedSupportFace_.isEmpty()){create=menu->addAction("Create Sketch");create->setObjectName("context.createSketch");}
        QAction* repeat=nullptr;if(!menuFace.isEmpty()){repeat=menu->addAction("Repeat Sphere");repeat->setObjectName("context.repeatSphere");}
        QAction* move=nullptr;if(!menuBody.isEmpty()){move=menu->addAction("Move/Copy");move->setObjectName("context.moveBody");}
        QJsonArray actions;for(const auto* action:menu->actions())actions.append(QJsonObject{{"id",action->objectName()},{"text",action->text()},{"enabled",action->isEnabled()}});
        lastContextMenu_["origin"]="viewport";lastContextMenu_["bodyId"]=menuBody;lastContextMenu_["support"]=menuFace;lastContextMenu_["actions"]=actions;lastContextMenu_["phase"]="open";++contextMenuShownCount_;
        connect(menu,&QMenu::triggered,this,[this,create,repeat,move,menuFace,menuBody](QAction* action){lastContextMenu_["result"]=action->objectName();
            if(action==create)dispatch("sketch.create",{{"support",menuFace.isEmpty() ? selectedSupportFace_ : menuFace}});
            else if(action==repeat)dispatch("solid.repeatSphere",{{"support",menuFace}});
            else if(action==move)dispatch("solid.moveDialog",{{"targetBodyId",menuBody}});
        });
        connect(menu,&QMenu::aboutToHide,this,[this]{lastContextMenu_["phase"]="closed";if(!lastContextMenu_.contains("result"))lastContextMenu_["result"]="cancelled";});
        menu->popup(e->globalPos());e->accept();return;
    }
    if(!sketchActive) {
        auto* menu=new QMenu(this);menu->setObjectName("context.orbitCenter");menu->setAttribute(Qt::WA_DeleteOnClose);
        auto* set=menu->addAction("Set Orbit Center");set->setObjectName("context.orbitCenter.set");
        set->setEnabled(bodyStageKey_.isEmpty() && std::any_of(documentBodies_.begin(),documentBodies_.end(),[](const auto& body){return body.visible && !body.shape.IsNull();}));
        auto* reset=menu->addAction("Reset Orbit Center");reset->setObjectName("context.orbitCenter.reset");
        reset->setEnabled(customOrbitCenter_.has_value());
        lastContextMenu_["origin"]="blankCanvas";
        lastContextMenu_["actions"]=QJsonArray{QJsonObject{{"id",set->objectName()},{"text",set->text()},{"enabled",set->isEnabled()}},
            QJsonObject{{"id",reset->objectName()},{"text",reset->text()},{"enabled",reset->isEnabled()}}};
        lastContextMenu_["phase"]="open";++contextMenuShownCount_;
        connect(menu,&QMenu::triggered,this,[this,set,reset](QAction* action){
            lastContextMenu_["result"]=action->objectName();
            if(action==set)dispatch("view.orbitCenterBegin",{});
            else if(action==reset)dispatch("view.orbitCenterReset",{});
        });
        connect(menu,&QMenu::aboutToHide,this,[this]{lastContextMenu_["phase"]="closed";
            if(!lastContextMenu_.contains("result"))lastContextMenu_["result"]="cancelled";});
        menu->popup(e->globalPos());e->accept();return;
    }
    QWidget::contextMenuEvent(e);
}
void ReferenceView::wheelEvent(QWheelEvent* e) {
    const auto pixel=e->pixelDelta(),angle=e->angleDelta();
    const bool touchpad=e->device() && e->device()->type()==QInputDevice::DeviceType::TouchPad;
    QString route="none";
    if(nativeGestureActive_ && touchpad) {
        // Native gesture Begin/End owns this physical stream while it is
        // active. Do not also pan/zoom from an interleaved wheel event.
        route="nativeGestureOwnsStream";
    } else if(dispatch && touchpad && !pixel.isNull()) {
        // A Mac two-finger swipe is normally a pixel wheel. The angle channel
        // on the same event is deliberately ignored, so one event pans or
        // orbits once and cannot also zoom.
        if(e->modifiers().testFlag(Qt::ShiftModifier)) {
            route="touchpadOrbit";
            dispatch("view.orbit",{{"deltaX",pixel.x()},{"deltaY",pixel.y()}});
        } else {
            route="touchpadPan";
            dispatch("view.pan",{{"x",pixel.x()},{"y",pixel.y()}});
        }
    } else if(dispatch && (angle.y()!=0 || (!touchpad && pixel.y()!=0))) {
        route="wheelZoom";
        const auto position=e->position();
        // Unidentified pixel-only wheels remain usable as zoom. When an
        // angle channel exists it wins and the pixel channel is not replayed.
        const double motion=angle.y()!=0 ? angle.y() : pixel.y();
        dispatch("view.zoom",{{"factor",std::clamp(std::pow(1.0015,motion),0.1,10.0)},
            {"x",std::clamp(position.x(),0.0,double(width()-1))},
            {"y",std::clamp(position.y(),0.0,double(height()-1))}});
    } else if(dispatch && !touchpad && pixel.x()!=0) {
        route="horizontalPixelPan";
        dispatch("view.pan",{{"x",pixel.x()},{"y",0}});
    }
    lastNavigationInput_={{"event","wheel"},{"route",route},{"deviceType",touchpad ? "touchpad" : "other"},
        {"pixelDelta",QJsonArray{pixel.x(),pixel.y()}},{"angleDelta",QJsonArray{angle.x(),angle.y()}},
        {"phase",static_cast<int>(e->phase())},{"modifiers",static_cast<int>(e->modifiers())},
        {"inverted",e->inverted()}};
    e->accept();
}
bool ReferenceView::event(QEvent* e) {
    if(e->type()==QEvent::WindowDeactivate || e->type()==QEvent::Hide || e->type()==QEvent::FocusOut) {
        nativeGestureActive_=false;
        cancelInputGesture();
        cancelCubeGesture(cubePressHandled_);
        cancelNavigationGesture(navigationPressActive_);
    }
    if(e->type()!=QEvent::NativeGesture)return QWidget::event(e);
    auto* gesture=static_cast<QNativeGestureEvent*>(e);
    QString route="none";
    if(gesture->gestureType()==Qt::BeginNativeGesture) {
        nativeGestureActive_=true;route="begin";
    } else if(gesture->gestureType()==Qt::EndNativeGesture) {
        nativeGestureActive_=false;route="end";
    } else if(dispatch && gesture->gestureType()==Qt::ZoomNativeGesture && std::isfinite(gesture->value())) {
        const double factor=1+gesture->value();
        const auto position=gesture->position();
        if(factor>0 && std::isfinite(position.x()) && std::isfinite(position.y())) {
            route="pinchZoom";
            dispatch("view.zoom",{{"factor",std::clamp(factor,0.1,10.0)},
                {"x",std::clamp(position.x(),0.0,double(width()-1))},
                {"y",std::clamp(position.y(),0.0,double(height()-1))}});
        } else route="invalidPinch";
    } else if(dispatch && gesture->gestureType()==Qt::PanNativeGesture) {
        const auto delta=gesture->delta();
        if(std::isfinite(delta.x()) && std::isfinite(delta.y()) && (delta.x()!=0 || delta.y()!=0)) {
            const bool orbit=gesture->modifiers().testFlag(Qt::ShiftModifier);
            route=orbit ? "nativeOrbit" : "nativePan";
            dispatch(orbit ? "view.orbit" : "view.pan",{{orbit ? "deltaX" : "x",std::clamp(delta.x(),-10000.0,10000.0)},
                {orbit ? "deltaY" : "y",std::clamp(delta.y(),-10000.0,10000.0)}});
        }
    }
    lastNavigationInput_={{"event","nativeGesture"},{"route",route},
        {"gestureType",static_cast<int>(gesture->gestureType())},{"value",gesture->value()},
        {"delta",QJsonArray{gesture->delta().x(),gesture->delta().y()}},
        {"modifiers",static_cast<int>(gesture->modifiers())}};
    e->accept();return true;
}

void ReferenceView::setMeshExportPreview(const std::vector<gp_Pnt>& facets,const QString& instanceKey) {
    if(context_.IsNull())return;
    if(!meshExportPreview_.IsNull()){context_->Remove(meshExportPreview_,false);meshExportPreview_.Nullify();}
    meshExportPreviewInstance_=instanceKey;
    if(!facets.empty()) {
        meshExportPreview_=new ExportFacetPresentation(facets);
        context_->Display(meshExportPreview_,0,-1,false);
        context_->Deactivate(meshExportPreview_);
    }
    applyEditDisplay(false);
    context_->UpdateCurrentViewer();
}

QJsonObject ReferenceView::pickSketchAxis3d(double x,double y) const {for(const auto& row:sketchAxis3dMarkers_){const auto marker=row.toObject();if(std::hypot(x-marker["x"].toDouble(),y-marker["y"].toDouble())<=marker["hitRadius"].toDouble())return marker;}return {};}
