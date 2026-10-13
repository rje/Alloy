// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include <BRepClass3d_SolidClassifier.hxx>
#include <QCoreApplication>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
#include <numbers>

namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void near(double actual,double expected,double tolerance=1e-5){check(std::abs(actual-expected)<tolerance,"Pattern analytic oracle differs");}
QJsonObject solved(const SketchGeometry::SolveResult& result){if(!result.ok)throw std::runtime_error(result.error.toStdString());return result.feature;}
void commit(CadDocument& doc,const QJsonObject& candidate){const auto error=doc.commit(candidate,"Pattern fixture");if(!error.isEmpty())throw std::runtime_error(error.toStdString());}
bool material(const TopoDS_Shape& shape,double x,double y,double z){BRepClass3d_SolidClassifier classifier(shape);classifier.Perform(gp_Pnt(x,y,z),1e-7);return classifier.State()==TopAbs_IN;}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    CadDocument doc;auto base=solved(SketchGeometry::addCenterRectangle(
        SketchGeometry::newFeature(doc.authored()["documentId"].toString(),"XY"),
        {{"centerUMm",0},{"centerVMm",0},{"widthMm",75},{"heightMm",50}}));
    const auto widthId=base["parameters"].toObject()["dimensions"].toArray()[0].toObject()["id"].toString();
    commit(doc,SketchGeometry::replaceFeature(doc.authored(),base));
    commit(doc,CadDocument::extrusionCandidate(doc.authored(),{{"sketchId",base["id"]},
        {"profileId",doc.sketches()[0].profiles[0].profileId},{"distanceMm",10}}));
    const auto extrusionId=doc.authored()["features"].toArray().last().toObject()["id"].toString();
    const auto target=doc.bodies()[0].bodyId;QJsonArray corners;
    for(const auto& edge:doc.bodies()[0].solidEdges)if(edge.provenance["role"]=="sideCorner")corners.append(edge.edgeId);
    check(corners.size()==4,"Four vertical edge seeds required");
    commit(doc,CadDocument::filletCandidate(doc.authored(),{{"targetBodyId",target},{"edgeIds",corners},{"radiusMm",5}}));
    EvaluatedSupportFace cap;bool found=false;for(const auto& face:doc.bodies()[0].supportFaces)if(face.role=="extrude.endCap"){cap=face;found=true;break;}
    check(found,"Rounded top support required");
    const QJsonObject support{{"producerFeatureId",cap.producerFeatureId},{"bodyId",target},{"faceId",cap.faceId}};
    auto sketch=SketchGeometry::newSupportedFeature(doc.authored()["documentId"].toString(),support,"Hole point",GeometryKernel::supportEdgeDescriptors(cap));
    const auto projection=GeometryKernel::projectionContext(doc.evaluation(),sketch);
    sketch=solved(SketchGeometry::addStandalonePoint(sketch,{{"uMm",-27.5},{"vMm",15}},projection));
    const auto point=sketch["parameters"].toObject()["entities"].toArray()[0].toObject();
    QString left,upper;for(const auto& value:projection["lines"].toArray()){
        const auto line=value.toObject();
        if(std::abs(line["u1Mm"].toDouble()+37.5)<1e-6 && std::abs(line["u2Mm"].toDouble()+37.5)<1e-6)left=line["id"].toString();
        if(std::abs(line["v1Mm"].toDouble()-25)<1e-6 && std::abs(line["v2Mm"].toDouble()-25)<1e-6)upper=line["id"].toString();
    }check(!left.isEmpty() && !upper.isEmpty(),"Retained rim dimension references required");
    QString leftDimensionId;
    for(const auto& edge:{left,upper}){
        sketch=solved(SketchGeometry::addDimension(sketch,
            {{"kind","pointLineDistance"},{"pointIds",QJsonArray{point["pointId"]}},{"entityId",edge},{"valueMm",10}},projection));
        if(edge==left)leftDimensionId=sketch["parameters"].toObject()["dimensions"].toArray().last().toObject()["id"].toString();
    }
    commit(doc,SketchGeometry::replaceFeature(doc.authored(),sketch));
    commit(doc,CadDocument::holeCandidate(doc.authored(),{{"sourceSketchId",sketch["id"]},{"sourceEntityId",point["id"]},
        {"sourcePointId",point["pointId"]},{"targetBodyId",target},{"boreDiameterMm",4},{"sinkDiameterMm",10},
        {"includedAngleDeg",90}}));
    const auto holeId=doc.authored()["features"].toArray().last().toObject()["id"].toString();
    QJsonObject horizontal,vertical;double xSign=0,ySign=0;
    for(const auto& edge:doc.bodies()[0].solidEdges)if(edge.provenance["role"]=="endRim" && !edge.edge.IsNull()){
        const auto a=edge.provenance["startMm"].toArray(),b=edge.provenance["endMm"].toArray();
        if(a.size()!=3 || b.size()!=3 || std::abs(a[2].toDouble()-10)>1e-6 || std::abs(b[2].toDouble()-10)>1e-6)continue;
        const double dx=b[0].toDouble()-a[0].toDouble(),dy=b[1].toDouble()-a[1].toDouble();
        if(std::abs(dx)>1 && std::abs(dy)<1e-6 && horizontal.isEmpty()){horizontal={{"edgeId",edge.edgeId},{"quantity",3},{"spacingMm",dx>0 ? 27.5:-27.5},{"direction","one"}};xSign=dx>0 ? 1:-1;}
        if(std::abs(dy)>1 && std::abs(dx)<1e-6 && vertical.isEmpty()){vertical={{"edgeId",edge.edgeId},{"quantity",3},{"spacingMm",dy<0 ? 15:-15},{"direction","one"}};ySign=dy>0 ? 1:-1;}
    }check(!horizontal.isEmpty() && !vertical.isEmpty() && xSign!=0 && ySign!=0,"Two retained oriented rim axes required");
    QJsonObject pattern{{"sourceFeatureId",holeId},{"targetBodyId",target},{"distribution","spacing"},{"computeType","adjust"},
        {"axes",QJsonArray{horizontal,vertical}}};
    const auto beforePattern=doc.authored();
    commit(doc,CadDocument::patternCandidate(doc.authored(),pattern));
    auto after=doc.authored();const auto patternId=after["features"].toArray().last().toObject()["id"].toString();
    auto summary=doc.bodies()[0].summary;
    check(summary["valid"].toBool() && summary["closed"].toBool() && summary["instanceCount"]==9,"Nine-hole valid solid");
    near(summary["volumeMm3"].toDouble(),36500-353*std::numbers::pi);
    check(summary["distribution"]=="Spacing" && summary["computeType"]=="Adjust","Pattern mode reported honestly");
    for(const auto& value:summary["instances"].toArray()){
        const auto item=value.toObject();const auto center=item["centerMm"].toArray();
        near(center[0].toDouble(),-27.5+27.5*item["index1"].toInt(),1e-6);
        near(center[1].toDouble(),15-15*item["index2"].toInt(),1e-6);
        near(center[2].toDouble(),10,1e-6);
    }
    auto widened=solved(SketchGeometry::updateDimension(base,{{"dimensionId",widthId},{"valueMm",85}}));
    commit(doc,SketchGeometry::replaceFeature(doc.authored(),widened));
    near(doc.bodies()[0].summary["seedCenterMm"].toArray()[0].toDouble(),-32.5,1e-6);
    near(doc.bodies()[0].summary["volumeMm3"].toDouble(),41500-353*std::numbers::pi);
    doc.history().undo();check(doc.authored()==after,"Upstream centered width edit restores Pattern exactly");
    commit(doc,CadDocument::extrusionCandidate(doc.authored(),{{"featureId",extrusionId},{"sketchId",base["id"]},
        {"profileId",doc.sketches()[0].profiles[0].profileId},{"distanceMm",12}}));
    near(doc.bodies()[0].summary["seedCenterMm"].toArray()[2].toDouble(),12,1e-6);
    near(doc.bodies()[0].summary["volumeMm3"].toDouble(),43800-375*std::numbers::pi);
    doc.history().undo();check(doc.authored()==after,"Upstream Thickness edit restores Pattern exactly");
    auto shifted=solved(SketchGeometry::updateDimension(sketch,{{"dimensionId",leftDimensionId},{"valueMm",12}},projection));
    commit(doc,SketchGeometry::replaceFeature(doc.authored(),shifted));
    near(doc.bodies()[0].summary["seedCenterMm"].toArray()[0].toDouble(),-25.5,1e-6);
    near(doc.bodies()[0].summary["volumeMm3"].toDouble(),36500-353*std::numbers::pi);
    doc.history().undo();check(doc.authored()==after,"Retained point dimension edit restores Pattern exactly");
    doc.history().undo();check(doc.authored()==beforePattern,"Exact Pattern undo");doc.history().redo();check(doc.authored()==after,"Exact Pattern redo");
    auto editedPattern=pattern;editedPattern["featureId"]=patternId;
    auto editedAxes=editedPattern["axes"].toArray();auto editedHorizontal=editedAxes[0].toObject();
    editedHorizontal["spacingMm"]=xSign*25;editedAxes[0]=editedHorizontal;editedPattern["axes"]=editedAxes;
    commit(doc,CadDocument::patternCandidate(doc.authored(),editedPattern));const auto edited=doc.authored();
    check(edited["features"].toArray().last().toObject()["id"]==patternId,"Pattern edit retains feature ID");
    for(const auto& value:doc.bodies()[0].summary["instances"].toArray())if(value.toObject()["index1"]==2)
        near(value.toObject()["centerMm"].toArray()[0].toDouble(),22.5,1e-6);
    doc.history().undo();check(doc.authored()==after,"Exact Pattern edit undo");
    doc.history().redo();check(doc.authored()==edited,"Exact Pattern edit redo");
    after=edited;
    auto bad=pattern;bad["featureId"]=patternId;auto axes=bad["axes"].toArray();auto first=axes[0].toObject();first["spacingMm"]=xSign*5;axes[0]=first;bad["axes"]=axes;
    const auto revision=doc.revision();check(!doc.commit(CadDocument::patternCandidate(after,bad),"Overlapping" ).isEmpty()
        && doc.revision()==revision && doc.authored()==after,"Overlapping opening refusal atomic");
    const auto diagnostic=doc.lastEvaluationError();
    check(diagnostic["code"]=="pattern_overlapping_openings" && diagnostic["featureId"]==patternId
        && diagnostic["sourceFeatureId"]==holeId && diagnostic["targetBodyId"]==target,
        "Pattern refusal attributes seed and target");
    doc.history().undo();doc.history().undo();check(doc.authored()==beforePattern,"Return to single Hole");
    // A joined underside pad deepens only the middle-column target. Reusing a
    // transformed ten-millimeter seed cutter would leave three blind bores.
    auto pad=SketchGeometry::addRectangle(SketchGeometry::newFeature(doc.authored()["documentId"].toString(),"XY"),
        {{"u1Mm",-7},{"v1Mm",-25},{"u2Mm",7},{"v2Mm",25}});
    commit(doc,SketchGeometry::replaceFeature(doc.authored(),pad));
    commit(doc,CadDocument::extrusionCandidate(doc.authored(),{{"sketchId",pad["id"]},
        {"profileId",doc.sketches().back().profiles[0].profileId},{"distanceMm",-3},
        {"operation","join"},{"targetBodyIds",QJsonArray{target}}}));
    check(doc.bodies().size()==1 && doc.bodies()[0].bodyId==target,"Pad Join retains target body");
    commit(doc,CadDocument::patternCandidate(doc.authored(),pattern));
    summary=doc.bodies()[0].summary;
    near(summary["volumeMm3"].toDouble(),36500-353*std::numbers::pi+2100-36*std::numbers::pi);
    for(const double y:{15.0,0.0,-15.0}){
        check(!material(doc.bodies()[0].shape,0,y,-2.9),"Middle-column bore exits underside pad");
        check(material(doc.bodies()[0].shape,3,y,-2.9),"Pad beside middle bore remains solid");
    }
    const auto padded=doc.authored();QTemporaryDir temp;const auto path=temp.filePath("pattern.alloy");
    check(doc.save(path).isEmpty(),"Save patterned pad");CadDocument reopened;
    check(reopened.open(path).isEmpty() && reopened.authored()==padded,"Exact padded Pattern reopen");
    near(reopened.bodies()[0].summary["volumeMm3"].toDouble(),summary["volumeMm3"].toDouble());
    std::cout<<"Hole Spacing/Adjust grid, oriented axes, pad depths, edits/history/refusal/storage passed\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
