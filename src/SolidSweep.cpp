// SPDX-License-Identifier: MIT
#include "SolidSweep.h"
#include "PlanarFaces.h"
#include <BRepOffsetAPI_MakePipe.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepAlgoAPI_Check.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <TopoDS.hxx>
#include <TopExp_Explorer.hxx>
#include <QUuid>
#include <QSet>
#include <limits>
#include <cmath>
#include <algorithm>

EvaluatedBody SolidSweep::evaluate(const QJsonObject& feature,const EvaluatedDocument& prior,const QJsonObject& pathSource) {
    const auto p=feature["parameters"].toObject();
    auto reject=[&](const QString& code,const QString& message,QJsonObject extra=QJsonObject{}){extra["featureId"]=feature["id"];extra["featureType"]="sweep";extra["profileSketchId"]=p["profileSketchId"];extra["profileId"]=p["profileId"];extra["pathSketchId"]=p["pathSketchId"];extra["pathEntityIds"]=p["pathEntityIds"];extra["pathStartPointId"]=p["pathStartPointId"];extra["code"]=code;extra["message"]=message;throw GeometryKernel::EvaluationError(extra);};
    const EvaluatedSketch *profileSketch=nullptr,*pathSketch=nullptr;
    for(const auto& s:prior.sketches){if(s.featureId==p["profileSketchId"])profileSketch=&s;if(s.featureId==p["pathSketchId"])pathSketch=&s;}
    if(!profileSketch || !pathSketch)reject("source_unavailable","Sweep requires two earlier retained sketch sources");
    const EvaluatedProfile* profile=nullptr;for(const auto& q:profileSketch->profiles)if(q.profileId==p["profileId"])profile=&q;
    if(!profile)reject("profile_unavailable","Sweep profile is unavailable; correct the retained profile source");
    const auto profileIds=profile->summary["sourceEntityIds"].toArray();QJsonObject circle;
    if(profileIds.size()==1)for(const auto& q:profileSketch->summary["entities"].toArray())if(q.toObject()["id"]==profileIds.first())circle=q.toObject();
    if(circle["type"]!="circle" || profile->summary["innerLoopCount"].toInt()!=0)reject("unsupported_profile","This Sweep requires one circular disk profile without inner loops");
    QMap<QString,gp_Pnt> positions;for(const auto& q:pathSketch->summary["worldPoints"].toArray()){auto x=q.toObject();const auto a=x["positionMm"].toArray();positions[x["id"].toString()]=gp_Pnt(a[0].toDouble(),a[1].toDouble(),a[2].toDouble());}
    QMap<QString,QJsonObject> entities;for(const auto& q:pathSketch->summary["entities"].toArray())entities[q.toObject()["id"].toString()]=q.toObject();
    QMap<QString,QString> roots;for(auto i=positions.begin();i!=positions.end();++i)roots[i.key()]=i.key();
    auto merge=[&](const QString& a,const QString& b){const auto keep=std::min(roots.value(a),roots.value(b)),remove=std::max(roots.value(a),roots.value(b));if(keep.isEmpty())return;for(auto i=roots.begin();i!=roots.end();++i)if(i.value()==remove)i.value()=keep;};
    for(const auto& q:pathSource["parameters"].toObject()["constraints"].toArray()){auto c=q.toObject();auto ids=c["pointIds"].toArray();if(c["type"]=="coincident" && ids.size()==2)merge(ids[0].toString(),ids[1].toString());}
    const auto ids=p["pathEntityIds"].toArray();QSet<QString> endpoints;
    for(const auto& q:ids){const auto id=q.toString();const auto e=entities.value(id);if(e.isEmpty() || !pathSketch->entityEdges.contains(id))reject("path_entity_unavailable","A retained path entity is unavailable",{{"entityId",id}});if((e["type"]!="line" && e["type"]!="arc") || e["construction"].toBool())reject("unsupported_path_entity","Sweep path supports ordinary analytic lines and arcs",{{"entityId",id}});endpoints.insert(e["startPointId"].toString());endpoints.insert(e["endPointId"].toString());}
    // Exact-coordinate distinct endpoints remain valid. Only floating-point
    // roundoff is ignored; this is not a sketch closure/healing tolerance.
    const auto endpointList=endpoints.values();for(int i=0;i<endpointList.size();++i)for(int j=i+1;j<endpointList.size();++j){const auto a=positions.value(endpointList[i]),b=positions.value(endpointList[j]);const double scale=std::max({1.,std::abs(a.X()),std::abs(a.Y()),std::abs(a.Z()),std::abs(b.X()),std::abs(b.Y()),std::abs(b.Z())});if(a.Distance(b)<=64*std::numeric_limits<double>::epsilon()*scale)merge(endpointList[i],endpointList[j]);}
    QMap<QString,int> degree;for(const auto& q:ids){const auto e=entities[q.toString()];++degree[roots.value(e["startPointId"].toString())];++degree[roots.value(e["endPointId"].toString())];}
    int ends=0;for(auto i=degree.begin();i!=degree.end();++i){if(i.value()>2)reject("branched_path","Select one unbranched Sweep path",{{"pointId",i.key()}});if(i.value()==1)++ends;}
    const auto start=p["pathStartPointId"].toString();if(ends!=2)reject("disconnected_path","Sweep requires one open chain with exactly two endpoints; gaps and disconnected pieces are not healed",{{"endpointCount",ends},{"closureApplied",false}});if(!positions.contains(start) || !endpoints.contains(start) || degree.value(roots.value(start))!=1)reject("open_path_required","Sweep start must be a retained endpoint of the selected open path");
    QString current=start;BRepBuilderAPI_MakeWire wire;gp_Vec initialTangent,lastTangent;gp_Pnt finish;double length=0.;QJsonArray oriented;
    for(const auto& q:ids){const auto id=q.toString();const auto e=entities[id];const auto a=e["startPointId"].toString(),b=e["endPointId"].toString();bool reverse=false;
        if(roots.value(current)==roots.value(a))reverse=false;else if(roots.value(current)==roots.value(b))reverse=true;else reject("disconnected_path","Ordered Sweep entities do not continue from the retained start",{{"entityId",id},{"pointId",current},{"distanceToStartMm",positions[current].Distance(positions[a])},{"distanceToEndMm",positions[current].Distance(positions[b])},{"closureApplied",false}});
        auto edge=TopoDS::Edge(BRepBuilderAPI_Copy(pathSketch->entityEdges[id],false,false).Shape());BRepAdaptor_Curve curve(edge);gp_Pnt first,last;gp_Vec firstTangent,endTangent;
        const bool backwards=(edge.Orientation()==TopAbs_REVERSED)!=reverse;curve.D1(backwards ? curve.LastParameter():curve.FirstParameter(),first,firstTangent);curve.D1(backwards ? curve.FirstParameter():curve.LastParameter(),last,endTangent);if(backwards){firstTangent.Reverse();endTangent.Reverse();}if(reverse)edge.Reverse();
        if(firstTangent.Magnitude()<1e-12 || endTangent.Magnitude()<1e-12)reject("degenerate_path","Sweep path has a degenerate endpoint tangent",{{"entityId",id}});
        firstTangent.Normalize();endTangent.Normalize();if(oriented.isEmpty())initialTangent=firstTangent;else if(lastTangent.Dot(firstTangent)<1.-1e-10)reject("non_g1_path","Sweep path junction must be tangent-continuous",{{"entityId",id},{"tangentDot",lastTangent.Dot(firstTangent)}});
        wire.Add(edge);if(!wire.IsDone())reject("path_wire_failed","OCCT could not connect the retained path; correct its endpoint relations",{{"entityId",id}});
        GProp_GProps measure;BRepGProp::LinearProperties(edge,measure);length+=measure.Mass();lastTangent=endTangent;finish=last;current=reverse?a:b;oriented.append(QJsonObject{{"entityId",id},{"reversed",reverse}});
    }
    if(degree.value(roots.value(current))!=1 || roots.value(current)==roots.value(start))reject("disconnected_path","Ordered Sweep path must traverse all selected entities exactly once");
    const auto centerValues=circle["centerMm"].toArray();const gp_Pnt center(centerValues[0].toDouble(),centerValues[1].toDouble(),centerValues[2].toDouble());const auto origin=positions[start];
    if(center.Distance(origin)>1e-7)reject("profile_start_misaligned","Circle profile center must coincide with the retained path start",{{"distanceMm",center.Distance(origin)}});
    const auto normalValues=profileSketch->resolvedFrame["normal"].toArray();const gp_Vec normal(normalValues[0].toDouble(),normalValues[1].toDouble(),normalValues[2].toDouble());if(std::abs(normal.Dot(initialTangent))<1.-1e-10)reject("profile_plane_misaligned","Circle profile plane must be perpendicular to the path start tangent");
    const auto face=BRepBuilderAPI_Copy(profile->face,false,false).Shape();BRepOffsetAPI_MakePipe sweep(wire.Wire(),face,GeomFill_IsCorrectedFrenet,false);if(!sweep.IsDone())reject("sweep_failed","OCCT Sweep did not complete");
    EvaluatedBody body;body.featureId=feature["id"].toString();body.bodyId=feature["bodyId"].toString();body.name=feature["name"].toString();body.visible=feature["visible"].toBool();body.shape=sweep.Shape();body.summary=GeometryKernel::summarize(body.shape);
    if(!body.summary["valid"].toBool() || !body.summary["closed"].toBool() || body.summary["topology"].toObject()["solids"]!=1 || !std::isfinite(body.summary["volumeMm3"].toDouble()) || body.summary["volumeMm3"].toDouble()<=0)reject("sweep_invalid","Sweep must produce one valid closed positive-volume solid");
    BRepAlgoAPI_Check interference(body.shape,false,true);if(!interference.IsValid())reject("sweep_self_intersection","Sweep result has geometric self-interference; correct the path or reduce the profile radius");
    // First/LastShape are builder-provided lineage, never proximity matches.
    for(int end=0;end<2;++end){const auto lineage=end?sweep.LastShape():sweep.FirstShape();if(lineage.IsNull() || lineage.ShapeType()!=TopAbs_FACE)continue;TopoDS_Face cap;int matches=0;for(TopExp_Explorer f(body.shape,TopAbs_FACE);f.More();f.Next())if(f.Current().IsSame(lineage)){cap=TopoDS::Face(f.Current());++matches;}if(matches!=1 || BRepAdaptor_Surface(cap).GetType()!=GeomAbs_Plane)continue;const auto n=end?lastTangent:-initialTangent;const auto oldU=profileSketch->resolvedFrame["uAxis"].toArray();gp_Vec u(oldU[0].toDouble(),oldU[1].toDouble(),oldU[2].toDouble());u-=n*u.Dot(n);if(u.Magnitude()<1e-8){u=gp_Vec(1,0,0)-n*n.X();if(u.Magnitude()<1e-8)u=gp_Vec(0,1,0)-n*n.Y();}u.Normalize();const auto v=n.Crossed(u);const auto o=end?finish:origin;const QJsonObject frame{{"origin",QJsonArray{o.X(),o.Y(),o.Z()}},{"uAxis",QJsonArray{u.X(),u.Y(),u.Z()}},{"vAxis",QJsonArray{v.X(),v.Y(),v.Z()}},{"normal",QJsonArray{n.X(),n.Y(),n.Z()}}};const QString role=end?"sweep.end":"sweep.start";body.supportFaces.push_back({QUuid::createUuidV5(QUuid(body.featureId),("alloy-face/"+role+"/1").toUtf8()).toString(QUuid::WithoutBraces),body.bodyId,body.featureId,role,cap,frame,{}});}
    body.summary["supportFaces"]=PlanarFaces::descriptors(body.supportFaces);body.summary["operation"]="sweep";body.summary["profileSketchId"]=p["profileSketchId"];body.summary["sourceProfileId"]=p["profileId"];body.summary["pathSketchId"]=p["pathSketchId"];body.summary["pathEntityIds"]=ids;body.summary["pathStartPointId"]=p["pathStartPointId"];body.summary["orientedPath"]=oriented;body.summary["pathLengthMm"]=length;body.summary["sectionRadiusMm"]=circle["radiusMm"];body.summary["extent"]="full";body.summary["orientation"]="perpendicular";body.summary["pathStartMm"]=QJsonArray{origin.X(),origin.Y(),origin.Z()};body.summary["pathEndMm"]=QJsonArray{finish.X(),finish.Y(),finish.Z()};return body;
}
