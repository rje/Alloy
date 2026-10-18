// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QJsonArray>
#include <QSet>
#include <QUuid>
#include <algorithm>
#include <cmath>
#include <iostream>
namespace {
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
struct Names {QSet<QString> faces,edges;};
Names inspect(const QJsonObject& sketch,const QString& generator,int rotation,double width) {
    const auto evaluated=SketchGeometry::evaluateFeature(sketch);
    auto regions=evaluated.profiles;
    check(regions.size()==3,"three tangent-circle regions");
    std::rotate(regions.begin(),regions.begin()+rotation%regions.size(),regions.end());
    if(rotation%2)std::reverse(regions.begin(),regions.end());
    const auto united=GeometryKernel::unionProfiles(regions);
    check(united.boundaryEdges.size()==4,"circular seam removed without losing real boundaries");
    for(const auto& b:united.boundaryEdges)
        check(!b.startContact.isEmpty() && !b.endContact.isEmpty(),"retained contact at every capsule boundary endpoint");
    std::vector<EvaluatedSolidEdge> edges;std::vector<EvaluatedSupportFace> faces;
    const auto shape=GeometryKernel::extrusion(united,10,nullptr,&edges,generator,&faces);
    const auto summary=GeometryKernel::summarize(shape);
    check(summary["valid"].toBool() && summary["topology"].toObject()["solids"].toInt()==1,"one valid united prism");
    check(std::abs(summary["volumeMm3"].toDouble()-(10*width+25*std::acos(-1.))*10)<1e-6,"exact capsule volume");
    Names names;int cylinders=0;
    for(const auto& face:faces){check(!names.faces.contains(face.faceId),"unique face ID");names.faces.insert(face.faceId);
        if(face.role!="extrude.cylindricalSide")continue;++cylinders;
        const auto signature=(united.profileId+"/"+face.provenance["sourceEntityId"].toString()).toUtf8();
        check(face.faceId==QUuid::createUuidV5(QUuid(generator),QByteArray("alloy-cylindrical-face/1\n")+signature).toString(QUuid::WithoutBraces),"established cylindrical face ID preserved");}
    check(cylinders==2 && faces.size()==5,"two cylindrical sides, two planar sides and base");
    for(const auto& edge:edges){check(!names.edges.contains(edge.edgeId),"unique edge ID");names.edges.insert(edge.edgeId);}
    check(edges.size()==12,"four base rims, four end rims and four real corners");
    // Two different exact faces claiming one source still refuse. No ambiguity
    // policy in prism publication is relaxed by removing an unreferenced seam.
    auto ambiguous=united;QString firstCircle;
    for(auto& b:ambiguous.boundaryEdges)if(b.type=="circle"){
        if(firstCircle.isEmpty())firstCircle=b.sourceEntityId;else b.sourceEntityId=firstCircle;
    }
    bool rejected=false;
    try{GeometryKernel::extrusion(ambiguous,10,nullptr,nullptr,generator,&faces);}
    catch(const std::exception& e){rejected=QString::fromUtf8(e.what()).contains("Ambiguous cylindrical prism face provenance");}
    check(rejected,"conflicting source identity still refuses");
    return names;
}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try {
    CadDocument doc;
    auto f=SketchGeometry::newFeature(doc.authored()["documentId"].toString(),"XY","Tangent circle region union");
    f=SketchGeometry::addLine(f,{{"u1Mm",0},{"v1Mm",0},{"u2Mm",20},{"v2Mm",0}});
    f=SketchGeometry::addLine(f,{{"u1Mm",0},{"v1Mm",10},{"u2Mm",20},{"v2Mm",10}});
    for(int x:{0,20})f=SketchGeometry::addCircle(f,{{"centerUMm",x},{"centerVMm",5},{"diameterMm",10}});
    const auto generator=CadDocument::newId();const auto names=inspect(f,generator,0,20);
    for(int i=1;i<6;++i){const auto next=inspect(f,generator,i,20);check(next.faces==names.faces && next.edges==names.edges,"region order preserves face and edge IDs");}
    auto edited=f;auto p=edited["parameters"].toObject();auto points=p["points"].toArray();
    for(int i=0;i<points.size();++i){auto point=points[i].toObject();if(point["uMm"].toDouble()>10)point["uMm"]=point["uMm"].toDouble()+2;points[i]=point;}
    p["points"]=points;edited["parameters"]=p;
    const auto updated=inspect(edited,generator,0,22);
    check(updated.faces==names.faces && updated.edges==names.edges,"source edit preserves retained IDs");
    std::cout<<"PASS: exact seam history, region-order/source-edit IDs, established face IDs and ambiguity refusal\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
