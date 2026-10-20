// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <iostream>
#include <numbers>
#include <cmath>

namespace {
void check(bool ok,const QString& message){if(!ok)throw std::runtime_error(message.toStdString());}
void near(double actual,double expected,const char* message){check(std::abs(actual-expected)<1e-5,message);}
QJsonObject movePoints(QJsonObject authored,const QString& sketchId,const QString& pointId,double u,double v){
    auto rows=authored["features"].toArray();for(int i=0;i<rows.size();++i){auto f=rows[i].toObject();if(f["id"]!=sketchId)continue;
        auto p=f["parameters"].toObject();auto points=p["points"].toArray();for(int j=0;j<points.size();++j){auto q=points[j].toObject();if(q["id"]==pointId){q["uMm"]=u;q["vMm"]=v;points[j]=q;}}
        p["points"]=points;f["parameters"]=p;rows[i]=f;}authored["features"]=rows;return authored;
}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    CadDocument doc;auto sketch=SketchGeometry::newFeature(doc.authored()["documentId"].toString(),"XZ","Open section");
    sketch=SketchGeometry::addLine(sketch,{{"u1Mm",20},{"v1Mm",-10},{"u2Mm",20},{"v2Mm",10}});
    const auto source=sketch["parameters"].toObject()["entities"].toArray().last().toObject();
    sketch=SketchGeometry::addLine(sketch,{{"u1Mm",0},{"v1Mm",-15},{"u2Mm",0},{"v2Mm",15}});
    const auto axis=sketch["parameters"].toObject()["entities"].toArray().last().toObject();
    const auto construction=SketchGeometry::setConstruction(sketch,{{"entityIds",QJsonArray{axis["id"]}},{"construction",true}});
    check(construction.ok,"construction axis");sketch=construction.feature;
    check(doc.commit(SketchGeometry::replaceFeature(doc.authored(),sketch),"Source").isEmpty(),"source commit");
    QJsonObject args{{"sketchId",sketch["id"]},{"entityId",source["id"]},{"axisEntityId",axis["id"]}};
    const auto candidate=CadDocument::surfaceRevolveCandidate(doc.authored(),args);
    const auto preview=CadDocument::evaluateCandidate(candidate);check(preview.ok,preview.error);
    check(doc.bodies().empty(),"preview has no authored effect");
    check(doc.commit(candidate,"Revolve").isEmpty(),"sheet commit");
    const auto first=doc.bodies().front();const auto bodyId=first.bodyId;const auto faceId=first.supportFaces.front().faceId;
    auto verify=[&](double area){const auto& b=doc.bodies().front();const auto t=b.summary["topology"].toObject();
        check(b.summary["type"]=="surface" && b.summary["valid"]==true && b.summary["closed"]==false
            && t["faces"]==1 && t["solids"]==0 && t["edges"]==4 && b.summary["volumeMm3"].isNull(),"one uncapped open sheet");
        near(b.summary["areaMm2"].toDouble(),area,"analytic area");
        check(b.bodyId==bodyId && b.supportFaces.front().faceId==faceId && b.solidEdges.size()==4,"stable retained lineage");
        for(int i=0;i<4;++i)check(b.solidEdges[i].edgeId==first.solidEdges[i].edgeId,"stable boundary IDs");
    };
    verify(200*std::numbers::pi);
    const auto bounds=first.summary["boundsMm"].toObject();const auto minimum=bounds["min"].toArray(),maximum=bounds["max"].toArray();
    near(minimum[0].toDouble(),0,"quarter x minimum");near(maximum[0].toDouble(),20,"quarter x maximum");
    near(minimum[1].toDouble(),0,"quarter y minimum");near(maximum[1].toDouble(),20,"quarter y maximum");
    near(minimum[2].toDouble(),-10,"quarter z minimum");near(maximum[2].toDouble(),10,"quarter z maximum");
    const auto initial=doc.authored();args={{"featureId",first.featureId},{"angleDeg",45.},{"reverseDirection",true}};
    check(doc.commit(CadDocument::surfaceRevolveCandidate(initial,args),"Angle edit").isEmpty(),"partial edit");verify(100*std::numbers::pi);
    check(doc.bodies().front().summary["boundsMm"].toObject()["max"].toArray()[1].toDouble()<1e-5,"reverse quadrant");
    const auto reversed=doc.authored();doc.history().undo();check(doc.authored()==initial,"undo exact authoring");verify(200*std::numbers::pi);
    doc.history().redo();check(doc.authored()==reversed,"redo exact authoring");verify(100*std::numbers::pi);
    auto changed=movePoints(doc.authored(),sketch["id"].toString(),source["endPointId"].toString(),20,20);
    check(doc.commit(changed,"Source edit").isEmpty(),"retained section recompute");verify(150*std::numbers::pi);
    changed=movePoints(doc.authored(),sketch["id"].toString(),axis["startPointId"].toString(),5,-15);
    changed=movePoints(changed,sketch["id"].toString(),axis["endPointId"].toString(),5,15);
    check(doc.commit(changed,"Axis edit").isEmpty(),"retained solved axis recompute");verify(112.5*std::numbers::pi);
    const auto accepted=doc.authored();const auto revision=doc.revision();
    auto cross=movePoints(accepted,sketch["id"].toString(),source["startPointId"].toString(),0,-10);
    check(!doc.commit(cross,"Crossing").isEmpty() && doc.lastEvaluationError()["code"]=="axis_crosses_profile"
        && doc.authored()==accepted && doc.revision()==revision,"atomic crossing refusal");
    auto touch=movePoints(accepted,sketch["id"].toString(),source["startPointId"].toString(),5,-10);
    check(!doc.commit(touch,"Touching").isEmpty() && doc.lastEvaluationError()["code"]=="axis_crosses_profile" && doc.authored()==accepted,"touching refusal");
    auto bad=args;bad["axisEntityId"]=CadDocument::newId();
    check(!doc.commit(CadDocument::surfaceRevolveCandidate(accepted,bad),"Missing axis").isEmpty()
        && doc.lastEvaluationError()["code"]=="axis_unavailable" && doc.authored()==accepted,"missing axis atomic");
    for(double angle:{0.,-1.,360.}){bool refused=false;try{CadDocument::surfaceRevolveCandidate(accepted,{{"featureId",first.featureId},{"angleDeg",angle}});}catch(const std::invalid_argument&){refused=true;}check(refused,"invalid angle refused");}
    check(doc.commit(CadDocument::surfaceRevolveCandidate(accepted,{{"featureId",first.featureId},{"angleDeg",90.}}),"Valid retry").isEmpty(),"valid retry");verify(225*std::numbers::pi);
    QTemporaryDir temp;const auto path=temp.filePath("surface-revolve.alloy");check(doc.save(path).isEmpty(),"save");CadDocument reopened;
    check(reopened.open(path).isEmpty() && reopened.authored()==doc.authored() && reopened.bodies().front().bodyId==bodyId,"reopen exact retained input");
    near(reopened.bodies().front().summary["areaMm2"].toDouble(),225*std::numbers::pi,"reopen geometry");
    std::cout<<"PASS retained surface Revolve area/bounds/lineage, solved source+axis edits, atomic refusal/retry, undo/redo/reopen\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
