// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SolidChamfer.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_Common.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <TopoDS.hxx>
#include <gp_Ax1.hxx>
#include <gp_Ax2.hxx>
#include <cmath>
#include <iostream>
#include <stdexcept>
namespace {
void check(bool value,const char* why){if(!value)throw std::runtime_error(why);}
double volume(const TopoDS_Shape& s){GProp_GProps p;BRepGProp::VolumeProperties(s,p);return p.Mass();}
const EvaluatedBody& body(const CadDocument& doc,const QString& id){
    for(const auto& b:doc.bodies())if(b.bodyId==id)return b;
    throw std::runtime_error("Target body missing");
}
}
int main(int argc,char**argv){QCoreApplication app(argc,argv);try {
    check(argc==2,"Pass the source-shaped first-two-finishes fixture");
    CadDocument doc;check(doc.open(QString::fromLocal8Bit(argv[1])).isEmpty(),"Fixture open failed");
    QJsonObject join;for(const auto&v:doc.authored()["features"].toArray()){
        const auto f=v.toObject();if(f["type"]=="extrude" && f["parameters"].toObject()["operation"]=="join")join=f;
    }
    check(!join.isEmpty(),"Source Join missing");const auto jp=join["parameters"].toObject();
    const auto target=jp["targetBodyIds"].toArray().first().toString();
    const auto sourceBodies=doc.bodies();const auto original=body(doc,target);QString root;
    for(const auto&e:original.solidEdges)if(!e.edge.IsNull() && e.provenance["role"]=="join.root"){
        check(root.isEmpty(),"Ambiguous root");root=e.edgeId;
    }
    check(!root.isEmpty(),"Published root missing");
    const QJsonObject parameters{{"targetBodyId",target},{"ownerId",join["ownerId"]},
        {"edgeIds",QJsonArray{root}},{"distanceMm",2}};
    const auto before=doc.authored();
    const auto error=doc.commit(CadDocument::chamferCandidate(before,parameters),"Exact root Chamfer");
    if(!error.isEmpty())throw std::runtime_error(error.toStdString());
    check(body(doc,target).summary["chamferMethod"]=="planarConcave","Exact limit did not exercise certified construction");
    std::cout<<"Exact root accepted\n"<<std::flush;
    check(std::abs(volume(body(doc,target).shape)-volume(original.shape)-11)<1e-6,"Exact added wedge volume differs");
    for(const auto& other:sourceBodies)if(other.bodyId!=target) {
        BRepAlgoAPI_Common beforeCommon(original.shape,other.shape),afterCommon(body(doc,target).shape,other.shape);
        check(beforeCommon.IsDone() && !beforeCommon.HasErrors() && afterCommon.IsDone() && !afterCommon.HasErrors(),"Assembly interference check failed");
        check(volume(afterCommon.Shape())-volume(beforeCommon.Shape())<=1e-6,"Exact root added assembly interference");
    }
    const auto finished=doc.authored();
    // Rigid placement invariance: the production fallback uses incident geometry,
    // never fixture coordinates or an axis-aligned special case.
    gp_Trsf transform;transform.SetRotation(gp_Ax1(gp_Pnt(0,0,0),gp_Dir(1,2,3)),0.63);
    transform.SetTranslationPart(gp_Vec(7,-12,4));
    gp_Trsf reflection;reflection.SetMirror(gp_Ax2(gp_Pnt(0,0,0),gp_Dir(1,0,0)));
    for(const auto& placement:std::vector<gp_Trsf>{transform,reflection}) {
        BRepBuilderAPI_Transform moved(original.shape,placement,true);
        auto placed=original;placed.shape=moved.Shape();placed.supportFaces.clear();
        for(auto&e:placed.solidEdges)if(!e.edge.IsNull())e.edge=TopoDS::Edge(moved.ModifiedShape(e.edge));
        SolidChamfer::evaluate(placed,parameters,CadDocument::newId());
        check(std::abs(volume(placed.shape)-volume(original.shape)-11)<1e-6,"Placed exact root changed result volume");
    }
    auto invalid=parameters;invalid["distanceMm"]=4;const auto revision=doc.revision();
    check(!doc.commit(CadDocument::chamferCandidate(before,invalid),"Oversized root").isEmpty(),"Unsupported oversized root accepted");
    check(doc.authored()==finished && doc.revision()==revision,"Rejected finish changed state");
    auto edit=QJsonObject{{"featureId",join["id"]},{"sketchId",jp["sketchId"]},
        {"profileIds",jp["profileIds"]},{"distanceMm",5.25}};
    check(doc.commit(CadDocument::extrusionCandidate(doc.authored(),edit),"Edit source Join").isEmpty(),"Source edit failed");
    const auto edited=doc.authored();doc.history().undo();check(doc.authored()==finished,"Undo lost exact finish");
    doc.history().redo();check(doc.authored()==edited,"Redo lost source edit");
    QTemporaryDir temp;check(doc.save(temp.path()+"/root.alloy").isEmpty(),"Save failed");
    CadDocument reopened;check(reopened.open(temp.path()+"/root.alloy").isEmpty() && reopened.authored()==edited,"Reopen failed");
    check(std::abs(volume(body(reopened,target).shape)-volume(body(doc,target).shape))<1e-6,"Reopen geometry changed");
    std::cout<<"PLANAR_CONCAVE_CHAMFER_PASS\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
