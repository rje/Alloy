// SPDX-License-Identifier: MIT
#include "FaceNormalInspector.h"
#include <BRepAdaptor_Surface.hxx>
#include <BRepClass_FaceClassifier.hxx>
#include <BRepBndLib.hxx>
#include <BRepTools.hxx>
#include <Bnd_Box.hxx>
#include <Standard_Failure.hxx>
#include <gp_Pnt2d.hxx>
#include <TopExp_Explorer.hxx>
#include <cmath>
namespace {
QJsonObject refused(const QString& status,const QString& code,const QString& message){return {{"status",status},{"error",QJsonObject{{"code",code},{"message",message}}}};}
}
QJsonObject FaceNormalInspector::sample(const TopoDS_Face& face) {
    if(face.IsNull())return refused("stale","face_normals_stale","The selected face is no longer available");
    try {
        BRepAdaptor_Surface surface(face);
        if(surface.GetType()!=GeomAbs_Plane && surface.GetType()!=GeomAbs_Cylinder)
            return refused("unsupported","face_normals_unsupported","Face normals currently support planar and cylindrical faces");
        if(face.Orientation()!=TopAbs_FORWARD && face.Orientation()!=TopAbs_REVERSED)
            return refused("unsupported","face_normals_orientation","The face has no forward or reversed orientation");
        double u0,u1,v0,v1;BRepTools::UVBounds(face,u0,u1,v0,v1);
        if(!std::isfinite(u0) || !std::isfinite(u1) || !std::isfinite(v0) || !std::isfinite(v1) || u1<=u0 || v1<=v0)
            return refused("unsupported","face_normals_unbounded","A finite trimmed face is required");
        // Midpoint first; then bounded, deterministic interior UV grids. Every
        // candidate must be strictly IN the trimmed face, never merely on it.
        for(int denominator:{2,4,8,16})for(int i=1;i<denominator;++i)for(int j=1;j<denominator;++j){
            if(denominator>2 && i%2==0 && j%2==0)continue;
            const double u=u0+(u1-u0)*i/denominator,v=v0+(v1-v0)*j/denominator;
            BRepClass_FaceClassifier classifier(face,gp_Pnt2d(u,v),1e-8);
            if(classifier.State()!=TopAbs_IN)continue;
            gp_Pnt point;gp_Vec du,dv;surface.D1(u,v,point,du,dv);auto normal=du.Crossed(dv);
            if(!std::isfinite(normal.SquareMagnitude()) || normal.SquareMagnitude()<1e-20)continue;
            normal.Normalize();if(face.Orientation()==TopAbs_REVERSED)normal.Reverse();
            Bnd_Box box;BRepBndLib::AddOptimal(face,box,false,false);double x0,y0,z0,x1,y1,z1;box.Get(x0,y0,z0,x1,y1,z1);
            return {{"status","ready"},{"surfaceType",surface.GetType()==GeomAbs_Plane ? "plane":"cylinder"},
                {"uv",QJsonArray{u,v}},{"pointMm",QJsonArray{point.X(),point.Y(),point.Z()}},
                {"unitNormal",QJsonArray{normal.X(),normal.Y(),normal.Z()}},
                {"orientation",face.Orientation()==TopAbs_REVERSED ? "reversed":"forward"},
                {"faceDiagonalMm",gp_Pnt(x0,y0,z0).Distance(gp_Pnt(x1,y1,z1))}};
        }
        return refused("unsupported","face_normals_no_interior_sample","No nonsingular interior UV sample was found in the bounded grid");
    }catch(const Standard_Failure& error){return refused("unsupported","face_normals_evaluation",QString::fromUtf8(error.GetMessageString()));}
}
QJsonObject FaceNormalInspector::evaluate(const std::vector<EvaluatedBody>& bodies,const QJsonObject& source) {
    const TopoDS_Face* face=nullptr;const EvaluatedBody* owner=nullptr;int matches=0;
    for(const auto& body:bodies)if(body.visible && body.bodyId==source.value("bodyId").toString()
        && (!source.contains("occurrenceId") || body.occurrenceId==source.value("occurrenceId").toString()))
        for(const auto& candidate:body.supportFaces)if(candidate.faceId==source.value("faceId").toString()
            && candidate.producerFeatureId==source.value("producerFeatureId").toString()){face=&candidate.face;owner=&body;++matches;}
    if(matches!=1)return refused("stale",matches>1 ? "face_normals_ambiguous":"face_normals_stale",matches>1 ? "Specify one exact face occurrence":"The selected face is hidden or no longer current");
    bool belongs=false;for(TopExp_Explorer it(owner->shape,TopAbs_FACE);it.More();it.Next())if(it.Current().IsSame(*face)){belongs=true;break;}
    if(!belongs)return refused("stale","face_normals_stale","The selected retained face is not part of the current body");
    auto result=sample(*face);result["source"]=source;return result;
}
