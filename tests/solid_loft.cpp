// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include <BRepAlgoAPI_Section.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <gp_Pln.hxx>
#include <QCoreApplication>
#include <QTemporaryDir>
#include <iostream>
#include <cmath>
#include <stdexcept>
namespace {
void check(bool okay,const QString& message){if(!okay)throw std::runtime_error(message.toStdString());}
void append(CadDocument& d,const QJsonObject& f){auto a=d.authored();auto fs=a["features"].toArray();fs.append(f);a["features"]=fs;check(d.commit(a,"Append").isEmpty(),d.lastEvaluationError()["message"].toString());}
double radiusAt(const TopoDS_Shape& shape,double z) {
    BRepAlgoAPI_Section section(shape,gp_Pln(gp_Pnt(0,0,z),gp_Dir(0,0,1)),false);section.Build();check(section.IsDone(),"Section failed");double sum=0,minimum=1e9,maximum=0;int count=0;
    for(TopExp_Explorer edges(section.Shape(),TopAbs_EDGE);edges.More();edges.Next()){BRepAdaptor_Curve c(TopoDS::Edge(edges.Current()));for(int i=0;i<32;++i){auto p=c.Value(c.FirstParameter()+(c.LastParameter()-c.FirstParameter())*i/32);const auto radius=std::hypot(p.X(),p.Y());minimum=std::min(minimum,radius);maximum=std::max(maximum,radius);sum+=radius;++count;}}
    check(count>0,"No section edges");check(maximum-minimum<1e-4,"Section is not circular");return sum/count;
}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try {
    CadDocument d;const auto owner=d.authored()["documentId"].toString();QJsonArray refs;QString preceding;
    const double diameters[]{38,16,30,44};
    for(int i=0;i<4;++i){QJsonObject sketch;if(i==0)sketch=SketchGeometry::newFeature(owner,"XY","Base");else{const auto planeId=CadDocument::newId();append(d,{{"id",planeId},{"ownerId",owner},{"type","constructionPlane"},{"name","Plane"},{"visible",true},{"parameters",QJsonObject{{"source",QJsonObject{{"sketchId",preceding}}},{"offsetMm",i==1 ? 18 : 20}}}});sketch=SketchGeometry::newConstructionPlaneFeature(owner,planeId,"Section");}
        sketch=SketchGeometry::addCircle(sketch,{{"centerUMm",0},{"centerVMm",0},{"diameterMm",diameters[i]}});append(d,sketch);preceding=sketch["id"].toString();refs.append(QJsonObject{{"sketchId",preceding},{"profileId",d.sketches().back().profiles[0].profileId}});
    }
    auto candidate=CadDocument::loftCandidate(d.authored(),{{"profiles",refs},{"name","Egg cup loft"}});check(d.commit(candidate,"Loft").isEmpty(),"Loft failed: "+d.lastEvaluationError()["message"].toString());
    check(d.bodies().size()==1 && d.bodies()[0].summary["valid"].toBool() && d.bodies()[0].summary["closed"].toBool(),"Loft valid closed body");
    const auto body=d.bodies()[0];const double heights[]{0,18,38,58};for(int i=0;i<4;++i)check(std::abs(radiusAt(body.shape,heights[i])-diameters[i]/2)<1e-4,"Section radius mismatch");
    const auto middle=radiusAt(body.shape,9);check(std::abs(middle-13.5)>0.1,"Ruled substitute detected");std::cout<<"Radius at z9: "<<middle<<'\n';
    const auto cap=body.supportFaces.back();QJsonObject support{{"producerFeatureId",cap.producerFeatureId},{"bodyId",cap.bodyId},{"faceId",cap.faceId}};
    check(d.commit(CadDocument::shellCandidate(d.authored(),{{"targetBodyId",body.bodyId},{"removedFace",support},{"thicknessMm",2}}),"Shell").isEmpty(),"Shell 2mm failed: "+d.lastEvaluationError()["message"].toString());
    check(d.bodies()[0].summary["valid"].toBool() && d.bodies()[0].summary["closed"].toBool(),"Shell is not valid closed material");
    const auto stable=d.authored();d.history().undo();check(d.authored()==candidate,"Exact shell undo");d.history().redo();check(d.authored()==stable,"Exact shell redo");
    auto bad=refs;const QJsonValue saved=bad[1];bad[1]=bad[2];bad[2]=saved;const auto revision=d.revision();check(!d.commit(CadDocument::loftCandidate(d.authored(),{{"featureId",candidate["features"].toArray().last().toObject()["id"]},{"profiles",bad}}),"Bad order").isEmpty(),"Bad ordering accepted");check(d.authored()==stable && d.revision()==revision,"Bad order atomic");
    auto edited=d.authored();auto fs=edited["features"].toArray();auto plane=fs[1].toObject();auto params=plane["parameters"].toObject();params["offsetMm"]=19;plane["parameters"]=params;fs[1]=plane;edited["features"]=fs;check(d.commit(edited,"Edit plane").isEmpty(),"Plane edit and Shell rebuild failed");
    check(std::abs(d.bodies()[0].summary["boundsMm"].toObject()["max"].toArray()[2].toDouble()-59)<1e-4,"Edited plane not reflected");
    edited=d.authored();fs=edited["features"].toArray();auto base=fs[0].toObject();params=base["parameters"].toObject();auto entities=params["entities"].toArray();auto circle=entities[0].toObject();circle["radiusMm"]=20;entities[0]=circle;params["entities"]=entities;base["parameters"]=params;fs[0]=base;edited["features"]=fs;
    check(d.commit(edited,"Edit diameter").isEmpty(),"Diameter edit Loft/Shell rebuild failed");
    check(d.bodies()[0].bodyId==body.bodyId && d.bodies()[0].supportFaces[0].faceId==cap.faceId,"Retained body/rim IDs changed after source edits");
    auto stale=refs;auto reference=stale[1].toObject();reference["profileId"]=CadDocument::newId();stale[1]=reference;const auto beforeStale=d.authored();
    check(!d.commit(CadDocument::loftCandidate(d.authored(),{{"featureId",candidate["features"].toArray().last().toObject()["id"]},{"profiles",stale}}),"Missing profile").isEmpty() && d.authored()==beforeStale,"Missing profile did not fail atomically");
    QTemporaryDir temp;const auto path=temp.filePath("loft.alloy");check(d.save(path).isEmpty(),"Save");CadDocument reopened;check(reopened.open(path).isEmpty() && reopened.authored()==d.authored(),"Exact reopen");
    std::cout<<"Smooth exact-section Loft, 2mm Shell, order refusal, offset rebuild and storage passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
