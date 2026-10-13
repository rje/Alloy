// SPDX-License-Identifier: MIT
#include "ReferenceView.h"
#include <AIS_Shape.hxx>
#include <TopoDS_Wire.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <Graphic3d_ZLayerId.hxx>
#include <Prs3d_LineAspect.hxx>
#include <numbers>
#include <cmath>
namespace {
gp_Pnt point(const QJsonArray& p){return {p[0].toDouble(),p[1].toDouble(),p[2].toDouble()};}
gp_Vec vector(const QJsonArray& p){return {p[0].toDouble(),p[1].toDouble(),p[2].toDouble()};}
QJsonArray json(const auto& p){return {p.X(),p.Y(),p.Z()};}
}
void ReferenceView::clearRevolveCue(bool resetGesture){if(resetGesture)revolveDrag_={};if(!context_.IsNull())for(const auto& s:revolveCue_)context_->Remove(s,false);revolveCue_.clear();revolveCueState_={};}
void ReferenceView::showRevolveCue(const QJsonObject& geometry,const EvaluatedProfile& profile){
    clearRevolveCue();if(context_.IsNull())return;
    const auto axis=vector(geometry["axisDirection"].toArray());const auto start=point(geometry["axisStartMm"].toArray());
    GProp_GProps props;BRepGProp::SurfaceProperties(profile.face,props);const auto mass=props.CentreOfMass();const auto center=start.Translated(axis*gp_Vec(start,mass).Dot(axis));
    const gp_Vec radial(center,mass);if(radial.Magnitude()<1e-6)return;const double radius=radial.Magnitude()*1.12;const auto u=radial.Normalized()*radius,v=axis.Crossed(u);const double angle=geometry["angleDeg"].toDouble()*std::numbers::pi/180.;
    BRepBuilderAPI_MakePolygon ring;for(int i=0;i<=96;++i){const double t=2*std::numbers::pi*i/96;ring.Add(center.Translated(u*std::cos(t)+v*std::sin(t)));}
    auto display=[&](const TopoDS_Shape& shape,double width){Handle(AIS_Shape) cue=new AIS_Shape(shape);cue->SetColor(Quantity_Color(.92,.38,.06,Quantity_TOC_sRGB));cue->SetWidth(width*devicePixelRatioF());cue->SetZLayer(Graphic3d_ZLayerId_Topmost);context_->Display(cue,AIS_WireFrame,-1,false);context_->Deactivate(cue);revolveCue_.push_back(cue);};
    display(ring.Wire(),1.6);const auto end=center.Translated(u*std::cos(angle)+v*std::sin(angle));display(BRepBuilderAPI_MakeEdge(center,end).Edge(),2.3);
    const auto radialUnit=(u*std::cos(angle)+v*std::sin(angle)).Normalized(),tangent=axis.Crossed(radialUnit);const double size=radius*.08;display(BRepBuilderAPI_MakeEdge(end.Translated(radialUnit*size),end.Translated(radialUnit*(-size))).Edge(),4);display(BRepBuilderAPI_MakeEdge(end.Translated(tangent*size),end.Translated(tangent*(-size))).Edge(),4);
    revolveCueState_={{"centerMm",json(center)},{"basisUMm",json(u)},{"basisVMm",json(v)},{"axisDirection",json(axis)},{"radiusMm",radius},{"angleDeg",geometry["angleDeg"]},{"extent",geometry["extent"]},{"sketchId",profile.sketchId},{"profileId",profile.profileId}};refreshRevolveCue();context_->UpdateCurrentViewer();
}
void ReferenceView::refreshRevolveCue(){if(view_.IsNull() || revolveCueState_.isEmpty())return;const auto center=point(revolveCueState_["centerMm"].toArray());const auto u=vector(revolveCueState_["basisUMm"].toArray()),v=vector(revolveCueState_["basisVMm"].toArray());auto project=[&](const gp_Pnt& p){int x,y;view_->Convert(p.X(),p.Y(),p.Z(),x,y);const auto scale=windowScale();return QJsonObject{{"x",x/scale.x()},{"y",y/scale.y()}};};revolveCueState_["centerScreen"]=project(center);revolveCueState_["basisUScreen"]=project(center.Translated(u));revolveCueState_["basisVScreen"]=project(center.Translated(v));const double angle=revolveCueState_["angleDeg"].toDouble()*std::numbers::pi/180.;revolveCueState_["endScreen"]=project(center.Translated(u*std::cos(angle)+v*std::sin(angle)));QJsonArray ring;for(int i=0;i<=96;++i){const double t=2*std::numbers::pi*i/96;ring.append(project(center.Translated(u*std::cos(t)+v*std::sin(t))));}revolveCueState_["ringScreen"]=ring;auto delta=[&](const char* key){auto p=revolveCueState_[key].toObject(),c=revolveCueState_["centerScreen"].toObject();return QPointF(p["x"].toDouble()-c["x"].toDouble(),p["y"].toDouble()-c["y"].toDouble());};const auto a=delta("basisUScreen"),b=delta("basisVScreen");revolveCueState_["dragAvailable"]=std::abs(a.x()*b.y()-a.y()*b.x())>4;revolveCueState_["dragActive"]=!revolveDrag_.isEmpty();}
QJsonObject ReferenceView::revolvePointerAngle(double x,double y,const QString& phase){
    revolvePointerState_["phase"]=phase;
    if(phase=="begin"){
        auto screen=[](const QJsonObject& p){return QPointF(p["x"].toDouble(),p["y"].toDouble());};const QPointF pointer(x,y);
        bool hit=!revolveCueState_.isEmpty() && QLineF(pointer,screen(revolveCueState_["endScreen"].toObject())).length()<11;
        const auto ring=revolveCueState_["ringScreen"].toArray();for(int i=1;i<ring.size() && !hit;++i){const auto a=screen(ring[i-1].toObject()),b=screen(ring[i].toObject()),d=b-a;const auto length=QPointF::dotProduct(d,d);const auto t=length>1e-8 ? std::clamp(QPointF::dotProduct(pointer-a,d)/length,0.0,1.0):0.;hit=QLineF(pointer,a+d*t).length()<7;}
        if(!hit){revolvePointerState_["error"]="pointer did not hit angular control";return {};}revolveDrag_=revolveCueState_;
    }
    if(revolveDrag_.isEmpty() || !revolveDrag_["dragAvailable"].toBool()){revolvePointerState_["error"]="control unavailable";return {};}
    auto p=[](const QJsonObject& q){return QPointF(q["x"].toDouble(),q["y"].toDouble());};
    const auto center=p(revolveDrag_["centerScreen"].toObject()),u=p(revolveDrag_["basisUScreen"].toObject())-center,v=p(revolveDrag_["basisVScreen"].toObject())-center,q=QPointF(x,y)-center;
    const double determinant=u.x()*v.y()-u.y()*v.x();if(std::abs(determinant)<1e-8)return {};
    double angle=std::atan2((u.x()*q.y()-u.y()*q.x())/determinant,(q.x()*v.y()-q.y()*v.x())/determinant)*180/std::numbers::pi;if(angle<0)angle+=360;angle=std::clamp(angle,.001,359.999);
    revolvePointerState_["derivedAngleDeg"]=angle;
    if(phase=="end")revolveDrag_={};return {{"angleDeg",angle},{"x",x},{"y",y},{"phase",phase},{"mapping","projected axis-normal ring"}};
}
void ReferenceView::updateRevolveDrag(const QPointF& position,const QString& phase){if(dispatch)dispatch("solid.revolveDrag",{{"x",position.x()},{"y",position.y()},{"phase",phase}});}
