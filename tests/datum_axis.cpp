// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
namespace {
void check(bool value,const char* text){if(!value)throw std::runtime_error(text);}
void append(CadDocument& d,const QJsonObject& f){auto a=d.authored();auto rows=a.value("features").toArray();rows.append(f);a["features"]=rows;const auto error=d.commit(a,"Append");if(!error.isEmpty())throw std::runtime_error(error.toStdString());}
QJsonObject replace(QJsonObject a,int index,QJsonObject feature){auto rows=a.value("features").toArray();rows[index]=feature;a["features"]=rows;return a;}
void near(QJsonArray actual,QJsonArray expected){check(actual.size()==3,"Missing axis vector");for(int i=0;i<3;++i)check(std::abs(actual[i].toDouble()-expected[i].toDouble())<1e-8,"Unexpected axis vector");}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
 CadDocument d;auto sketch=SketchGeometry::newFeature(d.activeOwnerId(),"XY","Sources");
 for(auto pos:{QJsonArray{2,3},QJsonArray{8,11}}){const auto result=SketchGeometry::addStandalonePoint(sketch,{{"uMm",pos[0]},{"vMm",pos[1]}});check(result.ok,"Standalone Point failed");sketch=result.feature;}
 sketch=SketchGeometry::addLine(sketch,{{"u1Mm",-5},{"v1Mm",0},{"u2Mm",15},{"v2Mm",0}});append(d,sketch);
 auto p=sketch.value("parameters").toObject();const auto entities=p.value("entities").toArray();const auto a=entities[0].toObject(),b=entities[1].toObject(),line=entities[2].toObject();
 const auto ref=[&](QJsonObject e){return QJsonObject{{"sketchId",sketch.value("id")},{"entityId",e.value("id")},{"pointId",e.value("pointId")}};};
 QJsonObject pointSource{{"points",QJsonArray{ref(a),ref(b)}}};
 const QJsonObject lineRef{{"sketchId",sketch.value("id")},{"entityId",line.value("id")},{"startPointId",line.value("startPointId")},{"endPointId",line.value("endPointId")}};
 const auto axis=[&](QJsonObject source){return QJsonObject{{"id",CadDocument::newId()},{"ownerId",d.activeOwnerId()},{"type","constructionAxis"},{"name","Axis"},{"visible",true},{"parameters",QJsonObject{{"source",source}}}};};
 auto two=axis(pointSource);append(d,two);append(d,axis({{"line",lineRef}}));
 check(d.bodies().empty() && d.evaluation().constructionAxes.size()==2,"Axis created body or missing result");near(d.evaluation().constructionAxes[0].line.value("originMm").toArray(),{2,3,0});near(d.evaluation().constructionAxes[0].line.value("direction").toArray(),{.6,.8,0});near(d.evaluation().constructionAxes[1].line.value("direction").toArray(),{1,0,0});
 // Source edit follows exact solved world positions while both retained forms survive.
 auto points=p.value("points").toArray();for(int i=0;i<points.size();++i){auto row=points[i].toObject();if(row.value("id")==b.value("pointId")){row["uMm"]=2;row["vMm"]=13;points[i]=row;}}
 p["points"]=points;sketch["parameters"]=p;check(d.commit(replace(d.authored(),0,sketch),"Move source").isEmpty(),"Source move failed");near(d.evaluation().constructionAxes[0].line.value("direction").toArray(),{0,1,0});
 const auto stable=d.authored();const auto stableLine=d.evaluation().constructionAxes[0].line;
 const auto refuse=[&](QJsonObject candidate){const int count=d.history().count(),index=d.history().index();check(!d.commit(candidate,"Invalid").isEmpty(),"Bad reference accepted");check(d.authored()==stable && d.evaluation().constructionAxes[0].line==stableLine && d.history().count()==count && d.history().index()==index,"Refusal mutated document/history");};
 auto bad=two;bad["parameters"]=QJsonObject{{"source",QJsonObject{{"points",QJsonArray{ref(a),ref(a)}}}}};refuse(replace(stable,1,bad));
 auto missing=ref(b);missing["pointId"]=CadDocument::newId();bad["parameters"]=QJsonObject{{"source",QJsonObject{{"points",QJsonArray{ref(a),missing}}}}};refuse(replace(stable,1,bad));
 auto wrong=lineRef;wrong["startPointId"]=line.value("endPointId");wrong["endPointId"]=line.value("startPointId");bad["parameters"]=QJsonObject{{"source",QJsonObject{{"line",wrong}}}};refuse(replace(stable,1,bad));
 auto rows=stable.value("features").toArray();rows.removeAt(0);auto deleted=stable;deleted["features"]=rows;refuse(deleted);
 // The visibility fast path must update the datum, not only authored JSON.
 auto hidden=two;hidden["visible"]=false;check(d.commit(replace(stable,1,hidden),"Hide Axis").isEmpty(),"Hide failed");check(!d.evaluation().constructionAxes[0].visible && d.evaluation().recompute=="visibility_only","Axis visibility fast path failed");d.history().undo();check(d.authored()==stable && d.evaluation().constructionAxes[0].visible,"Undo visibility failed");d.history().redo();check(!d.evaluation().constructionAxes[0].visible,"Redo visibility failed");
 QTemporaryDir dir;const auto path=dir.filePath("axis.alloy");check(d.save(path).isEmpty(),"Save failed");CadDocument opened;check(opened.open(path).isEmpty() && opened.authored()==d.authored() && opened.evaluation().constructionAxes[0].line==stableLine && !opened.evaluation().constructionAxes[0].visible,"Reopen failed");
 std::cout<<"datum Axis exact Point/Line analytic/source/history/visibility/reopen checks pass\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
