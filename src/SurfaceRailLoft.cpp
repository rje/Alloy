// SPDX-License-Identifier: MIT
#include "SurfaceRailLoft.h"
#include "PlanarFaces.h"
#include <BRepOffsetAPI_MakePipeShell.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRep_Tool.hxx>
#include <GeomAPI_ProjectPointOnCurve.hxx>
#include <Geom_BSplineCurve.hxx>
#include <Geom_BSplineSurface.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <GeomConvert.hxx>
#include <BSplCLib.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_Sewing.hxx>
#include <TColgp_Array2OfPnt.hxx>
#include <TColStd_Array2OfReal.hxx>
#include <map>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <QUuid>
#include <array>
#include <algorithm>
#include <cmath>

namespace {
constexpr double contactToleranceMm=1e-6;
constexpr double boundaryToleranceMm=1e-6;
constexpr double surfaceToleranceMm=1e-4;
QString id(const QString& feature,const QByteArray& role){
    return QUuid::createUuidV5(QUuid(feature),QByteArray("alloy-rail-loft/1/")+role).toString(QUuid::WithoutBraces);
}
struct Source {const EvaluatedSketch* sketch=nullptr;QJsonObject entity;TopoDS_Edge edge;};
QJsonObject provenance(const QString& feature,const QString& sketch,const QString& entity,const QString& role){
    return {{"generatorFeatureId",feature},{"sourceSketchId",sketch},{"sourceEntityId",entity},
        {"role",role},{"lineage","railLoftBuilder"}};
}
bool contains(const TopoDS_Shape& container,const TopoDS_Shape& child){
    for(TopExp_Explorer explorer(container,child.ShapeType());explorer.More();explorer.Next())if(explorer.Current().IsSame(child))return true;
    return false;
}
double pointToEdge(const gp_Pnt& point,const TopoDS_Edge& edge){
    Standard_Real first,last;const auto curve=BRep_Tool::Curve(edge,first,last);
    if(curve.IsNull())return 1e100;
    GeomAPI_ProjectPointOnCurve projected(point,curve,first,last);
    return projected.NbPoints() ? projected.LowerDistance() : 1e100;
}
double correspondence(const TopoDS_Edge& from,const TopoDS_Edge& to){
    BRepAdaptor_Curve curve(from);double maximum=0;
    for(int sample=0;sample<=40;++sample){
        const auto parameter=curve.FirstParameter()+(curve.LastParameter()-curve.FirstParameter())*sample/40.0;
        maximum=std::max(maximum,pointToEdge(curve.Value(parameter),to));
    }
    return maximum;
}
bool matches(const TopoDS_Edge& a,const TopoDS_Edge& b){
    return correspondence(a,b)<boundaryToleranceMm && correspondence(b,a)<boundaryToleranceMm;
}
std::pair<gp_Pnt,gp_Pnt> ends(const TopoDS_Edge& edge){
    TopoDS_Vertex first,last;TopExp::Vertices(edge,first,last,true);
    return {BRep_Tool::Pnt(first),BRep_Tool::Pnt(last)};
}
std::pair<gp_Pnt,gp_Pnt> parameterEnds(const TopoDS_Edge& edge){
    BRepAdaptor_Curve curve(edge);return {curve.Value(curve.FirstParameter()),curve.Value(curve.LastParameter())};
}
TopoDS_Edge uniqueGeneratedEdge(BRepOffsetAPI_MakePipeShell& loft,const TopoDS_Vertex& vertex,
    const TopTools_IndexedMapOfShape& current){
    TopTools_IndexedMapOfShape found;
    for(const auto& shape:loft.Generated(vertex))if(shape.ShapeType()==TopAbs_EDGE && current.Contains(shape))found.Add(shape);
    return found.Extent()==1 ? TopoDS::Edge(current(current.FindIndex(found(1)))) : TopoDS_Edge{};
}
struct Boundary {QString edgeId;TopoDS_Edge edge;QJsonObject provenance;};
// This bounded construction is an exact tensor product of the solved sources.
// The two sides share one support; the tip is its known collapsed boundary.
EvaluatedBody bilateral(const QJsonObject& feature,const Source& arc,const Source& left,
    const Source& right,const Source& rail){
    const auto featureId=feature["id"].toString();const auto parameters=feature["parameters"].toObject();
    const auto sections=parameters["sections"].toArray();const auto railRef=parameters["rail"].toObject();
    auto reject=[&](const QString& code,const QString& message){throw GeometryKernel::EvaluationError({
        {"featureId",featureId},{"featureType","loft"},{"code",code},{"message",message}});};
    auto require=[&](bool condition,const QString& message){if(!condition)reject("unsupported_bilateral_rail_network",message);};
    constexpr double exactTolerance=1e-7;
    const auto leftId=left.entity["id"].toString(),rightId=right.entity["id"].toString();
    int mirrorPairs=0;
    for(const auto& value:left.sketch->summary["mirrorGroups"].toArray())
        for(const auto& pairValue:value.toObject()["entityPairs"].toArray()){
            const auto pair=pairValue.toObject();
            if((pair["sourceEntityId"]==leftId && pair["resultEntityId"]==rightId)
                || (pair["sourceEntityId"]==rightId && pair["resultEntityId"]==leftId))++mirrorPairs;
        }
    require(left.sketch==right.sketch && mirrorPairs==1,
        "Bilateral Rail Loft requires one retained Mirror pair of lower fit splines");
    auto spline=[&](const Source& source){
        BRepAdaptor_Curve adaptor(source.edge);
        require(!adaptor.IsClosed() && (source.entity["type"]=="arc" || !adaptor.IsPeriodic()),
            "Bilateral sources must be finite open curves");
        double first,last;const auto curve=BRep_Tool::Curve(source.edge,first,last);
        require(!curve.IsNull() && std::isfinite(first) && std::isfinite(last) && last>first,
            "Bilateral source curve parameters are unavailable");
        Handle(Geom_Curve) finite=new Geom_TrimmedCurve(curve,first,last);
        auto result=Handle(Geom_BSplineCurve)::DownCast(GeomConvert::CurveToBSplineCurve(finite)->Copy());
        require(!result.IsNull() && !result->IsPeriodic()
            && (source.entity["type"]=="arc" || result->IsCN(1)),
            "Bilateral sources must admit tangent-continuous nonperiodic B-splines");
        auto knots=result->Knots();BSplCLib::Reparametrize(0,1,knots);result->SetKnots(knots);
        if(source.entity["type"]=="fitSpline"){
            require(!result->IsRational(),"Rational longitudinal curves are outside bilateral Rail Loft");
            const auto pointIds=source.entity["pointIds"].toArray();
            auto solvedPoint=[&](const QJsonValue& pointId){
                for(const auto& rowValue:source.sketch->summary["worldPoints"].toArray()){
                    const auto row=rowValue.toObject();const auto xyz=row["positionMm"].toArray();
                    if(row["id"]==pointId && xyz.size()==3)return gp_Pnt(xyz[0].toDouble(),xyz[1].toDouble(),xyz[2].toDouble());
                }
                reject("rail_contact_unavailable","Source fit-point endpoint identity no longer resolves");return gp_Pnt{};
            };
            require(pointIds.size()>=2 && result->Value(0).Distance(solvedPoint(pointIds.first()))<exactTolerance
                && result->Value(1).Distance(solvedPoint(pointIds.last()))<exactTolerance,
                "Solved spline must retain authored fit-point traversal");
        }
        return result;
    };
    BRepAdaptor_Curve arcAdaptor(arc.edge);
    require(arcAdaptor.GetType()==GeomAbs_Circle,"The first bilateral section must be one planar open arc");
    auto A=spline(arc),L=spline(left),R=spline(right),C=spline(rail);
    // Resolve the unique shared lower endpoint, then orient both heel to tip.
    int shared=0;bool reverseL=false,reverseR=false;gp_Pnt tip;
    for(int i=0;i<2;++i)for(int j=0;j<2;++j)if(L->Value(i).Distance(R->Value(j))<exactTolerance){
        ++shared;tip=L->Value(i);reverseL=i==0;reverseR=j==0;
    }
    require(shared==1,"The lower Mirror pair needs exactly one common tip endpoint");
    if(reverseL)L->Reverse();if(reverseR)R->Reverse();
    const bool direct=A->Value(0).Distance(L->Value(0))<exactTolerance && A->Value(1).Distance(R->Value(0))<exactTolerance;
    const bool reversed=A->Value(1).Distance(L->Value(0))<exactTolerance && A->Value(0).Distance(R->Value(0))<exactTolerance;
    require(direct!=reversed,"Back arc ends must uniquely meet the two lower heels");if(reversed)A->Reverse();
    const bool tipStart=C->Value(0).Distance(tip)<exactTolerance,tipEnd=C->Value(1).Distance(tip)<exactTolerance;
    require(tipStart!=tipEnd,"One rail endpoint must uniquely meet the common lower tip");if(tipStart)C->Reverse();
    GeomAPI_ProjectPointOnCurve contact(C->Value(0),A,0,1);
    require(contact.NbPoints()>0 && contact.LowerDistance()<exactTolerance,"Rail must meet the actual back arc");
    const double seam=contact.LowerDistanceParameter();
    require(seam>1e-8 && seam<1-1e-8,"Bilateral rail contact must be interior to the back arc");
    for(int i=1;i<=contact.NbPoints();++i)if(contact.Distance(i)<exactTolerance)
        require(std::abs(contact.Parameter(i)-seam)<1e-8,"The back-arc rail contact must be unique");
    const gp_Pnt heel((L->Value(0).XYZ()+R->Value(0).XYZ())/2);
    const gp_Vec transverse(heel,R->Value(0)),height(heel,C->Value(0));
    const double vv=transverse.SquareMagnitude(),hh=height.SquareMagnitude(),vh=transverse.Dot(height),det=vv*hh-vh*vh;
    require(vv>1e-12 && hh>1e-12 && det>1e-12*vv*hh,"Back-arc transverse and height basis must be independent");
    auto coefficients=[&](const gp_Pnt& point){const gp_Vec d(heel,point);
        return std::pair<double,double>{(d.Dot(transverse)*hh-d.Dot(height)*vh)/det,
            (d.Dot(height)*vv-d.Dot(transverse)*vh)/det};};
    const auto [ac,bc]=coefficients(A->Value(seam));
    require(std::abs(ac)<1e-8 && std::abs(bc-1)<1e-8,"Rail contact must lie on the back arc's symmetry plane");
    const auto across=transverse.Normalized(),backNormal=transverse.Crossed(height).Normalized();
    const gp_Vec lowerLongitudinal(heel,tip);
    require(lowerLongitudinal.Crossed(across).Magnitude()>exactTolerance,"Lower section plane is degenerate");
    const auto lowerNormal=lowerLongitudinal.Crossed(across).Normalized();
    gp_Pnt at;gp_Vec tangent;A->D1(seam,at,tangent);
    require(tangent.Magnitude()>1e-12 && tangent.Normalized().Crossed(across).Magnitude()<1e-7,
        "Back-arc contact tangent must be transverse to the rail plane");
    for(const auto& curve:{L,R}){curve->D1(1,at,tangent);
        require(tangent.Magnitude()>1e-12 && tangent.Normalized().Crossed(across).Magnitude()<1e-7,
            "Lower tip tangents must be transverse to the rail plane");}
    // Exact degree elevation and knot insertion: no fitting or source-pole edits.
    const int degree=std::max({L->Degree(),R->Degree(),C->Degree()});
    const std::array<Handle(Geom_BSplineCurve),3> longitudinal{L,R,C};
    for(const auto& curve:longitudinal)curve->IncreaseDegree(degree);
    std::map<double,int> unionKnots;
    for(const auto& curve:longitudinal)for(int i=1;i<=curve->NbKnots();++i)
        unionKnots[curve->Knot(i)]=std::max(unionKnots[curve->Knot(i)],curve->Multiplicity(i));
    TColStd_Array1OfReal knots(1,unionKnots.size());TColStd_Array1OfInteger multiplicities(1,unionKnots.size());
    int knotIndex=1;for(const auto& [knot,multiplicity]:unionKnots){knots.SetValue(knotIndex,knot);multiplicities.SetValue(knotIndex++,multiplicity);}
    for(const auto& curve:longitudinal){curve->InsertKnots(knots,multiplicities,1e-12,false);
        require(curve->NbPoles()==L->NbPoles() && curve->NbKnots()==L->NbKnots(),"Longitudinal common basis is incompatible");
        for(int i=1;i<=L->NbKnots();++i)require(std::abs(curve->Knot(i)-L->Knot(i))<1e-12
            && curve->Multiplicity(i)==L->Multiplicity(i),"Longitudinal common knots are incompatible");}
    // Qualify reflection and planes on all poles, which is stronger than sampling.
    for(int i=1;i<=L->NbPoles();++i){
        const gp_Pnt midpoint((L->Pole(i).XYZ()+R->Pole(i).XYZ())/2);
        require(std::abs(gp_Vec(heel,midpoint).Dot(across))<exactTolerance
            && gp_Vec(L->Pole(i),R->Pole(i)).Crossed(across).Magnitude()<exactTolerance,
            "Solved lower curves must have the same reflected parameterization");
        require(std::abs(gp_Vec(heel,L->Pole(i)).Dot(lowerNormal))<exactTolerance
            && std::abs(gp_Vec(heel,R->Pole(i)).Dot(lowerNormal))<exactTolerance,
            "Solved lower curves must remain in their common section plane");
        require(std::abs(gp_Vec(heel,C->Pole(i)).Dot(across))<exactTolerance,
            "Solved rail must remain in the lower pair's middle plane");
    }
    TColgp_Array2OfPnt poles(1,A->NbPoles(),1,L->NbPoles());
    TColStd_Array2OfReal weights(1,A->NbPoles(),1,L->NbPoles());
    for(int u=1;u<=A->NbPoles();++u){
        const auto [a,b]=coefficients(A->Pole(u));
        require(A->Weight(u)>0 && std::abs(gp_Vec(heel,A->Pole(u)).Dot(backNormal))<exactTolerance
            && heel.Translated(a*transverse+b*height).Distance(A->Pole(u))<exactTolerance,
            "Back arc must have positive rational weights in the source-local plane");
        for(int t=1;t<=L->NbPoles();++t){
            const auto base=(L->Pole(t).XYZ()+R->Pole(t).XYZ())/2;
            const auto width=(R->Pole(t).XYZ()-L->Pole(t).XYZ())/2;
            poles.SetValue(u,t,gp_Pnt(base+a*width+b*(C->Pole(t).XYZ()-base)));weights.SetValue(u,t,A->Weight(u));
        }
    }
    Handle(Geom_BSplineSurface) surface=new Geom_BSplineSurface(poles,weights,A->Knots(),L->Knots(),
        A->Multiplicities(),L->Multiplicities(),A->Degree(),L->Degree(),false,false);
    double sourceError=0,minJacobian=1e100,minSine=1;
    for(int i=0;i<=200;++i){const double t=i/200.;
        sourceError=std::max({sourceError,surface->Value(t,0).Distance(A->Value(t)),
            surface->Value(0,t).Distance(L->Value(t)),surface->Value(1,t).Distance(R->Value(t)),
            surface->Value(seam,t).Distance(C->Value(t))});}
    require(sourceError<exactTolerance,"Tensor surface must preserve all solved source restrictions");
    // Bounded sampled regularity excludes only the known collapsed t=1 tip.
    for(int iu=0;iu<=80;++iu)for(int it=0;it<100;++it){gp_Vec du,dt;
        surface->D1(iu/80.,it/100.,at,du,dt);const double jacobian=du.Crossed(dt).Magnitude();
        const double scale=du.Magnitude()*dt.Magnitude();
        require(std::isfinite(jacobian) && jacobian>1e-8 && scale>1e-12 && jacobian/scale>1e-8,
            "Bilateral source network produces a singular sampled interior");
        minJacobian=std::min(minJacobian,jacobian);minSine=std::min(minSine,jacobian/scale);
    }
    BRepBuilderAPI_MakeFace makeLeft(surface,0,seam,0,1,exactTolerance),makeRight(surface,seam,1,0,1,exactTolerance);
    require(makeLeft.IsDone() && makeRight.IsDone(),"Bilateral faces could not be built");
    BRepBuilderAPI_Sewing sewing(exactTolerance);sewing.Add(makeLeft.Face());sewing.Add(makeRight.Face());sewing.Perform();
    EvaluatedBody body;body.featureId=featureId;body.bodyId=feature["bodyId"].toString();
    body.name=feature["name"].toString();body.visible=feature["visible"].toBool();body.shape=sewing.SewedShape();
    body.summary=GeometryKernel::summarize(body.shape);const auto topology=body.summary["topology"].toObject();
    require(body.summary["valid"].toBool() && topology["faces"].toInt()==2 && topology["edges"].toInt()==7
        && topology["solids"].toInt()==0 && sewing.NbFreeEdges()==4 && sewing.NbMultipleEdges()==0,
        "Bilateral Loft must produce two valid sewn faces with the known collapsed tips");
    TopTools_IndexedMapOfShape currentEdges,currentFaces;TopExp::MapShapes(body.shape,TopAbs_EDGE,currentEdges);
    TopExp::MapShapes(body.shape,TopAbs_FACE,currentFaces);
    auto matching=[&](const TopoDS_Edge& source){TopoDS_Edge result;int count=0;
        for(int i=1;i<=currentEdges.Extent();++i){const auto edge=TopoDS::Edge(currentEdges(i));
            if(!BRep_Tool::Degenerated(edge) && matches(source,edge)){result=edge;++count;}}
        require(count==1,"Each exact source boundary needs unique current ancestry");return result;};
    const auto railEdge=matching(rail.edge);const std::array<Source,2> lowerSources{left,right};
    std::array<TopoDS_Edge,2> lowerEdges{matching(left.edge),matching(right.edge)},backEdges;
    std::array<TopoDS_Face,2> faces;
    int collapsed=0;for(int i=1;i<=currentEdges.Extent();++i)if(BRep_Tool::Degenerated(TopoDS::Edge(currentEdges(i))))++collapsed;
    require(collapsed==2 && !lowerEdges[0].IsSame(lowerEdges[1]),"Collapsed tip artifacts or lower ancestry are ambiguous");
    for(int side=0;side<2;++side){int count=0;
        for(int i=1;i<=currentFaces.Extent();++i)if(contains(currentFaces(i),lowerEdges[side])){faces[side]=TopoDS::Face(currentFaces(i));++count;}
        require(count==1 && contains(faces[side],railEdge),"Each lower source must identify one face sharing the retained rail");
        count=0;for(TopExp_Explorer it(faces[side],TopAbs_EDGE);it.More();it.Next()){
            const auto edge=TopoDS::Edge(it.Current());if(BRep_Tool::Degenerated(edge) || edge.IsSame(railEdge) || edge.IsSame(lowerEdges[side]))continue;
            if(correspondence(edge,arc.edge)<boundaryToleranceMm){backEdges[side]=edge;++count;}}
        require(count==1,"Each face needs one unique contact-split back-arc boundary");
    }
    require(!faces[0].IsSame(faces[1]) && !backEdges[0].IsSame(backEdges[1])
        && BRep_Tool::Surface(faces[0])==BRep_Tool::Surface(faces[1]),"Bilateral sides must share one support with distinct ancestry");
    // Contact metadata uses the original circle parameter, never the normalized rational parameter.
    double first,last;const auto originalArc=BRep_Tool::Curve(arc.edge,first,last);
    GeomAPI_ProjectPointOnCurve originalContact(C->Value(0),originalArc,first,last);
    require(originalContact.NbPoints()>0 && originalContact.LowerDistance()<exactTolerance,"Original back contact parameter unavailable");
    std::vector<Boundary> boundaries{{id(featureId,"edge/rail"),railEdge,
        provenance(featureId,rail.sketch->featureId,rail.entity["id"].toString(),"railSeam")}};
    boundaries[0].provenance["backArcContactParameter"]=originalContact.LowerDistanceParameter();
    for(int side=0;side<2;++side){const auto span=lowerSources[side].entity["id"].toString();
        auto backProvenance=provenance(featureId,arc.sketch->featureId,arc.entity["id"].toString(),"back");
        backProvenance["lowerSourceEntityId"]=span;backProvenance["contactParameter"]=originalContact.LowerDistanceParameter();
        boundaries.push_back({id(featureId,"edge/back/"+span.toUtf8()),backEdges[side],backProvenance});
        boundaries.push_back({id(featureId,"edge/lower/"+span.toUtf8()),lowerEdges[side],
            provenance(featureId,left.sketch->featureId,span,"lower")});
    }
    for(int side=0;side<2;++side){EvaluatedSupportFace support{
        id(featureId,"face/"+lowerSources[side].entity["id"].toString().toUtf8()),body.bodyId,featureId,"loft.railSurface",faces[side],{}, {}};
        for(const auto& boundary:boundaries)if(contains(faces[side],boundary.edge))
            support.boundaryEdges.push_back({boundary.edgeId,"curve",boundary.edge,boundary.provenance});
        require(support.boundaryEdges.size()==3,"Each bilateral face needs three ordinary source-derived boundaries");body.supportFaces.push_back(support);
    }
    for(const auto& boundary:boundaries)body.solidEdges.push_back({boundary.edgeId,boundary.edge,boundary.provenance});
    body.summary["supportFaces"]=PlanarFaces::descriptors(body.supportFaces);
    body.summary["operation"]="newComponent";body.summary["guideType"]="rail";
    body.summary["sections"]=sections;body.summary["rail"]=railRef;
    body.summary["interpolation"]=QJsonObject{{"kernel","OCCT exact BSpline tensor"},{"mode","bilateralInteriorRail"},
        {"correspondence","finite source parameters normalized linearly to [0,1]"},
        {"sampledSourceRestrictionErrorMm",sourceError},{"sourceRestrictionToleranceMm",exactTolerance},
        {"sampledMinimumJacobian",minJacobian},{"sampledMinimumDerivativeSine",minSine},
        {"regularitySamples",QJsonArray{81,100}},{"regularityLongitudinalEnd",.99},
        {"collapsedTipEdges",collapsed},{"selectableOrdinaryEdges",5}};
    return body;
}

}

EvaluatedBody SurfaceRailLoft::evaluate(const QJsonObject& feature,const EvaluatedDocument& prior){
    const auto parameters=feature["parameters"].toObject();
    const auto sections=parameters["sections"].toArray();
    const auto featureId=feature["id"].toString();
    auto reject=[&](const QString& code,const QString& message){throw GeometryKernel::EvaluationError({
        {"featureId",featureId},{"featureType","loft"},{"code",code},{"message",message}});};
    auto source=[&](const QString& sketchId,const QString& entityId,const QString& type)->Source{
        for(const auto& sketch:prior.sketches)if(sketch.featureId==sketchId){
            if(!sketch.entityEdges.contains(entityId))break;
            for(const auto& row:sketch.summary["entities"].toArray()){
                const auto entity=row.toObject();
                if(entity["id"]==entityId && (type.isEmpty() || entity["type"]==type) && !entity["construction"].toBool())
                    return {&sketch,entity,TopoDS::Edge(BRepBuilderAPI_Copy(sketch.entityEdges.value(entityId),false,false).Shape())};
            }
            break;
        }
        reject("loft_source_unavailable","Select exact earlier ordinary arc, two lower curves and fit-spline rail");return {};
    };
    const auto first=sections[0].toObject(),last=sections[1].toObject(),railRef=parameters["rail"].toObject();
    const auto firstIds=first["entityIds"].toArray(),lastIds=last["entityIds"].toArray();
    auto arc=source(first["sketchId"].toString(),firstIds[0].toString(),"arc");
    auto line0=source(last["sketchId"].toString(),lastIds[0].toString(),{});
    auto line1=source(last["sketchId"].toString(),lastIds[1].toString(),{});
    auto rail=source(railRef["sketchId"].toString(),railRef["entityId"].toString(),"fitSpline");
    if(line0.entity["type"]=="fitSpline" && line1.entity["type"]=="fitSpline")
        return bilateral(feature,arc,line0,line1,rail);
    BRepAdaptor_Curve railCurve(rail.edge),arcCurve(arc.edge),leftCurve(line0.edge),rightCurve(line1.edge);
    if(railCurve.GetType()!=GeomAbs_BSplineCurve || arcCurve.GetType()!=GeomAbs_Circle
        || leftCurve.GetType()!=GeomAbs_Line || rightCurve.GetType()!=GeomAbs_Line
        || arcCurve.IsClosed() || leftCurve.IsClosed() || rightCurve.IsClosed())
        reject("unsupported_rail_loft_source","Rail Loft supports one open arc, an ordered two-line chain and one open fit spline");
    const auto spline=railCurve.BSpline();
    if(spline.IsNull() || !spline->IsCN(1) || railCurve.IsClosed())
        reject("rail_not_tangent_continuous","The selected rail must be open and tangent-continuous");
    const auto [railStart,railEnd]=parameterEnds(rail.edge);
    const auto railPoints=rail.entity["pointIds"].toArray();
    auto solvedPoint=[&](const QString& pointId)->gp_Pnt{
        for(const auto& value:rail.sketch->summary["worldPoints"].toArray()){
            const auto row=value.toObject();const auto xyz=row["positionMm"].toArray();
            if(row["id"]==pointId && xyz.size()==3)
                return gp_Pnt(xyz[0].toDouble(),xyz[1].toDouble(),xyz[2].toDouble());
        }
        reject("rail_contact_unavailable","Rail fit-point identity no longer resolves in the solved sketch");return {};
    };
    if(railPoints.size()<2 || railStart.Distance(solvedPoint(railPoints.first().toString()))>=contactToleranceMm
        || railEnd.Distance(solvedPoint(railPoints.last().toString()))>=contactToleranceMm)
        reject("rail_contact_unavailable","Rail evaluation must follow retained fit-point endpoint order");
    if(railStart.Distance(railEnd)<contactToleranceMm)
        reject("rail_contact_unavailable","Rail endpoints must be distinct");
    const auto [arcA,arcB]=parameterEnds(arc.edge);
    const auto startA=railStart.Distance(arcA)<contactToleranceMm,startB=railStart.Distance(arcB)<contactToleranceMm;
    if(startA==startB)reject("rail_contact_unavailable","Rail start must meet one unique back-arc endpoint");
    if(startB)arc.edge.Reverse();
    const auto [lineA,lineB]=parameterEnds(line0.edge);
    const auto endA=railEnd.Distance(lineA)<contactToleranceMm,endB=railEnd.Distance(lineB)<contactToleranceMm;
    if(endA==endB)reject("rail_contact_unavailable","Rail end must meet the first lower-line free endpoint");
    if(endB)line0.edge.Reverse();
    const auto [leftA,leftB]=ends(line0.edge);
    const auto [rightA,rightB]=ends(line1.edge);
    const auto sharedA=leftB.Distance(rightA)<contactToleranceMm,sharedB=leftB.Distance(rightB)<contactToleranceMm;
    if(sharedA==sharedB || leftA.Distance(rightA)<contactToleranceMm || leftA.Distance(rightB)<contactToleranceMm)
        reject("loft_chain_disconnected","The two lower lines must form one ordered open chain from the rail contact");
    if(sharedB)line1.edge.Reverse();
    if(ends(line1.edge).second.Distance(railStart)<contactToleranceMm)
        reject("loft_chain_closed","The lower section must remain an open chain");
    BRepBuilderAPI_MakeWire spine(rail.edge),back(arc.edge),lower(line0.edge,line1.edge);
    if(!spine.IsDone() || !back.IsDone() || !lower.IsDone())
        reject("loft_wire_failed","OCCT could not build retained rail and section wires");
    TopoDS_Vertex arcContact,arcOpposite;TopExp::Vertices(arc.edge,arcContact,arcOpposite,true);
    if(BRep_Tool::Pnt(arcOpposite).Distance(railStart)<contactToleranceMm)std::swap(arcContact,arcOpposite);
    if(BRep_Tool::Pnt(arcContact).Distance(railStart)>=contactToleranceMm)
        reject("rail_contact_unavailable","The selected arc endpoint is unavailable in the evaluated edge");
    BRepOffsetAPI_MakePipeShell loft(spine.Wire());loft.SetMode(false);loft.SetTolerance(1e-5,1e-5,1e-4);
    loft.Add(back.Wire(),BRepBuilderAPI_MakeVertex(railStart).Vertex(),false,false);
    loft.Add(lower.Wire(),BRepBuilderAPI_MakeVertex(railEnd).Vertex(),false,false);
    loft.Build();
    if(!loft.IsDone() || !std::isfinite(loft.ErrorOnSurface()) || loft.ErrorOnSurface()>surfaceToleranceMm)
        reject("loft_failed","The intersecting rail could not build a bounded surface");
    EvaluatedBody body;body.featureId=featureId;body.bodyId=feature["bodyId"].toString();
    body.name=feature["name"].toString();body.visible=feature["visible"].toBool();body.shape=loft.Shape();
    body.summary=GeometryKernel::summarize(body.shape);
    const auto topology=body.summary["topology"].toObject();
    if(!body.summary["valid"].toBool() || topology["faces"].toInt()!=2
        || topology["edges"].toInt()!=7 || topology["solids"].toInt()!=0)
        reject("loft_invalid","Rail Loft must produce two valid open faces, seven edges and no solid");
    TopTools_IndexedMapOfShape currentFaces,currentEdges,startEdges,endEdges;
    TopExp::MapShapes(body.shape,TopAbs_FACE,currentFaces);TopExp::MapShapes(body.shape,TopAbs_EDGE,currentEdges);
    if(loft.FirstShape().ShapeType()!=TopAbs_WIRE || loft.LastShape().ShapeType()!=TopAbs_WIRE)
        reject("loft_lineage_unavailable","Loft section wire provenance is unavailable");
    TopExp::MapShapes(loft.FirstShape(),TopAbs_EDGE,startEdges);
    TopExp::MapShapes(loft.LastShape(),TopAbs_EDGE,endEdges);
    if(startEdges.Extent()!=2 || endEdges.Extent()!=2)
        reject("loft_lineage_unavailable","Loft must retain both section boundary spans");
    const auto railEdge=uniqueGeneratedEdge(loft,arcContact,currentEdges);
    const auto oppositeEdge=uniqueGeneratedEdge(loft,arcOpposite,currentEdges);
    if(railEdge.IsNull() || oppositeEdge.IsNull() || railEdge.IsSame(oppositeEdge)
        || !matches(rail.edge,railEdge))
        reject("loft_rail_lineage_unavailable","The selected rail needs one current free boundary with sampled curve correspondence");
    std::array<TopoDS_Edge,2> lowerEdges;
    const std::array<Source,2> sources{line0,line1};
    for(int i=0;i<2;++i){
        TopTools_IndexedMapOfShape matched;
        for(int j=1;j<=endEdges.Extent();++j){const auto edge=TopoDS::Edge(endEdges(j));
            if(currentEdges.Contains(edge) && matches(sources[i].edge,edge))matched.Add(edge);}
        if(matched.Extent()!=1)reject("loft_section_lineage_unavailable","Each lower source line must retain one unique current boundary");
        lowerEdges[i]=TopoDS::Edge(currentEdges(currentEdges.FindIndex(matched(1))));
    }
    if(lowerEdges[0].IsSame(lowerEdges[1]))
        reject("loft_section_lineage_unavailable","Lower source lines merged ambiguously");
    std::array<TopoDS_Face,2> faces;
    for(int i=0;i<2;++i){
        TopTools_IndexedMapOfShape matched;
        for(const auto& generated:loft.Generated(arc.edge))if(generated.ShapeType()==TopAbs_FACE
            && currentFaces.Contains(generated) && contains(generated,lowerEdges[i]))matched.Add(generated);
        if(matched.Extent()!=1)reject("loft_face_lineage_unavailable","Each lower source span must identify one generated face");
        faces[i]=TopoDS::Face(currentFaces(currentFaces.FindIndex(matched(1))));
    }
    if(faces[0].IsSame(faces[1]))reject("loft_face_lineage_unavailable","Source spans mapped to the same face");
    std::array<TopoDS_Edge,2> backEdges;
    for(int i=0;i<2;++i){
        TopTools_IndexedMapOfShape matched;
        for(int j=1;j<=startEdges.Extent();++j)if(currentEdges.Contains(startEdges(j)) && contains(faces[i],startEdges(j)))
            matched.Add(startEdges(j));
        if(matched.Extent()!=1)reject("loft_section_lineage_unavailable","Back arc span has no unique current face boundary");
        backEdges[i]=TopoDS::Edge(currentEdges(currentEdges.FindIndex(matched(1))));
        if(correspondence(backEdges[i],arc.edge)>boundaryToleranceMm)
            reject("loft_section_lineage_unavailable","A generated back span departed from the selected arc");
    }
    if(backEdges[0].IsSame(backEdges[1]))reject("loft_section_lineage_unavailable","Back arc spans merged ambiguously");
    TopoDS_Edge middle;
    for(int i=1;i<=currentEdges.Extent();++i){const auto edge=TopoDS::Edge(currentEdges(i));
        if(edge.IsSame(railEdge)||edge.IsSame(oppositeEdge)||edge.IsSame(lowerEdges[0])||edge.IsSame(lowerEdges[1])
            ||edge.IsSame(backEdges[0])||edge.IsSame(backEdges[1]))continue;
        if(!middle.IsNull())reject("loft_lineage_unavailable","Multiple unnamed interior boundaries");middle=edge;}
    if(middle.IsNull())reject("loft_lineage_unavailable","Interior boundary unavailable");
    const auto railId=railRef["entityId"].toString(),arcId=firstIds[0].toString();
    std::vector<Boundary> boundaries;
    boundaries.push_back({id(featureId,"edge/rail"),railEdge,provenance(featureId,railRef["sketchId"].toString(),railId,"rail")});
    boundaries.push_back({id(featureId,"edge/opposite"),oppositeEdge,provenance(featureId,first["sketchId"].toString(),arcId,"opposite")});
    boundaries.push_back({id(featureId,"edge/middle"),middle,provenance(featureId,last["sketchId"].toString(),lastIds[0].toString(),"middle")});
    for(int i=0;i<2;++i){
        const auto span=lastIds[i].toString().toUtf8();
        boundaries.push_back({id(featureId,"edge/back/"+span),backEdges[i],provenance(featureId,first["sketchId"].toString(),arcId,"back")});
        boundaries.push_back({id(featureId,"edge/lower/"+span),lowerEdges[i],provenance(featureId,last["sketchId"].toString(),lastIds[i].toString(),"lower")});
    }
    int railFaces=0,middleFaces=0;
    for(int i=0;i<2;++i){
        EvaluatedSupportFace support{id(featureId,"face/"+lastIds[i].toString().toUtf8()),body.bodyId,featureId,
            "loft.railSurface",faces[i],{}, {}};
        for(const auto& boundary:boundaries)if(contains(faces[i],boundary.edge)){
            BRepAdaptor_Curve curve(boundary.edge);
            support.boundaryEdges.push_back({boundary.edgeId,curve.GetType()==GeomAbs_Line ? "line" : "curve",
                boundary.edge,boundary.provenance});
            if(boundary.edge.IsSame(railEdge))++railFaces;
            if(boundary.edge.IsSame(middle))++middleFaces;
        }
        if(support.boundaryEdges.size()!=4 || !contains(faces[i],lowerEdges[i]) || !contains(faces[i],backEdges[i]))
            reject("loft_lineage_unavailable","Each generated face needs four uniquely named boundaries");
        body.supportFaces.push_back(support);
    }
    if(railFaces!=1 || middleFaces!=2)
        reject("loft_rail_lineage_unavailable","Selected rail must be a free boundary and middle edge must join both faces");
    for(const auto& boundary:boundaries)body.solidEdges.push_back({boundary.edgeId,boundary.edge,boundary.provenance});
    body.summary["supportFaces"]=PlanarFaces::descriptors(body.supportFaces);
    body.summary["operation"]="newComponent";body.summary["guideType"]="rail";
    body.summary["sections"]=sections;body.summary["rail"]=railRef;
    body.summary["interpolation"]=QJsonObject{{"kernel","OCCT MakePipeShell"},{"mode","correctedFrenet"},
        {"contact","source endpoints on rail"},{"outputSurfaceErrorMm",loft.ErrorOnSurface()},
        {"sampledBoundaryCorrespondenceToleranceMm",boundaryToleranceMm}};
    return body;
}
