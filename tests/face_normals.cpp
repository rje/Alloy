// SPDX-License-Identifier: MIT
#include "FaceNormalInspector.h"
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRep_Builder.hxx>
#include <Geom_CylindricalSurface.hxx>
#include <gp_Cylinder.hxx>
#include <gp_Pln.hxx>
#include <gp_Sphere.hxx>
#include <TopoDS.hxx>
#include <QCoreApplication>
#include <cmath>
#include <iostream>
#include <numbers>
namespace {
void check(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
void near(double a,double b){check(std::abs(a-b)<1e-8,"analytic component mismatch");}
QJsonArray point(const QJsonObject& sample){check(sample.value("status")=="ready","sample not ready");return sample.value("pointMm").toArray();}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    const auto plane=BRepBuilderAPI_MakeFace(gp_Pln(gp_Pnt(0,0,0),gp_Dir(0,0,1)),0,40,0,30).Face();
    const auto sample=FaceNormalInspector::sample(plane);const auto p=point(sample);near(p[0].toDouble(),20);near(p[1].toDouble(),15);near(p[2].toDouble(),0);
    const auto n=sample.value("unitNormal").toArray();near(n[0].toDouble(),0);near(n[1].toDouble(),0);near(n[2].toDouble(),1);
    near(FaceNormalInspector::sample(TopoDS::Face(plane.Reversed())).value("unitNormal").toArray()[2].toDouble(),-1);
    gp_Trsf transform;transform.SetRotation(gp_Ax1(gp_Pnt(0,0,0),gp_Dir(1,0,0)),std::numbers::pi/2);transform.SetTranslationPart(gp_Vec(2,3,4));
    const auto placed=FaceNormalInspector::sample(TopoDS::Face(plane.Moved(TopLoc_Location(transform))));const auto pn=placed.value("unitNormal").toArray();near(pn[1].toDouble(),-1);near(pn[2].toDouble(),0);near(point(placed)[2].toDouble(),19);
    for(double theta:{-std::numbers::pi/6,std::numbers::pi/6}){
        const auto cylinder=BRepBuilderAPI_MakeFace(gp_Cylinder(gp_Ax3(),10),theta-.2,theta+.2,0,30).Face();
        const auto s=FaceNormalInspector::sample(cylinder);const auto cp=point(s),cn=s.value("unitNormal").toArray();
        near(cp[0].toDouble(),10*std::cos(theta));near(cp[1].toDouble(),10*std::sin(theta));near(cp[2].toDouble(),15);
        near(cn[0].toDouble(),std::cos(theta));near(cn[1].toDouble(),std::sin(theta));near(cn[2].toDouble(),0);
    }
    // A central hole excludes UV midpoint: fallback must stay strictly inside.
    BRepBuilderAPI_MakePolygon outer;outer.Add({0,0,0});outer.Add({40,0,0});outer.Add({40,30,0});outer.Add({0,30,0});outer.Close();
    BRepBuilderAPI_MakePolygon hole;hole.Add({15,10,0});hole.Add({15,20,0});hole.Add({25,20,0});hole.Add({25,10,0});hole.Close();
    BRepBuilderAPI_MakeFace trimmed(outer.Wire());trimmed.Add(hole.Wire());const auto fallback=point(FaceNormalInspector::sample(trimmed.Face()));
    check(!(fallback[0].toDouble()>15 && fallback[0].toDouble()<25 && fallback[1].toDouble()>10 && fallback[1].toDouble()<20),"sample inside hole");
    const auto unsupported=FaceNormalInspector::sample(BRepBuilderAPI_MakeFace(gp_Sphere(gp_Ax3(),10)).Face());check(unsupported.value("status")=="unsupported","unsupported sphere must refuse");
    EvaluatedBody body;body.bodyId="body";body.visible=true;body.shape=plane;EvaluatedSupportFace face;face.face=plane;face.faceId="face";face.producerFeatureId="feature";body.supportFaces.push_back(face);
    const QJsonObject ref{{"bodyId","body"},{"faceId","face"},{"producerFeatureId","feature"}};
    check(FaceNormalInspector::evaluate({body},ref).value("status")=="ready","exact identity");
    auto detached=body;detached.supportFaces.front().face=TopoDS::Face(plane.Moved(TopLoc_Location(transform)));check(FaceNormalInspector::evaluate({detached},ref).value("status")=="stale","off-body registry face");
    body.visible=false;check(FaceNormalInspector::evaluate({body},ref).value("status")=="stale","hidden face");body.visible=true;
    auto stale=ref;stale["producerFeatureId"]="old";check(FaceNormalInspector::evaluate({body},stale).value("status")=="stale","stale producer");
    auto other=body;body.occurrenceId="one";other.occurrenceId="two";check(FaceNormalInspector::evaluate({body,other},ref).value("error").toObject().value("code")=="face_normals_ambiguous","ambiguous instances");
    auto exact=ref;exact["occurrenceId"]="two";check(FaceNormalInspector::evaluate({body,other},exact).value("status")=="ready","exact instance");
    std::cout<<"face normals analytic geometry, orientation, transform, trim and identity checks passed\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
