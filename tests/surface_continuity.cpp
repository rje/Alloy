// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include "SurfaceContinuity.h"
#include <QTemporaryDir>
#include <QJsonDocument>
#include <BRep_Builder.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS.hxx>
#include <cmath>
#include <numbers>
#include <QCoreApplication>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <BRep_Tool.hxx>
#include <gp_Ax1.hxx>
#include <iostream>
namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void commit(CadDocument& d,const QJsonObject& candidate){const auto error=d.commit(candidate,"Fixture");if(!error.isEmpty())throw std::runtime_error(error.toStdString());}
QJsonObject entity(const QJsonObject& s,int i){return s.value("parameters").toObject().value("entities").toArray()[i].toObject();}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
 CadDocument d;auto s=SketchGeometry::newFeature(d.authored().value("documentId").toString(),"XY","Continuity fixture");
 s=SketchGeometry::addLine(s,{{"u1Mm",-10},{"v1Mm",0},{"u2Mm",0},{"v2Mm",0}});
 s=SketchGeometry::addLine(s,{{"u1Mm",0},{"v1Mm",0},{"u2Mm",10},{"v2Mm",0},{"startPointId",entity(s,0).value("endPointId")}});
 auto authored=d.authored();auto features=authored.value("features").toArray();features.append(s);authored["features"]=features;commit(d,authored);
 commit(d,CadDocument::surfaceExtrudeCandidate(d.authored(),{{"resultMode","perProfile"},{"sketchId",s.value("id")},{"profiles",QJsonArray{QJsonObject{{"entityIds",QJsonArray{entity(s,0).value("id"),entity(s,1).value("id")}}}}},{"sideOneMm",20},{"sideTwoMm",20}}));
 const auto& b=d.bodies().back();check(b.supportFaces.size()==2,"Two named coplanar faces required");
 TopTools_IndexedMapOfShape faces,edges;TopExp::MapShapes(b.shape,TopAbs_FACE,faces);TopExp::MapShapes(b.shape,TopAbs_EDGE,edges);check(faces.Extent()==2 && edges.Extent()==7,"Connected two-Line prism must retain two faces and seven edges");
 int shared=0;QString edgeId;
 for(const auto& a:b.supportFaces[0].boundaryEdges)for(const auto& c:b.supportFaces[1].boundaryEdges)if(a.edge.IsSame(c.edge)){
   ++shared;edgeId=a.edgeId;check(a.edgeId==c.edgeId && edges.Contains(a.edge),"Shared exact edge must have one retained ID");check(BRep_Tool::SameParameter(a.edge) && BRep_Tool::SameRange(a.edge),"Shared edge parameter contract required");
 }
 check(shared==1 && !edgeId.isEmpty(),"Exactly one retained topological shared edge required");
 QJsonArray refs;for(const auto& face:b.supportFaces)refs.append(QJsonObject{{"bodyId",b.bodyId},{"faceId",face.faceId},{"producerFeatureId",face.producerFeatureId}});
 const auto before=d.authored();const auto revision=d.revision();const auto clean=d.isDirty();const auto topology=b.summary;
 auto sample=[&](const std::vector<EvaluatedBody>& bodies,const QJsonArray& sources,const QString& id,double expected){
   const auto r=SurfaceContinuity::evaluate(bodies,sources,id);check(r.value("status")=="ready",QJsonDocument(r).toJson().constData());
   check(r.value("sampleCount")==9 && r.value("samples").toArray().size()==9,"Fixed sample count");
   check(std::abs(r.value("maxGapMm").toDouble())<1e-10 && std::abs(r.value("maxTangentPlaneAngleDeg").toDouble()-expected)<1e-10,"Analytic sampled maxima");
   check(r.value("sampledG0Pass").toBool() && r.value("sampledG1Pass").toBool()==(expected<.1),"Independent G0 and G1 statuses");
   const auto rows=r.value("samples").toArray();for(int i=0;i<rows.size();++i){const auto row=rows[i].toObject();check(row.value("fraction").toDouble()==i/8.0,"Uniform parameters include endpoints");check(std::abs(row.value("gapMm").toDouble())<1e-10 && std::abs(row.value("tangentPlaneAngleDeg").toDouble()-expected)<1e-10,"Every analytic sample");const auto sides=row.value("faces").toArray();check(sides.size()==2,"Two per-face evaluations");for(const auto& side:sides){const auto face=side.toObject();check(face.value("uv").toArray().size()==2 && face.value("pointMm").toArray().size()==3 && face.value("orientedUnitNormal").toArray().size()==3,"UV world point and oriented unit normal retained");}}
   return r;
 };
 const auto continuous=sample(d.bodies(),refs,edgeId,0);
 check(d.authored()==before && d.revision()==revision && d.isDirty()==clean && d.bodies().back().summary==topology,"Inspection leaves document and evaluated geometry unchanged");
 auto changed=s;auto params=s.value("parameters").toObject();auto points=params.value("points").toArray();const auto end=entity(s,1).value("endPointId");for(int i=0;i<points.size();++i){auto point=points[i].toObject();if(point.value("id")==end){point["vMm"]=10;points[i]=point;}}params["points"]=points;changed["parameters"]=params;
 commit(d,SketchGeometry::replaceFeature(d.authored(),changed));const auto kink=sample(d.bodies(),refs,edgeId,45);
 d.history().undo();check(sample(d.bodies(),refs,edgeId,0)==continuous,"Undo restores deterministic samples");d.history().redo();check(sample(d.bodies(),refs,edgeId,45)==kink,"Redo restores deterministic samples");
 QTemporaryDir temp;check(d.save(temp.filePath("continuity.alloy")).isEmpty(),"Save fixture");CadDocument reopened;check(reopened.open(temp.filePath("continuity.alloy")).isEmpty(),"Reopen fixture");check(sample(reopened.bodies(),refs,edgeId,45)==kink,"Exact deterministic result on reopen");
 const auto inch=SurfaceContinuity::evaluate(d.bodies(),refs,edgeId,"in");check(inch.value("samples")==kink.value("samples") && inch.value("display").toObject().value("lengthUnit")=="in" && std::abs(inch.value("display").toObject().value("gapThreshold").toDouble()-.001/25.4)<1e-15,"Display units preserve canonical results");
 auto reject=[&](const std::vector<EvaluatedBody>& bodies,const QJsonArray& sources,const QString& id,const char* code){const auto r=SurfaceContinuity::evaluate(bodies,sources,id);check(r.value("status")!="ready" && r.value("error").toObject().value("code")==code,QJsonDocument(r).toJson().constData());};
 reject(d.bodies(),QJsonArray{refs[0],refs[0]},edgeId,"continuity_duplicate_face");reject(d.bodies(),refs,"missing","continuity_stale_edge");reject(d.bodies(),{},edgeId,"continuity_inputs");
 auto stale=refs;auto ref=stale[0].toObject();ref["producerFeatureId"]="old";stale[0]=ref;reject(d.bodies(),stale,edgeId,"continuity_stale");
 auto hidden=d.bodies();hidden.back().visible=false;reject(hidden,refs,edgeId,"continuity_stale");
 auto detached=d.bodies();detached.back().supportFaces[0].face=d.bodies().back().supportFaces[1].face;reject(detached,refs,edgeId,"continuity_duplicate_face");
 auto noRegistry=d.bodies();noRegistry.back().supportFaces[0].boundaryEdges.clear();reject(noRegistry,refs,edgeId,"continuity_not_shared");
 QString outer;for(const auto& e:d.bodies().back().solidEdges)if(e.edgeId!=edgeId){outer=e.edgeId;break;}reject(d.bodies(),refs,outer,"continuity_not_shared");
 auto occurrence=d.bodies();occurrence.back().occurrenceId="one";occurrence.push_back(occurrence.back());occurrence.back().occurrenceId="two";reject(occurrence,refs,edgeId,"continuity_ambiguous");
 auto exactRefs=refs;for(int i=0;i<2;++i){auto source=refs[i].toObject();source["occurrenceId"]="two";exactRefs[i]=source;}sample(occurrence,exactRefs,edgeId,45);
 // Reverse one actual current face: tangent-plane continuity is unoriented;
 // the independent normal diagnostic must still report the reversal.
 auto reversed=GeometryKernel::evaluateDocument(before).bodies;auto& reversedBody=reversed.back();reversedBody.supportFaces[1].face=TopoDS::Face(reversedBody.supportFaces[1].face.Reversed());
 TopoDS_Compound compound;BRep_Builder builder;builder.MakeCompound(compound);for(const auto& face:reversedBody.supportFaces)builder.Add(compound,face.face);reversedBody.shape=compound;
 const auto orientation=sample(reversed,refs,edgeId,0);check(orientation.value("normalOrientation").toObject().value("oppositeHemisphereSampleCount")==9 && orientation.value("normalOrientation").toObject().value("maxOrientedNormalAngleDeg")==180,"Reversed normals reported independently from tangent-plane angle");
 auto located=d.bodies();gp_Trsf transform;transform.SetRotation(gp_Ax1(gp_Pnt(0,0,0),gp_Dir(1,0,0)),std::numbers::pi/2);transform.SetTranslationPart(gp_Vec(2,3,4));const TopLoc_Location location(transform);
 for(auto& current:located){current.shape.Move(location);for(auto& f:current.supportFaces){f.face.Move(location);for(auto& e:f.boundaryEdges)e.edge.Move(location);}for(auto& e:current.solidEdges)e.edge.Move(location);}
 const auto placed=sample(located,refs,edgeId,45);const auto firstPoint=placed.value("samples").toArray()[0].toObject().value("faces").toArray()[0].toObject().value("pointMm").toArray();check(std::abs(firstPoint[0].toDouble()-2)<1e-10 && std::abs(firstPoint[1].toDouble()-23)<1e-10 && std::abs(firstPoint[2].toDouble()-4)<1e-10,"D1 points evaluated in world location");
 auto offBody=d.bodies();offBody.back().supportFaces[0].face.Move(location);reject(offBody,refs,edgeId,"continuity_stale");
 auto parameter=d.bodies();TopoDS_Edge sharedEdge;for(const auto& e:parameter.back().solidEdges)if(e.edgeId==edgeId)sharedEdge=e.edge;builder.SameParameter(sharedEdge,false);reject(parameter,refs,edgeId,"continuity_parameter_contract");builder.SameParameter(sharedEdge,true);builder.SameRange(sharedEdge,false);reject(parameter,refs,edgeId,"continuity_parameter_contract");builder.SameRange(sharedEdge,true);
 std::cout<<"SURFACE_CONTINUITY_CORE_OK coplanar kink45 allSamples orientation units exactReferences sourceEdit undo redo reopen readOnly\n";
 std::cout<<"SURFACE_CONTINUITY_TOPOLOGY_GATE_OK twoNamedCoplanarFaces sevenEdges oneSharedRetainedEdge sameParameter sameRange\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
