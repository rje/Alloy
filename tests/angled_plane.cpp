// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
#include <stdexcept>
namespace {
void check(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
void append(CadDocument& d,const QJsonObject& f){auto a=d.authored();auto rows=a.value("features").toArray();rows.append(f);a["features"]=rows;const auto error=d.commit(a,"Append");if(!error.isEmpty())throw std::runtime_error(error.toStdString());}
void near(const QJsonArray& actual,const QJsonArray& expected){check(actual.size()==3,"Missing frame vector");for(int i=0;i<3;++i)check(std::abs(actual[i].toDouble()-expected[i].toDouble())<1e-7,"Unexpected plane frame");}
QJsonObject replaceParameters(QJsonObject candidate,int index,const QJsonObject& p){auto rows=candidate.value("features").toArray();auto row=rows[index].toObject();row["parameters"]=p;rows[index]=row;candidate["features"]=rows;return candidate;}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
 CadDocument d;const auto owner=d.activeOwnerId(),supportId=CadDocument::newId();
 QJsonObject support{{"id",supportId},{"ownerId",owner},{"type","constructionPlane"},{"name","Pivot support"},{"visible",true},{"parameters",QJsonObject{{"source",QJsonObject{{"plane","XY"}}},{"offsetMm",0}}}};
 append(d,support);
 auto sketch=SketchGeometry::newConstructionPlaneFeature(owner,supportId,"Pivot");
 sketch=SketchGeometry::addLine(sketch,{{"u1Mm",0},{"v1Mm",0},{"u2Mm",0},{"v2Mm",20}});
 auto sp=sketch.value("parameters").toObject();auto entities=sp.value("entities").toArray();auto entity=entities[0].toObject();entity["construction"]=true;entities[0]=entity;sp["entities"]=entities;sketch["parameters"]=sp;
 auto fixed=SketchGeometry::addConstraint(sketch,{{"type","fixed"},{"pointIds",QJsonArray{entity.value("startPointId")}}});check(fixed.ok,"Fix failed");sketch=fixed.feature;
 auto dimension=SketchGeometry::addDimension(sketch,{{"kind","vertical"},{"pointIds",QJsonArray{entity.value("startPointId"),entity.value("endPointId")}},{"valueMm",20}});check(dimension.ok,"Dimension failed");sketch=dimension.feature;append(d,sketch);
 const QJsonObject line{{"sketchId",sketch.value("id")},{"entityId",entity.value("id")},{"startPointId",entity.value("startPointId")},{"endPointId",entity.value("endPointId")}};
 QJsonObject angle{{"id",CadDocument::newId()},{"ownerId",owner},{"type","constructionPlane"},{"name","Angled plane"},{"visible",true},{"parameters",QJsonObject{{"source",QJsonObject{{"angled",QJsonObject{{"line",line},{"plane",QJsonObject{{"planeId",supportId}}}}}}},{"angleDeg",30},{"flipSide",false}}}};
 append(d,angle);
 auto frame=d.evaluation().constructionPlanes.back().frame;near(frame.value("normal").toArray(),{.5,0,std::sqrt(3.)/2});near(frame.value("uAxis").toArray(),{0,1,0});near(frame.value("vAxis").toArray(),{-std::sqrt(3.)/2,0,.5});
 auto section=SketchGeometry::newConstructionPlaneFeature(owner,angle.value("id").toString(),"Downstream");section=SketchGeometry::addRectangle(section,{{"u1Mm",-5},{"v1Mm",-3},{"u2Mm",5},{"v2Mm",3}});append(d,section);
 near(d.sketches().back().resolvedFrame.value("normal").toArray(),{.5,0,std::sqrt(3.)/2});
 auto p=angle.value("parameters").toObject();p["flipSide"]=true;check(d.commit(replaceParameters(d.authored(),2,p),"Flip").isEmpty(),"Flip failed");near(d.evaluation().constructionPlanes.back().frame.value("normal").toArray(),{-.5,0,std::sqrt(3.)/2});
 p["angleDeg"]=-30;check(d.commit(replaceParameters(d.authored(),2,p),"Signed flip").isEmpty(),"Signed flip failed");near(d.evaluation().constructionPlanes.back().frame.value("normal").toArray(),{.5,0,std::sqrt(3.)/2});
 p["angleDeg"]=45;p["flipSide"]=false;check(d.commit(replaceParameters(d.authored(),2,p),"Angle edit").isEmpty(),"Angle edit failed");
 auto candidate=d.authored();auto supportP=support.value("parameters").toObject();supportP["offsetMm"]=4;candidate=replaceParameters(candidate,0,supportP);check(d.commit(candidate,"Support edit").isEmpty(),"Attached source edit failed");
 near(d.evaluation().constructionPlanes.back().frame.value("origin").toArray(),{0,0,4});near(d.sketches().back().resolvedFrame.value("normal").toArray(),{std::sqrt(.5),0,std::sqrt(.5)});
 sp=sketch.value("parameters").toObject();auto dimensions=sp.value("dimensions").toArray();auto dim=dimensions[0].toObject();dim["valueMm"]=25;dimensions[0]=dim;sp["dimensions"]=dimensions;check(d.commit(replaceParameters(d.authored(),1,sp),"Solved source edit").isEmpty(),"Solved source edit failed");
 check(d.authored().value("features").toArray()[2].toObject().value("parameters").toObject().value("source").toObject().value("angled").toObject().value("line")==line,"Source identity changed");
 const auto stable=d.authored();const auto stableFrame=d.evaluation().constructionPlanes.back().frame;d.history().undo();d.history().redo();check(d.authored()==stable && d.evaluation().constructionPlanes.back().frame==stableFrame,"History changed retained plane");
 QTemporaryDir dir;const auto path=dir.filePath("angled.alloy");check(d.save(path).isEmpty(),"Save failed");CadDocument opened;check(opened.open(path).isEmpty() && opened.authored()==stable && opened.evaluation().constructionPlanes.back().frame==stableFrame,"Reopen failed");
 const auto refuse=[&](const QJsonObject& bad){check(!opened.commit(bad,"Refused edit").isEmpty() && opened.authored()==stable && opened.evaluation().constructionPlanes.back().frame==stableFrame,"Refusal was not atomic");};
 auto angled=p.value("source").toObject().value("angled").toObject();angled["plane"]=QJsonObject{{"plane","XY"}};auto badP=p;badP["source"]=QJsonObject{{"angled",angled}};refuse(replaceParameters(stable,2,badP));
 auto badLine=line;badLine["entityId"]=CadDocument::newId();angled["line"]=badLine;angled["plane"]=QJsonObject{{"planeId",supportId}};badP["source"]=QJsonObject{{"angled",angled}};refuse(replaceParameters(stable,2,badP));
 badLine=line;badLine["startPointId"]=line.value("endPointId");badLine["endPointId"]=line.value("startPointId");angled["line"]=badLine;badP["source"]=QJsonObject{{"angled",angled}};refuse(replaceParameters(stable,2,badP));
 auto badSp=sp;entities=badSp.value("entities").toArray();entity=entities[0].toObject();const auto start=entity.value("startPointId");entity["startPointId"]=entity.value("endPointId");entity["endPointId"]=start;entities[0]=entity;badSp["entities"]=entities;refuse(replaceParameters(stable,1,badSp));
 angled["line"]=line;angled["plane"]=QJsonObject{{"planeId",angle.value("id")}};badP["source"]=QJsonObject{{"angled",angled}};refuse(replaceParameters(stable,2,badP));
 candidate=stable;auto rows=candidate.value("features").toArray();const auto pivot=rows.takeAt(1);rows.append(pivot);candidate["features"]=rows;refuse(candidate);
 badP=p;badP["angleDeg"]=361;refuse(replaceParameters(stable,2,badP));
 // Zero, negative, full-turn and ordinary Line policies share the same evaluator.
 for(double a:{0.,-30.,360.}){auto parameter=p;parameter["angleDeg"]=a;check(opened.commit(replaceParameters(opened.authored(),2,parameter),"Boundary angle").isEmpty(),"Valid boundary angle refused");const double r=a*std::acos(-1.)/180;near(opened.evaluation().constructionPlanes.back().frame.value("normal").toArray(),{std::sin(r),0,std::cos(r)});}
 auto ordinary=sp;entities=ordinary.value("entities").toArray();entity=entities[0].toObject();entity["construction"]=false;entities[0]=entity;ordinary["entities"]=entities;check(opened.commit(replaceParameters(opened.authored(),1,ordinary),"Ordinary Line").isEmpty(),"Ordinary Line refused");
 std::cout<<"retained angled plane analytic/history/source/refusal checks pass\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
