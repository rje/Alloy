// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool okay,const char* message){if(!okay)throw std::runtime_error(message);}
QJsonObject ref(const EvaluatedSupportFace& face){return {{"producerFeatureId",face.producerFeatureId},
    {"bodyId",face.bodyId},{"faceId",face.faceId}};}
void append(CadDocument& document,const QJsonObject& feature){auto candidate=document.authored();
    auto features=candidate["features"].toArray();features.append(feature);candidate["features"]=features;
    const auto error=document.commit(candidate,"Append feature");
    if(!error.isEmpty())throw std::runtime_error(("Feature commit failed: "+error).toStdString());}
double z(const QJsonObject& frame){return frame["origin"].toArray()[2].toDouble();}
}

int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    CadDocument document;const auto owner=document.authored()["documentId"].toString();
    QJsonObject box{{"id",CadDocument::newId()},{"bodyId",CadDocument::newId()},{"ownerId",owner},
        {"type","box"},{"name","Enclosure"},{"visible",true},
        {"parameters",QJsonObject{{"widthMm",50},{"depthMm",80},{"heightMm",40},
            {"xMm",0},{"yMm",0},{"zMm",0}}}};
    append(document,box);
    const auto faces=document.bodies()[0].supportFaces;
    const auto top=std::find_if(faces.begin(),faces.end(),[](const auto& face){return face.role=="box.z1";});
    const auto bottom=std::find_if(faces.begin(),faces.end(),[](const auto& face){return face.role=="box.z0";});
    check(top!=faces.end() && bottom!=faces.end(),"Exact box top and bottom unavailable");
    const auto topRef=ref(*top),bottomRef=ref(*bottom);
    QJsonObject mid{{"id",CadDocument::newId()},{"ownerId",owner},{"type","constructionPlane"},
        {"name","Midplane"},{"visible",true},{"parameters",QJsonObject{{"source",QJsonObject{{"faces",QJsonArray{topRef,bottomRef}}}}}}};
    append(document,mid);
    check(document.evaluation().constructionPlanes.size()==1 &&
        std::abs(z(document.evaluation().constructionPlanes[0].frame)-20)<1e-7,"Midplane is not halfway between exact faces");
    const auto display=document.evaluation().constructionPlanes[0].displayBoundsMm;
    check(display.size()==4 && display[1].toDouble()-display[0].toDouble()>=50 &&
        display[3].toDouble()-display[2].toDouble()>=50,"Midplane display did not fit the selected faces");
    auto sketch=SketchGeometry::newConstructionPlaneFeature(owner,mid["id"].toString(),"Split profile");
    sketch=SketchGeometry::addRectangle(sketch,{{"u1Mm",0},{"v1Mm",0},{"u2Mm",10},{"v2Mm",10}});
    append(document,sketch);
    check(std::abs(z(document.sketches()[0].resolvedFrame)-20)<1e-7,"Dependent sketch not on Midplane");
    auto edited=document.authored();auto features=edited["features"].toArray();auto resized=features[0].toObject();
    auto parameters=resized["parameters"].toObject();parameters["heightMm"]=60;resized["parameters"]=parameters;
    features[0]=resized;edited["features"]=features;
    check(document.commit(edited,"Resize case").isEmpty(),"Case height edit failed");
    check(std::abs(z(document.evaluation().constructionPlanes[0].frame)-30)<1e-7 &&
        std::abs(z(document.sketches()[0].resolvedFrame)-30)<1e-7,"Midplane or dependent sketch did not follow height");
    const auto stable=document.authored();document.history().undo();
    check(std::abs(z(document.evaluation().constructionPlanes[0].frame)-20)<1e-7,"Undo did not restore Midplane");
    document.history().redo();check(document.authored()==stable,"Redo did not restore authored Midplane");
    QTemporaryDir directory;check(directory.isValid(),"Temp directory unavailable");
    const auto path=directory.filePath("midplane.alloy");check(document.save(path).isEmpty(),"Save failed");
    CadDocument reopened;check(reopened.open(path).isEmpty() && reopened.authored()==stable,"Reopen changed authored Midplane");
    check(std::abs(z(reopened.evaluation().constructionPlanes[0].frame)-30)<1e-7,"Reopen lost evaluated Midplane");
    auto invalid=stable;features=invalid["features"].toArray();auto bad=features[1].toObject();
    auto source=bad["parameters"].toObject()["source"].toObject();source["faces"]=QJsonArray{topRef,topRef};
    bad["parameters"]=QJsonObject{{"source",source}};features[1]=bad;invalid["features"]=features;
    check(!reopened.commit(invalid,"Invalid face pair").isEmpty() && reopened.authored()==stable,
        "Duplicate face edit was not atomic");
    const auto side=std::find_if(faces.begin(),faces.end(),[](const auto& face){return face.role=="box.x0";});
    check(side!=faces.end(),"Side face unavailable");
    invalid=stable;features=invalid["features"].toArray();bad=features[1].toObject();
    bad["parameters"]=QJsonObject{{"source",QJsonObject{{"faces",QJsonArray{topRef,ref(*side)}}}}};
    features[1]=bad;invalid["features"]=features;
    check(!reopened.commit(invalid,"Nonparallel faces").isEmpty() && reopened.authored()==stable,
        "Nonparallel Midplane edit was not atomic");
    std::cout<<"retained midplane core pass\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
