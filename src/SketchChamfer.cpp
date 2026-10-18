// SPDX-License-Identifier: MIT
#include "SketchChamfer.h"
#include "SketchGeometry.h"
#include <QJsonArray>
#include <QMap>
#include <QPointF>
#include <QUuid>
#include <cmath>
#include <stdexcept>
namespace {
void require(bool value,const char* message){if(!value)throw std::invalid_argument(message);}
QString fresh(){return QUuid::createUuid().toString(QUuid::WithoutBraces);}
struct Corner {QJsonObject lines[2];QString keys[2];QPointF vertex,directions[2];double lengths[2];};
Corner corner(const QJsonObject& p,const QJsonArray& ids){
    require(ids.size()==2 && ids[0]!=ids[1],"Chamfer requires two distinct source lines");
    QMap<QString,QPointF> points;QMap<QString,QString> roots;
    for(const auto& row:p.value("points").toArray()){const auto point=row.toObject();const auto id=point.value("id").toString();points[id]={point.value("uMm").toDouble(),point.value("vMm").toDouble()};roots[id]=id;}
    const auto root=[&](QString id){require(roots.contains(id),"Chamfer source endpoint is unavailable");while(roots[id]!=id)id=roots[id];return id;};
    for(const auto& row:p.value("constraints").toArray()){const auto c=row.toObject();if(c.value("type")!="coincident")continue;const auto refs=c.value("pointIds").toArray();roots[root(refs[1].toString())]=root(refs[0].toString());}
    Corner result;
    for(const auto& row:p.value("entities").toArray())for(int j=0;j<2;++j)if(row.toObject().value("id")==ids[j])result.lines[j]=row.toObject();
    for(const auto& line:result.lines)require(line.value("type")=="line" && !line.value("construction").toBool(),"Chamfer supports normal authored straight lines only");
    int matches=0;
    for(const auto& a:{QString("startPointId"),QString("endPointId")})for(const auto& b:{QString("startPointId"),QString("endPointId")})if(root(result.lines[0].value(a).toString())==root(result.lines[1].value(b).toString())){result.keys[0]=a;result.keys[1]=b;++matches;}
    require(matches==1,"Chamfer requires one unambiguous connected corner");
    const auto vertexRoot=root(result.lines[0].value(result.keys[0]).toString());result.vertex=points[result.lines[0].value(result.keys[0]).toString()];
    for(const auto& row:p.value("entities").toArray()){
        const auto e=row.toObject();if(ids.contains(e.value("id")) || e.value("construction").toBool())continue;
        for(const auto& key:{"startPointId","endPointId","centerPointId","pointId"})if(e.contains(key))require(root(e.value(key).toString())!=vertexRoot,"Chamfer corner has unrelated incident geometry");
    }
    for(int j=0;j<2;++j){const auto other=result.keys[j]=="startPointId" ? "endPointId" : "startPointId";result.directions[j]=points[result.lines[j].value(other).toString()]-result.vertex;result.lengths[j]=std::hypot(result.directions[j].x(),result.directions[j].y());require(result.lengths[j]>0.001,"Chamfer source line is too short");result.directions[j]/=result.lengths[j];}
    const auto cross=result.directions[0].x()*result.directions[1].y()-result.directions[0].y()*result.directions[1].x();require(std::abs(cross)>1e-6,"Chamfer corner lines must not be collinear");
    return result;
}
}
QString SketchChamfer::validate(const QJsonObject& p,QSet<QString>& ids){
    try{
        require(p.value("chamferGroups").isArray() && !p.value("chamferGroups").toArray().isEmpty(),"Chamfer requires retained corners");
        for(const auto& row:p.value("chamferGroups").toArray()){
            require(row.isObject(),"Invalid retained Chamfer group");
            const auto g=row.toObject();
            const bool two=g.value("type")=="twoDistance";
            require(g.size()==(two?7:6) && (two || g.value("type")=="equalDistance") && g.value("sourceEntityIds").isArray() && g.value("sourceEntityIds").toArray().size()==2 && g.value("pointIds").isArray() && g.value("pointIds").toArray().size()==2,"Invalid retained Chamfer");
            require(g.value("distanceMm").isDouble() && std::isfinite(g.value("distanceMm").toDouble()) && g.value("distanceMm").toDouble()>=0.001 && g.value("distanceMm").toDouble()<=100000,"Chamfer distance must be 0.001..100000 mm");
            if(two)require(g.value("distance2Mm").isDouble() && std::isfinite(g.value("distance2Mm").toDouble()) && g.value("distance2Mm").toDouble()>=0.001 && g.value("distance2Mm").toDouble()<=100000,"Chamfer second distance must be 0.001..100000 mm");
            QJsonArray owned{g.value("id"),g.value("lineId")};for(const auto& id:g.value("pointIds").toArray())owned.append(id);
            for(const auto& id:owned){require(id.isString() && !QUuid(id.toString()).isNull() && !ids.contains(id.toString()),"Invalid or duplicate Chamfer generated UUID");ids.insert(id.toString());}
            for(const auto& id:g.value("sourceEntityIds").toArray())require(id.isString() && !QUuid(id.toString()).isNull(),"Chamfer requires stable source line UUIDs");
            corner(p,g.value("sourceEntityIds").toArray());
        }
        return {};
    }catch(const std::exception& e){return QString::fromUtf8(e.what());}
}
QJsonObject SketchChamfer::derive(const QJsonObject& p){
    if(!p.contains("chamferGroups"))return p;
    auto result=p;auto entities=p.value("entities").toArray(),points=p.value("points").toArray();
    // Resolve every corner against the unchanged authored lines. Adjacent cuts
    // own opposite endpoints of their shared line, never each other's trim.
    QMap<QString,QMap<QString,double>> setbacks;
    for(const auto& row:p.value("chamferGroups").toArray()){
        const auto g=row.toObject();const auto c=corner(p,g.value("sourceEntityIds").toArray());
        for(int j=0;j<2;++j){
            const double distance=g.value(j==1 && g.value("type")=="twoDistance"?"distance2Mm":"distanceMm").toDouble();
            const auto sourceId=c.lines[j].value("id").toString();
            auto& ends=setbacks[sourceId];
            require(!ends.contains(c.keys[j]),"Chamfer corner already has a retained cut");
            ends[c.keys[j]]=distance;
            require(distance>=0.001 && ends.value("startPointId")+ends.value("endPointId")<c.lengths[j]-0.001,
                    "Chamfer setbacks overlap or exceed available source line length");
            const auto at=c.vertex+c.directions[j]*distance;
            const QJsonValue id=g.value("pointIds").toArray().at(j);
            points.append(QJsonObject{{"id",id},{"uMm",at.x()},{"vMm",at.y()}});
            for(int i=0;i<entities.size();++i)if(entities[i].toObject().value("id")==c.lines[j].value("id")){
                auto line=entities[i].toObject();line[c.keys[j]]=id;entities[i]=line;break;
            }
        }
        entities.append(QJsonObject{{"id",g.value("lineId")},{"type","line"},{"startPointId",g.value("pointIds").toArray()[0]},{"endPointId",g.value("pointIds").toArray()[1]}});
    }
    result["points"]=points;result["entities"]=entities;return result;
}
QJsonObject SketchGeometry::chamferPresentation(const QJsonObject& feature){auto result=feature;result["parameters"]=SketchChamfer::derive(feature.value("parameters").toObject());return result;}
SketchGeometry::SolveResult SketchGeometry::chamferCorner(const QJsonObject& draft,const QJsonObject& args,const QJsonObject& context){
    try{
        for(auto it=args.begin();it!=args.end();++it)require(QSet<QString>{"entityIds","distanceMm","distance2Mm","chamferId","type"}.contains(it.key()),"Unknown Chamfer argument");
        auto p=draft.value("parameters").toObject();auto groups=p.value("chamferGroups").toArray();QJsonObject g;int groupIndex=-1;
        if(args.contains("chamferId")){
            for(int i=0;i<groups.size();++i)if(groups[i].toObject().value("id")==args.value("chamferId"))groupIndex=i;
            require(groupIndex>=0,"Retained Chamfer is unavailable");g=groups[groupIndex].toObject();
            require(!args.contains("entityIds"),"Chamfer source identities cannot change while editing");
        } else {
            require(args.value("entityIds").isArray(),"Chamfer needs two source line IDs");
            g={{"id",fresh()},{"type","equalDistance"},{"sourceEntityIds",args.value("entityIds")},{"lineId",fresh()},{"pointIds",QJsonArray{fresh(),fresh()}}};
        }
        // Alloy side-order policy: sourceEntityIds[0] owns Distance 1 and
        // sourceEntityIds[1] owns Distance 2. Editing never reorders sources.
        // API/two-line input preserves pick order; vertex picks use authored order.
        const auto type=args.value("type").toString(g.value("type").toString());
        require(type=="equalDistance" || type=="twoDistance","Unsupported Chamfer mode");
        g["type"]=type;
        if(args.contains("distanceMm")){require(args.value("distanceMm").isDouble(),"Chamfer needs a positive first distance");g["distanceMm"]=args.value("distanceMm");}
        require(g.value("distanceMm").isDouble(),"Chamfer needs a positive first distance");
        if(type=="twoDistance"){
            if(args.contains("distance2Mm")){require(args.value("distance2Mm").isDouble(),"Chamfer needs a positive second distance");g["distance2Mm"]=args.value("distance2Mm");}
            else if(!g.contains("distance2Mm"))g["distance2Mm"]=g.value("distanceMm");
        } else {
            require(!args.contains("distance2Mm"),"Equal Distance has no independent second setback");g.remove("distance2Mm");
        }
        if(groupIndex>=0)groups[groupIndex]=g;else groups.append(g);
        p["chamferGroups"]=groups;auto candidate=draft;candidate["parameters"]=p;
        auto result=solveDraft(candidate,context);if(!result.ok)return {false,draft,result.diagnostics,result.error};result.diagnostics["chamferId"]=g.value("id");result.diagnostics["createdLineId"]=g.value("lineId");result.diagnostics["retainedSourceEntityIds"]=g.value("sourceEntityIds");result.diagnostics["retainedConstraintCount"]=p.value("constraints").toArray().size();result.diagnostics["retainedDimensionCount"]=p.value("dimensions").toArray().size();return result;
    }catch(const std::exception& e){return {false,draft,{{"status","invalid"}},QString::fromUtf8(e.what())};}
}
