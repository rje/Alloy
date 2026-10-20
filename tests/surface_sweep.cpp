// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QJsonDocument>

#include <QTemporaryDir>
#include <iostream>
#include <numbers>
#include <cmath>
namespace {
void check(bool ok,const QString& text){if(!ok)throw std::runtime_error(text.toStdString());}
void near(double a,double b,const QString& text){check(std::abs(a-b)<1e-5,text+QString(" %1 != %2").arg(a).arg(b));}
QJsonObject move(QJsonObject doc,const QString& sketch,const QString& point,double u,double v){auto rows=doc["features"].toArray();for(int i=0;i<rows.size();++i){auto f=rows[i].toObject();if(f["id"]!=sketch)continue;auto p=f["parameters"].toObject();auto pts=p["points"].toArray();for(int j=0;j<pts.size();++j){auto q=pts[j].toObject();if(q["id"]==point){q["uMm"]=u;q["vMm"]=v;pts[j]=q;}}p["points"]=pts;f["parameters"]=p;rows[i]=f;}doc["features"]=rows;return doc;}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{for(int fixture:{0,1,2}){const bool arc=fixture!=0,negative=fixture==2;
    CadDocument doc;const auto owner=doc.authored()["documentId"].toString();auto section=SketchGeometry::newFeature(owner,arc?"XZ":"YZ","Section");section=SketchGeometry::addLine(section,{{"u1Mm",arc?20:0},{"v1Mm",-5},{"u2Mm",arc?20:0},{"v2Mm",5}});const auto e=section["parameters"].toObject()["entities"].toArray().last().toObject();
    check(doc.commit(SketchGeometry::replaceFeature(doc.authored(),section),"Section").isEmpty(),"section");
    auto path=SketchGeometry::newFeature(owner,"XY","Path");path=arc?SketchGeometry::addArc(path,{{"centerUMm",0},{"centerVMm",0},{"radiusMm",20},{"startAngleDeg",0},{"sweepAngleDeg",negative?-90:90}}):SketchGeometry::addLine(path,{{"u1Mm",0},{"v1Mm",0},{"u2Mm",40},{"v2Mm",0}});const auto pe=path["parameters"].toObject()["entities"].toArray().last().toObject();check(doc.commit(SketchGeometry::replaceFeature(doc.authored(),path),"Path").isEmpty(),"path");
    QJsonObject args{{"sketchId",section["id"]},{"entityId",e["id"]},{"pathSketchId",path["id"]},{"pathEntityId",pe["id"]},{"pathStartPointId",pe["startPointId"]}};const auto candidate=CadDocument::surfaceSweepCandidate(doc.authored(),args);auto preview=CadDocument::evaluateCandidate(candidate);check(preview.ok,preview.error+QString::fromUtf8(QJsonDocument(preview.diagnostics).toJson()));check(doc.bodies().empty(),"preview isolation");check(doc.commit(candidate,"Sweep").isEmpty(),"sweep");const auto first=doc.bodies().front();const double area=arc?100*std::numbers::pi:400;
    auto verify=[&](double expected){const auto& b=doc.bodies().front();const auto t=b.summary["topology"].toObject();check(b.summary["valid"]==true && b.summary["closed"]==false && t["faces"]==1 && t["solids"]==0 && t["edges"]==4 && b.summary["volumeMm3"].isNull(),"open sheet topology");near(b.summary["areaMm2"].toDouble(),expected,"area");check(b.bodyId==first.bodyId && b.supportFaces.front().faceId==first.supportFaces.front().faceId && b.solidEdges.size()==4,"identity");for(int i=0;i<4;++i)check(b.solidEdges[i].edgeId==first.solidEdges[i].edgeId,"boundary identity");};verify(area);
    const auto bounds=first.summary["boundsMm"].toObject();const auto low=bounds["min"].toArray(),high=bounds["max"].toArray();near(low[0].toDouble(),0,"xmin");near(high[0].toDouble(),arc?20:40,"xmax");near(low[1].toDouble(),negative?-20:0,"ymin");near(high[1].toDouble(),arc&&!negative?20:0,"ymax");near(low[2].toDouble(),-5,"zmin");near(high[2].toDouble(),5,"zmax");
    const auto initial=doc.authored();auto changed=move(initial,section["id"].toString(),e["startPointId"].toString(),arc?20:0,-10);changed=move(changed,section["id"].toString(),e["endPointId"].toString(),arc?20:0,10);check(doc.commit(changed,"Source edit").isEmpty(),"source edit");verify(area*2);doc.history().undo();verify(area);check(doc.authored()==initial,"undo");doc.history().redo();verify(area*2);
    check(doc.commit(CadDocument::surfaceSweepCandidate(doc.authored(),{{"featureId",first.featureId},{"name","Edited Sweep"}}),"Feature edit").isEmpty(),"retained edit");verify(area*2);
    const auto accepted=doc.authored();const auto revision=doc.revision();changed=move(accepted,section["id"].toString(),e["endPointId"].toString(),arc?21:1,10);check(!doc.commit(changed,"Misalign").isEmpty() && doc.authored()==accepted && doc.revision()==revision,"atomic misalignment");
    auto bad=CadDocument::surfaceSweepCandidate(accepted,{{"featureId",first.featureId},{"pathEntityId",CadDocument::newId()}});check(!doc.commit(bad,"Missing path").isEmpty() && doc.authored()==accepted,"atomic unavailable path");
    QTemporaryDir tmp;const auto file=tmp.filePath("sweep.alloy");check(doc.save(file).isEmpty(),"save");CadDocument reopened;check(reopened.open(file).isEmpty() && reopened.authored()==accepted,"reopen authoring");near(reopened.bodies().front().summary["areaMm2"].toDouble(),area*2,"reopen area");
}std::cout<<"PASS retained line/arc Surface Sweep area, bounds, lineage, edits, atomic refusal, undo/redo/reopen\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
