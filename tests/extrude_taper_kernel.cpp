// SPDX-License-Identifier: MIT
#include "GeometryKernel.h"
#include "SketchGeometry.h"
#include <BRepAdaptor_Surface.hxx>
#include <BRepBndLib.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRep_Tool.hxx>
#include <TopoDS_Vertex.hxx>
#include <Bnd_Box.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <QCoreApplication>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <limits>
#include <numbers>
#include <stdexcept>
namespace {
void check(bool v,const char* m){if(!v)throw std::runtime_error(m);}
void near(double a,double b){if(std::abs(a-b)>1e-5)throw std::runtime_error("Mismatch: "+std::to_string(a)+" != "+std::to_string(b));}
EvaluatedProfile polygon(std::vector<std::pair<double,double>> points){
    auto sketch=SketchGeometry::newFeature("98e750c3-53d3-4a78-923a-a9925762ab21","XY");
    for(size_t i=0;i<points.size();++i){auto a=points[i],b=points[(i+1)%points.size()];
        sketch=SketchGeometry::addLine(sketch,{{"u1Mm",a.first},{"v1Mm",a.second},{"u2Mm",b.first},{"v2Mm",b.second}});}
    auto result=SketchProfiles::derive(sketch);check(result.ok,"derive polygon");return result.profile;
}
void fixture(const EvaluatedProfile& p,double h,double angle,double volume,double topW,double topH){
    EvaluatedSupportFace cap;const auto shape=GeometryKernel::taperedExtrusion(p,h,angle,&cap);
    check(BRepCheck_Analyzer(shape).IsValid(),"valid BRep");
    TopTools_IndexedMapOfShape solids;TopExp::MapShapes(shape,TopAbs_SOLID,solids);check(solids.Extent()==1,"one solid");
    auto summary=GeometryKernel::summarize(shape);near(summary.value("volumeMm3").toDouble(),volume);
    near(summary.value("boundsMm").toObject().value("min").toArray()[2].toDouble(),0);
    near(summary.value("boundsMm").toObject().value("max").toArray()[2].toDouble(),h);
    TopTools_IndexedMapOfShape faces;TopExp::MapShapes(shape,TopAbs_FACE,faces);check(faces.Contains(cap.face),"actual retained cap");
    check(cap.role=="extrude.endCap" && cap.boundaryEdges.size()==p.boundaryEdges.size(),"cap lineage");
    near(cap.frame.value("origin").toArray()[2].toDouble(),h);
    Bnd_Box bounds;BRepBndLib::AddOptimal(cap.face,bounds,false,false);double x0,y0,z0,x1,y1,z1;bounds.Get(x0,y0,z0,x1,y1,z1);
    near(x1-x0,topW);near(y1-y0,topH);near(z0,h);near(z1,h);
    for(const auto& edge:cap.boundaryEdges){
        TopoDS_Vertex a,b;TopExp::Vertices(edge.edge,a,b,true);
        for(const char* key:{"startMm","endMm"}){const auto v=edge.provenance.value(key).toArray();const gp_Pnt point(v[0].toDouble(),v[1].toDouble(),v[2].toDouble());
            check(std::min(point.Distance(BRep_Tool::Pnt(a)),point.Distance(BRep_Tool::Pnt(b)))<1e-7,"cap endpoint provenance on actual edge");}
    }
    for(TopExp_Explorer f(shape,TopAbs_FACE);f.More();f.Next()){
        const BRepAdaptor_Surface surface(TopoDS::Face(f.Current()));check(surface.GetType()==GeomAbs_Plane,"analytic planar walls");
        const double vertical=std::abs(surface.Plane().Axis().Direction().Z());
        if(vertical<1-1e-8)near(vertical,std::abs(std::sin(angle*std::numbers::pi/180)));
    }
    std::cout<<std::setprecision(12)<<"angle="<<angle<<" h="<<h<<" volume="<<summary.value("volumeMm3").toDouble()<<" top="<<x1-x0<<" x "<<y1-y0<<'\n';
}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    const auto rectangle=polygon({{-20,-10},{20,-10},{20,10},{-20,10}});
    for(double angle:{-5.,0.,5.}){const double t=std::tan(angle*std::numbers::pi/180);
        fixture(rectangle,10,angle,10*(800+60*t*10+4./3*t*t*100),40+20*t,20+20*t);}
    const auto l=polygon({{0,0},{40,0},{40,10},{10,10},{10,30},{0,30}});
    const double t=std::tan(5*std::numbers::pi/180);
    fixture(l,8,-5,4800-70*t*64+4./3*t*t*512,40-16*t,30-16*t);
    EvaluatedSupportFace before,after;GeometryKernel::taperedExtrusion(rectangle,10,-5,&before);GeometryKernel::taperedExtrusion(rectangle,10,-3,&after);
    check(before.frame==after.frame,"stable cap frame on edit");
    for(size_t i=0;i<before.boundaryEdges.size();++i)check(before.boundaryEdges[i].edgeId==after.boundaryEdges[i].edgeId,"stable edge identity on edit");
    for(double angle:{-80.,-90.,90.,std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity()}){
        bool refused=false;auto preserved=before;try{GeometryKernel::taperedExtrusion(rectangle,10,angle,&preserved);}catch(const GeometryKernel::EvaluationError&){refused=true;}
        check(refused,"invalid/collapsed taper refused");check(preserved.face.IsSame(before.face),"output unchanged on failure");}
    std::cout<<"PASS tapered polygon kernel and far-cap lineage\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
