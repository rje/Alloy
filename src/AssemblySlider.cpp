#include "AssemblySlider.h"
#include "PlanarFaces.h"
#include "SolidFillet.h"
#include <QUuid>
#include <QSet>
#include <BRepAdaptor_Curve.hxx>
#include <GeomAbs_CurveType.hxx>
#include <TopLoc_Location.hxx>
#include <TopoDS.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>
#include <cmath>
#include <stdexcept>

namespace {
void require(bool condition,const char* message){if(!condition)throw std::invalid_argument(message);}
QJsonArray translated(const QJsonArray& coordinates,double y){
    if(coordinates.size()!=3)return coordinates;
    auto result=coordinates;result[1]=coordinates[1].toDouble()+y;return result;
}
QJsonObject movedFrame(QJsonObject frame,double y){if(frame.contains("origin"))frame["origin"]=translated(frame["origin"].toArray(),y);return frame;}
QJsonObject boundsOf(const EvaluatedBody& body,double y){
    auto bounds=body.summary["boundsMm"].toObject();bounds["min"]=translated(bounds["min"].toArray(),y);
    bounds["max"]=translated(bounds["max"].toArray(),y);return bounds;
}
QString definitionFor(const QJsonObject& authored,const QString& occurrenceId){
    for(const auto& item:authored.value("occurrences").toArray()){
        const auto occurrence=item.toObject();if(occurrence["id"]==occurrenceId)return occurrence["definitionId"].toString();
    }
    return {};
}
QJsonObject joint(const QJsonObject& authored){const auto rows=authored.value("joints").toArray();return rows.isEmpty() ? QJsonObject{} : rows[0].toObject();}
void checkPosition(const QJsonObject& record,double position){
    require(std::isfinite(position) && std::abs(position)<=100000,"Slider position must be a finite length within 100000 mm");
    if(record["minimumEnabled"].toBool())require(position>=record["minimumMm"].toDouble()-1e-9,"Slider position is below the minimum limit");
    if(record["maximumEnabled"].toBool())require(position<=record["maximumMm"].toDouble()+1e-9,"Slider position is above the maximum limit");
}
QJsonObject origin(const QJsonObject& record,const QJsonObject& authored,const EvaluatedDocument& definitions){
    const auto fixed=definitionFor(authored,record["component2OccurrenceId"].toString());
    require(!fixed.isEmpty(),"Slider reference occurrence is unavailable");
    const auto position=record["position"].toObject();
    for(const auto& body:definitions.bodies)if(body.bodyId==position["bodyId"].toString()){
        require(body.ownerId==fixed,"Slider Position edge must belong to Component 2");
        for(const auto& edge:body.solidEdges)if(edge.edgeId==position["edgeId"].toString()){
            require(!edge.edge.IsNull(),"Slider Position edge is unavailable");
            const BRepAdaptor_Curve curve(edge.edge);
            require(curve.GetType()==GeomAbs_Line,"Slider Position requires a straight retained ledge edge");
            const auto point=curve.Value((curve.FirstParameter()+curve.LastParameter())*0.5);
            return {{"bodyId",body.bodyId},{"edgeId",edge.edgeId},{"pointMm",QJsonArray{point.X(),point.Y(),point.Z()}}};
        }
    }
    throw std::invalid_argument("Slider Position edge is unavailable");
}
}
QJsonObject AssemblySlider::candidate(const QJsonObject& authored,const QJsonObject& arguments){
    const auto existing=joint(authored);const auto edit=arguments.value("jointId").toString();
    require(edit.isEmpty() ? existing.isEmpty() : existing["id"]==edit,"This slice supports one available as-built Slider joint");
    auto record=existing.isEmpty() ? QJsonObject{{"id",QUuid::createUuid().toString(QUuid::WithoutBraces)},{"type","asBuiltSlider"},{"name","Slider"},
        {"axis","Y"},{"reverseDirection",false},{"minimumEnabled",false},{"minimumMm",0},{"maximumEnabled",false},{"maximumMm",0},{"positionMm",0}} : existing;
    const QSet<QString> allowed{"jointId","name","component1OccurrenceId","component2OccurrenceId","position","axis","reverseDirection","minimumEnabled","minimumMm","maximumEnabled","maximumMm","positionMm"};
    for(auto it=arguments.begin();it!=arguments.end();++it){require(allowed.contains(it.key()),"Unknown Slider argument");if(it.key()!="jointId")record[it.key()]=it.value();}
    if(!record.contains("reverseDirection"))record["reverseDirection"]=false;
    auto result=authored;result["joints"]=QJsonArray{record};return result;
}
double AssemblySlider::position(const QJsonObject& authored,std::optional<double> transientPositionMm){
    const auto record=joint(authored);if(record.isEmpty())return 0;
    const double coordinate=transientPositionMm.value_or(record["positionMm"].toDouble());checkPosition(record,coordinate);return coordinate;
}
QJsonObject AssemblySlider::inspect(const QJsonObject& authored,const EvaluatedDocument& definitions,std::optional<double> transientPositionMm){
    const auto record=joint(authored);if(record.isEmpty())return {{"joints",QJsonArray{}},{"occurrences",authored.value("occurrences").toArray()}};
    require(record["component1OccurrenceId"]!=record["component2OccurrenceId"],"Slider requires two distinct occurrences");
    const auto moving=definitionFor(authored,record["component1OccurrenceId"].toString());
    const auto fixed=definitionFor(authored,record["component2OccurrenceId"].toString());
    require(!moving.isEmpty() && !fixed.isEmpty(),"Slider occurrences are unavailable");
    const auto point=origin(record,authored,definitions);
    const auto coordinate=position(authored,transientPositionMm);
    const auto displacement=record["reverseDirection"].toBool() ? -coordinate : coordinate;
    QJsonArray world;
    for(const auto& item:authored.value("occurrences").toArray()){
        const auto occurrence=item.toObject();const double y=occurrence["id"]==record["component1OccurrenceId"] ? displacement : 0;
        QJsonArray bodies;for(const auto& body:definitions.bodies)if(body.ownerId==occurrence["definitionId"].toString())
            bodies.append(QJsonObject{{"bodyId",body.bodyId},{"boundsMm",boundsOf(body,y)}});
        world.append(QJsonObject{{"occurrenceId",occurrence["id"]},{"definitionId",occurrence["definitionId"]},
            {"translationMm",QJsonArray{0,y,0}},{"bodies",bodies}});
    }
    auto resolved=record;resolved["origin"]=point;resolved["coordinateMm"]=coordinate;
    resolved["reverseDirection"]=record["reverseDirection"].toBool();
    resolved["relativeDegreesOfFreedom"]=1;resolved["gaugeOccurrenceId"]=record["component2OccurrenceId"];
    resolved["transient"]=transientPositionMm.has_value();
    return {{"joints",QJsonArray{resolved}},{"occurrences",world}};
}
std::vector<EvaluatedBody> AssemblySlider::worldBodies(const QJsonObject& authored,const EvaluatedDocument& definitions,std::optional<double> transientPositionMm){
    auto bodies=definitions.bodies;const auto record=joint(authored);if(record.isEmpty())return bodies;
    const auto moving=definitionFor(authored,record["component1OccurrenceId"].toString());
    const double coordinate=position(authored,transientPositionMm);
    const double y=record["reverseDirection"].toBool() ? -coordinate : coordinate;
    if(y==0)return bodies;
    gp_Trsf shift;shift.SetTranslation(gp_Vec(0,y,0));const TopLoc_Location placement(shift);
    for(auto& body:bodies)if(body.ownerId==moving){
        body.shape=body.shape.Moved(placement);body.summary["boundsMm"]=boundsOf(body,y);
        for(auto& face:body.supportFaces){face.face=TopoDS::Face(face.face.Moved(placement));face.frame=movedFrame(face.frame,y);
            for(auto& edge:face.boundaryEdges){edge.edge=TopoDS::Edge(edge.edge.Moved(placement));
                for(const auto& key:{"startMm","endMm"})if(edge.provenance.contains(key))edge.provenance[key]=translated(edge.provenance[key].toArray(),y);
            }
        }
        for(auto& edge:body.solidEdges){edge.edge=TopoDS::Edge(edge.edge.Moved(placement));
            for(const auto& key:{"startMm","endMm"})if(edge.provenance.contains(key))edge.provenance[key]=translated(edge.provenance[key].toArray(),y);
        }
        if(body.summary.contains("supportFaces"))body.summary["supportFaces"]=PlanarFaces::descriptors(body.supportFaces);
        if(body.summary.contains("solidEdges"))body.summary["solidEdges"]=SolidFillet::descriptors(body.solidEdges);
    }
    return bodies;
}
