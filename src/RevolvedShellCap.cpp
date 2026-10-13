// SPDX-License-Identifier: MIT
#include "RevolvedShellCap.h"
#include "PlanarFaces.h"
#include "SolidFillet.h"
#include <BRepAdaptor_Surface.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRep_Tool.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepAlgoAPI_Check.hxx>
#include <BRepAlgoAPI_BooleanOperation.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepPrimAPI_MakeRevol.hxx>
#include <BRepClass3d_SolidClassifier.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <Geom_SurfaceOfRevolution.hxx>
#include <Geom_RectangularTrimmedSurface.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <Geom_Circle.hxx>
#include <Geom_Line.hxx>
#include <GeomAPI.hxx>
#include <Geom2dAPI_InterCurveCurve.hxx>
#include <Geom2dInt_GInter.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <numbers>
#include <cmath>

namespace {
Handle(Geom_Surface) basisSurface(Handle(Geom_Surface) surface) {
    while(!Handle(Geom_RectangularTrimmedSurface)::DownCast(surface).IsNull())
        surface=Handle(Geom_RectangularTrimmedSurface)::DownCast(surface)->BasisSurface();
    return surface;
}
Handle(Geom_Curve) basisCurve(Handle(Geom_Curve) curve) {
    while(!Handle(Geom_TrimmedCurve)::DownCast(curve).IsNull())
        curve=Handle(Geom_TrimmedCurve)::DownCast(curve)->BasisCurve();
    return curve;
}
void require(bool condition,const QString& message,const QString& featureId) {
    if(!condition)throw GeometryKernel::EvaluationError({{"code","shell_closed_cap_failed"},
        {"featureId",featureId},{"message",message}});
}
bool validArgument(const TopoDS_Shape& shape) {
    return BRepCheck_Analyzer(shape).IsValid() && BRepAlgoAPI_Check(shape,true,true).IsValid();
}
}
std::optional<EvaluatedBody> RevolvedShellCap::evaluate(const EvaluatedBody& source,
    const EvaluatedSupportFace& selected,double thickness,const QString& featureId) {
    // Provenance and exact geometry both constrain admission. A circle-looking
    // spline, modified body, partial sweep or extra face is not this policy.
    if(selected.role!="revolve.radial" || selected.producerFeatureId!=source.featureId
        || source.summary["operation"]!="revolve" || source.summary["extent"]!="full"
        || source.summary.contains("lastModifierFeatureId") || source.summary.contains("circularPattern")
        || source.supportFaces.size()!=1)return std::nullopt;
    TopTools_IndexedMapOfShape faces;TopExp::MapShapes(source.shape,TopAbs_FACE,faces);
    if(faces.Extent()!=2 || !faces.Contains(selected.face))return std::nullopt;
    const auto other=TopoDS::Face(faces(faces.FindIndex(selected.face)==1?2:1));
    const auto meridianSurface=Handle(Geom_SurfaceOfRevolution)::DownCast(basisSurface(BRep_Tool::Surface(other)));
    if(meridianSurface.IsNull())return std::nullopt;
    const auto circle=Handle(Geom_Circle)::DownCast(basisCurve(meridianSurface->BasisCurve()));
    if(circle.IsNull())return std::nullopt;
    const BRepAdaptor_Surface side(other),base(selected.face);
    const auto axis=meridianSurface->Axis();
    if(base.GetType()!=GeomAbs_Plane || !base.Plane().Axis().Direction().IsParallel(axis.Direction(),1e-9)
        || std::abs(side.LastUParameter()-side.FirstUParameter()-2*std::numbers::pi)>1e-8
        || std::abs(circle->Position().Direction().Dot(axis.Direction()))>1e-9)return std::nullopt;
    // The selected disk must be the actual unique circular radial boundary.
    TopTools_IndexedMapOfShape baseWires,baseEdges;TopExp::MapShapes(selected.face,TopAbs_WIRE,baseWires);TopExp::MapShapes(selected.face,TopAbs_EDGE,baseEdges);
    if(baseWires.Extent()!=1 || baseEdges.Extent()!=1)return std::nullopt;
    const BRepAdaptor_Curve rim(TopoDS::Edge(baseEdges(1)));
    if(rim.GetType()!=GeomAbs_Circle || gp_Lin(axis).Distance(rim.Circle().Location())>1e-7)return std::nullopt;
    const auto middle=side.Value(side.FirstUParameter(),(side.FirstVParameter()+side.LastVParameter())/2);
    const gp_Vec axial(axis.Direction());auto radial=gp_Vec(axis.Location(),middle);radial-=axial*radial.Dot(axial);
    if(radial.Magnitude()<1e-7)return std::nullopt;radial.Normalize();
    const double inwardRadius=circle->Radius()-thickness;
    if(inwardRadius<=gp_Lin(axis).Distance(circle->Location())+1e-7)return std::nullopt;
    Handle(Geom_Circle) inner=new Geom_Circle(circle->Circ());inner->SetRadius(inwardRadius);
    const gp_Pln meridianPlane(circle->Position());Handle(Geom_Line) axisCurve=new Geom_Line(axis);
    Geom2dAPI_InterCurveCurve intersections(GeomAPI::To2d(inner,meridianPlane),GeomAPI::To2d(axisCurve,meridianPlane),1e-9);
    if(intersections.NbPoints()!=2)return std::nullopt;
    double first=intersections.Intersector().Point(1).ParamOnFirst(),last=intersections.Intersector().Point(2).ParamOnFirst();
    if(last<first)std::swap(first,last);
    if(gp_Vec(axis.Location(),inner->Value((first+last)/2)).Dot(radial)<0){const double prior=first;first=last;last=prior+2*std::numbers::pi;}
    const auto a=inner->Value(first),b=inner->Value(last);
    auto inwardNormal=base.Plane().Axis().Direction();
    const auto diskCenter=rim.Circle().Location();
    BRepClass3d_SolidClassifier plus(source.shape,diskCenter.Translated(gp_Vec(inwardNormal)*1e-5),1e-7);
    BRepClass3d_SolidClassifier minus(source.shape,diskCenter.Translated(gp_Vec(inwardNormal)*-1e-5),1e-7);
    if(plus.State()==TopAbs_IN && minus.State()==TopAbs_OUT) {}
    else if(minus.State()==TopAbs_IN && plus.State()==TopAbs_OUT) inwardNormal.Reverse();
    else return std::nullopt;
    // Offsets which reach the selected plane still use the ordinary open Shell.
    if(gp_Vec(base.Plane().Location(),a).Dot(gp_Vec(inwardNormal))<=1e-6
        || gp_Vec(base.Plane().Location(),b).Dot(gp_Vec(inwardNormal))<=1e-6)return std::nullopt;
    for(int sample=1;sample<16;++sample) {
        const auto point=inner->Value(first+(last-first)*sample/16.);
        require(gp_Vec(axis.Location(),point).Dot(radial)>0,"Closed cavity meridian changes radial side",featureId);
        BRepClass3d_SolidClassifier inside(source.shape,point,1e-7);
        require(inside.State()==TopAbs_IN,"Closed circular cavity is not strictly inside the source solid",featureId);
    }
    Handle(Geom_TrimmedCurve) arc=new Geom_TrimmedCurve(inner,first,last);
    BRepBuilderAPI_MakeWire wire(BRepBuilderAPI_MakeEdge(arc).Edge(),BRepBuilderAPI_MakeEdge(b,a).Edge());
    BRepBuilderAPI_MakeFace profile(meridianPlane,wire.Wire(),true);
    BRepPrimAPI_MakeRevol revolve(profile.Face(),axis,2*std::numbers::pi,true);
    require(revolve.IsDone() && validArgument(revolve.Shape()),"Exact circular cavity is not a valid Boolean solid",featureId);
    require(validArgument(source.shape),"Original circular Revolve is not a valid Boolean solid",featureId);
    TopTools_ListOfShape objects,tools;objects.Append(source.shape);tools.Append(revolve.Shape());
    BRepAlgoAPI_BooleanOperation cut;cut.SetArguments(objects);cut.SetTools(tools);cut.SetOperation(BOPAlgo_CUT);
    cut.SetNonDestructive(true);cut.SetRunParallel(false);cut.SetFuzzyValue(0);cut.Build();
    require(cut.IsDone()&&!cut.HasErrors(),"Exact closed cavity subtraction failed",featureId);
    const auto shape=cut.Shape();const auto summary=GeometryKernel::summarize(shape);
    require(validArgument(shape)&&summary["closed"].toBool()&&summary["topology"].toObject()["solids"].toInt()==1
        && summary["volumeMm3"].toDouble()>0,"Closed cap Shell must remain one valid closed Boolean solid",featureId);
    auto retained=PlanarFaces::transfer(cut,source.supportFaces,shape);
    require(retained.size()==1&&retained[0].faceId==selected.faceId&&retained[0].face.IsSame(selected.face),
        "Closed cap did not preserve the selected disk through kernel history",featureId);
    retained[0].role="shell-closed-cap";
    auto output=source;output.shape=shape;output.supportFaces=std::move(retained);
    output.solidEdges=SolidFillet::transferEdges(cut,source.solidEdges,shape);output.summary=summary;
    output.summary["supportFaces"]=PlanarFaces::descriptors(output.supportFaces);output.summary["solidEdges"]=SolidFillet::descriptors(output.solidEdges);
    output.summary["operation"]="shell";output.summary["lastModifierFeatureId"]=featureId;output.summary["producerFeatureId"]=source.featureId;
    output.summary["thicknessMm"]=thickness;output.summary["removedFace"]=QJsonObject{{"bodyId",selected.bodyId},{"faceId",selected.faceId},{"producerFeatureId",selected.producerFeatureId}};
    output.summary["shellPolicy"]="exactCircularClosedCap";
    output.summary["cavityAxisPointsMm"]=QJsonArray{QJsonArray{a.X(),a.Y(),a.Z()},QJsonArray{b.X(),b.Y(),b.Z()}};
    return output;
}
