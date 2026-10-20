// SPDX-License-Identifier: MIT
#include "RetainedSweepPath.h"
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <TopoDS.hxx>
#include <TopExp_Explorer.hxx>
#include <QSet>
#include <limits>
#include <cmath>
#include <algorithm>

RetainedSweepPath retainedSweepPath(const QJsonObject& p,const EvaluatedSketch& sketch,const QJsonObject& pathSource,const std::function<void(const QString&,const QString&,QJsonObject)>& reject) {
    QMap<QString,gp_Pnt> positions;for(const auto& q:sketch.summary["worldPoints"].toArray()){auto x=q.toObject();const auto a=x["positionMm"].toArray();positions[x["id"].toString()]=gp_Pnt(a[0].toDouble(),a[1].toDouble(),a[2].toDouble());}
    QMap<QString,QJsonObject> entities;for(const auto& q:sketch.summary["entities"].toArray())entities[q.toObject()["id"].toString()]=q.toObject();
    QMap<QString,QString> roots;for(auto i=positions.begin();i!=positions.end();++i)roots[i.key()]=i.key();
    auto merge=[&](const QString& a,const QString& b){const auto keep=std::min(roots.value(a),roots.value(b)),remove=std::max(roots.value(a),roots.value(b));if(keep.isEmpty())return;for(auto i=roots.begin();i!=roots.end();++i)if(i.value()==remove)i.value()=keep;};
    for(const auto& q:pathSource["parameters"].toObject()["constraints"].toArray()){auto c=q.toObject();auto ids=c["pointIds"].toArray();if(c["type"]=="coincident" && ids.size()==2)merge(ids[0].toString(),ids[1].toString());}
    const auto ids=p["pathEntityIds"].toArray();QSet<QString> endpoints;
    for(const auto& q:ids){const auto id=q.toString();const auto e=entities.value(id);if(e.isEmpty() || !sketch.entityEdges.contains(id))reject("path_entity_unavailable","A retained path entity is unavailable",{{"entityId",id}});if((e["type"]!="line" && e["type"]!="arc") || e["construction"].toBool())reject("unsupported_path_entity","Sweep/Pipe path supports ordinary analytic lines and arcs",{{"entityId",id}});endpoints.insert(e["startPointId"].toString());endpoints.insert(e["endPointId"].toString());}
    // Exact-coordinate distinct endpoints remain valid. Only floating-point
    // roundoff is ignored; this is not a sketch closure/healing tolerance.
    const auto endpointList=endpoints.values();for(int i=0;i<endpointList.size();++i)for(int j=i+1;j<endpointList.size();++j){const auto a=positions.value(endpointList[i]),b=positions.value(endpointList[j]);const double scale=std::max({1.,std::abs(a.X()),std::abs(a.Y()),std::abs(a.Z()),std::abs(b.X()),std::abs(b.Y()),std::abs(b.Z())});if(a.Distance(b)<=64*std::numeric_limits<double>::epsilon()*scale)merge(endpointList[i],endpointList[j]);}
    QMap<QString,int> degree;for(const auto& q:ids){const auto e=entities[q.toString()];++degree[roots.value(e["startPointId"].toString())];++degree[roots.value(e["endPointId"].toString())];}
    int ends=0;for(auto i=degree.begin();i!=degree.end();++i){if(i.value()>2)reject("branched_path","Select one unbranched Sweep/Pipe path",{{"pointId",i.key()}});if(i.value()==1)++ends;}
    const auto start=p["pathStartPointId"].toString();if(ends!=2)reject("disconnected_path","Sweep/Pipe requires one open chain with exactly two endpoints; gaps and disconnected pieces are not healed",{{"endpointCount",ends},{"closureApplied",false}});if(!positions.contains(start) || !endpoints.contains(start) || degree.value(roots.value(start))!=1)reject("open_path_required","Sweep/Pipe start must be a retained endpoint of the selected open path",{});
    QString current=start;BRepBuilderAPI_MakeWire wire;gp_Vec initialTangent,lastTangent;gp_Pnt finish;double length=0.;QJsonArray oriented;
    for(const auto& q:ids){const auto id=q.toString();const auto e=entities[id];const auto a=e["startPointId"].toString(),b=e["endPointId"].toString();bool reverse=false;
        if(roots.value(current)==roots.value(a))reverse=false;else if(roots.value(current)==roots.value(b))reverse=true;else reject("disconnected_path","Ordered Sweep/Pipe entities do not continue from the retained start",{{"entityId",id},{"pointId",current},{"distanceToStartMm",positions[current].Distance(positions[a])},{"distanceToEndMm",positions[current].Distance(positions[b])},{"closureApplied",false}});
        auto edge=TopoDS::Edge(BRepBuilderAPI_Copy(sketch.entityEdges[id],false,false).Shape());BRepAdaptor_Curve curve(edge);gp_Pnt first,last;gp_Vec firstTangent,endTangent;
        const bool backwards=(edge.Orientation()==TopAbs_REVERSED)!=reverse;curve.D1(backwards ? curve.LastParameter():curve.FirstParameter(),first,firstTangent);curve.D1(backwards ? curve.FirstParameter():curve.LastParameter(),last,endTangent);if(backwards){firstTangent.Reverse();endTangent.Reverse();}if(reverse)edge.Reverse();
        if(firstTangent.Magnitude()<1e-12 || endTangent.Magnitude()<1e-12)reject("degenerate_path","Sweep/Pipe path has a degenerate endpoint tangent",{{"entityId",id}});
        firstTangent.Normalize();endTangent.Normalize();if(oriented.isEmpty())initialTangent=firstTangent;else if(lastTangent.Dot(firstTangent)<1.-1e-10)reject("non_g1_path","Sweep/Pipe path junction must be tangent-continuous",{{"entityId",id},{"tangentDot",lastTangent.Dot(firstTangent)}});
        wire.Add(edge);if(!wire.IsDone())reject("path_wire_failed","OCCT could not connect the retained path; correct its endpoint relations",{{"entityId",id}});
        GProp_GProps measure;BRepGProp::LinearProperties(edge,measure);length+=measure.Mass();lastTangent=endTangent;finish=last;current=reverse?a:b;oriented.append(QJsonObject{{"entityId",id},{"reversed",reverse}});
    }
    if(degree.value(roots.value(current))!=1 || roots.value(current)==roots.value(start))reject("disconnected_path","Ordered Sweep/Pipe path must traverse all selected entities exactly once",{});
    return {wire.Wire(),positions[start],finish,initialTangent,lastTangent,length,oriented};
}
