// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QSet>
#include <iostream>
#include <cmath>
#include <numbers>
namespace {
void check(bool pass,const QString& reason){if(!pass)throw std::runtime_error(reason.toStdString());}
void commit(CadDocument& d,const QJsonObject& value){const auto e=d.commit(value,"Fixture");check(e.isEmpty(),e);}
QSet<QString> faces(const EvaluatedBody& b){QSet<QString> result;for(const auto& f:b.supportFaces)result.insert(f.faceId);return result;}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    CadDocument d;auto a=d.authored();auto sketch=SketchGeometry::newFeature(a["documentId"].toString(),"XY","Die");
    sketch=SketchGeometry::addRectangle(sketch,{{"u1Mm",-10},{"v1Mm",-10},{"u2Mm",10},{"v2Mm",10}});a["features"]=QJsonArray{sketch};commit(d,a);
    commit(d,CadDocument::extrusionCandidate(d.authored(),{{"sketchId",sketch["id"]},{"profileId",d.sketches()[0].profiles[0].profileId},{"distanceMm",20}}));
    const auto body=d.bodies()[0].bodyId;QJsonArray edges;for(const auto& e:d.bodies()[0].solidEdges)edges.append(e.edgeId);
    QJsonObject rounding{{"targetBodyId",body},{"edgeIds",edges},{"radiusMm",1.0},{"tangentChain",true}};commit(d,CadDocument::filletCandidate(d.authored(),rounding));
    const auto rounded=d.authored();const auto supports=d.bodies()[0].supportFaces;const auto ids=faces(d.bodies()[0]);check(ids.size()==6,"Six retained rounded cube faces");
    const double before=d.bodies()[0].summary["volumeMm3"].toDouble();int cuts=0;QJsonObject first;
    for(const auto& face:supports){
        const auto normal=face.frame["normal"].toArray(),origin=face.frame["origin"].toArray(),u=face.frame["uAxis"].toArray(),v=face.frame["vAxis"].toArray();
        int axis=0;for(int k=1;k<3;++k)if(std::abs(normal[k].toDouble())>std::abs(normal[axis].toDouble()))axis=k;
        const bool positive=normal[axis].toDouble()>0;const int count=axis==2 ? (positive ? 1:6):axis==0 ? (positive ? 2:5):(positive ? 3:4);
        QJsonArray offsets;
        if(count==1 || count==3 || count==5)offsets.append(QJsonArray{0,0});
        if(count==2 || count==3){offsets.append(QJsonArray{-5,-5});offsets.append(QJsonArray{5,5});}
        if(count==4 || count==5)for(int x:{-5,5})for(int y:{-5,5})offsets.append(QJsonArray{x,y});
        if(count==6)for(int x:{-5,5})for(int y:{-5,0,5})offsets.append(QJsonArray{x,y});
        double center[3]={0,0,10};center[axis]=axis==2 ? (positive ? 20:0):(positive ? 10:-10);
        double uv[2]={};for(int k=0;k<3;++k){uv[0]+=(center[k]-origin[k].toDouble())*u[k].toDouble();uv[1]+=(center[k]-origin[k].toDouble())*v[k].toDouble();}
        for(const auto& offset:offsets){QJsonObject p{{"support",QJsonObject{{"bodyId",body},{"producerFeatureId",face.producerFeatureId},{"faceId",face.faceId}}},{"targetBodyId",body},{"operation","cut"},{"uMm",uv[0]+offset.toArray()[0].toDouble()},{"vMm",uv[1]+offset.toArray()[1].toDouble()},{"diameterMm",3}};
            commit(d,CadDocument::sphereCandidate(d.authored(),p));if(first.isEmpty())first=p;++cuts;check(faces(d.bodies()[0])==ids,"Sphere changed surviving planar face identity");}
    }
    check(cuts==21 && std::abs(before-d.bodies()[0].summary["volumeMm3"].toDouble()-47.25*std::numbers::pi)<1e-5,"21 hemispheres volume");
    const auto complete=d.authored();const auto revision=d.revision();const auto repeat=CadDocument::sphereCandidate(complete,first);check(!d.commit(repeat,"Invalid repeat").isEmpty() && d.authored()==complete && d.revision()==revision,"Center in existing opening must fail atomically");
    rounding["featureId"]=rounded["features"].toArray().last().toObject()["id"];rounding["radiusMm"]=.8;commit(d,CadDocument::filletCandidate(d.authored(),rounding));check(faces(d.bodies()[0])==ids,"Upstream fillet lost six support IDs");d.history().undo();check(d.authored()==complete,"Upstream undo");d.history().redo();d.history().undo();
    QTemporaryDir directory;const auto path=directory.filePath("die.alloy");check(d.save(path).isEmpty(),"Save");CadDocument reopened;check(reopened.open(path).isEmpty() && reopened.authored()==complete,"Reopen");
    auto newBody=first;newBody["operation"]="newBody";newBody.remove("targetBodyId");auto candidate=CadDocument::sphereCandidate(rounded,newBody);const auto evaluated=CadDocument::evaluateCandidate(candidate);check(evaluated.ok && evaluated.evaluation->bodies.size()==2,"Face-positioned New Body Sphere");
    std::cout<<"PASS face Sphere:21 cuts, six retained faces, volume, edit/history/reopen, New Body\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
