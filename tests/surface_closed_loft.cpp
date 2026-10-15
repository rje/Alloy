// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include "SketchSpline.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <BRepAdaptor_Curve.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <QJsonDocument>
#include <QStringList>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <numbers>
#include <stdexcept>

namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void append(CadDocument& document,const QJsonObject& feature) {
    auto candidate=document.authored();auto rows=candidate["features"].toArray();rows.append(feature);candidate["features"]=rows;
    const auto error=document.commit(candidate,"Add Loft section");
    if(!error.isEmpty())throw std::runtime_error(error.toStdString());
}
QJsonArray samples(double bulge,int sign) {
    QJsonArray points;
    for(int i=0;i<5;++i){const auto angle=i*std::numbers::pi/4;
        points.append(QJsonObject{{"uMm",60-40*std::cos(angle)},{"vMm",30+sign*bulge*std::sin(angle)}});}
    return points;
}
QJsonObject lens(const QString& owner,const QString& plane,double bulge,const QString& name) {
    auto sketch=SketchGeometry::newConstructionPlaneFeature(owner,plane,name);
    for(int sign:{1,-1}){
        const auto result=SketchSpline::add(sketch,{{"points",samples(bulge,sign)}});
        if(!result.ok)throw std::runtime_error(result.error.toStdString());sketch=result.feature;
    }
    return sketch;
}
QJsonArray refs(const QJsonObject& sketch,const QString& type) {
    QJsonArray result;for(const auto& row:sketch["parameters"].toObject()["entities"].toArray())
        if(row.toObject()["type"]==type)result.append(row.toObject()["id"]);
    return result;
}
void sectionAgreement(const CadDocument& document,const QJsonArray& sections,const EvaluatedBody& body) {
    for(const auto& value:sections){const auto ref=value.toObject();bool found=false;
        for(const auto& sketch:document.sketches())if(sketch.featureId==ref["sketchId"]){
            found=true;for(const auto& id:ref["entityIds"].toArray()){
                const auto edge=sketch.entityEdges.value(id.toString());BRepAdaptor_Curve curve(edge);
                for(int sample=0;sample<=32;++sample){const auto t=curve.FirstParameter()+(curve.LastParameter()-curve.FirstParameter())*sample/32.;
                    BRepExtrema_DistShapeShape distance(BRepBuilderAPI_MakeVertex(curve.Value(t)),body.shape);distance.Perform();
                    check(distance.IsDone() && distance.Value()<1e-4,"A retained section moved off the evaluated shell");
                }
            }break;
        }check(found,"Retained sketch unavailable");
    }
}
QJsonObject editedPoint(const QJsonObject& authored,const QString& sketchId,const QString& pointId) {
    auto candidate=authored;auto features=candidate["features"].toArray();bool edited=false;
    for(int i=0;i<features.size();++i)if(features[i].toObject()["id"]==sketchId){
        auto sketch=features[i].toObject(),p=sketch["parameters"].toObject();auto points=p["points"].toArray();
        for(int j=0;j<points.size();++j)if(points[j].toObject()["id"]==pointId){
            auto point=points[j].toObject();point["vMm"]=48.0;points[j]=point;edited=true;break;}
        p["points"]=points;sketch["parameters"]=p;features[i]=sketch;break;
    }
    check(edited,"Bulge edit source point unavailable");candidate["features"]=features;return candidate;
}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    CadDocument document;const auto root=document.authored()["documentId"].toString();
    auto sectionPlane=[&](double offset,const QString& name){
        QJsonObject plane{{"id",CadDocument::newId()},{"ownerId",root},{"type","constructionPlane"},
            {"name",name},{"visible",true},{"parameters",QJsonObject{{"source",QJsonObject{{"plane","XZ"}}},{"offsetMm",offset}}}};
        append(document,plane);return plane["id"].toString();};
    const auto firstPlane=sectionPlane(25,"First section plane");
    auto first=lens(root,firstPlane,15,"First closed spline section");append(document,first);
    auto middle=SketchGeometry::newFeature(root,"XZ","Middle full ellipse");
    const auto ellipse=SketchGeometry::addCenterPointEllipse(middle,{{"centerUMm",60},{"centerVMm",30},
        {"firstUMm",100},{"firstVMm",30},{"contourUMm",60},{"contourVMm",45}});
    check(ellipse.ok,"Middle ellipse could not be authored");middle=ellipse.feature;append(document,middle);
    const auto lastPlane=sectionPlane(-25,"Last section plane");
    auto last=lens(root,lastPlane,15,"Last closed spline section");append(document,last);
    const QJsonArray sections{QJsonObject{{"sketchId",first["id"]},{"entityIds",refs(first,"fitSpline")}},
        QJsonObject{{"sketchId",middle["id"]},{"entityIds",refs(middle,"ellipse")}},
        QJsonObject{{"sketchId",last["id"]},{"entityIds",refs(last,"fitSpline")}}};
    const QJsonObject args{{"operation","newComponent"},{"guideType","none"},
        {"name","No-rail three-section sheet"},{"componentName","Iron upper"},{"sections",sections}};
    if(argc>1)check(document.save(QString::fromLocal8Bit(argv[1])).isEmpty(),"Save prepared sections");
    auto candidate=CadDocument::loftCandidate(document.authored(),args);
    auto preview=CadDocument::evaluateCandidate(candidate);
    if(!preview.ok)throw std::runtime_error((preview.error+" "+QString::fromUtf8(QJsonDocument(preview.diagnostics).toJson(QJsonDocument::Compact))).toStdString());
    check(document.commit(candidate,"No-guide Loft").isEmpty(),"Loft commit");
    const auto initial=document.authored();
    const auto body=document.bodies().back();
    check(body.summary["valid"].toBool() && body.summary["type"]=="surface" && !body.summary["closed"].toBool()
        && body.summary["topology"].toObject()["faces"]==2 && body.summary["topology"].toObject()["solids"]==0,
        "Expected one valid uncapped two-face sheet");
    check(body.supportFaces.size()==2 && body.solidEdges.size()==6,"Source-derived surface lineage unavailable");
    sectionAgreement(document,sections,body);
    const auto face0=body.supportFaces[0].faceId,face1=body.supportFaces[1].faceId;
    QStringList initialEdgeIds;for(const auto& edge:body.solidEdges)initialEdgeIds.append(edge.edgeId);
    std::sort(initialEdgeIds.begin(),initialEdgeIds.end());
    auto hidden=initial;auto hiddenFeatures=hidden["features"].toArray();
    for(int i=0;i<hiddenFeatures.size();++i)if(hiddenFeatures[i].toObject()["id"]==first["id"]){
        auto sketch=hiddenFeatures[i].toObject();sketch["visible"]=false;hiddenFeatures[i]=sketch;break;}
    hidden["features"]=hiddenFeatures;
    check(document.commit(hidden,"Hide source sketch").isEmpty()
        && document.bodies().back().supportFaces[0].faceId==face0,
        "Hiding an already selected source sketch must not break Loft replay");
    document.history().undo();check(document.authored()==initial,"Undo source visibility");
    const auto area=body.summary["areaMm2"].toDouble();
    const auto firstEntity=first["parameters"].toObject()["entities"].toArray()[0].toObject();
    const auto midPoint=firstEntity["pointIds"].toArray()[2].toString();
    auto changed=editedPoint(initial,first["id"].toString(),midPoint);
    check(document.commit(changed,"Edit first section bulge").isEmpty(),"Source edit failed");
    const auto editedBody=document.bodies().back();
    check(editedBody.supportFaces[0].faceId==face0 && editedBody.supportFaces[1].faceId==face1
        && std::abs(editedBody.summary["areaMm2"].toDouble()-area)>1,
        "Meaningful source edit must change area while retaining face identities");
    QStringList editedEdgeIds;for(const auto& edge:editedBody.solidEdges)editedEdgeIds.append(edge.edgeId);
    std::sort(editedEdgeIds.begin(),editedEdgeIds.end());
    check(editedEdgeIds==initialEdgeIds,"Current boundaries lost their retained entity identities after source edit");
    sectionAgreement(document,sections,editedBody);
    const auto edited=document.authored();document.history().undo();
    check(document.authored()==initial && document.bodies().back().supportFaces[0].faceId==face0,"Undo source edit");
    document.history().redo();check(document.authored()==edited,"Redo source edit");
    QTemporaryDir directory;check(directory.isValid(),"Temporary save directory");
    const auto path=directory.filePath("three-section-loft.alloy");
    check(document.save(path).isEmpty(),"Save retained Loft");CadDocument reopened;
    check(reopened.open(path).isEmpty() && reopened.authored()==edited
        && reopened.bodies().back().supportFaces[0].faceId==face0,"Reopen retained source and Loft");
    auto bad=args;auto badSections=sections;auto badLast=badSections[2].toObject();
    badLast["entityIds"]=QJsonArray{refs(last,"fitSpline")[0],CadDocument::newId()};badSections[2]=badLast;
    bad["sections"]=badSections;bad["featureId"]=body.featureId;
    const auto badCandidate=CadDocument::loftCandidate(reopened.authored(),bad);
    check(!reopened.commit(badCandidate,"Invalid last section").isEmpty() && reopened.authored()==edited,
        "Invalid retained entity must refuse without mutating authored document");
    std::cout<<"SURFACE_CLOSED_LOFT_OK area="<<area<<" edited="<<editedBody.summary["areaMm2"].toDouble()<<'\n';
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
