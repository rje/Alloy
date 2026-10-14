// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include "SketchSpline.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QJsonDocument>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
void append(CadDocument& doc,const QJsonObject& feature){auto authored=doc.authored();auto features=authored["features"].toArray();features.append(feature);authored["features"]=features;const auto error=doc.commit(authored,"Add sketch");if(!error.isEmpty())throw std::runtime_error(error.toStdString());}
QJsonObject spline(const QJsonArray& points){return {{"points",points}};}
QJsonArray ids(const QJsonObject& feature){QJsonArray result;for(const auto& value:feature["parameters"].toObject()["entities"].toArray())result.append(value.toObject()["id"]);return result;}
void good(const CadDocument::CandidateEvaluation& evaluation){if(!evaluation.ok)throw std::runtime_error((evaluation.error+" "+QString::fromUtf8(QJsonDocument(evaluation.diagnostics).toJson(QJsonDocument::Compact))).toStdString());}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    CadDocument doc;const auto owner=doc.authored()["documentId"].toString();
    auto sketch=SketchGeometry::newFeature(owner,"XZ","Back and bottom");
    auto first=SketchSpline::add(sketch,spline(QJsonArray{QJsonObject{{"uMm",0},{"vMm",0}},QJsonObject{{"uMm",15},{"vMm",13}},QJsonObject{{"uMm",25},{"vMm",10}}}));
    check(first.ok,"First open spline");sketch=first.feature;
    auto second=SketchSpline::add(sketch,spline(QJsonArray{QJsonObject{{"uMm",25},{"vMm",10}},QJsonObject{{"uMm",35},{"vMm",13}},QJsonObject{{"uMm",50},{"vMm",0}}}));
    check(second.ok,"Second open spline");sketch=second.feature;
    sketch=SketchGeometry::addLine(sketch,{{"u1Mm",25},{"v1Mm",10},{"u2Mm",25},{"v2Mm",5}});
    const auto sourceIds=ids(sketch);check(sourceIds.size()==3,"Initial source selection");append(doc,sketch);
    if(argc>1)check(doc.save(QString::fromLocal8Bit(argv[1])).isEmpty(),"Save prepared open sketch");
    const QJsonObject args{{"sketchId",sketch["id"]},{"entityIds",sourceIds},
        {"sideOneMm",25.0},{"sideTwoMm",30.0},{"name","Back sheets"}};
    auto candidate=CadDocument::surfaceExtrudeCandidate(doc.authored(),args);
    good(CadDocument::evaluateCandidate(candidate));
    check(doc.commit(candidate,"Surface Extrude").isEmpty(),"Commit Two Sides");
    const auto initial=doc.authored();const auto sheet=doc.bodies().back();
    check(sheet.summary["type"]=="surface" && sheet.summary["valid"]==true
        && sheet.summary["topology"].toObject()["solids"]==0
        && sheet.summary["topology"].toObject()["faces"]==3
        && sheet.summary["volumeMm3"].isNull() && sheet.summary["areaMm2"].toDouble()>0,
        "Three zero-volume source sheets");
    check(sheet.supportFaces.size()==3 && sheet.solidEdges.size()==12,"Exact face and boundary publication");
    const auto descriptors=sheet.summary["supportFaces"].toArray();
    check(descriptors[0].toObject()["planar"]==false && descriptors[1].toObject()["planar"]==false
        && descriptors[2].toObject()["planar"]==true,"Truthful spline and line sheet planarity");
    const auto planarFrame=descriptors[2].toObject()["frame"].toObject();
    check(planarFrame["origin"].toArray().size()==3 && planarFrame["uAxis"].toArray().size()==3
        && planarFrame["vAxis"].toArray().size()==3 && planarFrame["normal"].toArray().size()==3,
        "Planar line sheet has resolved support frame");
    const QJsonObject support{{"producerFeatureId",sheet.featureId},{"bodyId",sheet.bodyId},
        {"faceId",sheet.supportFaces[2].faceId}};
    const auto attached=SketchGeometry::newSupportedFeature(owner,support,"Attached to planar sheet");
    auto attachment=initial;auto attachmentFeatures=attachment["features"].toArray();
    attachmentFeatures.append(attached);attachment["features"]=attachmentFeatures;
    const auto resolvedAttachment=CadDocument::evaluateCandidate(attachment);good(resolvedAttachment);
    check(resolvedAttachment.evaluation->sketches.back().resolvedFrame["normal"].toArray().size()==3,
        "Dependent sketch resolves against actual planar sheet face");
    const auto faceId=sheet.supportFaces[0].faceId,firstEdge=sheet.supportFaces[0].boundaryEdges[0].edgeId;
    const auto limits=sheet.summary["boundsMm"].toObject();
    check(limits["min"].toArray()[1].toDouble()<-29.99 && limits["max"].toArray()[1].toDouble()>24.99,
        "Independently authored opposite-side reaches");
    auto changed=sketch;auto points=changed["parameters"].toObject()["points"].toArray();
    const auto middle=changed["parameters"].toObject()["entities"].toArray()[0].toObject()["pointIds"].toArray()[1];
    for(int i=0;i<points.size();++i)if(points[i].toObject()["id"]==middle){auto point=points[i].toObject();point["vMm"]=18.0;points[i]=point;}
    auto parameters=changed["parameters"].toObject();parameters["points"]=points;changed["parameters"]=parameters;
    check(doc.commit(SketchGeometry::replaceFeature(doc.authored(),changed),"Edit back spline").isEmpty(),"Source spline edit");
    check(doc.bodies().back().supportFaces[0].faceId==faceId
        && doc.bodies().back().supportFaces[0].boundaryEdges[0].edgeId==firstEdge
        && std::abs(doc.bodies().back().summary["areaMm2"].toDouble()-sheet.summary["areaMm2"].toDouble())>1,
        "Current area follows source edit with stable IDs");
    changed=SketchGeometry::addLine(changed,{{"u1Mm",0},{"v1Mm",0},{"u2Mm",50},{"v2Mm",0}});
    check(doc.commit(SketchGeometry::replaceFeature(doc.authored(),changed),"Add bottom line").isEmpty(),"Bottom line source edit");
    auto selected=ids(changed);check(selected.size()==4,"Bottom line authored in same sketch");
    const auto feature=initial["features"].toArray().last().toObject();
    auto include=CadDocument::surfaceExtrudeCandidate(doc.authored(),{{"featureId",feature["id"]},
        {"sketchId",sketch["id"]},{"entityIds",selected},{"sideOneMm",25.0},{"sideTwoMm",30.0}});
    check(doc.commit(include,"Include bottom line").isEmpty(),"Edit same Surface Extrude");
    check(doc.bodies().back().bodyId==sheet.bodyId && doc.bodies().back().supportFaces.size()==4
        && doc.bodies().back().supportFaces[0].faceId==faceId,"Stable body/source faces after explicit selection edit");
    const auto final=doc.authored();doc.history().undo();check(doc.bodies().back().supportFaces.size()==3,"Undo selection edit");
    doc.history().redo();check(doc.authored()==final && doc.bodies().back().supportFaces.size()==4,"Redo selection edit");
    QTemporaryDir temp;const auto path=temp.filePath("surface-extrude.alloy");
    check(doc.save(path).isEmpty(),"Save surface");CadDocument reopened;
    check(reopened.open(path).isEmpty() && reopened.authored()==final
        && reopened.bodies().back().supportFaces.size()==4,"Reopen same surface feature");
    auto bad=CadDocument::surfaceExtrudeCandidate(reopened.authored(),{{"featureId",feature["id"]},
        {"sketchId",sketch["id"]},{"entityIds",QJsonArray{CadDocument::newId()}},
        {"sideOneMm",25.0},{"sideTwoMm",30.0}});
    check(!reopened.commit(bad,"Invalid surface source").isEmpty() && reopened.authored()==final,
        "Invalid source edit must be atomic");
    std::cout<<"SURFACE_EXTRUDE_OK area="<<doc.bodies().back().summary["areaMm2"].toDouble()<<"\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
