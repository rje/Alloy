// SPDX-License-Identifier: MIT
#include "SurfaceContinuity.h"
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRep_Tool.hxx>
#include <Geom2d_Curve.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <Standard_Failure.hxx>
#include <algorithm>
#include <cmath>
#include <numbers>
namespace {
QJsonObject refused(const QString& status,const QString& code,const QString& message){return {{"status",status},{"error",QJsonObject{{"code",code},{"message",message}}}};}
bool member(const TopoDS_Shape& owner,const TopoDS_Shape& target,TopAbs_ShapeEnum kind){
    if(owner.IsNull() || target.IsNull())return false;
    TopTools_IndexedMapOfShape shapes;TopExp::MapShapes(owner,kind,shapes);const int index=shapes.FindIndex(target);return index>0 && (kind!=TopAbs_FACE || shapes(index).Orientation()==target.Orientation());
}
QJsonArray xyz(const gp_Pnt& p){return {p.X(),p.Y(),p.Z()};}
QJsonArray xyz(const gp_Vec& p){return {p.X(),p.Y(),p.Z()};}
bool finite(const gp_Pnt& p){return std::isfinite(p.X()) && std::isfinite(p.Y()) && std::isfinite(p.Z());}
}
QJsonObject SurfaceContinuity::evaluate(const std::vector<EvaluatedBody>& bodies,const QJsonArray& refs,const QString& edgeId,const QString& units){
    if(refs.size()!=2 || !refs[0].isObject() || !refs[1].isObject() || edgeId.isEmpty())
        return refused("unavailable","continuity_inputs","Choose two exact planar faces and one shared edge");
    const auto a=refs[0].toObject(),b=refs[1].toObject();
    for(const auto& ref:{a,b})for(const auto* key:{"bodyId","faceId","producerFeatureId"})
        if(!ref.value(key).isString() || ref.value(key).toString().isEmpty())return refused("unavailable","continuity_inputs","Each face requires bodyId, faceId and producerFeatureId");
    if(a.value("bodyId")!=b.value("bodyId") || a.value("occurrenceId").toString()!=b.value("occurrenceId").toString())
        return refused("unsupported","continuity_same_body","Choose faces on the same visible body occurrence");
    const EvaluatedBody* body=nullptr;int bodyMatches=0;
    for(const auto& current:bodies)if(current.visible && current.bodyId==a.value("bodyId").toString()
        && (!a.contains("occurrenceId") || current.occurrenceId==a.value("occurrenceId").toString())){body=&current;++bodyMatches;}
    if(bodyMatches!=1)return refused("stale",bodyMatches>1 ? "continuity_ambiguous":"continuity_stale",bodyMatches>1 ? "Specify one exact body occurrence":"The selected body is hidden or no longer current");
    const EvaluatedSupportFace* faces[2]{};
    for(int i=0;i<2;++i){const auto ref=refs[i].toObject();int matches=0;
        for(const auto& f:body->supportFaces)if(f.faceId==ref.value("faceId").toString() && f.producerFeatureId==ref.value("producerFeatureId").toString()){faces[i]=&f;++matches;}
        if(matches!=1 || faces[i]->bodyId!=body->bodyId || !member(body->shape,faces[i]->face,TopAbs_FACE))return refused("stale","continuity_stale","A selected retained face is ambiguous or absent from the current body");
    }
    if(faces[0]->face.IsSame(faces[1]->face) || faces[0]->faceId==faces[1]->faceId)return refused("unsupported","continuity_duplicate_face","Choose two distinct current faces");
    const EvaluatedSolidEdge* retained=nullptr;int edgeMatches=0;
    for(const auto& e:body->solidEdges)if(e.edgeId==edgeId){retained=&e;++edgeMatches;}
    if(edgeMatches!=1 || !member(body->shape,retained->edge,TopAbs_EDGE))return refused("stale","continuity_stale_edge","The retained edge is ambiguous or absent from the current body");
    const auto& edge=retained->edge;
    for(const auto* face:faces){int matches=0;for(const auto& boundary:face->boundaryEdges)if(boundary.edgeId==edgeId && boundary.edge.IsSame(edge))++matches;
        if(matches!=1 || !member(face->face,edge,TopAbs_EDGE))return refused("unsupported","continuity_not_shared","The edge must be the same exact retained boundary on both faces");}
    TopTools_IndexedMapOfShape currentFaces;TopExp::MapShapes(body->shape,TopAbs_FACE,currentFaces);int incident=0;
    for(int i=1;i<=currentFaces.Extent();++i)if(member(currentFaces(i),edge,TopAbs_EDGE))++incident;
    if(incident!=2)return refused("unsupported","continuity_incidence","The shared edge must have exactly two incident current faces");
    try {
        BRepAdaptor_Curve curve(edge);const double first=curve.FirstParameter(),last=curve.LastParameter();
        if(curve.GetType()!=GeomAbs_Line || BRep_Tool::Degenerated(edge) || !std::isfinite(first) || !std::isfinite(last) || !std::isfinite(last-first) || last<=first)
            return refused("unsupported","continuity_straight_edge","This inspection requires one finite nondegenerate straight shared edge");
        if(!BRep_Tool::SameParameter(edge) || !BRep_Tool::SameRange(edge))return refused("unsupported","continuity_parameter_contract","The shared edge must retain SameParameter and SameRange; inspection does not heal topology");
        BRepAdaptor_Surface surfaces[2]{BRepAdaptor_Surface(faces[0]->face),BRepAdaptor_Surface(faces[1]->face)};
        Handle(Geom2d_Curve) pcurves[2];
        for(int i=0;i<2;++i){const auto& face=faces[i]->face;
            if(surfaces[i].GetType()!=GeomAbs_Plane)return refused("unsupported","continuity_planar_faces","This bounded inspection supports two planar faces only");
            if(face.Orientation()!=TopAbs_FORWARD && face.Orientation()!=TopAbs_REVERSED)return refused("unsupported","continuity_orientation","Each face requires a forward or reversed orientation");
            double start,end;pcurves[i]=BRep_Tool::CurveOnSurface(edge,face,start,end);
            if(pcurves[i].IsNull() || !std::isfinite(start) || !std::isfinite(end) || std::abs(start-first)>1e-9 || std::abs(end-last)>1e-9)
                return refused("unsupported","continuity_pcurve","Each face requires a pcurve with the shared edge parameter range");
        }
        QJsonArray samples;double maxGap=0,maxAngle=0,maxOriented=0;int maxGapIndex=0,maxAngleIndex=0,opposite=0;
        for(int index=0;index<9;++index){const double fraction=index/8.0,t=first+(last-first)*fraction;gp_Pnt points[2];gp_Vec normals[2];QJsonArray sides;
            for(int side=0;side<2;++side){const auto uv=pcurves[side]->Value(t);gp_Vec du,dv;surfaces[side].D1(uv.X(),uv.Y(),points[side],du,dv);normals[side]=du.Crossed(dv);
                if(!std::isfinite(uv.X()) || !std::isfinite(uv.Y()) || !finite(points[side]) || !std::isfinite(normals[side].SquareMagnitude()) || normals[side].SquareMagnitude()<1e-20)
                    return refused("unsupported","continuity_singular_sample","A boundary sample has no finite nonsingular surface derivative");
                normals[side].Normalize();if(faces[side]->face.Orientation()==TopAbs_REVERSED)normals[side].Reverse();
                sides.append(QJsonObject{{"uv",QJsonArray{uv.X(),uv.Y()}},{"pointMm",xyz(points[side])},{"orientedUnitNormal",xyz(normals[side])},{"faceOrientation",faces[side]->face.Orientation()==TopAbs_REVERSED ? "reversed":"forward"}});
            }
            const double gap=points[0].Distance(points[1]),dot=std::clamp(normals[0].Dot(normals[1]),-1.0,1.0),cross=normals[0].Crossed(normals[1]).Magnitude();
            const double angle=std::atan2(cross,std::abs(dot))*180/std::numbers::pi,oriented=std::atan2(cross,dot)*180/std::numbers::pi;
            if(gap>maxGap){maxGap=gap;maxGapIndex=index;}if(angle>maxAngle){maxAngle=angle;maxAngleIndex=index;}maxOriented=std::max(maxOriented,oriented);if(dot<0)++opposite;
            samples.append(QJsonObject{{"index",index},{"fraction",fraction},{"edgeParameter",t},{"faces",sides},{"gapMm",gap},{"tangentPlaneAngleDeg",angle},{"orientedNormalAngleDeg",oriented},{"normalDot",dot}});
        }
        const double scale=units=="in" ? 1/25.4 : 1;const QString lengthUnit=units=="in" ? "in":"mm";
        return {{"status","ready"},{"faces",refs},{"edgeId",edgeId},{"sampleCount",9},{"method","uniform_shared_edge_parameter_including_endpoints"},
            {"methodDetail","Face-specific edge pcurves and world BRepAdaptor_Surface D1; unoriented tangent-plane angle atan2(|nA cross nB|,abs(nA dot nB))."},
            {"edgeParameterRange",QJsonArray{first,last}},{"samples",samples},{"maxGapMm",maxGap},{"maxGapSampleIndex",maxGapIndex},{"maxTangentPlaneAngleDeg",maxAngle},{"maxTangentPlaneAngleSampleIndex",maxAngleIndex},
            {"thresholds",QJsonObject{{"gapMm",0.001},{"tangentPlaneAngleDeg",0.1},{"policy","Explicit Alloy choices; inclusive sampled thresholds, not reference defaults"}}},
            {"sampledG0Pass",maxGap<=0.001},{"sampledG1Pass",maxAngle<=0.1},{"normalOrientation",QJsonObject{{"oppositeHemisphereSampleCount",opposite},{"maxOrientedNormalAngleDeg",maxOriented},{"diagnosticOnly",true}}},
            {"display",QJsonObject{{"maxGap",maxGap*scale},{"gapThreshold",0.001*scale},{"lengthUnit",lengthUnit},{"maxTangentPlaneAngle",maxAngle},{"angleThreshold",0.1},{"angleUnit","deg"}}},
            {"persistence","session_only"},{"authoredChange",false},{"limits","Only nine boundary samples on two current planar faces sharing one exact straight topological edge. No guarantee between samples, separate-edge gap comparison, curved faces, G2, healing, saved analysis or reference UI parity."}};
    }catch(const Standard_Failure& e){return refused("unsupported","continuity_evaluation",QString::fromUtf8(e.GetMessageString()));}
}
