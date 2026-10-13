// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include "PlanarFaces.h"
#include "SolidFillet.h"
#include <BRepBuilderAPI_MakeShape.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRep_Builder.hxx>
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
// Explicit history control: the face survives while one original rim is split
// or absent. Other result geometry is deliberately present, never a fallback.
class BoundaryHistory final:public BRepBuilderAPI_MakeShape {
public:
    TopoDS_Edge source;TopTools_ListOfShape successors;
    const TopTools_ListOfShape& Modified(const TopoDS_Shape& s) override {return s.IsSame(source) ? successors : empty;}
private:TopTools_ListOfShape empty;
};
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    CadDocument d;auto base=accept(SketchGeometry::addCenterRectangle(SketchGeometry::newFeature(d.authored()["documentId"].toString(),"XY"),{{"centerUMm",0},{"centerVMm",0},{"widthMm",75},{"heightMm",50}}));
    const auto widthId=base["parameters"].toObject()["dimensions"].toArray()[0].toObject()["id"];
    commit(d,SketchGeometry::replaceFeature(d.authored(),base));commit(d,CadDocument::extrusionCandidate(d.authored(),{{"sketchId",base["id"]},{"profileId",d.sketches()[0].profiles[0].profileId},{"distanceMm",10}}));
    QJsonArray selected;for(const auto& edge:d.bodies()[0].solidEdges)if(edge.provenance["role"]=="sideCorner")selected.append(edge.edgeId);check(selected.size()==4,"four vertical corners");
    const auto target=d.bodies()[0].bodyId;QJsonObject filletArgs{{"targetBodyId",target},{"edgeIds",selected},{"radiusMm",5}};
    commit(d,CadDocument::filletCandidate(d.authored(),filletArgs));filletArgs["featureId"]=d.authored()["features"].toArray().last().toObject()["id"];verifyRims(d,75,5);
    const auto top=cap(d);QJsonObject support{{"bodyId",target},{"producerFeatureId",top.producerFeatureId},{"faceId",top.faceId}};
    // This fixture exercises existing point-to-edge authority via a circle
    // center. Standalone sketch Point authoring is a separate deliverable.
    auto hole=SketchGeometry::newSupportedFeature(d.authored()["documentId"].toString(),support,"Dimension foundation",GeometryKernel::supportEdgeDescriptors(top));
    const auto context=GeometryKernel::projectionContext(d.evaluation(),hole);QString left,upper;
    for(const auto& v:context["lines"].toArray()){const auto l=v.toObject();if(std::abs(l["u1Mm"].toDouble()+37.5)<1e-6 && std::abs(l["u2Mm"].toDouble()+37.5)<1e-6)left=l["id"].toString();if(std::abs(l["v1Mm"].toDouble()-25)<1e-6 && std::abs(l["v2Mm"].toDouble()-25)<1e-6)upper=l["id"].toString();}
    check(!left.isEmpty() && !upper.isEmpty(),"adjacent shortened actual edges");hole=SketchGeometry::addCircle(hole,{{"centerUMm",-27.5},{"centerVMm",15},{"diameterMm",2}});const auto circle=hole["parameters"].toObject()["entities"].toArray()[0].toObject();const auto point=circle["centerPointId"].toString();
    for(const auto& edge:{left,upper})hole=accept(SketchGeometry::addDimension(hole,{{"kind","pointLineDistance"},{"pointIds",QJsonArray{point}},{"entityId",edge},{"valueMm",10}},context));
    hole=accept(SketchGeometry::addDimension(hole,{{"kind","radius"},{"entityId",circle["id"]},{"valueMm",1}},context));commit(d,SketchGeometry::replaceFeature(d.authored(),hole));const auto before=d.authored();
    auto center=[&](const CadDocument& doc,double x){for(const auto& v:doc.sketches().back().summary["worldPoints"].toArray())if(v.toObject()["id"]==point){const auto p=v.toObject()["positionMm"].toArray();near(p[0].toDouble(),x);near(p[1].toDouble(),15);near(p[2].toDouble(),10);return;}throw std::runtime_error("point unavailable");};center(d,-27.5);
    base=accept(SketchGeometry::updateDimension(base,{{"dimensionId",widthId},{"valueMm",85}}));commit(d,SketchGeometry::replaceFeature(d.authored(),base));verifyRims(d,85,5);center(d,-32.5);
    filletArgs["radiusMm"]=4;commit(d,CadDocument::filletCandidate(d.authored(),filletArgs));verifyRims(d,85,4);center(d,-32.5);const auto after=d.authored();
    const auto retained=SketchGeometry::findFeature(after,hole["id"].toString())["parameters"].toObject();check(retained["dimensions"]==hole["parameters"].toObject()["dimensions"] && retained["projections"]==hole["parameters"].toObject()["projections"],"edge equations/references retained");
    d.history().undo();verifyRims(d,85,5);d.history().undo();check(d.authored()==before,"exact two-edit undo");center(d,-27.5);d.history().redo();d.history().redo();check(d.authored()==after,"exact redo");
    const auto revision=d.revision();filletArgs["radiusMm"]=30;check(!d.commit(CadDocument::filletCandidate(after,filletArgs),"Impossible").isEmpty() && d.authored()==after && d.revision()==revision,"downstream failure atomic");
    QTemporaryDir tmp;const auto path=tmp.filePath("rim.alloy");check(d.save(path).isEmpty(),"save");CadDocument reopened;check(reopened.open(path).isEmpty() && reopened.authored()==after,"exact reopen");verifyRims(reopened,85,4);center(reopened,-32.5);
    auto prior=top;const auto source=top.boundaryEdges[0];prior.boundaryEdges={source};BoundaryHistory op;op.source=source.edge;
    // Make two real edge descendants on the result face, with the source edge
    // not present. Both are history candidates, hence never choose either.
    auto control=top.face;BRep_Builder builder;
    const auto a=source.provenance["startMm"].toArray(),b=source.provenance["endMm"].toArray();gp_Pnt p(a[0].toDouble(),a[1].toDouble(),a[2].toDouble()),q(b[0].toDouble(),b[1].toDouble(),b[2].toDouble());const auto middle=p.Translated(gp_Vec(p,q)*.5);
    const TopoDS_Edge e1=BRepBuilderAPI_MakeEdge(p,middle),e2=BRepBuilderAPI_MakeEdge(middle,q);
    // A synthetic planar face carrying only the two history descendants.
    TopoDS_Face splitFace=TopoDS::Face(control.EmptyCopied());TopoDS_Wire wire;builder.MakeWire(wire);builder.Add(wire,e1);builder.Add(wire,e2);builder.Add(splitFace,wire);prior.face=splitFace;
    op.successors.Append(e1);op.successors.Append(e2);const auto mapped=PlanarFaces::transfer(op,{prior},splitFace,true);check(mapped.size()==1 && mapped[0].boundaryEdges.empty(),"split support edge withheld");
    EvaluatedDocument missing=d.evaluation();auto missingCap=cap(d);missingCap.boundaryEdges=mapped[0].boundaryEdges;missing.bodies[0].supportFaces={missingCap};bool refused=false;try{GeometryKernel::projectionContext(missing,hole);}catch(const GeometryKernel::EvaluationError& e){refused=e.diagnostics()["code"]=="projection_unavailable";}check(refused,"retained projection fails explicitly");
    const EvaluatedSolidEdge named{source.edgeId,source.edge,source.provenance};const auto split=SolidFillet::transferEdges(op,{named},splitFace,true);check(split[0].edge.IsNull() && split[0].provenance["resolutionError"]=="ambiguous_split","split direction unavailable");
    auto oriented=source.provenance;const TopoDS_Edge reversed=BRepBuilderAPI_MakeEdge(q,p);
    check(PlanarFaces::updateStraightBoundary(source.edge,reversed,oriented),"reversed unique straight history accepted");
    check(oriented["startMm"]==source.provenance["startMm"] && oriented["endMm"]==source.provenance["endMm"],"endpoint role does not swap on curve reversal");
    op.successors.Clear();op.successors.Append(e1);
    const auto shortenedAgain=SolidFillet::transferEdges(op,{named},splitFace);
    check(!shortenedAgain[0].edge.IsNull(),"later Boolean retains one live straight descendant");
    const auto refreshed=SolidFillet::descriptors(shortenedAgain)[0].toObject();
    const auto ra=refreshed["startMm"].toArray(),rb=refreshed["endMm"].toArray();
    near(gp_Pnt(ra[0].toDouble(),ra[1].toDouble(),ra[2].toDouble()).Distance(gp_Pnt(rb[0].toDouble(),rb[1].toDouble(),rb[2].toDouble())),p.Distance(middle));
    op.successors.Clear();const auto absent=SolidFillet::transferEdges(op,{named},splitFace,true);check(absent[0].edge.IsNull() && absent[0].provenance["resolutionError"]=="deleted","missing direction unavailable");
    auto invalid=d.authored();auto fs=invalid["features"].toArray();for(int i=0;i<fs.size();++i)if(fs[i].toObject()["type"]=="fillet"){auto f=fs[i].toObject(),params=f["parameters"].toObject();params["edgeIds"]=QJsonArray{source.edgeId};f["parameters"]=params;fs[i]=f;}invalid["features"]=fs;
    check(!d.commit(invalid,"Unavailable seed").isEmpty() && d.authored()==after && d.revision()==revision,"missing authored identity refuses atomically");
    std::cout<<"Shortened rim descriptors/dimensions/width/radius/history/storage and missing/split controls passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
