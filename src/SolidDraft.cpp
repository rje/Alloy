// SPDX-License-Identifier: MIT
#include "SolidDraft.h"
#include "PlanarFaces.h"
#include "SolidFillet.h"
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepOffsetAPI_DraftAngle.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRep_Tool.hxx>
#include <TopExp.hxx>
#include <TopoDS.hxx>
#include <QUuid>
#include <cmath>
#include <algorithm>
namespace {
gp_Pnt point(const QJsonArray& a){return {a[0].toDouble(),a[1].toDouble(),a[2].toDouble()};}
gp_Vec vector(const QJsonArray& a){return {a[0].toDouble(),a[1].toDouble(),a[2].toDouble()};}
QJsonArray xyz(const gp_XYZ& a){return {a.X(),a.Y(),a.Z()};}
gp_Dir normal(const TopoDS_Face& f){auto n=BRepAdaptor_Surface(f).Plane().Axis().Direction();if(f.Orientation()==TopAbs_REVERSED)n.Reverse();return n;}
}
void SolidDraft::evaluate(EvaluatedDocument& document,const QJsonObject& feature){
 const auto p=feature.value("parameters").toObject(),ref=p.value("face").toObject();const auto id=feature.value("id").toString();
 const auto fail=[&](const QString& code,const QString& message){throw GeometryKernel::EvaluationError({{"code",code},{"message",message},{"featureId",id},{"featureType","draft"},{"face",ref}});};
 auto body=std::find_if(document.bodies.begin(),document.bodies.end(),[&](const auto& b){return b.bodyId==p.value("targetBodyId").toString();});
 if(body==document.bodies.end())fail("draft_body_unavailable","Draft requires an earlier active local solid");
 const EvaluatedConstructionPlane* neutral=nullptr;for(const auto& plane:document.constructionPlanes)if(plane.featureId==p.value("neutralPlaneId").toString())neutral=&plane;
 if(!neutral)fail("draft_plane_unavailable","Choose an earlier retained construction plane");
 const auto before=GeometryKernel::summarize(body->shape),topology=before.value("topology").toObject();
 if(!body->occurrenceId.isEmpty() || !before.value("valid").toBool() || !before.value("closed").toBool() || topology.value("solids").toInt()!=1)
  fail("draft_source_unsupported","Draft requires one valid closed local rectangular prism");
 TopTools_IndexedMapOfShape faces,edges,vertices;TopExp::MapShapes(body->shape,TopAbs_FACE,faces);TopExp::MapShapes(body->shape,TopAbs_EDGE,edges);TopExp::MapShapes(body->shape,TopAbs_VERTEX,vertices);
 if(faces.Extent()!=6 || edges.Extent()!=12 || vertices.Extent()!=8)fail("draft_source_unsupported","This Draft slice requires an unmodified six-face rectangular prism");
 const EvaluatedSupportFace* selected=nullptr;for(const auto& f:body->supportFaces)if(f.producerFeatureId==ref.value("producerFeatureId") && f.faceId==ref.value("faceId") && f.bodyId==ref.value("bodyId")){if(selected)fail("draft_face_ambiguous","Selected face has multiple descendants");selected=&f;}
 if(!selected || selected->face.IsNull() || !faces.Contains(selected->face))fail("draft_face_unavailable","The exact selected face is unavailable");
 for(int i=1;i<=faces.Extent();++i)if(BRepAdaptor_Surface(TopoDS::Face(faces(i))).GetType()!=GeomAbs_Plane)fail("draft_source_unsupported","Draft requires six planar faces");
 const auto original=selected->face;const auto n=normal(original);const auto origin=point(neutral->frame.value("origin").toArray());
 auto pull=gp_Dir(vector(neutral->frame.value("normal").toArray()));if(p.value("flipPullDirection").toBool())pull.Reverse();
 if(std::abs(n.Dot(pull))>1e-9)fail("draft_face_unsupported","Select a side face perpendicular to the neutral plane");
 const gp_Dir hinge(gp_Vec(n).Crossed(gp_Vec(pull)));const gp_Pln plane(origin,pull);
 // Admit only the orthogonal prism family; no holes, curved faces or tangent propagation.
 int caps=0;for(int i=1;i<=faces.Extent();++i){const auto f=TopoDS::Face(faces(i));const auto fn=normal(f);const double a=std::abs(fn.Dot(n)),b=std::abs(fn.Dot(pull)),c=std::abs(fn.Dot(hinge));if(std::max({a,b,c})<1-1e-9)fail("draft_source_unsupported","Draft requires an orthogonal rectangular prism");if(b>1-1e-9 && BRepAdaptor_Surface(f).Plane().Distance(origin)<1e-6)++caps;}
 if(caps!=1)fail("draft_plane_unsupported","The neutral construction plane must coincide with one prism cap");
 for(int i=1;i<=edges.Extent();++i)if(BRepAdaptor_Curve(TopoDS::Edge(edges(i))).GetType()!=GeomAbs_Line)fail("draft_source_unsupported","Draft requires straight prism boundaries");
 const double angle=p.value("angleDeg").toDouble()*std::acos(-1.)/180.;
 double minH=1e100,maxH=-1e100,minW=1e100,maxW=-1e100,minL=1e100,maxL=-1e100;
 const auto sideOrigin=BRepAdaptor_Surface(original).Plane().Location();
 for(int i=1;i<=vertices.Extent();++i){const auto v=BRep_Tool::Pnt(TopoDS::Vertex(vertices(i)));const double h=gp_Vec(origin,v).Dot(gp_Vec(pull)),w=gp_Vec(sideOrigin,v).Dot(gp_Vec(n)),l=gp_Vec(origin,v).Dot(gp_Vec(hinge));minH=std::min(minH,h);maxH=std::max(maxH,h);minW=std::min(minW,w);maxW=std::max(maxW,w);minL=std::min(minL,l);maxL=std::max(maxL,l);}
 const double h=std::abs(maxH)>std::abs(minH)?maxH:minH,width=maxW-minW,length=maxL-minL;
 if(width<1e-6 || length<1e-6 || std::abs(h)<1e-6 || std::abs(std::min(std::abs(minH),std::abs(maxH)))>1e-6 || maxW>1e-6)
  fail("draft_source_unsupported","The selected face and cap must bound the rectangular prism");
 if(h*std::tan(angle)>=width-1e-6)fail("draft_topology_change","Draft would cross the opposite wall; reduce the angle");
 const double expectedVolume=before.value("volumeMm3").toDouble()-.5*length*h*std::abs(h)*std::tan(angle);
 BRepBuilderAPI_Copy copy(body->shape,true,false);const auto copied=TopoDS::Face(copy.ModifiedShape(original));
 BRepOffsetAPI_DraftAngle operation(copy.Shape());
 // OCCT 7.9.3 populates ConnectedFaces during Build, not immediately after Add.
 QString stage="Add";try{operation.Add(copied,pull,angle,plane);if(!operation.AddDone())fail("draft_failed","The kernel refused this Draft angle");stage="Build";operation.Build();stage="ConnectedFaces";for(const auto& f:operation.ConnectedFaces(copied))if(!f.IsSame(copied))fail("draft_tangent_propagation","This Draft slice does not propagate to tangent faces");}catch(const Standard_Failure& e){fail("draft_failed",QString("The kernel could not construct this Draft (%1): %2").arg(stage,QString::fromUtf8(e.GetMessageString())));}
 if(!operation.IsDone() || operation.Shape().IsNull())fail("draft_failed","Draft did not produce a result");
 const auto result=operation.Shape();auto summary=GeometryKernel::summarize(result);
 if(!summary.value("valid").toBool() || !summary.value("closed").toBool() || summary.value("topology").toObject()!=topology || std::abs(summary.value("volumeMm3").toDouble()-expectedVolume)>std::max(1e-5,std::abs(expectedVolume)*1e-9))fail("draft_invalid_result","Draft must retain prism topology and the requested signed wedge volume");
 TopTools_IndexedMapOfShape resultFaces;TopExp::MapShapes(result,TopAbs_FACE,resultFaces);
 std::vector<TopoDS_Face> images(faces.Extent()+1);std::vector<int> owners(resultFaces.Extent()+1);
 for(int i=1;i<=faces.Extent();++i){const auto source=copy.ModifiedShape(faces(i));TopTools_IndexedMapOfShape choices;const auto offer=[&](const TopoDS_Shape& s){if(!s.IsNull() && s.ShapeType()==TopAbs_FACE && resultFaces.Contains(s))choices.Add(resultFaces(resultFaces.FindIndex(s)));};offer(source);for(const auto& s:operation.Modified(source))offer(s);for(const auto& s:operation.Generated(source))offer(s);if(choices.Extent()!=1)fail("draft_face_lineage_unavailable","Draft requires a unique kernel descendant for every source face");images[i]=TopoDS::Face(choices(1));++owners[resultFaces.FindIndex(choices(1))];}
 for(int i=1;i<=resultFaces.Extent();++i)if(owners[i]!=1)fail("draft_face_lineage_ambiguous","Draft face history is not one-to-one");
 const auto moved=images[faces.FindIndex(original)];const auto expectedNormal=gp_Vec(n)*std::cos(angle)+gp_Vec(pull)*std::sin(angle);
 if(BRepAdaptor_Surface(moved).GetType()!=GeomAbs_Plane || normal(moved).Dot(gp_Dir(expectedNormal))<1-1e-9)fail("draft_angle_mismatch",QString("Draft result normal %1,%2,%3 differs from expected %4,%5,%6").arg(normal(moved).X()).arg(normal(moved).Y()).arg(normal(moved).Z()).arg(expectedNormal.X()).arg(expectedNormal.Y()).arg(expectedNormal.Z()));
 const auto oldOrigin=point(selected->frame.value("origin").toArray());const auto hingeOrigin=oldOrigin.Translated(gp_Vec(pull)*(-gp_Vec(origin,oldOrigin).Dot(gp_Vec(pull))));
 gp_Trsf rotation;rotation.SetRotation(gp_Ax1(hingeOrigin,hinge),angle);
 std::vector<EvaluatedSupportFace> retained;
 for(auto f:body->supportFaces){if(f.face.IsNull() || !faces.Contains(f.face))continue;const bool drafted=f.face.IsSame(original);const auto next=images[faces.FindIndex(f.face)];f.face=next;f.boundaryEdges.clear();if(drafted){f.frame["origin"]=xyz(oldOrigin.Transformed(rotation).XYZ());for(const auto& key:{"uAxis","vAxis","normal"})f.frame[key]=xyz(vector(f.frame.value(key).toArray()).Transformed(rotation).XYZ());f.producerFeatureId=id;f.faceId=QUuid::createUuidV5(QUuid(id),QByteArray("alloy-draft-face/1\n")+selected->producerFeatureId.toUtf8()+'\n'+selected->faceId.toUtf8()).toString(QUuid::WithoutBraces);f.role="draft.side";f.provenance={{"role","draft.side"},{"sourceFace",ref},{"generatorFeatureId",id}};}
 const auto frameOrigin=point(f.frame.value("origin").toArray());if(BRepAdaptor_Surface(next).Plane().Distance(frameOrigin)>1e-6 || normal(next).Dot(gp_Dir(vector(f.frame.value("normal").toArray())))<1-1e-9)fail("draft_frame_mismatch","The retained face frame does not match its exact descendant");retained.push_back(f);}
 // Changed boundary-edge histories are deliberately unavailable in this slice.
 auto retainedEdges=body->solidEdges;for(auto& e:retainedEdges){e.edge.Nullify();e.provenance["resolutionError"]="modified_by_draft_unqualified";}
 summary["supportFaces"]=PlanarFaces::descriptors(retained);summary["solidEdges"]=SolidFillet::descriptors(retainedEdges);summary["lastModifierFeatureId"]=id;summary["producerFeatureId"]=body->featureId;summary["operation"]="draft";summary["draft"]=p;
 body->shape=result;body->supportFaces=std::move(retained);body->solidEdges=std::move(retainedEdges);body->summary=summary;
}
