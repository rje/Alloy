// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include "SolidFaceOffset.h"
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRep_Tool.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <QCoreApplication>
#include <QTemporaryDir>
#include <iostream>
#include <cmath>
#include <algorithm>
static void check(bool b,const char* m){if(!b)throw std::runtime_error(m);}
static void commit(CadDocument& d,const QJsonObject& a){auto e=d.commit(a,"Offset Face fixture");if(!e.isEmpty())throw std::runtime_error(e.toStdString());}
static QJsonObject ref(const EvaluatedSupportFace& f){return {{"bodyId",f.bodyId},{"producerFeatureId",f.producerFeatureId},{"faceId",f.faceId}};}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
 // Exercise the actual evaluator with a sloped adjacent wall. The volume
 // integral is 20 * (20*h - h*h/4), not a constant-section prism addition.
 BRepBuilderAPI_MakePolygon wire;wire.Add(gp_Pnt(0,0,0));wire.Add(gp_Pnt(20,0,0));wire.Add(gp_Pnt(15,0,10));wire.Add(gp_Pnt(0,0,10));wire.Close();
 EvaluatedBody angled;angled.bodyId=CadDocument::newId();angled.featureId=CadDocument::newId();
 angled.shape=BRepPrimAPI_MakePrism(BRepBuilderAPI_MakeFace(wire.Wire()).Face(),gp_Vec(0,20,0)).Shape();angled.summary=GeometryKernel::summarize(angled.shape);
 for(TopExp_Explorer it(angled.shape,TopAbs_FACE);it.More();it.Next()){
    auto face=TopoDS::Face(it.Current());const BRepAdaptor_Surface plane(face);
    if(plane.GetType()==GeomAbs_Plane&&plane.Plane().Axis().Direction().Z()>.99&&plane.Plane().Location().Z()>9)
        angled.supportFaces.push_back({CadDocument::newId(),angled.bodyId,angled.featureId,"fixture.top",face,
            {{"origin",QJsonArray{0,0,10}},{"normal",QJsonArray{0,0,1}},{"uAxis",QJsonArray{1,0,0}},{"vAxis",QJsonArray{0,1,0}}},{},{}});
 }
 check(angled.supportFaces.size()==1,"Angled top identity");
 QString slopedRimId;
 for(TopExp_Explorer it(angled.supportFaces[0].face,TopAbs_EDGE);it.More();it.Next()){
    const auto edge=TopoDS::Edge(it.Current());const BRepAdaptor_Curve curve(edge);
    if(curve.GetType()!=GeomAbs_Line)continue;
    const auto start=curve.Value(curve.FirstParameter()),end=curve.Value(curve.LastParameter());
    if(std::abs(start.X()-15)>1e-6||std::abs(end.X()-15)>1e-6||
        std::abs(start.Z()-10)>1e-6||std::abs(end.Z()-10)>1e-6)continue;
    check(slopedRimId.isEmpty(),"Unique sloped top rim");slopedRimId=CadDocument::newId();
    const QJsonObject provenance{{"endpointProvenance","currentKernelBoundary"},
        {"startMm",QJsonArray{start.X(),start.Y(),start.Z()}},{"endMm",QJsonArray{end.X(),end.Y(),end.Z()}}};
    angled.solidEdges.push_back({slopedRimId,edge,provenance});
    angled.supportFaces[0].boundaryEdges.push_back({slopedRimId,"line",edge,provenance});
 }
 check(!slopedRimId.isEmpty(),"Sloped source rim");
 for(double distance:{2.,-2.}){auto result=angled;
    SolidFaceOffset::evaluate(result,{{"face",ref(angled.supportFaces[0])},{"targetBodyId",angled.bodyId},{"distanceMm",distance}},CadDocument::newId());
    check(std::abs(result.summary["volumeMm3"].toDouble()-(distance>0?4080:2880))<1e-5,"Angled-adjacent true offset volume");
    check(result.supportFaces.size()==1&&std::abs(result.supportFaces[0].frame["origin"].toArray()[2].toDouble()-10-distance)<1e-6,"Angled exact moved support");
    auto named=std::find_if(result.solidEdges.begin(),result.solidEdges.end(),[&](const auto& edge){return edge.edgeId==slopedRimId;});
    check(named!=result.solidEdges.end()&&!named->edge.IsNull(),"Sloped rim exact descendant");
    const BRepAdaptor_Curve current(named->edge);
    for(const auto& point:{current.Value(current.FirstParameter()),current.Value(current.LastParameter())})
        check(std::abs(point.X()-(distance>0?14:16))<1e-5&&std::abs(point.Z()-10-distance)<1e-5,"Sloped rim true retrimmed coordinates");
    const auto start=named->provenance["startMm"].toArray(),end=named->provenance["endMm"].toArray();
    check(start.size()==3&&end.size()==3&&std::abs(start[0].toDouble()-(distance>0?14:16))<1e-5
        &&std::abs(end[0].toDouble()-(distance>0?14:16))<1e-5,"Sloped rim refreshed endpoint metadata");
    check(result.supportFaces[0].boundaryEdges.size()==1&&result.supportFaces[0].boundaryEdges[0].edgeId==slopedRimId,
        "Sloped top boundary retains exact rim identity");
 }
 // The ledge becomes flush with the upper step at +5. OCCT still returns one
 // valid solid, but both short vertical step edges have no final descendants.
 BRepBuilderAPI_MakePolygon steppedWire;
 for(const auto& point: {gp_Pnt(0,0,0),gp_Pnt(20,0,0),gp_Pnt(20,0,5),
                         gp_Pnt(10,0,5),gp_Pnt(10,0,10),gp_Pnt(0,0,10)})
     steppedWire.Add(point);
 steppedWire.Close();
 EvaluatedBody stepped;stepped.bodyId=CadDocument::newId();stepped.featureId=CadDocument::newId();
 stepped.shape=BRepPrimAPI_MakePrism(BRepBuilderAPI_MakeFace(steppedWire.Wire()).Face(),gp_Vec(0,10,0)).Shape();
 stepped.summary=GeometryKernel::summarize(stepped.shape);
 check(stepped.summary["valid"].toBool(),"Stepped source valid");
 for(TopExp_Explorer it(stepped.shape,TopAbs_FACE);it.More();it.Next()) {
     const auto face=TopoDS::Face(it.Current());const BRepAdaptor_Surface plane(face);
     if(plane.GetType()!=GeomAbs_Plane || std::abs(plane.Plane().Location().Z()-5)>1e-6
         || std::abs(plane.Plane().Axis().Direction().Z())<.99)continue;
     stepped.supportFaces.push_back({CadDocument::newId(),stepped.bodyId,stepped.featureId,
         "fixture.ledge",face,{{"origin",QJsonArray{15,0,5}},{"normal",QJsonArray{0,0,1}},
         {"uAxis",QJsonArray{1,0,0}},{"vAxis",QJsonArray{0,1,0}}},{},{}});
 }
 check(stepped.supportFaces.size()==1,"Unique stepped ledge support");
 QString deletedId,stableId;
 for(TopExp_Explorer it(stepped.shape,TopAbs_EDGE);it.More();it.Next()) {
     const auto edge=TopoDS::Edge(it.Current());const BRepAdaptor_Curve curve(edge);
     if(curve.GetType()!=GeomAbs_Line)continue;
     const auto first=curve.Value(curve.FirstParameter()),last=curve.Value(curve.LastParameter());
     const bool deleted=std::abs(first.X()-10)<1e-6 && std::abs(last.X()-10)<1e-6
         && std::abs(first.Y())<1e-6 && std::abs(last.Y())<1e-6
         && std::abs(std::min(first.Z(),last.Z())-5)<1e-6
         && std::abs(std::max(first.Z(),last.Z())-10)<1e-6;
     const bool stable=std::abs(first.Z())<1e-6 && std::abs(last.Z())<1e-6
         && std::abs(first.Y())<1e-6 && std::abs(last.Y())<1e-6
         && std::abs(std::abs(first.X()-last.X())-20)<1e-6;
     if(!deleted&&!stable)continue;
     const auto edgeId=CadDocument::newId();
     stepped.solidEdges.push_back({edgeId,edge,{{"startMm",QJsonArray{first.X(),first.Y(),first.Z()}},
         {"endMm",QJsonArray{last.X(),last.Y(),last.Z()}}}});
     if(deleted)deletedId=edgeId;else stableId=edgeId;
 }
 check(!deletedId.isEmpty()&&!stableId.isEmpty(),"Named deleted and unaffected source edges");
 SolidFaceOffset::evaluate(stepped,{{"face",ref(stepped.supportFaces[0])},
     {"targetBodyId",stepped.bodyId},{"distanceMm",5}},CadDocument::newId());
 check(stepped.summary["valid"].toBool()&&stepped.summary["closed"].toBool()
     &&std::abs(stepped.summary["volumeMm3"].toDouble()-2000)<1e-5,
     "Deleted-edge offset still yields one valid solid");
 const auto resolved=[&](const QString& edgeId){return std::find_if(stepped.solidEdges.begin(),stepped.solidEdges.end(),
     [&](const auto& edge){return edge.edgeId==edgeId;});};
 check(resolved(deletedId)!=stepped.solidEdges.end()&&resolved(deletedId)->edge.IsNull()
     &&resolved(deletedId)->provenance["resolutionError"]=="deleted",
     "Vanished step edge explicitly unavailable");
 check(resolved(stableId)!=stepped.solidEdges.end()&&!resolved(stableId)->edge.IsNull(),
     "Unaffected bottom edge remains available");
 CadDocument d;auto a=d.authored();QJsonObject box{{"id",CadDocument::newId()},{"bodyId",CadDocument::newId()},{"ownerId",a["documentId"]},{"type","box"},{"name","Box"},{"visible",true},{"parameters",QJsonObject{{"xMm",0},{"yMm",0},{"zMm",0},{"widthMm",20},{"depthMm",20},{"heightMm",10}}}};
 a["features"]=QJsonArray{box};commit(d,a);QJsonObject selected;
 QString boxTopRimId,boxBottomRimId;
 for(const auto& edge:d.bodies()[0].solidEdges){
    if(edge.provenance["boxEdge"]=="x*y0z1")boxTopRimId=edge.edgeId;
    if(edge.provenance["boxEdge"]=="x*y0z0")boxBottomRimId=edge.edgeId;
 }
 check(!boxTopRimId.isEmpty()&&!boxBottomRimId.isEmpty(),"Box source rim identities");
 for(const auto& f:d.bodies()[0].supportFaces)if(f.frame["normal"].toArray()[2].toDouble()>.99)selected=ref(f);
 check(!selected.isEmpty(),"Top support");
 auto candidate=CadDocument::faceOffsetCandidate(a,{{"targetBodyId",box["bodyId"]},{"face",selected},{"distanceMm",2}});commit(d,candidate);
 const auto id=d.authored()["features"].toArray().last().toObject().value("id");
 check(std::abs(d.bodies()[0].summary["volumeMm3"].toDouble()-4800)<1e-5,"Positive offset volume");
 auto rim=[&](const QString& edgeId){const auto& edges=d.bodies()[0].solidEdges;
    return std::find_if(edges.begin(),edges.end(),[&](const auto& edge){return edge.edgeId==edgeId;});};
 check(rim(boxTopRimId)!=d.bodies()[0].solidEdges.end()&&!rim(boxTopRimId)->edge.IsNull(),"Top rim retained");
 check(rim(boxBottomRimId)!=d.bodies()[0].solidEdges.end()&&!rim(boxBottomRimId)->edge.IsNull(),"Bottom rim retained");
 check(std::abs(rim(boxTopRimId)->provenance["startMm"].toArray()[2].toDouble()-12)<1e-6,"Top rim endpoint moved");
 check(std::abs(rim(boxBottomRimId)->provenance["startMm"].toArray()[2].toDouble())<1e-6,"Bottom rim endpoint unchanged");
 bool capHasRim=false;for(const auto& face:d.bodies()[0].supportFaces)if(face.role=="faceOffset.end")
    for(const auto& edge:face.boundaryEdges)if(edge.edgeId==boxTopRimId)capHasRim=true;
 check(capHasRim,"Moved cap boundary retains top rim identity");
 const auto created=d.authored();d.history().undo();check(d.authored()==a,"Undo");d.history().redo();check(d.authored()==created,"Redo");
 commit(d,CadDocument::faceOffsetCandidate(d.authored(),{{"featureId",id},{"distanceMm",-2}}));
 check(std::abs(d.bodies()[0].summary["volumeMm3"].toDouble()-3200)<1e-5,"Negative offset volume");
 check(rim(boxTopRimId)!=d.bodies()[0].solidEdges.end()&&!rim(boxTopRimId)->edge.IsNull()
    &&std::abs(rim(boxTopRimId)->provenance["startMm"].toArray()[2].toDouble()-8)<1e-6,
    "Top rim stable on negative edit");
 commit(d,CadDocument::faceOffsetCandidate(d.authored(),{{"featureId",id},{"distanceMm",2}}));
 EvaluatedSupportFace moved;for(const auto& f:d.bodies()[0].supportFaces)if(f.role=="faceOffset.end")moved=f;
 check(!moved.face.IsNull()&&moved.producerFeatureId==id.toString(),"Exact retained moved support");
 auto sketch=SketchGeometry::newSupportedFeature(a["documentId"].toString(),ref(moved),"Hole point",{});
 const auto context=GeometryKernel::projectionContext(d.evaluation(),sketch);
 auto solved=SketchGeometry::addStandalonePoint(sketch,{{"uMm",10},{"vMm",10}},context);check(solved.ok,"Point solve");sketch=solved.feature;
 auto point=sketch["parameters"].toObject()["entities"].toArray()[0].toObject();commit(d,SketchGeometry::replaceFeature(d.authored(),sketch));
 commit(d,CadDocument::holeCandidate(d.authored(),{{"sourceSketchId",sketch["id"]},{"sourceEntityId",point["id"]},{"sourcePointId",point["pointId"]},{"targetBodyId",box["bodyId"]},{"boreDiameterMm",2},{"sinkDiameterMm",4},{"includedAngleDeg",90}}));
 commit(d,CadDocument::faceOffsetCandidate(d.authored(),{{"featureId",id},{"distanceMm",4}}));
 check(std::abs(d.bodies()[0].summary["entryMm"].toArray()[2].toDouble()-14)<1e-5,"Later Hole follows offset edit");
 const auto stable=d.authored();const auto revision=d.revision();
 check(!d.commit(CadDocument::faceOffsetCandidate(stable,{{"featureId",id},{"distanceMm",-20}}),"Collapse").isEmpty(),"Collapse refused");
 check(d.authored()==stable&&d.revision()==revision,"Collapse atomic");
 auto stale=selected;stale["faceId"]=CadDocument::newId();check(!d.commit(CadDocument::faceOffsetCandidate(stable,{{"featureId",id},{"face",stale}}),"Stale").isEmpty(),"Stale refused");check(d.authored()==stable&&d.revision()==revision,"Stale atomic");
 QTemporaryDir temp;auto path=temp.filePath("offset.alloy");check(d.save(path).isEmpty(),"Save");CadDocument reopened;check(reopened.open(path).isEmpty()&&reopened.authored()==stable,"Reopen");
 std::cout<<"Offset Face signed volume, retained moved support, downstream Hole/edit, atomic refusal, undo/redo and reopen passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
