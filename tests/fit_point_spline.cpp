// SPDX-License-Identifier: MIT
#include "SketchSpline.h"
#include "CadDocument.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QJsonArray>
#include <iostream>
#include <stdexcept>
#include <cmath>
static void check(bool b,const QString& s){if(!b)throw std::runtime_error(s.toStdString());}
static QJsonObject accept(const SketchGeometry::SolveResult& r){check(r.ok,r.error);return r.feature;}
static QJsonObject entity(const QJsonObject& d){return d["parameters"].toObject()["entities"].toArray().first().toObject();}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
 CadDocument doc;auto draft=SketchGeometry::newFeature(doc.authored()["documentId"].toString(),"XY");
 const QJsonArray picks{QJsonObject{{"uMm",0},{"vMm",0}},QJsonObject{{"uMm",15},{"vMm",30}},QJsonObject{{"uMm",100},{"vMm",0}}};draft=accept(SketchSpline::add(draft,{{"points",picks}}));
 const auto splineId=entity(draft)["id"].toString();auto ids=entity(draft)["pointIds"].toArray();const auto first=ids[0].toString(),middle=ids[1].toString(),last=ids[2].toString();
 auto fixed=draft;for(const auto& id:ids)fixed=accept(SketchGeometry::addConstraint(fixed,{{"type","fixed"},{"pointIds",QJsonArray{id}}}));const auto fixedSolve=SketchGeometry::solveDraft(fixed);check(fixedSolve.ok&&fixedSolve.diagnostics["pointSolverDof"].toInt(-1)==0&&fixedSolve.diagnostics["splineHandleDof"].toInt()==6&&fixedSolve.diagnostics["status"]=="underconstrained","fixed positions still expose editable handle freedom");
 const auto empty=SketchGeometry::newFeature(doc.authored()["documentId"].toString(),"XY");const auto duplicate=SketchSpline::add(empty,{{"points",QJsonArray{QJsonObject{{"uMm",0},{"vMm",0}},QJsonObject{{"uMm",0},{"vMm",0}}}}});check(!duplicate.ok&&duplicate.feature==empty,"duplicate points refuse atomically");
 auto eval=[&]{return SketchSpline::evaluate(draft["parameters"].toObject(),entity(draft));};
 auto fit=eval();check(fit.curve->Degree()==5&&fit.curve->IsCN(2),"real degree5 C2");check(fit.parameters[1]<0.3,"nonuniform spans");check(fit.curve->Value(fit.parameters[1]).Distance(gp_Pnt(15,30,0))<1e-7,"through fit point");
 draft=accept(SketchSpline::edit(draft,"tangent",{{"entityId",splineId},{"pointId",first},{"duMm",50},{"dvMm",130}}));
 draft=accept(SketchSpline::edit(draft,"remove",{{"entityId",splineId},{"pointId",middle}}));check(entity(draft)["pointIds"].toArray()==QJsonArray{first,last},"remaining fit identities");fit=eval();gp_Pnt q;gp_Vec d,a;fit.curve->D2(0,q,d,a);check((d-gp_Vec(50,130,0)).Magnitude()<1e-6,"2point active endpoint tangent survives");
 draft=accept(SketchSpline::edit(draft,"insert",{{"entityId",splineId},{"parameter",0.25}}));ids=entity(draft)["pointIds"].toArray();const auto inserted=ids[1].toString();check(ids[0]==first&&ids[2]==last&&inserted!=middle,"insert new point with stable retained identities");
 draft=accept(SketchSpline::edit(draft,"tangent",{{"entityId",splineId},{"pointId",inserted},{"duMm",80},{"dvMm",40}}));fit=eval();const double t=fit.parameters[1];fit.curve->D2(t,q,d,a);check((d-gp_Vec(80,40,0)).Magnitude()<1e-5,"interior authored D1 magnitude and direction");
 gp_Pnt pl,pr;gp_Vec dl,dr,al,ar;fit.curve->D2(t-1e-9,pl,dl,al);fit.curve->D2(t+1e-9,pr,dr,ar);check((dl-dr).Magnitude()<1e-4&&(al-ar).Magnitude()<1e-3,"sameglobalparameter C2 joins");const auto before=fit.curve->Value(t/2);
 draft=accept(SketchSpline::edit(draft,"tangent",{{"entityId",splineId},{"pointId",inserted},{"duMm",160},{"dvMm",80}}));fit=eval();check(fit.curve->Value(t/2).Distance(before)>0.1,"weight edit changes real curve");
 const auto rejected=SketchSpline::edit(draft,"tangent",{{"entityId",splineId},{"pointId",inserted},{"duMm",0},{"dvMm",0}});check(!rejected.ok&&rejected.feature==draft,"invalid tangent atomic");
 auto closed=SketchGeometry::addLine(draft,{{"u1Mm",100},{"v1Mm",0},{"u2Mm",0},{"v2Mm",0},{"startPointId",last},{"endPointId",first}});closed=accept(SketchGeometry::solveDraft(closed));const auto shape=SketchGeometry::evaluateFeature(closed);check(shape.profiles.size()==1,"spline plus exact line makes one real profile");double integral=0;constexpr int steps=2000;for(int i=0;i<=steps;++i){gp_Pnt position;gp_Vec tangent;fit.curve->D1(double(i)/steps,position,tangent);integral+=(i==0||i==steps?1:i%2?4:2)*(position.X()*tangent.Y()-position.Y()*tangent.X());}const double quadratureArea=std::abs(integral/(6*steps));check(std::abs(shape.profiles[0].summary["areaMm2"].toDouble()-quadratureArea)<0.01,"BRep area agrees with independent Simpson boundary integration");
 check(doc.commit(SketchGeometry::replaceFeature(doc.authored(),closed),"Fit spline").isEmpty(),"commit");const auto authored=doc.authored();doc.history().undo();check(doc.authored()["features"].toArray().isEmpty(),"undo");doc.history().redo();check(doc.authored()==authored,"redo exact authoring");QTemporaryDir temp;const auto path=temp.filePath("spline.alloy");check(doc.save(path).isEmpty(),"save");CadDocument reopened;check(reopened.open(path).isEmpty()&&reopened.authored()==authored,"reopen exact authoring");
 std::cout<<"fit spline authoring/degree5/nonuniform C2/edit/delete/insert/profile/history/storage passed\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
