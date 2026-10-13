#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QMap>
#include <QPointF>
#include <QLineF>
#include <cmath>
#include <iostream>
#include <stdexcept>
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
QString lineAt(const QJsonObject& sketch,double u,double v){const auto p=sketch["parameters"].toObject();QMap<QString,QPointF> pts;for(const auto& value:p["points"].toArray()){const auto x=value.toObject();pts[x["id"].toString()]={x["uMm"].toDouble(),x["vMm"].toDouble()};}for(const auto& value:p["entities"].toArray()){const auto e=value.toObject();if(e["type"]!="line")continue;auto a=pts[e["startPointId"].toString()],b=pts[e["endPointId"].toString()],d=b-a;const auto t=QPointF::dotProduct(QPointF(u,v)-a,d)/QPointF::dotProduct(d,d);if(t>=0 && t<=1 && QLineF(QPointF(u,v),a+d*t).length()<1e-6)return e["id"].toString();}throw std::runtime_error("line not found");}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
 CadDocument doc;auto sketch=SketchGeometry::newFeature(doc.authored()["documentId"].toString(),"XY");sketch=SketchGeometry::addRectangle(sketch,{{"u1Mm",0},{"v1Mm",0},{"u2Mm",32},{"v2Mm",-10}});
 sketch=SketchGeometry::addLine(sketch,{{"u1Mm",0},{"v1Mm",-5},{"u2Mm",7.5},{"v2Mm",-5}});
 sketch=SketchGeometry::addCircle(sketch,{{"centerUMm",7.5},{"centerVMm",-5},{"diameterMm",15}});const auto circle=sketch["parameters"].toObject()["entities"].toArray().last().toObject();
 auto dimension=[&](QString id,QString kind,double value){auto result=SketchGeometry::addDimension(sketch,{{"entityId",id},{"kind",kind},{"valueMm",value},{"driving",true}});check(result.ok,result.error.toUtf8().constData());sketch=result.feature;};
 dimension(lineAt(sketch,16,0),"horizontal",32);dimension(lineAt(sketch,0,-2),"vertical",10);dimension(lineAt(sketch,4,-5),"horizontal",7.5);dimension(circle["id"].toString(),"diameter",15);
 for(int x:{23,26,29}){auto result=SketchGeometry::addPointerEntity(sketch,"rectangle",{{"u1Mm",x},{"v1Mm",0},{"u2Mm",x+3},{"v2Mm",-10}});check(result.ok,result.error.toUtf8().constData());sketch=result.feature;}
 const auto original=sketch;auto trim=[&](double u,double v){std::cerr<<"trim "<<u<<","<<v<<"\n";const auto id=lineAt(sketch,u,v);auto result=SketchGeometry::trimLine(sketch,{{"entityId",id},{"uMm",u},{"vMm",v}});check(result.ok,result.error.toUtf8().constData());sketch=result.feature;return result;};
 auto top=trim(7.5,0);const auto segment=top.diagnostics["trimmedSegment"].toObject();check(std::abs(segment["u1Mm"].toDouble()-(7.5-std::sqrt(31.25)))<1e-6,"left circle crossing");check(std::abs(segment["u2Mm"].toDouble()-(7.5+std::sqrt(31.25)))<1e-6,"right circle crossing");
 check(top.diagnostics["removedDimensionIds"].toArray().isEmpty(),"split keeps32 spanning outer endpoints");
 trim(0,-2.5);trim(0,-7.5);trim(4,-5);trim(7.5,-10);
 auto p=sketch["parameters"].toObject();bool sameCircle=false;for(const auto& value:p["entities"].toArray())if(value.toObject()["id"]==circle["id"]){check(value.toObject()==circle,"circle untouched");sameCircle=true;}check(sameCircle,"circle retained");
 check(p["dimensions"].toArray().size()==2,"only32/15 dimensions remain");check(lineAt(sketch,1,0)!=QString{} && lineAt(sketch,1,-10)!=QString{},"outsideleft stubs preserved");
 auto authored=doc.authored();authored["features"]=QJsonArray{sketch};check(doc.commit(authored,"Trim sketch").isEmpty(),"commit");doc.history().undo();check(doc.authored()["features"].toArray().isEmpty(),"undo sketch");doc.history().redo();check(doc.authored()==authored,"redo exact");QTemporaryDir tmp;auto path=tmp.filePath("trim.alloy");check(doc.save(path).isEmpty(),"save");CadDocument other;check(other.open(path).isEmpty() && other.authored()==authored,"reopen exact");
 auto overlap=SketchGeometry::trimLine(original,{{"entityId",lineAt(original,24,0)},{"uMm",24},{"vMm",0}});check(!overlap.ok && overlap.feature==original,"selected overlapping interval refuses atomically");
 std::cout<<"Trim source fixture/contact/dimensions/history/storage passed\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
