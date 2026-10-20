// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include "SurfaceOffset.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <BRepAdaptor_Surface.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepTools.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <numbers>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <gp_Cylinder.hxx>
#include <iostream>
#include <cmath>
#include <limits>
namespace {
QJsonObject movePoint(QJsonObject authored,const QString& sketchId,const QString& pointId,double u,double v){
    auto rows=authored.value("features").toArray();for(int i=0;i<rows.size();++i){auto f=rows[i].toObject();if(f.value("id")!=sketchId)continue;
        auto p=f.value("parameters").toObject();auto points=p.value("points").toArray();for(int j=0;j<points.size();++j){auto q=points[j].toObject();if(q.value("id")==pointId){q["uMm"]=u;q["vMm"]=v;points[j]=q;}}
        p["points"]=points;f["parameters"]=p;rows[i]=f;}authored["features"]=rows;return authored;
}
void check(bool ok,const QString& s){if(!ok)throw std::runtime_error(s.toStdString());}void near(double a,double b){check(std::abs(a-b)<1e-6,QString("%1 != %2").arg(a).arg(b));}}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    CadDocument doc;const auto owner=doc.authored().value("documentId").toString();
    auto sketch=SketchGeometry::addRectangle(SketchGeometry::newFeature(owner,"XY","Boundary"),{{"u1Mm",0},{"v1Mm",0},{"u2Mm",40},{"v2Mm",30}});
    const auto entities=sketch.value("parameters").toObject().value("entities").toArray();
    sketch=SketchGeometry::addDimension(sketch,{{"kind","horizontal"},{"entityId",entities[0].toObject().value("id")},{"valueMm",40}}).feature;
    const auto dimension=sketch.value("parameters").toObject().value("dimensions").toArray().last().toObject().value("id").toString();
    check(doc.commit(SketchGeometry::replaceFeature(doc.authored(),sketch),"Boundary").isEmpty(),"sketch");
    const auto profile=doc.evaluation().sketches.front().profiles.front().profileId;
    check(doc.commit(CadDocument::surfacePatchCandidate(doc.authored(),{{"sketchId",sketch.value("id")},{"profileId",profile}}),"Patch").isEmpty(),"patch");
    const auto source=doc.bodies().front();const auto face=source.supportFaces.front();
    const QJsonObject ref{{"bodyId",source.bodyId},{"producerFeatureId",face.producerFeatureId},{"faceId",face.faceId}};
    const auto baseline=doc.authored();
    auto candidate=CadDocument::surfaceOffsetCandidate(baseline,{{"sourceFace",ref},{"distanceMm",5.}});
    const auto preview=CadDocument::evaluateCandidate(candidate);check(preview.ok,preview.error);check(doc.authored()==baseline && doc.bodies().size()==1,"preview isolation");
    check(doc.commit(candidate,"Offset").isEmpty(),"offset");const auto result=doc.bodies().back();
    auto verify=[&](double z,double area){const auto& b=doc.bodies().back();const auto t=b.summary.value("topology").toObject();check(t.value("faces")==1 && t.value("solids")==0 && b.summary.value("volumeMm3").isNull(),"open sheet");near(b.summary.value("areaMm2").toDouble(),area);near(BRepAdaptor_Surface(b.supportFaces.front().face).Plane().Location().Z(),z);check(b.featureId==result.featureId && b.bodyId==result.bodyId && b.supportFaces.front().faceId==result.supportFaces.front().faceId,"stable output identity");check(doc.bodies().front().bodyId==source.bodyId,"preserved source");};verify(5,1200);
    for(double d:{-5.,2.5,-2.5,0.}){check(doc.commit(CadDocument::surfaceOffsetCandidate(doc.authored(),{{"featureId",result.featureId},{"distanceMm",d}}),"Edit").isEmpty(),"distance edit");verify(d,1200);}
    sketch=SketchGeometry::updateDimension(sketch,{{"dimensionId",dimension},{"valueMm",42}}).feature;check(doc.commit(SketchGeometry::replaceFeature(doc.authored(),sketch),"Width").isEmpty(),"source edit");verify(0,1260);doc.history().undo();verify(0,1200);doc.history().redo();verify(0,1260);
    const auto accepted=doc.authored();const auto rev=doc.revision();auto stale=ref;stale["faceId"]=CadDocument::newId();check(!doc.commit(CadDocument::surfaceOffsetCandidate(accepted,{{"featureId",result.featureId},{"sourceFace",stale}}),"Stale").isEmpty() && doc.authored()==accepted && doc.revision()==rev,"atomic stale refusal");
    auto corrupt=doc.evaluation();corrupt.bodies.front().supportFaces.front().face=BRepBuilderAPI_MakeFace(gp_Cylinder(gp_Ax3(),10),0,1,0,30).Face();bool refused=false;try{SurfaceOffset::evaluate(accepted.value("features").toArray().last().toObject(),corrupt);}catch(const GeometryKernel::EvaluationError&){refused=true;}check(refused,"curved unsupported refusal");
    for(const auto value:{QJsonValue(100001.),QJsonValue(std::numeric_limits<double>::infinity()),QJsonValue("5")}){bool rejected=false;try{CadDocument::surfaceOffsetCandidate(accepted,{{"featureId",result.featureId},{"distanceMm",value}});}catch(const std::invalid_argument&){rejected=true;}check(rejected && doc.authored()==accepted && doc.revision()==rev,"invalid numeric atomic refusal");}
    auto reversed=doc.evaluation();reversed.bodies.front().supportFaces.front().face.Reverse();auto reverseFeature=accepted.value("features").toArray().last().toObject();auto reverseParameters=reverseFeature.value("parameters").toObject();reverseParameters["distanceMm"]=5.;reverseFeature["parameters"]=reverseParameters;
    const auto reversedResult=SurfaceOffset::evaluate(reverseFeature,reversed);near(BRepAdaptor_Surface(reversedResult.supportFaces.front().face).Plane().Location().Z(),-5.);
    QTemporaryDir tmp;const auto file=tmp.filePath("offset.alloy");check(doc.save(file).isEmpty(),"save");CadDocument reopened;check(reopened.open(file).isEmpty() && reopened.authored()==accepted,"reopen");near(reopened.bodies().back().summary.value("areaMm2").toDouble(),1260);

    // A real retained Surface Revolve is the only newly admitted curved source.
    CadDocument cylindrical;auto section=SketchGeometry::newFeature(cylindrical.authored().value("documentId").toString(),"XZ","Cylinder section");
    section=SketchGeometry::addLine(section,{{"u1Mm",10},{"v1Mm",0},{"u2Mm",10},{"v2Mm",30}});
    const auto line=section.value("parameters").toObject().value("entities").toArray().last().toObject();
    section=SketchGeometry::addLine(section,{{"u1Mm",0},{"v1Mm",0},{"u2Mm",0},{"v2Mm",30}});
    const auto axis=section.value("parameters").toObject().value("entities").toArray().last().toObject();
    const auto construction=SketchGeometry::setConstruction(section,{{"entityIds",QJsonArray{axis.value("id")}},{"construction",true}});check(construction.ok,"cylinder construction axis");section=construction.feature;
    check(cylindrical.commit(SketchGeometry::replaceFeature(cylindrical.authored(),section),"Section").isEmpty(),"cylinder sketch");
    check(cylindrical.commit(CadDocument::surfaceRevolveCandidate(cylindrical.authored(),{{"sketchId",section.value("id")},{"entityId",line.value("id")},{"axisEntityId",axis.value("id")},{"angleDeg",120.}}),"Cylinder").isEmpty(),"cylinder revolve");
    const auto cylinderSource=cylindrical.bodies().front();const auto sourceSheet=cylinderSource.supportFaces.front();
    const QJsonObject cylinderRef{{"bodyId",cylinderSource.bodyId},{"producerFeatureId",sourceSheet.producerFeatureId},{"faceId",sourceSheet.faceId}};
    const auto cylinderBaseline=cylindrical.authored();
    auto cylindricalCandidate=CadDocument::surfaceOffsetCandidate(cylinderBaseline,{{"sourceFace",cylinderRef},{"distanceMm",2.}});
    const auto cylinderPreview=CadDocument::evaluateCandidate(cylindricalCandidate);check(cylinderPreview.ok,cylinderPreview.error);
    check(cylindrical.authored()==cylinderBaseline,"cylinder preview isolation");
    check(cylindrical.commit(cylindricalCandidate,"Cylindrical Offset").isEmpty(),"cylindrical offset");
    const auto initialCylinder=cylindrical.bodies().back();const auto cylinderOffsetId=initialCylinder.featureId;
    const double radialSign=initialCylinder.summary.value("normalRadialSign").toDouble();near(std::abs(radialSign),1.);
    auto verifyCylinder=[&](double sourceRadius,double height,double radialDistance){
        const auto& body=cylindrical.bodies().back();const auto& support=body.supportFaces.front();const BRepAdaptor_Surface surface(support.face);
        check(surface.GetType()==GeomAbs_Cylinder,"analytic cylindrical result");near(surface.Cylinder().Radius(),sourceRadius+radialDistance);
        near(surface.LastUParameter()-surface.FirstUParameter(),2.*std::numbers::pi/3.);near(surface.LastVParameter()-surface.FirstVParameter(),height);
        near(body.summary.value("areaMm2").toDouble(),(sourceRadius+radialDistance)*height*2.*std::numbers::pi/3.);
        const auto topology=body.summary.value("topology").toObject();check(topology.value("faces")==1 && topology.value("solids")==0 && topology.value("edges")==4 && body.summary.value("valid").toBool() && !body.summary.value("closed").toBool(),"regular open cylinder sheet");
        check(body.featureId==cylinderOffsetId && body.bodyId==initialCylinder.bodyId && support.faceId==initialCylinder.supportFaces.front().faceId && support.frame.isEmpty(),"cylinder output identity and no planar support frame");
        near(BRepAdaptor_Surface(cylindrical.bodies().front().supportFaces.front().face).Cylinder().Radius(),sourceRadius);
        for(size_t i=0;i<support.boundaryEdges.size();++i){const auto& boundary=support.boundaryEdges[i];check(boundary.edgeId==initialCylinder.supportFaces.front().boundaryEdges[i].edgeId,"stable cylinder edge identity");
            const auto detail=boundary.provenance;check(detail.value("sourceEdgeId")==sourceSheet.boundaryEdges[i].edgeId,"exact cylinder source edge ID");
            const BRepAdaptor_Curve curve(boundary.edge);const auto start=detail.value("startMm").toArray(),end=detail.value("endMm").toArray();
            check(start.size()==3 && end.size()==3,"current curved endpoint metadata");const auto a=curve.Value(curve.FirstParameter()),b=curve.Value(curve.LastParameter());
            near(gp_Pnt(start[0].toDouble(),start[1].toDouble(),start[2].toDouble()).Distance(a),0.);near(gp_Pnt(end[0].toDouble(),end[1].toDouble(),end[2].toDouble()).Distance(b),0.);
        }
    };
    verifyCylinder(10,30,radialSign*2.);
    for(double radialDistance:{2.,-4.,-9.,0.}){const auto args=QJsonObject{{"featureId",cylinderOffsetId},{"distanceMm",radialDistance*radialSign}};check(cylindrical.commit(CadDocument::surfaceOffsetCandidate(cylindrical.authored(),args),"Cylinder distance").isEmpty(),"signed cylindrical distance");verifyCylinder(10,30,radialDistance);}
    check(cylindrical.commit(CadDocument::surfaceOffsetCandidate(cylindrical.authored(),{{"featureId",cylinderOffsetId},{"distanceMm",-4.*radialSign}}),"Valid inward").isEmpty(),"valid inward baseline");
    const auto cylinderAccepted=cylindrical.authored();const auto cylinderRevision=cylindrical.revision();const auto undoCount=cylindrical.history().count(),undoIndex=cylindrical.history().index();
    const auto acceptedSummary=cylindrical.bodies().back().summary;
    for(double radialDistance:{-10.,-11.}){
        const auto failed=CadDocument::surfaceOffsetCandidate(cylinderAccepted,{{"featureId",cylinderOffsetId},{"distanceMm",radialDistance*radialSign}});
        const auto previewFailed=CadDocument::evaluateCandidate(failed);check(!previewFailed.ok && previewFailed.diagnostics.value("code")=="surface_offset_collapse" && previewFailed.diagnostics.value("sourceFace")==cylinderRef,"structured collapse preview refusal");
        check(!cylindrical.commit(failed,"Collapse").isEmpty() && cylindrical.lastEvaluationError().value("code")=="surface_offset_collapse","collapse commit refusal");
        check(cylindrical.authored()==cylinderAccepted && cylindrical.revision()==cylinderRevision && cylindrical.history().count()==undoCount && cylindrical.history().index()==undoIndex && cylindrical.bodies().back().summary==acceptedSummary,"atomic collapse state preservation");
        const auto fresh=CadDocument::evaluateCandidate(CadDocument::surfaceOffsetCandidate(cylinderBaseline,{{"sourceFace",cylinderRef},{"distanceMm",radialDistance*radialSign}}));check(!fresh.ok && fresh.diagnostics.value("code")=="surface_offset_collapse","fresh collapse refusal");
    }
    check(cylindrical.commit(CadDocument::surfaceOffsetCandidate(cylinderAccepted,{{"featureId",cylinderOffsetId},{"distanceMm",-3.*radialSign}}),"Valid retry").isEmpty(),"cylindrical valid retry");verifyCylinder(10,30,-3.);
    auto radiusEdit=movePoint(cylindrical.authored(),section.value("id").toString(),line.value("startPointId").toString(),12,0);
    radiusEdit=movePoint(radiusEdit,section.value("id").toString(),line.value("endPointId").toString(),12,36);
    check(cylindrical.commit(radiusEdit,"Cylinder radius and height").isEmpty(),"source cylinder radius height recompute");verifyCylinder(12,36,-3.);
    cylindrical.history().undo();verifyCylinder(10,30,-3.);cylindrical.history().redo();verifyCylinder(12,36,-3.);
    const auto cylinderFile=tmp.filePath("cylinder-offset.alloy");check(cylindrical.save(cylinderFile).isEmpty(),"cylinder save");CadDocument cylinderReopened;check(cylinderReopened.open(cylinderFile).isEmpty() && cylinderReopened.authored()==cylindrical.authored(),"cylinder reopen");near(cylinderReopened.bodies().back().summary.value("areaMm2").toDouble(),216*std::numbers::pi);
    // Reverse the actual face orientation while retaining its membership. Positive
    // distance must reverse radial direction without preflipping the kernel input.
    auto reverseCylinder=cylindrical.evaluation();reverseCylinder.bodies.front().supportFaces.front().face.Reverse();
    auto reverseCylinderFeature=cylindrical.authored().value("features").toArray().last().toObject();auto reverseCylinderParams=reverseCylinderFeature.value("parameters").toObject();reverseCylinderParams["distanceMm"]=2.*radialSign;reverseCylinderFeature["parameters"]=reverseCylinderParams;
    const auto reverseOffset=SurfaceOffset::evaluate(reverseCylinderFeature,reverseCylinder);near(BRepAdaptor_Surface(reverseOffset.supportFaces.front().face).Cylinder().Radius(),10.);near(reverseOffset.summary.value("normalRadialSign").toDouble(),-radialSign);
    reverseCylinderParams["distanceMm"]=12.*radialSign;reverseCylinderFeature["parameters"]=reverseCylinderParams;bool reverseCollapsed=false;try{SurfaceOffset::evaluate(reverseCylinderFeature,reverseCylinder);}catch(const GeometryKernel::EvaluationError& e){reverseCollapsed=e.diagnostics().value("code")=="surface_offset_collapse";}check(reverseCollapsed,"reversed orientation collapse sign");
    auto duplicated=cylindrical.evaluation();duplicated.bodies.front().supportFaces.push_back(duplicated.bodies.front().supportFaces.front());bool ambiguous=false;try{SurfaceOffset::resolveSource(duplicated,cylinderRef);}catch(const GeometryKernel::EvaluationError& e){ambiguous=e.diagnostics().value("code")=="ambiguous_surface_source";}check(ambiguous,"duplicate retained source refusal");
    auto staleCylinder=cylindrical.evaluation();staleCylinder.bodies.front().supportFaces.front().face=BRepBuilderAPI_MakeFace(gp_Cylinder(gp_Ax3(),12),0,2.*std::numbers::pi/3.,0,36).Face();bool staleRefused=false;try{SurfaceOffset::resolveSource(staleCylinder,cylinderRef);}catch(const GeometryKernel::EvaluationError&){staleRefused=true;}check(staleRefused,"stale face membership refusal");
    std::cout<<"PASS cylindrical signed radius/area/lineage, orientation, collapse atomicity/retry, source edits, undo/reopen; planar signed Surface Offset exact sheet, stable lineage, edit, source recompute, atomic refusal, undo/reopen\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
