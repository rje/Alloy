// SPDX-License-Identifier: MIT
#include "Measure.h"
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <Standard_Failure.hxx>
#include <QSet>
#include <cmath>
#include <algorithm>
namespace {
QJsonObject refused(const QString& code,const QString& message){return {{"ok",false},{"error",QJsonObject{{"code",code},{"message",message}}}};}
}
QJsonObject measureGeometry(const EvaluatedDocument& evaluation,const std::vector<EvaluatedBody>& bodies,
    const QJsonArray& sources,const QString& units) {
    if(sources.isEmpty() || sources.size()>2)return refused("measure_selection_count","Select one or two supported objects");
    struct Resolved {TopoDS_Shape shape;TopoDS_Edge edge;TopoDS_Face face;bool point=false;};
    std::vector<Resolved> resolved;
    QSet<QStringList> identities;
    try {
        for(const auto& item:sources){const auto source=item.toObject();const auto kind=source.value("kind").toString();Resolved result;int matches=0;QStringList identity;
            if(kind=="sketchEdge" || kind=="sketchPoint"){
                identity={kind,source.value("sketchId").toString(),source.value(kind=="sketchEdge"?"entityId":"pointId").toString()};
                for(const auto& sketch:evaluation.sketches)if(sketch.visible && sketch.featureId==source.value("sketchId").toString()){
                    if(kind=="sketchEdge" && sketch.entityEdges.contains(source.value("entityId").toString())){result.edge=sketch.entityEdges.value(source.value("entityId").toString());result.shape=result.edge;++matches;}
                    if(kind=="sketchPoint")for(const auto& entry:sketch.summary.value("standalonePoints").toArray()){
                        const auto point=entry.toObject();if(point.value("pointId")!=source.value("pointId"))continue;
                        const auto p=point.value("positionMm").toArray();if(p.size()!=3)continue;
                        result.shape=BRepBuilderAPI_MakeVertex(gp_Pnt(p[0].toDouble(),p[1].toDouble(),p[2].toDouble()));result.point=true;++matches;
                    }
                }
            }else if(kind=="edge" || kind=="face"){
                for(const auto& body:bodies)if(body.visible && body.bodyId==source.value("bodyId").toString()
                    && (!source.contains("occurrenceId") || body.occurrenceId==source.value("occurrenceId").toString())){
                    if(kind=="edge")for(const auto& edge:body.solidEdges)if(edge.edgeId==source.value("edgeId").toString()){identity={kind,body.bodyId,body.occurrenceId,edge.edgeId};result.edge=edge.edge;result.shape=edge.edge;++matches;}
                    if(kind=="face")for(const auto& face:body.supportFaces)if(face.faceId==source.value("faceId").toString()
                        && (!source.contains("producerFeatureId") || face.producerFeatureId==source.value("producerFeatureId").toString())){identity={kind,body.bodyId,body.occurrenceId,face.faceId};result.face=face.face;result.shape=face.face;++matches;}
                }
            }else return refused("measure_unsupported","Unsupported measurement source kind");
            if(matches!=1 || result.shape.IsNull())return refused(matches>1?"measure_ambiguous":"measure_unavailable","Measurement source is unavailable or ambiguous");
            if(identities.contains(identity))return refused("measure_duplicate","Choose two distinct sources");
            identities.insert(identity);resolved.push_back(result);
        }
        const double scale=units=="in"?25.4:1.;QJsonArray values;
        const auto add=[&](const QString& name,double canonical,int dimension){const auto unit=dimension==0?QString("deg"):units+(dimension==2?QString::fromUtf8("²"):QString{});
            values.append(QJsonObject{{"name",name},{"value",canonical/(dimension==0?1:std::pow(scale,dimension))},{"unit",unit},{"canonicalValue",canonical},{"canonicalUnit",dimension==0?"deg":dimension==2?"mm²":"mm"}});};
        for(const auto& r:resolved)if(!r.edge.IsNull() && BRepAdaptor_Curve(r.edge).GetType()!=GeomAbs_Line && sources.size()==2)
            return refused("measure_unsupported","Pair measurements currently require straight edges");
        if(resolved.size()==1){const auto& r=resolved.front();GProp_GProps props;
            if(!r.edge.IsNull()){BRepGProp::LinearProperties(r.edge,props);add("Length",props.Mass(),1);}
            else if(!r.face.IsNull()){
                const auto type=BRepAdaptor_Surface(r.face).GetType();
                if(type!=GeomAbs_Plane && type!=GeomAbs_Cylinder && type!=GeomAbs_Torus)return refused("measure_unsupported","Face area currently supports planar, cylindrical and toroidal faces");
                BRepGProp::SurfaceProperties(r.face,props);add("Area",props.Mass(),2);
            }else return refused("measure_unsupported","Select a straight edge as the second object to measure point distance");
        }else{
            const auto& a=resolved[0];const auto& b=resolved[1];
            if(!((a.point && !b.edge.IsNull()) || (b.point && !a.edge.IsNull()) || (!a.edge.IsNull() && !b.edge.IsNull())))
                return refused("measure_unsupported","Supported pairs are point/straight edge and two straight edges");
            BRepExtrema_DistShapeShape distance(a.shape,b.shape);if(!distance.IsDone())return refused("measure_failed","Exact distance evaluation failed");
            add("Distance",distance.Value(),1);
            if(!a.edge.IsNull() && !b.edge.IsNull()){
                const auto da=BRepAdaptor_Curve(a.edge).Line().Direction(),db=BRepAdaptor_Curve(b.edge).Line().Direction();
                const double angle=std::acos(std::clamp(std::abs(da.Dot(db)),0.,1.))*180/std::acos(-1.);
                if(std::abs(angle-90)>1e-7)return refused("measure_unsupported","Only the unambiguous right-angle line-pair convention is qualified");
                add("Angle",angle,0);
            }
        }
        return {{"sources",sources},{"measurements",values},{"units",units},{"authoredChange",false},{"distancePolicy","shortest_on_finite_geometry"}};
    }catch(const Standard_Failure& e){return refused("measure_failed",QString::fromUtf8(e.GetMessageString()));}
}
