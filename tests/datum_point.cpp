// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "DatumPoint.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
namespace {
void check(bool value,const char* text){if(!value)throw std::runtime_error(text);}
void append(CadDocument& d,const QJsonObject& f){auto a=d.authored();auto rows=a.value("features").toArray();rows.append(f);a["features"]=rows;const auto error=d.commit(a,"Append");if(!error.isEmpty())throw std::runtime_error(error.toStdString());}
QJsonObject replace(QJsonObject a,int i,QJsonObject f){auto rows=a.value("features").toArray();rows[i]=f;a["features"]=rows;return a;}
void near(QJsonArray a,QJsonArray b){check(a.size()==3,"Missing coordinate");for(int i=0;i<3;++i)check(std::abs(a[i].toDouble()-b[i].toDouble())<1e-8,"Incorrect point coordinate");}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
 CadDocument d;auto sketch=SketchGeometry::newFeature(d.activeOwnerId(),"XZ","Path");sketch=SketchGeometry::addLine(sketch,{{"u1Mm",2},{"v1Mm",3},{"u2Mm",8},{"v2Mm",11}});append(d,sketch);
 const auto line=sketch.value("parameters").toObject().value("entities").toArray()[0].toObject();const QJsonObject ref{{"sketchId",sketch.value("id")},{"entityId",line.value("id")},{"startPointId",line.value("startPointId")},{"endPointId",line.value("endPointId")}};
 const auto feature=[&](QJsonObject p){return QJsonObject{{"id",CadDocument::newId()},{"ownerId",d.activeOwnerId()},{"type","constructionPoint"},{"name","Point"},{"visible",true},{"parameters",p}};};
 for(double ratio:{0.,.25,1.})append(d,feature({{"source",QJsonObject{{"line",ref}}},{"location",QJsonObject{{"mode","proportional"},{"ratio",ratio}}}}));
 append(d,feature({{"source",QJsonObject{{"line",ref}}},{"location",QJsonObject{{"mode","physical"},{"distanceMm",5}}}}));
 check(d.bodies().empty() && d.evaluation().constructionPoints.size()==4,"Point created body or missing result");
 near(d.evaluation().constructionPoints[0].point.value("positionMm").toArray(),{2,0,3});near(d.evaluation().constructionPoints[1].point.value("positionMm").toArray(),{3.5,0,5});near(d.evaluation().constructionPoints[2].point.value("positionMm").toArray(),{8,0,11});near(d.evaluation().constructionPoints[3].point.value("positionMm").toArray(),{5,0,7});
 auto p=sketch.value("parameters").toObject();auto points=p.value("points").toArray();for(int i=0;i<points.size();++i){auto row=points[i].toObject();if(row.value("id")==line.value("endPointId")){row["uMm"]=14;row["vMm"]=19;points[i]=row;}}p["points"]=points;sketch["parameters"]=p;check(d.commit(replace(d.authored(),0,sketch),"Resize path").isEmpty(),"Path edit failed");
 near(d.evaluation().constructionPoints[1].point.value("positionMm").toArray(),{5,0,7});near(d.evaluation().constructionPoints[3].point.value("positionMm").toArray(),{5,0,7});
 const auto stable=d.authored();const auto evaluated=d.state().value("evaluation");const int count=d.history().count(),index=d.history().index();
 const auto refuse=[&](QJsonObject candidate){check(!d.commit(candidate,"Invalid").isEmpty(),"Invalid accepted");check(d.authored()==stable && d.state().value("evaluation")==evaluated && d.history().count()==count && d.history().index()==index,"Refusal changed document/history");};
 auto bad=stable.value("features").toArray()[1].toObject();for(const auto location:{QJsonObject{{"mode","proportional"},{"ratio",1.1}},QJsonObject{{"mode","physical"},{"distanceMm",21}},QJsonObject{{"mode","proportional"},{"ratio",-1}}}){bad["parameters"]=QJsonObject{{"source",QJsonObject{{"line",ref}}},{"location",location}};refuse(replace(stable,1,bad));}
 auto reversed=ref;reversed["startPointId"]=ref.value("endPointId");reversed["endPointId"]=ref.value("startPointId");bad["parameters"]=QJsonObject{{"source",QJsonObject{{"line",reversed}}},{"location",QJsonObject{{"mode","proportional"},{"ratio",.5}}}};refuse(replace(stable,1,bad));
 auto rows=stable.value("features").toArray();rows.removeAt(0);auto deleted=stable;deleted["features"]=rows;refuse(deleted);
 auto hidden=stable.value("features").toArray()[1].toObject();hidden["visible"]=false;check(d.commit(replace(stable,1,hidden),"Hide Point").isEmpty(),"Hide failed");check(!d.evaluation().constructionPoints[0].visible && d.evaluation().recompute=="visibility_only","Visibility fast path failed");d.history().undo();check(d.authored()==stable && d.evaluation().constructionPoints[0].visible,"Undo failed");d.history().redo();check(!d.evaluation().constructionPoints[0].visible,"Redo failed");
 QTemporaryDir dir;check(d.save(dir.filePath("point.alloy")).isEmpty(),"Save failed");CadDocument opened;check(opened.open(dir.filePath("point.alloy")).isEmpty() && opened.authored()==d.authored() && opened.evaluation().constructionPoints[1].point==d.evaluation().constructionPoints[1].point,"Reopen failed");
 CadDocument boxDoc;QJsonObject box{{"id",CadDocument::newId()},{"bodyId",CadDocument::newId()},{"ownerId",boxDoc.activeOwnerId()},{"type","box"},{"name","Box"},{"visible",true},{"parameters",QJsonObject{{"widthMm",10},{"depthMm",20},{"heightMm",30},{"xMm",1},{"yMm",2},{"zMm",3}}}};append(boxDoc,box);
 const auto body=boxDoc.bodies()[0];const EvaluatedSolidEdge* edge=nullptr;for(const auto& candidate:body.solidEdges)if(candidate.provenance.value("boxEdge")=="x*y0z0")edge=&candidate;check(edge,"Box edge unavailable");
 const QJsonObject vertex{{"bodyId",body.bodyId},{"edgeId",edge->edgeId},{"endpoint",1}};auto point=feature({{"source",QJsonObject{{"vertex",vertex}}}});point["ownerId"]=boxDoc.activeOwnerId();append(boxDoc,point);near(boxDoc.evaluation().constructionPoints[0].point.value("positionMm").toArray(),{11,2,3});auto bp=box.value("parameters").toObject();bp["widthMm"]=40;box["parameters"]=bp;check(boxDoc.commit(replace(boxDoc.authored(),0,box),"Resize Box").isEmpty(),"Box edit failed");near(boxDoc.evaluation().constructionPoints[0].point.value("positionMm").toArray(),{41,2,3});
 auto fake=boxDoc.evaluation();fake.bodies[0].solidEdges[0].provenance["role"]="fillet.edge";const auto fakeRef=QJsonObject{{"bodyId",body.bodyId},{"edgeId",fake.bodies[0].solidEdges[0].edgeId},{"endpoint",0}};bool refused=false;try{DatumPoint::evaluate(fake,{{"source",QJsonObject{{"vertex",fakeRef}}}},boxDoc.authored().value("features").toArray());}catch(const GeometryKernel::EvaluationError& e){refused=e.diagnostics().value("code")=="unsupported_point_source";}check(refused,"Unsupported provenance accepted");
 std::cout<<"datum Point analytic Line/Box/source/history/visibility/reopen checks pass\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
