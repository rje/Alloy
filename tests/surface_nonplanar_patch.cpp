// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <BRep_Tool.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <Geom_BezierSurface.hxx>
#include <Geom_BezierCurve.hxx>
#include <TopoDS.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <cmath>
#include <iostream>
namespace {
void check(bool ok,const QString& message){if(!ok)throw std::runtime_error(message.toStdString());}
void near(double a,double b,const QString& message){check(std::abs(a-b)<1e-6,message);}
QJsonObject move(QJsonObject doc,const QString& sketch,const QString& point,double u,double v){auto rows=doc.value("features").toArray();for(int i=0;i<rows.size();++i){auto f=rows[i].toObject();if(f.value("id")!=sketch)continue;auto p=f.value("parameters").toObject();auto points=p.value("points").toArray();for(int j=0;j<points.size();++j){auto q=points[j].toObject();if(q.value("id")==point){q["uMm"]=u;q["vMm"]=v;points[j]=q;}}p["points"]=points;f["parameters"]=p;rows[i]=f;}doc["features"]=rows;return doc;}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    CadDocument doc;const auto owner=doc.authored().value("documentId").toString();
    auto xy=SketchGeometry::newFeature(owner,"XY","AB BC");
    xy=SketchGeometry::addLine(xy,{{"u1Mm",0},{"v1Mm",0},{"u2Mm",40},{"v2Mm",0}});
    xy=SketchGeometry::addLine(xy,{{"u1Mm",40},{"v1Mm",0},{"u2Mm",0},{"v2Mm",30}});
    auto yz=SketchGeometry::newFeature(owner,"YZ","CD DA");
    yz=SketchGeometry::addLine(yz,{{"u1Mm",30},{"v1Mm",0},{"u2Mm",0},{"v2Mm",10}});
    yz=SketchGeometry::addLine(yz,{{"u1Mm",0},{"v1Mm",10},{"u2Mm",0},{"v2Mm",0}});
    check(doc.commit(SketchGeometry::replaceFeature(doc.authored(),xy),"XY").isEmpty(),"XY source");
    check(doc.commit(SketchGeometry::replaceFeature(doc.authored(),yz),"YZ").isEmpty(),"YZ source");
    const auto xe=xy.value("parameters").toObject().value("entities").toArray(),ye=yz.value("parameters").toObject().value("entities").toArray();
    QJsonArray refs;for(const auto& e:xe)refs.append(QJsonObject{{"sketchId",xy.value("id")},{"entityId",e.toObject().value("id")}});for(const auto& e:ye)refs.append(QJsonObject{{"sketchId",yz.value("id")},{"entityId",e.toObject().value("id")}});
    const auto source=doc.authored();const auto candidate=CadDocument::surfacePatchCandidate(source,{{"boundaryMode","fourEdges"},{"boundaryEdges",refs},{"continuity","g0"}});
    const auto preview=CadDocument::evaluateCandidate(candidate);check(preview.ok,preview.error+QString::fromUtf8(QJsonDocument(preview.diagnostics).toJson()));check(doc.bodies().empty() && doc.authored()==source,"isolated preview");
    check(doc.commit(candidate,"Nonplanar Patch").isEmpty(),"Patch commit");const auto first=doc.bodies().front();
    auto verify=[&](double height){const auto& body=doc.bodies().front();check(body.summary.value("valid").toBool() && body.summary.value("type")=="surface" && body.summary.value("volumeMm3").isNull(),"valid sheet");const auto t=body.summary.value("topology").toObject();check(t.value("faces")==1 && t.value("solids")==0 && t.value("edges")==4 && body.summary.value("closedBoundary")==true,"topology");
        check(body.bodyId==first.bodyId && body.supportFaces.front().faceId==first.supportFaces.front().faceId,"retained body face");
        const auto face=body.supportFaces.front().face;const auto surface=Handle(Geom_BezierSurface)::DownCast(BRep_Tool::Surface(face));check(!surface.IsNull() && surface->UDegree()==1 && surface->VDegree()==1,"actual bilinear support");near(surface->Pole(1,2).Z(),height,"actual changed pole");
        const auto a=surface->Pole(1,1),b=surface->Pole(2,1),c=surface->Pole(2,2),d=surface->Pole(1,2);near(std::abs(gp_Vec(a,b).Dot(gp_Vec(a,c).Crossed(gp_Vec(a,d)))),1200*height,"actual nonplanarity determinant");
        TopTools_IndexedMapOfShape output;TopExp::MapShapes(body.shape,TopAbs_EDGE,output);check(output.Extent()==4,"four actual boundaries");
        for(int i=0;i<4;++i){const auto& edge=body.solidEdges[i];check(output.Contains(edge.edge) && edge.edgeId==first.solidEdges[i].edgeId,"actual lineage and stable edge");const auto ref=refs[i].toObject();check(edge.provenance.value("sourceSketchId")==ref.value("sketchId") && edge.provenance.value("sourceEntityId")==ref.value("entityId"),"ordered refs");
            const EvaluatedSketch* sketch=nullptr;for(const auto& s:doc.evaluation().sketches)if(s.featureId==ref.value("sketchId"))sketch=&s;check(sketch,"source");const BRepAdaptor_Curve sourceCurve(sketch->entityEdges.value(ref.value("entityId").toString()));const auto p=sourceCurve.Value(sourceCurve.FirstParameter()),q=sourceCurve.Value(sourceCurve.LastParameter());const gp_Vec span(p,q);
            double f,l;const auto curve=BRep_Tool::Curve(edge.edge,f,l);const auto bezier=Handle(Geom_BezierCurve)::DownCast(curve);check(!bezier.IsNull() && bezier->Degree()==1 && !bezier->IsRational(),"actual whole-curve degree1");
            for(int k=1;k<=bezier->NbPoles();++k){const auto pole=bezier->Pole(k);const double along=gp_Vec(p,pole).Dot(span)/span.SquareMagnitude();check(along>=-1e-8 && along<=1+1e-8 && pole.Distance(p.Translated(span*along))<1e-7,"complete curve control hull on finite source segment");}}
        check(body.summary.value("maxBoundaryDeviationBoundMm").toDouble()<=0.01,"certified tolerance");
    };verify(10);
    const auto initial=doc.authored();doc.history().undo();check(doc.bodies().empty() && doc.authored()==source,"undo only Patch");doc.history().redo();verify(10);
    auto changed=move(initial,yz.value("id").toString(),ye[0].toObject().value("endPointId").toString(),0,15);changed=move(changed,yz.value("id").toString(),ye[1].toObject().value("startPointId").toString(),0,15);
    check(doc.commit(changed,"Source corner edit").isEmpty(),"source edit");verify(15);doc.history().undo();verify(10);doc.history().redo();verify(15);
    check(doc.commit(CadDocument::surfacePatchCandidate(doc.authored(),{{"featureId",first.featureId},{"name","Edited Patch"}}),"Edit Patch").isEmpty(),"feature edit");verify(15);
    const auto accepted=doc.authored();const auto revision=doc.revision();
    auto disconnected=move(accepted,yz.value("id").toString(),ye[0].toObject().value("startPointId").toString(),35,0);
    check(!doc.commit(disconnected,"Disconnect").isEmpty() && doc.authored()==accepted && doc.revision()==revision,"atomic disconnected source refusal");verify(15);
    QJsonArray wrongOrder{refs[0],refs[2],refs[1],refs[3]};check(!doc.commit(CadDocument::surfacePatchCandidate(accepted,{{"featureId",first.featureId},{"boundaryEdges",wrongOrder}}),"Wrong order").isEmpty() && doc.authored()==accepted && doc.revision()==revision,"atomic ordered-loop refusal");
    for(int fixture:{0,1,2}){bool refused=false;try{auto invalid=refs;if(fixture==0)invalid.removeLast();if(fixture==1)invalid[3]=invalid[0];CadDocument::surfacePatchCandidate(accepted,{{"featureId",first.featureId},{"boundaryEdges",invalid},{"continuity",fixture==2?"g1":"g0"}});}catch(const std::invalid_argument&){refused=true;}check(refused && doc.authored()==accepted && doc.revision()==revision,"underdefined duplicate higher-continuity atomic refusal");}
    QTemporaryDir tmp;const auto file=tmp.filePath("nonplanar-patch.alloy");check(doc.save(file).isEmpty(),"save");CadDocument reopened;check(reopened.open(file).isEmpty() && reopened.authored()==accepted,"reopen exact authored references");near(reopened.bodies().front().summary.value("areaMm2").toDouble(),doc.bodies().front().summary.value("areaMm2").toDouble(),"reopen area");
    std::cout<<"PASS nonplanar four-Line Patch actual bilinear support, complete boundary hull, source edit, atomic refusals, history/reopen\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
