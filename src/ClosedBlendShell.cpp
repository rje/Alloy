// SPDX-License-Identifier: MIT
#include "ClosedBlendShell.h"
#include "PrimitiveTopology.h"
#include "PlanarFaces.h"
#include "SolidFillet.h"
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepFilletAPI_MakeFillet.hxx>
#include <BRepOffsetAPI_MakeOffsetShape.hxx>
#include <BRepBuilderAPI_MakeSolid.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRep_Tool.hxx>
#include <BRepTools.hxx>
#include <BRepLib.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <TopoDS.hxx>
#include <Geom2dAdaptor_Curve.hxx>
#include <Precision.hxx>
#include <QMap>
#include <QSet>
#include <QUuid>
#include <array>
#include <cmath>
#include <optional>
#include <algorithm>
#include <limits>
#include <numbers>

namespace {
struct BoxBlend {
    double x,y,z,w,d,h,R,r,t,eps;
    QString stockId;
};
[[noreturn]] void fail(const QString& featureId,const QString& code,const QString& message){
    throw GeometryKernel::EvaluationError({{"featureId",featureId},{"code",code},{"message",message}});
}
QJsonArray xyz(const gp_Pnt& p){return {p.X(),p.Y(),p.Z()};}
gp_Pnt point(const QJsonArray& p){return {p[0].toDouble(),p[1].toDouble(),p[2].toDouble()};}
QString uid(const QString& featureId,const QByteArray& role){
    return QUuid::createUuidV5(QUuid(featureId),role).toString(QUuid::WithoutBraces);
}
bool near(double a,double b,double e){return std::abs(a-b)<=e;}
bool near(const gp_Pnt& a,const gp_Pnt& b,double e){return a.Distance(b)<=e;}
bool parallel(const gp_Dir& a,const gp_Dir& b){return std::abs(a.Dot(b))>1-1e-8;}
std::optional<BoxBlend> ancestry(const EvaluatedBody& body,const QJsonObject& shell,const QJsonArray& features,int index,
    const EvaluatedDocument& evaluated){
    if(!shell["removedFace"].toObject().isEmpty() || index<2)return {};
    const QString target=body.bodyId;
    int stockIndex=-1,filletIndex=-1;
    for(int j=0;j<index;++j){
        const auto f=features[j].toObject(),p=f["parameters"].toObject();
        if((f["type"]=="box" || f["type"]=="extrude") && f["bodyId"]==target){
            if(stockIndex>=0)return {};stockIndex=j;
        }
        if(f["type"]=="fillet" && p["targetBodyId"]==target){
            if(filletIndex>=0)return {};filletIndex=j;
        }
    }
    if(stockIndex<0 || filletIndex<=stockIndex
        || body.featureId!=features[stockIndex].toObject()["id"].toString())return {};
    const auto stock=features[stockIndex].toObject(),fillet=features[filletIndex].toObject();
    if(body.summary["operation"]!="fillet" || body.summary["lastModifierFeatureId"]!=fillet["id"])
        return {};
    // No intervening operation may touch this body's exact stock lineage.
    for(int j=stockIndex+1;j<index;++j){
        if(j==filletIndex)continue;
        const auto f=features[j].toObject(),p=f["parameters"].toObject();
        if(p["targetBodyId"]==target || p["sourceBodyId"]==target || p["targetBodyIds"].toArray().contains(target)
            || f["bodyId"]==target)return {};
    }
    double x=0,y=0,z=0,w=0,d=0,h=0;
    QString profileId;
    QSet<QString> vertical,rims;
    const auto sp=stock["parameters"].toObject();
    if(stock["type"]=="box"){
        x=sp["xMm"].toDouble();y=sp["yMm"].toDouble();z=sp["zMm"].toDouble();
        w=sp["widthMm"].toDouble();d=sp["depthMm"].toDouble();h=sp["heightMm"].toDouble();
        std::vector<EvaluatedSupportFace> boxFaces;std::vector<EvaluatedSolidEdge> boxEdges;
        PrimitiveTopology::box(GeometryKernel::primitive("box",sp),sp,stock["id"].toString(),target,boxFaces,boxEdges);
        for(const auto& edge:boxEdges)
            (edge.provenance["boxEdge"].toString().contains("z*")?vertical:rims).insert(edge.edgeId);
    }else{
        if(sp["operation"].toString("newBody")!="newBody" || sp["extent"].toString("distance")!="distance"
            || sp["start"].toString("profilePlane")!="profilePlane" || sp.contains("profileIds")
            || !sp["profileId"].isString() || sp["distanceMm"].toDouble()<=0)return {};
        profileId=sp["profileId"].toString();h=sp["distanceMm"].toDouble();
        const auto sketchId=sp["sketchId"].toString();
        QJsonObject sketchFeature;
        for(int j=0;j<stockIndex;++j)if(features[j].toObject()["id"]==sketchId){sketchFeature=features[j].toObject();break;}
        if(sketchFeature["type"]!="sketch" || sketchFeature["parameters"].toObject()["plane"]!="XY")return {};
        const auto sketch=std::find_if(evaluated.sketches.begin(),evaluated.sketches.end(),
            [&](const auto& item){return item.featureId==sketchId;});
        if(sketch==evaluated.sketches.end())return {};
        const auto profile=std::find_if(sketch->profiles.begin(),sketch->profiles.end(),
            [&](const auto& item){return item.profileId==profileId;});
        if(profile==sketch->profiles.end() || profile->boundaryEdges.size()!=4)return {};
        TopTools_IndexedMapOfShape vertices,edges;
        TopExp::MapShapes(profile->face,TopAbs_VERTEX,vertices);
        TopExp::MapShapes(profile->face,TopAbs_EDGE,edges);
        if(vertices.Extent()!=4 || edges.Extent()!=4)return {};
        x=std::numeric_limits<double>::infinity();y=x;z=x;
        double x1=-x,y1=-x;
        for(int i=1;i<=vertices.Extent();++i){
            const auto p=BRep_Tool::Pnt(TopoDS::Vertex(vertices(i)));
            x=std::min(x,p.X());x1=std::max(x1,p.X());
            y=std::min(y,p.Y());y1=std::max(y1,p.Y());z=std::min(z,p.Z());
        }
        w=x1-x;d=y1-y;
        const double tiny=std::max(20*Precision::Confusion(),std::max({w,d,h,1.0})*1e-9);
        QSet<int> cornersSeen;
        for(int i=1;i<=vertices.Extent();++i){
            const auto p=BRep_Tool::Pnt(TopoDS::Vertex(vertices(i)));
            if(!near(p.Z(),z,tiny))return {};
            const int sx=near(p.X(),x,tiny)?0:near(p.X(),x1,tiny)?1:-1;
            const int sy=near(p.Y(),y,tiny)?0:near(p.Y(),y1,tiny)?1:-1;
            if(sx<0 || sy<0 || cornersSeen.contains(sx*2+sy))return {};
            cornersSeen.insert(sx*2+sy);
        }
        if(cornersSeen.size()!=4)return {};
        for(int i=1;i<=edges.Extent();++i){
            const BRepAdaptor_Curve c(TopoDS::Edge(edges(i)));
            if(c.GetType()!=GeomAbs_Line)return {};
            const auto a=c.Value(c.FirstParameter()),b=c.Value(c.LastParameter());
            if(!near(a.Z(),z,tiny)||!near(b.Z(),z,tiny)
                || (near(a.X(),b.X(),tiny)==near(a.Y(),b.Y(),tiny)))return {};
        }
        // The exact prism-edge source roles are retained through Fillet even
        // when the resulting edge itself is consumed by a blend.
        for(const auto& edge:body.solidEdges){
            const auto p=edge.provenance;
            if(p["generatorFeatureId"]!=stock["id"] || p["sourceProfileId"]!=profileId)return {};
            const auto role=p["role"].toString();
            if(role=="sideCorner")vertical.insert(edge.edgeId);
            else if(role=="baseRim" || role=="endRim")rims.insert(edge.edgeId);
            else return {};
        }
    }
    if(vertical.size()!=4 || rims.size()!=8)return {};
    const auto fp=fillet["parameters"].toObject();const auto groups=fp["groups"].toArray();
    if(groups.size()!=2)return {};
    double R=-1,r=-1;QSet<QString> seen;
    for(const auto& value:groups){
        const auto group=value.toObject();const auto ids=group["edgeIds"].toArray();
        if(!group["tangentChain"].toBool() || ids.isEmpty())return {};
        QSet<QString> got;
        for(const auto& id:ids){
            if(!id.isString() || got.contains(id.toString()) || seen.contains(id.toString()))return {};
            got.insert(id.toString());seen.insert(id.toString());
        }
        if(got==vertical)R=group["radiusMm"].toDouble();
        else if(got==rims)r=group["radiusMm"].toDouble();
        else return {};
    }
    if(R<0 || r<0)return {};
    const double t=shell["thicknessMm"].toDouble();
    const double scale=std::max({w,d,h,std::abs(x),std::abs(y),std::abs(z),1.0});
    const double eps=std::max(20*Precision::Confusion(),scale*1e-9);
    if(eps>1e-4 || !std::isfinite(t) || !(r>eps && r+eps<R && R+eps<std::min(w,d)/2)
        || !(h>2*r+eps && t>=r && t+eps<R && h>2*t+eps && w>2*t+eps && d>2*t+eps))
        return {}; // Outside the analytic extension; use the existing Shell authority.
    return BoxBlend{x,y,z,w,d,h,R,r,t,eps,stock["id"].toString()};
}
TopoDS_Shape asSolid(const TopoDS_Shape& shape,const QString& featureId){
    if(shape.ShapeType()==TopAbs_SOLID)return shape;
    if(shape.ShapeType()!=TopAbs_SHELL)fail(featureId,"shell_blend_kernel","Canonical offset did not produce one shell");
    auto solid=BRepBuilderAPI_MakeSolid(TopoDS::Shell(shape)).Solid();
    if(!BRepLib::OrientClosedSolid(solid))fail(featureId,"shell_blend_kernel","Canonical shell orientation is invalid");
    return solid;
}
TopoDS_Shape roundedCore(const BoxBlend& b,const QString& featureId){
    const auto raw=BRepPrimAPI_MakeBox(gp_Pnt(b.x+b.r,b.y+b.r,b.z+b.r),b.w-2*b.r,b.d-2*b.r,b.h-2*b.r).Shape();
    BRepFilletAPI_MakeFillet fillet(raw);int vertical=0;
    TopTools_IndexedMapOfShape edges;TopExp::MapShapes(raw,TopAbs_EDGE,edges);
    for(int i=1;i<=edges.Extent();++i){
        const auto edge=TopoDS::Edge(edges(i));BRepAdaptor_Curve curve(edge);
        if(curve.GetType()!=GeomAbs_Line)continue;
        const auto a=curve.Value(curve.FirstParameter()),c=curve.Value(curve.LastParameter());
        if(near(a.X(),c.X(),b.eps) && near(a.Y(),c.Y(),b.eps) && near(std::abs(a.Z()-c.Z()),b.h-2*b.r,b.eps)){
            fillet.Add(b.R-b.r,edge);++vertical;
        }
    }
    if(vertical!=4)fail(featureId,"shell_blend_kernel","Canonical core has no unique four vertical edges");
    fillet.Build();if(!fillet.IsDone())fail(featureId,"shell_blend_kernel","Canonical rounded core failed");
    const auto shape=fillet.Shape();const auto summary=GeometryKernel::summarize(shape);
    if(!summary["valid"].toBool() || !summary["closed"].toBool() || summary["topology"].toObject()["solids"].toInt()!=1)
        fail(featureId,"shell_blend_kernel","Canonical rounded core is not one valid closed solid");
    return shape;
}
TopoDS_Shape canonicalOuter(const BoxBlend& b,const TopoDS_Shape& core,const QString& featureId){
    BRepOffsetAPI_MakeOffsetShape offset;
    offset.PerformByJoin(core,b.r,1e-6,BRepOffset_Skin,false,false,GeomAbs_Arc);
    if(!offset.IsDone() || offset.Shape().IsNull())fail(featureId,"shell_blend_kernel","Canonical outward blend failed");
    return asSolid(offset.Shape(),featureId);
}
TopoDS_Shape cavity(const BoxBlend& b,const TopoDS_Shape& core,const QString& featureId){
    const double s=b.t-b.r;if(s<=b.eps)return core;
    BRepOffsetAPI_MakeOffsetShape offset;
    offset.PerformByJoin(core,-s,1e-6,BRepOffset_Skin,false,false,GeomAbs_Intersection);
    if(!offset.IsDone() || offset.Shape().IsNull())fail(featureId,"shell_blend_kernel","Canonical inward core failed");
    return asSolid(offset.Shape(),featureId);
}
struct Role {QString key;int kind;gp_Pnt center;gp_Dir axis;double major=0,minor=0;};
std::vector<Role> roles(const BoxBlend& b){
    std::vector<Role> out;const auto x0=b.x,x1=b.x+b.w,y0=b.y,y1=b.y+b.d,z0=b.z,z1=b.z+b.h;
    out.push_back({"x0",0,gp_Pnt(x0,0,0),gp::DX()});out.push_back({"x1",0,gp_Pnt(x1,0,0),gp::DX()});
    out.push_back({"y0",0,gp_Pnt(0,y0,0),gp::DY()});out.push_back({"y1",0,gp_Pnt(0,y1,0),gp::DY()});
    out.push_back({"z0",0,gp_Pnt(0,0,z0),gp::DZ()});out.push_back({"z1",0,gp_Pnt(0,0,z1),gp::DZ()});
    for(int sx=0;sx<2;++sx)for(int sy=0;sy<2;++sy){
        const auto xc=sx?x1-b.R:x0+b.R,yc=sy?y1-b.R:y0+b.R;
        out.push_back({QString("vertical.%1.%2").arg(sx).arg(sy),1,gp_Pnt(xc,yc,0),gp::DZ(),b.R});
    }
    for(int sy=0;sy<2;++sy)for(int sz=0;sz<2;++sz)
        out.push_back({QString("rimX.%1.%2").arg(sy).arg(sz),1,
            gp_Pnt(0,sy?y1-b.r:y0+b.r,sz?z1-b.r:z0+b.r),gp::DX(),b.r});
    for(int sx=0;sx<2;++sx)for(int sz=0;sz<2;++sz)
        out.push_back({QString("rimY.%1.%2").arg(sx).arg(sz),1,
            gp_Pnt(sx?x1-b.r:x0+b.r,0,sz?z1-b.r:z0+b.r),gp::DY(),b.r});
    for(int sx=0;sx<2;++sx)for(int sy=0;sy<2;++sy)for(int sz=0;sz<2;++sz)
        out.push_back({QString("torus.%1.%2.%3").arg(sx).arg(sy).arg(sz),2,
            gp_Pnt(sx?x1-b.R:x0+b.R,sy?y1-b.R:y0+b.R,sz?z1-b.r:z0+b.r),gp::DZ(),b.R-b.r,b.r});
    return out;
}
double lineDistance(const gp_Pnt& a,const gp_Dir& axis,const gp_Pnt& b){
    return gp_Vec(a,b).Crossed(gp_Vec(axis)).Magnitude();
}
std::optional<QString> roleOf(const TopoDS_Face& face,const std::vector<Role>& expected,double eps){
    const BRepAdaptor_Surface s(face);const auto kind=s.GetType();QString match;
    for(const auto& role:expected){bool good=false;
        if(role.kind==0 && kind==GeomAbs_Plane){
            const auto plane=s.Plane();good=parallel(plane.Axis().Direction(),role.axis)
                && plane.Distance(role.center)<=eps;
        }else if(role.kind==1 && kind==GeomAbs_Cylinder){
            const auto cylinder=s.Cylinder();good=parallel(cylinder.Axis().Direction(),role.axis)
                && near(cylinder.Radius(),role.major,eps)
                && lineDistance(role.center,role.axis,cylinder.Location())<=eps;
        }else if(role.kind==2 && kind==GeomAbs_Torus){
            const auto torus=s.Torus();good=parallel(torus.Axis().Direction(),role.axis)
                && near(torus.MajorRadius(),role.major,eps) && near(torus.MinorRadius(),role.minor,eps)
                && near(torus.Location(),role.center,eps);
        }
        if(good){if(!match.isEmpty())return {};match=role.key;}
    }
    if(match.isEmpty())return {};return match;
}

std::vector<gp_Pnt> corners(const QString& key,const BoxBlend& b){
    const auto x0=b.x,x1=b.x+b.w,y0=b.y,y1=b.y+b.d,z0=b.z,z1=b.z+b.h;
    std::vector<gp_Pnt> out;
    const auto parts=key.split('.');
    if(parts[0]=="x0" || parts[0]=="x1"){
        const auto x=parts[0]=="x0"?x0:x1;
        for(double y:{y0+b.R,y1-b.R})for(double z:{z0+b.r,z1-b.r})out.emplace_back(x,y,z);
    }else if(parts[0]=="y0" || parts[0]=="y1"){
        const auto y=parts[0]=="y0"?y0:y1;
        for(double x:{x0+b.R,x1-b.R})for(double z:{z0+b.r,z1-b.r})out.emplace_back(x,y,z);
    }else if(parts[0]=="z0" || parts[0]=="z1"){
        const auto z=parts[0]=="z0"?z0:z1,q=b.R-b.r;
        for(int sx=0;sx<2;++sx)for(int sy=0;sy<2;++sy){
            const auto cx=sx?x1-b.R:x0+b.R,cy=sy?y1-b.R:y0+b.R;
            out.emplace_back(cx+(sx?1:-1)*q,cy,z);
            out.emplace_back(cx,cy+(sy?1:-1)*q,z);
        }
    }else if(parts[0]=="vertical"){
        const int sx=parts[1].toInt(),sy=parts[2].toInt();
        const auto cx=sx?x1-b.R:x0+b.R,cy=sy?y1-b.R:y0+b.R;
        for(double z:{z0+b.r,z1-b.r}){
            out.emplace_back(cx+(sx?1:-1)*b.R,cy,z);
            out.emplace_back(cx,cy+(sy?1:-1)*b.R,z);
        }
    }else if(parts[0]=="rimX"){
        const int sy=parts[1].toInt(),sz=parts[2].toInt();
        const auto cy=sy?y1-b.r:y0+b.r,cz=sz?z1-b.r:z0+b.r;
        for(double x:{x0+b.R,x1-b.R}){
            out.emplace_back(x,cy+(sy?1:-1)*b.r,cz);
            out.emplace_back(x,cy,cz+(sz?1:-1)*b.r);
        }
    }else if(parts[0]=="rimY"){
        const int sx=parts[1].toInt(),sz=parts[2].toInt();
        const auto cx=sx?x1-b.r:x0+b.r,cz=sz?z1-b.r:z0+b.r;
        for(double y:{y0+b.R,y1-b.R}){
            out.emplace_back(cx+(sx?1:-1)*b.r,y,cz);
            out.emplace_back(cx,y,cz+(sz?1:-1)*b.r);
        }
    }else if(parts[0]=="torus"){
        const int sx=parts[1].toInt(),sy=parts[2].toInt(),sz=parts[3].toInt();
        const auto cx=sx?x1-b.R:x0+b.R,cy=sy?y1-b.R:y0+b.R,cz=sz?z1-b.r:z0+b.r,q=b.R-b.r;
        for(double radius:{b.R,q}){
            const auto z=radius==b.R?cz:(sz?z1:z0);
            out.emplace_back(cx+(sx?1:-1)*radius,cy,z);
            out.emplace_back(cx,cy+(sy?1:-1)*radius,z);
        }
    }
    return out;
}
struct CurveSpan {int kind=-1;gp_Pnt start,end,middle,center;gp_Dir axis=gp::DZ();double radius=0,sweep=0;};
std::optional<CurveSpan> span(const TopoDS_Edge& edge,const BoxBlend& b){
    const BRepAdaptor_Curve c(edge);CurveSpan out;
    const double first=c.FirstParameter(),last=c.LastParameter();
    if(!std::isfinite(first)||!std::isfinite(last)||last-first<=Precision::Confusion())return {};
    out.start=c.Value(first);out.end=c.Value(last);out.middle=c.Value((first+last)/2);
    if(c.GetType()==GeomAbs_Line){out.kind=0;return out;}
    if(c.GetType()!=GeomAbs_Circle)return {};
    const auto circle=c.Circle();out.kind=1;out.center=circle.Location();out.axis=circle.Axis().Direction();
    out.radius=circle.Radius();out.sweep=last-first;
    if(out.sweep>1.571+b.eps || out.sweep<1e-6)return {};
    return out;
}
bool sameSpan(const CurveSpan& a,const CurveSpan& c,double eps){
    if(a.kind!=c.kind || !near(a.start,c.start,eps) && !near(a.start,c.end,eps)
        || !near(a.end,c.end,eps) && !near(a.end,c.start,eps))return false;
    if(a.kind==0)return true;
    return near(a.center,c.center,eps)&&parallel(a.axis,c.axis)
        && near(a.radius,c.radius,eps)&&near(a.sweep,c.sweep,1e-7)
        && near(a.middle,c.middle,eps);
}
struct CatalogFace {TopoDS_Face face;std::vector<CurveSpan> edges;
    std::vector<QSet<QString>> adjacentRoles;gp_Dir normal=gp::DZ();};
using Catalog=QMap<QString,CatalogFace>;
Catalog catalog(const TopoDS_Shape& shape,const BoxBlend& b,const QString& featureId,bool core=false){
    const auto summary=GeometryKernel::summarize(shape);
    if(!summary["valid"].toBool() || !summary["closed"].toBool()
        || summary["topology"].toObject()["solids"].toInt()!=1)
        fail(featureId,"shell_blend_uncertified","Rounded envelope is not one valid closed solid");
    auto expected=roles(b);if(core)expected.resize(10);Catalog out;
    TopTools_IndexedMapOfShape faces;TopExp::MapShapes(shape,TopAbs_FACE,faces);
    if(faces.Extent()!=static_cast<int>(expected.size()))
        fail(featureId,"shell_blend_uncertified","Rounded envelope has an unexpected face inventory");
    TopTools_IndexedDataMapOfShapeListOfShape adjacent;
    TopExp::MapShapesAndAncestors(shape,TopAbs_EDGE,TopAbs_FACE,adjacent);
    for(int fi=1;fi<=faces.Extent();++fi){
        const auto face=TopoDS::Face(faces(fi));const auto role=roleOf(face,expected,b.eps);
        if(!role || out.contains(*role))fail(featureId,"shell_blend_uncertified","Rounded envelope face support is ambiguous");
        TopTools_IndexedMapOfShape wires,vertices,edges;
        TopExp::MapShapes(face,TopAbs_WIRE,wires);TopExp::MapShapes(face,TopAbs_VERTEX,vertices);
        TopExp::MapShapes(face,TopAbs_EDGE,edges);
        const auto wanted=corners(*role,b);
        if(wires.Extent()!=1 || vertices.Extent()!=static_cast<int>(wanted.size())
            || edges.Extent()!=static_cast<int>(wanted.size()))
            fail(featureId,"shell_blend_uncertified","Rounded envelope face has incomplete trim topology");
        QSet<int> assigned;
        for(int vi=1;vi<=vertices.Extent();++vi){
            const auto vertex=BRep_Tool::Pnt(TopoDS::Vertex(vertices(vi)));int match=-1;
            for(size_t wi=0;wi<wanted.size();++wi)if(near(vertex,wanted[wi],b.eps)){
                if(match>=0)fail(featureId,"shell_blend_uncertified","Rounded envelope corner is ambiguous");match=static_cast<int>(wi);
            }
            if(match<0 || assigned.contains(match))fail(featureId,"shell_blend_uncertified","Rounded envelope trim does not span its convex domain");
            assigned.insert(match);
        }
        std::vector<CurveSpan> spans;std::vector<QSet<QString>> edgeRoles;
        Standard_Real u0,u1,v0,v1;BRepTools::UVBounds(face,u0,u1,v0,v1);
        const bool curved=!role->startsWith('x')&&!role->startsWith('y')&&!role->startsWith('z');
        if(curved){
            double expectedV=std::numbers::pi/2;
            if(role->startsWith("vertical"))expectedV=b.h-2*b.r;
            else if(role->startsWith("rimX"))expectedV=b.w-2*b.R;
            else if(role->startsWith("rimY"))expectedV=b.d-2*b.R;
            if(!near(u1-u0,std::numbers::pi/2,1e-6) || !near(v1-v0,expectedV,1e-6))
                fail(featureId,"shell_blend_uncertified","Curved face extends outside its convex quarter domain");
        }
        for(int ei=1;ei<=edges.Extent();++ei){
            const auto edge=TopoDS::Edge(edges(ei));auto curve=span(edge,b);
            if(!curve || adjacent.FindFromKey(edge).Extent()!=2)
                fail(featureId,"shell_blend_uncertified","Rounded envelope trim is not a complete analytic manifold");
            QSet<QString> pair;
            for(const auto& neighbor:adjacent.FindFromKey(edge)){
                const auto other=roleOf(TopoDS::Face(neighbor),expected,b.eps);
                if(!other)fail(featureId,"shell_blend_uncertified","Rounded boundary has an unknown adjacent face");
                pair.insert(*other);
            }
            if(pair.size()!=2 || !pair.contains(*role))
                fail(featureId,"shell_blend_uncertified","Rounded boundary face incidence is ambiguous");
            if(curved){
                Standard_Real first,last;const auto pc=BRep_Tool::CurveOnSurface(edge,face,first,last);
                if(pc.IsNull())fail(featureId,"shell_blend_uncertified","Curved trim lacks exact surface parameters");
                const Geom2dAdaptor_Curve planar(pc,first,last);
                if(planar.GetType()!=GeomAbs_Line)
                    fail(featureId,"shell_blend_uncertified","Curved trim is not an iso-parametric segment");
                const auto a=planar.Value(first),c=planar.Value(last);
                const auto covers=[](double first,double last,double low,double high){
                    return near(first,low,1e-6)&&near(last,high,1e-6)
                        || near(first,high,1e-6)&&near(last,low,1e-6);
                };
                const bool edgeOfRectangle=(near(a.X(),c.X(),1e-6)
                    && (near(a.X(),u0,1e-6)||near(a.X(),u1,1e-6))
                    && covers(a.Y(),c.Y(),v0,v1))
                    || (near(a.Y(),c.Y(),1e-6)
                    && (near(a.Y(),v0,1e-6)||near(a.Y(),v1,1e-6))
                    && covers(a.X(),c.X(),u0,u1));
                if(!edgeOfRectangle)
                    fail(featureId,"shell_blend_uncertified","Curved trim does not cover one full domain side");
            }
            spans.push_back(*curve);
            edgeRoles.push_back(pair);
        }
        const BRepAdaptor_Surface surface(face);
        gp_Pnt sample;gp_Vec du,dv;
        surface.D1((u0+u1)/2,(v0+v1)/2,sample,du,dv);
        auto cross=du.Crossed(dv);
        if(cross.Magnitude()<1e-8)fail(featureId,"shell_blend_uncertified","Rounded face orientation is singular");
        if(face.Orientation()==TopAbs_REVERSED)cross.Reverse();
        out.insert(*role,{face,spans,edgeRoles,gp_Dir(cross)});
    }
    return out;
}
void certify(const TopoDS_Shape& actual,const TopoDS_Shape& canonical,const BoxBlend& b,const QString& featureId){
    const auto a=catalog(actual,b,featureId),c=catalog(canonical,b,featureId);
    if(a.size()!=c.size())fail(featureId,"shell_blend_uncertified","Rounded face support does not match canonical envelope");
    for(auto it=a.cbegin();it!=a.cend();++it){
        if(!c.contains(it.key()))fail(featureId,"shell_blend_uncertified","Rounded face role has no canonical mate");
        const auto mate=c.value(it.key());const auto& expected=mate.edges;
        if(it.value().normal.Dot(mate.normal)<1-1e-7)
            fail(featureId,"shell_blend_uncertified","Rounded face orientation differs from canonical convex envelope");
        QSet<int> used;
        for(size_t ai=0;ai<it.value().edges.size();++ai){int match=-1;
            for(size_t n=0;n<expected.size();++n)if(sameSpan(it.value().edges[ai],expected[n],b.eps)
                && it.value().adjacentRoles[ai]==mate.adjacentRoles[n]){
                if(match>=0)fail(featureId,"shell_blend_uncertified","Rounded trim match is ambiguous");match=static_cast<int>(n);
            }
            if(match<0 || used.contains(match))fail(featureId,"shell_blend_uncertified","Rounded trim does not match canonical envelope");
            used.insert(match);
        }
    }
}

BoxBlend innerBox(const BoxBlend& b){
    auto out=b;out.x+=b.t;out.y+=b.t;out.z+=b.t;
    out.w-=2*b.t;out.d-=2*b.t;out.h-=2*b.t;
    out.R=b.R-b.t;out.r=0;return out;
}
QString innerEndpoint(const gp_Pnt& p,const BoxBlend& b,const QString& featureId){
    const auto coordinate=[&](double value,double low,double high){
        const std::array<double,4> levels{low,low+b.R,high-b.R,high};
        int found=-1;
        for(int i=0;i<4;++i)if(near(value,levels[i],b.eps)){
            if(found>=0)fail(featureId,"shell_blend_uncertified","Inner endpoint has ambiguous semantic level");
            found=i;
        }
        if(found<0)fail(featureId,"shell_blend_uncertified","Inner endpoint leaves the rounded cavity domain");
        return found;
    };
    const auto x=coordinate(p.X(),b.x,b.x+b.w),y=coordinate(p.Y(),b.y,b.y+b.d);
    const auto z=near(p.Z(),b.z,b.eps)?0:near(p.Z(),b.z+b.h,b.eps)?1:-1;
    if(z<0)fail(featureId,"shell_blend_uncertified","Inner endpoint has no axial role");
    return QString("x%1y%2z%3").arg(x).arg(y).arg(z);
}
std::vector<EvaluatedSupportFace> innerSupports(const EvaluatedBody& body,const TopoDS_Shape& result,
    const TopoDS_Shape& voidShape,const BoxBlend& b,const QString& featureId){
    const auto ib=innerBox(b);const auto voidCatalog=catalog(voidShape,ib,featureId,true);
    TopTools_IndexedMapOfShape faces;TopExp::MapShapes(result,TopAbs_FACE,faces);
    QMap<QString,TopoDS_Face> cutFaces;
    auto innerRoles=roles(ib);innerRoles.resize(6);
    for(int fi=1;fi<=faces.Extent();++fi){
        const auto face=TopoDS::Face(faces(fi));const auto role=roleOf(face,innerRoles,b.eps);
        if(!role)continue;
        if(cutFaces.contains(*role))fail(featureId,"shell_blend_uncertified","Inner planar face is ambiguous");
        cutFaces.insert(*role,face);
    }
    if(cutFaces.size()!=6)fail(featureId,"shell_blend_uncertified","Inner planar face inventory is incomplete");
    QMap<QString,EvaluatedSupportFace> sources;
    for(const auto& source:body.supportFaces){
        const auto frame=source.frame;const auto origin=frame["origin"].toArray(),normal=frame["normal"].toArray();
        if(origin.size()!=3 || normal.size()!=3)continue;
        const auto p=point(origin);const gp_Dir n(normal[0].toDouble(),normal[1].toDouble(),normal[2].toDouble());
        QString role;
        if(n.Dot(gp::DX().Reversed())>1-1e-7 && near(p.X(),b.x,b.eps))role="x0";
        else if(n.Dot(gp::DX())>1-1e-7 && near(p.X(),b.x+b.w,b.eps))role="x1";
        else if(n.Dot(gp::DY().Reversed())>1-1e-7 && near(p.Y(),b.y,b.eps))role="y0";
        else if(n.Dot(gp::DY())>1-1e-7 && near(p.Y(),b.y+b.d,b.eps))role="y1";
        else if(n.Dot(gp::DZ().Reversed())>1-1e-7 && near(p.Z(),b.z,b.eps))role="z0";
        else if(n.Dot(gp::DZ())>1-1e-7 && near(p.Z(),b.z+b.h,b.eps))role="z1";
        if(role.isEmpty())continue;
        if(sources.contains(role))fail(featureId,"shell_blend_uncertified","Original planar support identity is ambiguous");
        sources.insert(role,source);
    }
    if(sources.size()!=6)fail(featureId,"shell_blend_uncertified","Six original planar face lineages are required");
    std::vector<EvaluatedSupportFace> output;
    QMap<QString,TopoDS_Edge> namedEdges;
    for(auto it=sources.cbegin();it!=sources.cend();++it){
        const auto role=it.key();const auto source=it.value();
        const auto face=cutFaces.value(role);
        const auto innerCorners=corners(role,ib);
        TopTools_IndexedMapOfShape edges;TopExp::MapShapes(face,TopAbs_EDGE,edges);
        if(edges.Extent()!=static_cast<int>(innerCorners.size()))
            fail(featureId,"shell_blend_uncertified","Inner plane trim is incomplete");
        const auto canonical=voidCatalog.value(role);
        QSet<int> used;
        auto child=source;child.face=face;child.producerFeatureId=featureId;
        child.role="shell-inner";
        child.faceId=uid(featureId,QByteArray("alloy-shell-inner-face/1\n")+source.producerFeatureId.toUtf8()+'\n'+source.faceId.toUtf8());
        const auto origin=point(source.frame["origin"].toArray());
        const auto direction=point(source.frame["normal"].toArray());
        auto frame=source.frame;
        frame["origin"]=xyz(origin.Translated(gp_Vec(direction.X(),direction.Y(),direction.Z()).Multiplied(-b.t)));
        frame["normal"]=QJsonArray{-direction.X(),-direction.Y(),-direction.Z()};
        const auto v=frame["vAxis"].toArray();
        if(v.size()==3)frame["vAxis"]=QJsonArray{-v[0].toDouble(),-v[1].toDouble(),-v[2].toDouble()};
        child.frame=frame;child.boundaryEdges.clear();
        for(int ei=1;ei<=edges.Extent();++ei){
            const auto edge=TopoDS::Edge(edges(ei));auto curve=span(edge,b);
            if(!curve)fail(featureId,"shell_blend_uncertified","Inner plane edge is not analytic");
            int matched=-1;
            for(size_t ci=0;ci<canonical.edges.size();++ci)if(sameSpan(*curve,canonical.edges[ci],b.eps)){
                if(matched>=0)fail(featureId,"shell_blend_uncertified","Inner plane edge has ambiguous void lineage");
                matched=static_cast<int>(ci);
            }
            if(matched<0 || used.contains(matched))fail(featureId,"shell_blend_uncertified","Inner plane edge has no unique void lineage");
            used.insert(matched);
            auto vertexIndex=[&](const gp_Pnt& p){int found=-1;
                for(size_t i=0;i<innerCorners.size();++i)if(near(p,innerCorners[i],b.eps)){
                    if(found>=0)fail(featureId,"shell_blend_uncertified","Inner corner is ambiguous");
                    found=static_cast<int>(i);
                }
                if(found<0)fail(featureId,"shell_blend_uncertified","Inner edge misses a semantic corner");
                return found;
            };
            const auto a=vertexIndex(curve->start),c=vertexIndex(curve->end);
            if(a==c)fail(featureId,"shell_blend_uncertified","Inner edge has coincident corner roles");
            QString first=innerEndpoint(curve->start,ib,featureId),last=innerEndpoint(curve->end,ib,featureId);
            if(last<first)std::swap(first,last);
            const auto semantic=first+"/"+last;
            const auto edgeId=uid(featureId,QByteArray("alloy-shell-inner-edge/3\n")+b.stockId.toUtf8()+'\n'+semantic.toUtf8());
            if(namedEdges.contains(edgeId) && !namedEdges.value(edgeId).IsSame(edge))
                fail(featureId,"shell_blend_uncertified","One inner edge identity maps to distinct kernel edges");
            namedEdges.insert(edgeId,edge);
            QJsonObject provenance{{"role","shell.innerEdge"},{"generatorFeatureId",featureId},
                {"endpointProvenance","currentKernelBoundary"},{"semantic",semantic},
                {"startMm",xyz(curve->start)},{"endMm",xyz(curve->end)}};
            child.boundaryEdges.push_back({edgeId,curve->kind==0?"line":"circle",edge,provenance});
        }
        output.push_back(child);
    }
    if(output.size()!=6)fail(featureId,"shell_blend_uncertified","Six original planar face lineages are required");
    return output;
}
}

bool ClosedBlendShell::evaluate(EvaluatedBody& body,const QJsonObject& parameters,
    const QString& featureId,const QJsonArray& features,int shellIndex,const EvaluatedDocument& evaluated){
    const auto recognized=ancestry(body,parameters,features,shellIndex,evaluated);
    if(!recognized)return false;
    const auto b=*recognized;
    try {
        const auto core=roundedCore(b,featureId);
        const auto outer=canonicalOuter(b,core,featureId);
        certify(body.shape,outer,b,featureId);
        const auto voidShape=cavity(b,core,featureId);
        const auto ib=innerBox(b);
        catalog(voidShape,ib,featureId,true);
        const double expectedVoid=(ib.w*ib.d-(4-M_PI)*ib.R*ib.R)*ib.h;
        const auto voidSummary=GeometryKernel::summarize(voidShape);
        if(!near(voidSummary["volumeMm3"].toDouble(),expectedVoid,
            std::max(1e-4,expectedVoid*1e-8)) || !(expectedVoid>0)
            || !(expectedVoid<body.summary["volumeMm3"].toDouble()))
            fail(featureId,"shell_blend_uncertified","Inner cavity volume disagrees with analytic domain");
        BRepAlgoAPI_Cut cut(body.shape,voidShape);
        if(!cut.IsDone())fail(featureId,"shell_blend_kernel","Could not subtract certified convex cavity");
        const auto result=cut.Shape();auto summary=GeometryKernel::summarize(result);
        if(!summary["valid"].toBool() || !summary["closed"].toBool()
            || summary["topology"].toObject()["solids"].toInt()!=1
            || summary["topology"].toObject()["shells"].toInt()!=2)
            fail(featureId,"shell_blend_kernel","Closed blend Shell is not one valid hollow solid");
        TopTools_IndexedMapOfShape resultFaces,originalFaces;
        TopExp::MapShapes(result,TopAbs_FACE,resultFaces);
        TopExp::MapShapes(body.shape,TopAbs_FACE,originalFaces);
        for(int i=1;i<=originalFaces.Extent();++i)if(!resultFaces.Contains(originalFaces(i)))
            fail(featureId,"shell_blend_kernel","Closed Shell changed an original outer face");
        for(const auto& face:body.supportFaces)if(!resultFaces.Contains(face.face))
            fail(featureId,"shell_blend_kernel","Closed Shell changed a retained outer face");
        auto retained=body.supportFaces;
        auto derived=innerSupports(body,result,voidShape,b,featureId);
        retained.insert(retained.end(),derived.begin(),derived.end());
        body.shape=result;body.supportFaces=std::move(retained);
        summary["supportFaces"]=PlanarFaces::descriptors(body.supportFaces);
        summary["solidEdges"]=SolidFillet::descriptors(body.solidEdges);
        summary["lastModifierFeatureId"]=featureId;summary["producerFeatureId"]=body.featureId;
        summary["operation"]="shell";summary["thicknessMm"]=b.t;
        summary["removedFace"]=QJsonObject{};summary["closedCavity"]=true;
        body.summary=summary;
        return true;
    }catch(const Standard_Failure&){
        fail(featureId,"shell_blend_kernel","Certified rounded Box cavity could not be built");
    }
}
