// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include "PlanarFaces.h"
#include "SolidFillet.h"
#include <BRepAdaptor_Curve.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <QCoreApplication>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
#include <numbers>
namespace {
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
QJsonObject accept(const SketchGeometry::SolveResult& r){if(!r.ok)throw std::runtime_error(r.error.toStdString());return r.feature;}
void commit(CadDocument& d,const QJsonObject& a){const auto error=d.commit(a,"Rim fixture");if(!error.isEmpty())throw std::runtime_error(error.toStdString());}
void near(double a,double b){check(std::abs(a-b)<1e-6,"numeric oracle");}
EvaluatedSupportFace cap(const CadDocument& d){for(const auto& f:d.bodies()[0].supportFaces)if(f.role=="extrude.endCap")return f;throw std::runtime_error("top face missing");}
void verifyRims(const CadDocument& d,double width,double radius){
    const auto top=cap(d);check(top.boundaryEdges.size()==4,"four retained shortened straight rims");
    TopTools_IndexedMapOfShape actual;TopExp::MapShapes(top.face,TopAbs_EDGE,actual);
    for(const auto& edge:top.boundaryEdges){
        check(actual.Contains(edge.edge),"support descriptor is an actual face edge");
        check(edge.type=="line" && edge.provenance["lineage"]=="uniqueModifiedStraight","bounded lineage");
        check(!edge.provenance.contains("startContact") && !edge.provenance.contains("endContact")
            && edge.provenance["sourceBoundaryContacts"].isObject(),"ancestry is not current corner contact");
        const auto a=edge.provenance["startMm"].toArray(),b=edge.provenance["endMm"].toArray();
        BRepAdaptor_Curve curve(edge.edge);const gp_Pnt p(a[0].toDouble(),a[1].toDouble(),a[2].toDouble()),q(b[0].toDouble(),b[1].toDouble(),b[2].toDouble());
        const auto first=curve.Value(curve.FirstParameter()),last=curve.Value(curve.LastParameter());
        check((p.Distance(first)<1e-6 && q.Distance(last)<1e-6) || (q.Distance(first)<1e-6 && p.Distance(last)<1e-6),"descriptor current endpoints match OCCT");
        near(p.Z(),10);near(q.Z(),10);
        near(p.Distance(q),std::abs(p.X()-q.X())>1e-6 ? width-2*radius : 50-2*radius);
    }
    int retained=0;for(const auto& edge:d.bodies()[0].solidEdges)if(!edge.edge.IsNull() && edge.provenance["role"]=="endRim"){
        ++retained;check(actual.Contains(edge.edge),"named direction edge matches actual top rim");
        check(edge.provenance["endpointProvenance"]=="currentKernelBoundary","named direction current endpoint role");
    }check(retained==4,"four live top direction identities");
    near(d.bodies()[0].summary["volumeMm3"].toDouble(),(width*50-(4-std::numbers::pi)*radius*radius)*10);
}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    CadDocument d;auto base=accept(SketchGeometry::addCenterRectangle(SketchGeometry::newFeature(d.authored()["documentId"].toString(),"XY"),{{"centerUMm",0},{"centerVMm",0},{"widthMm",75},{"heightMm",50}}));
    const auto widthId=base["parameters"].toObject()["dimensions"].toArray()[0].toObject()["id"].toString();
    commit(d,SketchGeometry::replaceFeature(d.authored(),base));commit(d,CadDocument::extrusionCandidate(d.authored(),{{"sketchId",base["id"]},{"profileId",d.sketches()[0].profiles[0].profileId},{"distanceMm",10}}));
    QJsonArray selected;for(const auto& edge:d.bodies()[0].solidEdges)if(edge.provenance["role"]=="sideCorner")selected.append(edge.edgeId);check(selected.size()==4,"four vertical corners");
    const auto target=d.bodies()[0].bodyId;QJsonObject filletArgs{{"targetBodyId",target},{"edgeIds",selected},{"radiusMm",5}};
    commit(d,CadDocument::filletCandidate(d.authored(),filletArgs));filletArgs["featureId"]=d.authored()["features"].toArray().last().toObject()["id"];verifyRims(d,75,5);
    const auto top=cap(d);QJsonObject support{{"bodyId",target},{"producerFeatureId",top.producerFeatureId},{"faceId",top.faceId}};
    // A true standalone point has exactly two coordinate DOF and no profile.
    auto hole=SketchGeometry::newSupportedFeature(d.authored()["documentId"].toString(),support,"Dimension foundation",GeometryKernel::supportEdgeDescriptors(top));
    const auto context=GeometryKernel::projectionContext(d.evaluation(),hole);QString left,upper;
    for(const auto& v:context["lines"].toArray()){const auto l=v.toObject();if(std::abs(l["u1Mm"].toDouble()+37.5)<1e-6 && std::abs(l["u2Mm"].toDouble()+37.5)<1e-6)left=l["id"].toString();if(std::abs(l["v1Mm"].toDouble()-25)<1e-6 && std::abs(l["v2Mm"].toDouble()-25)<1e-6)upper=l["id"].toString();}
    check(!left.isEmpty() && !upper.isEmpty(),"adjacent shortened actual edges");hole=accept(SketchGeometry::addStandalonePoint(hole,{{"uMm",-27.5},{"vMm",15}},context));const auto pointEntity=hole["parameters"].toObject()["entities"].toArray()[0].toObject();const auto point=pointEntity["pointId"].toString();
    check(SketchGeometry::solveDraft(hole,context).diagnostics["dof"].toInt(-1)==2,"standalone point has two coordinate DOF");
    for(const auto& edge:{left,upper})hole=accept(SketchGeometry::addDimension(hole,{{"kind","pointLineDistance"},{"pointIds",QJsonArray{point}},{"entityId",edge},{"valueMm",10}},context));
    check(SketchGeometry::solveDraft(hole,context).diagnostics["dof"].toInt(-1)==0,"two point-edge dimensions consume only placement DOF");
    commit(d,SketchGeometry::replaceFeature(d.authored(),hole));const auto before=d.authored();
    auto center=[&](const CadDocument& doc,double x){for(const auto& v:doc.sketches().back().summary["worldPoints"].toArray())if(v.toObject()["id"]==point){const auto p=v.toObject()["positionMm"].toArray();near(p[0].toDouble(),x);near(p[1].toDouble(),15);near(p[2].toDouble(),10);return;}throw std::runtime_error("point unavailable");};center(d,-27.5);
    check(d.sketches().back().profiles.empty() && d.sketches().back().summary["standalonePointCount"]==1,"point-only sketch has no fabricated region");
    const auto inspect=d.sketches().back().summary["standalonePoints"].toArray()[0].toObject();check(inspect["id"]==pointEntity["id"] && inspect["pointId"]==point,"exact explicit entity/point relationship for Hole");
    base=accept(SketchGeometry::updateDimension(base,{{"dimensionId",widthId},{"valueMm",85}}));commit(d,SketchGeometry::replaceFeature(d.authored(),base));verifyRims(d,85,5);center(d,-32.5);
    filletArgs["radiusMm"]=4;commit(d,CadDocument::filletCandidate(d.authored(),filletArgs));verifyRims(d,85,4);center(d,-32.5);const auto after=d.authored();
    const auto retained=SketchGeometry::findFeature(after,hole["id"].toString())["parameters"].toObject();check(retained["dimensions"]==hole["parameters"].toObject()["dimensions"] && retained["projections"]==hole["parameters"].toObject()["projections"],"edge equations/references retained");
    d.history().undo();verifyRims(d,85,5);d.history().undo();check(d.authored()==before,"exact two-edit undo");center(d,-27.5);d.history().redo();d.history().redo();check(d.authored()==after,"exact redo");
    const auto revision=d.revision();filletArgs["radiusMm"]=30;check(!d.commit(CadDocument::filletCandidate(after,filletArgs),"Impossible").isEmpty() && d.authored()==after && d.revision()==revision,"downstream failure atomic");
    QTemporaryDir tmp;const auto path=tmp.filePath("rim.alloy");check(d.save(path).isEmpty(),"save");CadDocument reopened;check(reopened.open(path).isEmpty() && reopened.authored()==after,"exact reopen");verifyRims(reopened,85,4);center(reopened,-32.5);
    auto origin=SketchGeometry::newFeature(d.authored()["documentId"].toString(),"XY");
    origin=accept(SketchGeometry::addStandalonePoint(origin,{{"uMm",3},{"vMm",4}}));
    const auto evaluated=SketchGeometry::evaluateFeature(origin);check(evaluated.profiles.empty() && evaluated.entityEdges.empty(),"ordinary point evaluates no curve");
    TopTools_IndexedMapOfShape verts,edges;TopExp::MapShapes(evaluated.shape,TopAbs_VERTEX,verts);TopExp::MapShapes(evaluated.shape,TopAbs_EDGE,edges);check(verts.Extent()==1 && edges.Extent()==0,"point evaluates one actual OCCT vertex");
    auto malformed=origin;auto params=origin["parameters"].toObject();params["entities"]=QJsonArray{};malformed["parameters"]=params;
    check(!SketchGeometry::solveDraft(malformed).ok,"orphan solver point remains invalid");
    const auto bad=SketchGeometry::addStandalonePoint(origin,{{"uMm",100001},{"vMm",0}});check(!bad.ok && bad.feature==origin,"invalid placement atomic");
    const auto snapped=SketchGeometry::addStandalonePoint(SketchGeometry::newFeature(d.authored()["documentId"].toString(),"XY"),{{"uMm",0},{"vMm",0},{"snapTarget",QJsonObject{{"kind","origin"}}}});check(snapped.ok && snapped.diagnostics["dof"]==0,"explicit origin coincidence retains intent");
    auto mixed=accept(SketchGeometry::addStandalonePoint(base,{{"uMm",0},{"vMm",0}}));check(SketchGeometry::evaluateFeature(mixed).profiles[0].profileId==SketchGeometry::evaluateFeature(base).profiles[0].profileId,"point excluded from existing region identity");
    std::cout<<"Standalone point vertex/DOF/profiles/two-edge association/width/radius/history/storage/refusal checks passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
