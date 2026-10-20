// SPDX-License-Identifier: MIT
#include "SketchScale.h"
#include "SketchSolver.h"
#include <QJsonArray>
#include <QMap>
#include <QPointF>
#include <QUuid>
#include <cmath>
#include <stdexcept>
namespace {
void need(bool condition, const QString& message) {
    if (!condition) throw std::invalid_argument(message.toStdString());
}
QSet<QString> ids(const QJsonArray& values) {
    QSet<QString> result;
    for (const auto& value : values) {
        need(value.isString() && !QUuid(value.toString()).isNull() && !result.contains(value.toString()),
             "Scale requires distinct UUIDs");
        result.insert(value.toString());
    }
    return result;
}
QMap<QString,QJsonObject> rows(const QJsonObject& p, const char* key) {
    QMap<QString,QJsonObject> result;
    for (const auto& value : p[key].toArray()) result[value.toObject()["id"].toString()] = value.toObject();
    return result;
}
bool mentions(const QJsonValue& value, const QSet<QString>& targets) {
    if (value.isString()) return targets.contains(value.toString());
    if (value.isArray()) for (const auto& v : value.toArray()) if (mentions(v,targets)) return true;
    if (value.isObject()) for (const auto& v : value.toObject()) if (mentions(v,targets)) return true;
    return false;
}
void coordinate(const QJsonObject& p, const char* key) {
    need(p[key].isDouble() && std::isfinite(p[key].toDouble()) && std::abs(p[key].toDouble()) <= 100000,
         QString("Scale %1 must be finite within ±100000 mm").arg(key));
}
void factor(double value) {
    need(std::isfinite(value) && value >= 1e-6 && value <= 100000,
         "Scale Factor must be positive, within 0.000001..100000");
}
QJsonObject pivot(const QJsonObject& p, const QJsonObject& group) {
    const auto id = group["pivotPointId"].toString();
    const auto point = rows(p,"points").value(id);
    need(!point.isEmpty() && id != p["originPointId"].toString(), "Select a separate authored Scale Point");
    coordinate(point,"uMm"); coordinate(point,"vMm");
    int count = 0;
    for (const auto& v : p["entities"].toArray()) {
        const auto e = v.toObject();
        if (!mentions(e,QSet<QString>{id})) continue;
        need(e["type"] == "point" && e["pointId"] == id && !e["construction"].toBool(),
             "Scale Point must be standalone ordinary authored geometry");
        ++count;
    }
    need(count == 1, "Scale Point must be a separate standalone Point");
    for (auto it = p.begin(); it != p.end(); ++it)
        if (!QStringList{"scaleGroups","points","entities"}.contains(it.key()))
            need(!mentions(it.value(),QSet<QString>{id}), "Scale Point has a relation or another owner");
    return point;
}
QJsonObject output(const QJsonObject& point, const QJsonObject& center, double s) {
    return {{"uMm",center["uMm"].toDouble()+s*(point["uMm"].toDouble()-center["uMm"].toDouble())},
            {"vMm",center["vMm"].toDouble()+s*(point["vMm"].toDouble()-center["vMm"].toDouble())}};
}
}
namespace SketchScale {
QSet<QString> owned(const QJsonObject& p, bool points) {
    QSet<QString> result;
    for (const auto& v : p["scaleGroups"].toArray()) {
        const auto g = v.toObject();
        if (points) for (const auto& s : g["sourcePoints"].toArray()) result.insert(s.toObject()["pointId"].toString());
        else for (const auto& id : g["entityIds"].toArray()) result.insert(id.toString());
    }
    return result;
}
bool touches(const QJsonObject& p, const QJsonValue& value) {
    auto targets = owned(p) + owned(p,true);
    for (const auto& v : p["scaleGroups"].toArray()) targets.insert(v.toObject()["pivotPointId"].toString());
    return mentions(value,targets);
}
QString validate(const QJsonObject& p) {
    try {
        if (!p.contains("scaleGroups")) return {};
        need(p["scaleGroups"].isArray() && p["scaleGroups"].toArray().size() <= 1 && p["plane"] == "XY",
             "Scale supports one retained XY four-Line profile");
        const auto es = rows(p,"entities"), ps = rows(p,"points");
        for (const auto& value : p["scaleGroups"].toArray()) {
            const auto g = value.toObject();
            need(g.size() == 9 && !QUuid(g["id"].toString()).isNull() && g["type"] == "scale", "Invalid retained Scale definition");
            need(g["factor"].isDouble(), "Scale Factor must be numeric"); factor(g["factor"].toDouble());
            for (const auto* key : {"entityIds","sourcePoints","topology","constraints","dimensions"})
                need(g[key].isArray(), "Invalid Scale membership arrays");
            need(g["dimensions"].toArray().isEmpty(), "Scale dimensions are unsupported");
            const auto selected = ids(g["entityIds"].toArray());
            need(selected.size() == 4 && g["sourcePoints"].toArray().size() == 4 && g["topology"].toArray().size() == 4,
                 "Scale owns four Lines and four vertices");
            const auto center = pivot(p,g);
            QSet<QString> vertices, topology;
            QMap<QString,QPointF> source;
            QMap<QString,QStringList> neighbors;
            for (const auto& v : g["sourcePoints"].toArray()) {
                const auto s = v.toObject(); const auto id = s["pointId"].toString();
                need(s.size() == 3 && ps.contains(id) && !vertices.contains(id) && id != p["originPointId"] && id != g["pivotPointId"],
                     "Invalid Scale source identity or pivot membership");
                coordinate(s,"uMm"); coordinate(s,"vMm");
                const auto expected = output(s,center,g["factor"].toDouble()), actual = ps.value(id);
                coordinate(expected,"uMm"); coordinate(expected,"vMm");
                for (const auto* key : {"uMm","vMm"})
                    need(std::abs(actual[key].toDouble()-expected[key].toDouble()) <= 1e-9, "Scale output cache changed outside its command");
                vertices.insert(id); source[id] = {s["uMm"].toDouble(),s["vMm"].toDouble()};
            }
            for (const auto& v : g["topology"].toArray()) {
                const auto t = v.toObject(), e = es.value(t["entityId"].toString());
                const auto id = t["entityId"].toString(), a = t["startPointId"].toString(), b = t["endPointId"].toString();
                need(t.size() == 3 && selected.contains(id) && !topology.contains(id) && e["type"] == "line"
                     && !e["construction"].toBool() && !e["centerline"].toBool() && e["startPointId"] == t["startPointId"]
                     && e["endPointId"] == t["endPointId"] && vertices.contains(a) && vertices.contains(b) && a != b,
                     "Scale topology changed or unsupported");
                need(QPointF::dotProduct(source[a]-source[b],source[a]-source[b]) >= 1e-12, "Scale source edge is collapsed");
                topology.insert(id); neighbors[a].append(b); neighbors[b].append(a);
            }
            QSet<QString> visited; QStringList ordered; QString cursor = *vertices.begin(), previous;
            for (int n=0; n<4; ++n) {
                need(neighbors[cursor].size() == 2 && !visited.contains(cursor), "Scale requires one connected closed four-Line loop");
                visited.insert(cursor); ordered.append(cursor);
                const auto next = neighbors[cursor][0] == previous ? neighbors[cursor][1] : neighbors[cursor][0]; previous=cursor; cursor=next;
            }
            need(cursor == *vertices.begin() && visited == vertices, "Scale profile is disconnected");
            const auto cross=[](const QPointF& a,const QPointF& b){return a.x()*b.y()-a.y()*b.x();};
            double area=0; QPointF xy[4]; for(int n=0;n<4;++n)xy[n]=source[ordered[n]];
            for(int n=0;n<4;++n)area+=cross(xy[n],xy[(n+1)%4]);
            need(std::abs(area)>1e-6,"Scale requires a nondegenerate profile");
            for(int n=0;n<2;++n){const auto a=xy[n],b=xy[n+1],c=xy[n+2],d=xy[(n+3)%4];need(!(cross(b-a,c-a)*cross(b-a,d-a)<=0 && cross(d-c,a-c)*cross(d-c,b-c)<=0),"Scale requires a simple noncrossing profile");}
            const auto targets=selected+vertices;
            for(const auto& e:es)if(!selected.contains(e["id"].toString()))need(!mentions(e,vertices),"Scale shares an endpoint with unselected geometry");
            for(auto it=p.begin();it!=p.end();++it)if(!QStringList{"scaleGroups","points","entities","constraints","dimensions","constructionLines3d"}.contains(it.key()))
                need(!mentions(it.value(),targets),"Scale geometry has another owner or external reference");
            QJsonArray constraints;
            for(const auto& v:p["constraints"].toArray()){
                const auto c=v.toObject();if(!mentions(c,targets))continue;
                need(QStringList{"coincident","horizontal","vertical","parallel","perpendicular","equal"}.contains(c["type"].toString()),"Scale refuses fixed or non-invariant relation "+c["id"].toString());
                for(const auto& id:c["entityIds"].toArray())need(selected.contains(id.toString()),"Scale relation crosses an unselected entity");
                for(const auto& id:c["pointIds"].toArray())need(vertices.contains(id.toString()),"Scale relation crosses an unselected point");
                constraints.append(c);
            }
            for(const auto& d:p["dimensions"].toArray())need(!mentions(d,targets),"Scale refuses driving and reference dimensions on its profile");
            need(constraints==g["constraints"],"Scale source relations changed outside its command");
        }
        return {};
    }catch(const std::exception& e){return QString::fromUtf8(e.what());}
}
QJsonObject source(const QJsonObject& p) {
    auto result=p;auto points=p["points"].toArray();
    for(const auto& v:p["scaleGroups"].toArray())for(const auto& value:v.toObject()["sourcePoints"].toArray()){
        const auto s=value.toObject();for(int n=0;n<points.size();++n)if(points[n].toObject()["id"]==s["pointId"]){auto point=points[n].toObject();point["uMm"]=s["uMm"];point["vMm"]=s["vMm"];points[n]=point;}
    }
    result["points"]=points;result.remove("scaleGroups");return result;
}
QJsonObject materialize(const QJsonObject& p,const QJsonArray& groups) {
    auto result=p;auto points=p["points"].toArray();
    for(const auto& v:groups){const auto g=v.toObject(),center=pivot(p,g);for(const auto& value:g["sourcePoints"].toArray()){
        const auto s=value.toObject(),out=output(s,center,g["factor"].toDouble());coordinate(out,"uMm");coordinate(out,"vMm");bool found=false;
        for(int n=0;n<points.size();++n)if(points[n].toObject()["id"]==s["pointId"]){auto point=points[n].toObject();point["uMm"]=out["uMm"];point["vMm"]=out["vMm"];points[n]=point;found=true;}
        need(found,"Scale source point unavailable");
    }}result["points"]=points;if(!groups.isEmpty())result["scaleGroups"]=groups;return result;
}
void verifySource(const QJsonObject& p,const QJsonObject& solved) {
    const auto actual=rows(solved,"points");
    for(const auto& v:p["scaleGroups"].toArray())for(const auto& value:v.toObject()["sourcePoints"].toArray()){
        const auto s=value.toObject(),a=actual.value(s["pointId"].toString());need(!a.isEmpty(),"Scale source solver changed membership");
        for(const auto* key:{"uMm","vMm"})need(std::abs(a[key].toDouble()-s[key].toDouble())<=1e-6,"Scale source solve drifted outside its explicit source edit");
    }
}
QJsonObject create(const QJsonObject& p,const QJsonArray& entityIds,const QString& pivotId,double value,const QString& groupId) {
    const auto error=validate(p);need(error.isEmpty(),error);factor(value);
    auto groups=p["scaleGroups"].toArray();QJsonObject g;
    if(groupId.isEmpty()){
        need(groups.isEmpty(),"This sketch already has a Scale; edit it");const auto selected=ids(entityIds);need(selected.size()==4,"Scale requires four distinct ordinary Line IDs");const auto es=rows(p,"entities");QSet<QString> vertices;QJsonArray sources,topology,constraints;
        for(const auto& id:entityIds){const auto e=es.value(id.toString());need(e["type"]=="line","Scale requires ordinary authored Lines");vertices.insert(e["startPointId"].toString());vertices.insert(e["endPointId"].toString());topology.append(QJsonObject{{"entityId",id},{"startPointId",e["startPointId"]},{"endPointId",e["endPointId"]}});}
        for(const auto& v:p["points"].toArray()){const auto point=v.toObject();if(vertices.contains(point["id"].toString()))sources.append(QJsonObject{{"pointId",point["id"]},{"uMm",point["uMm"]},{"vMm",point["vMm"]}});}
        for(const auto& v:p["constraints"].toArray())if(mentions(v,selected+vertices))constraints.append(v);
        g={{"id",QUuid::createUuid().toString(QUuid::WithoutBraces)},{"type","scale"},{"entityIds",entityIds},{"sourcePoints",sources},{"topology",topology},{"constraints",constraints},{"dimensions",QJsonArray{}}};
    }else{
        need(groups.size()==1 && groups.first().toObject()["id"]==groupId,"Retained Scale group unavailable");g=groups.first().toObject();
        need(entityIds.isEmpty() || ids(entityIds)==ids(g["entityIds"].toArray()),"Edit Scale retains its original four Lines");
    }
    g["pivotPointId"]=pivotId;g["factor"]=value;const auto result=materialize(source(p),QJsonArray{g});const auto checked=validate(result);need(checked.isEmpty(),checked);return result;
}
QJsonObject editPoint(const QJsonObject& p,const QString& pointId,double u,double v) {
    const auto error=validate(p);need(error.isEmpty(),error);auto groups=p["scaleGroups"].toArray();need(groups.size()==1,"Scale owner unavailable");auto g=groups.first().toObject();
    const auto center=pivot(p,g);const bool isPivot=pointId==g["pivotPointId"].toString();need(isPivot || owned(p,true).contains(pointId),"Point is not a Scale source or pivot");
    const auto target=isPivot ? QJsonObject{{"uMm",u},{"vMm",v}} : output(QJsonObject{{"uMm",u},{"vMm",v}},center,1/g["factor"].toDouble());coordinate(target,"uMm");coordinate(target,"vMm");
    auto original=source(p);auto points=original["points"].toArray();for(int n=0;n<points.size();++n)if(points[n].toObject()["id"]==pointId){auto point=points[n].toObject();point["uMm"]=target["uMm"];point["vMm"]=target["vMm"];points[n]=point;}
    original["points"]=points;auto planar=original;planar.remove("constructionLines3d");const auto solved=SketchSolver::solve(planar,{pointId});need(solved.accepted,"Scale source edit conflicts with its relations: "+solved.error);
    const auto actual=rows(solved.parameters,"points");for(const auto& v:points){const auto wanted=v.toObject(),a=actual.value(wanted["id"].toString());need(!a.isEmpty(),"Scale source edit changed membership");for(const auto* key:{"uMm","vMm"})need(std::abs(a[key].toDouble()-wanted[key].toDouble())<=1e-6,"Scale source edit would move another point or violate its target");}
    auto sources=g["sourcePoints"].toArray();if(!isPivot)for(int n=0;n<sources.size();++n)if(sources[n].toObject()["pointId"]==pointId){auto s=sources[n].toObject();s["uMm"]=target["uMm"];s["vMm"]=target["vMm"];sources[n]=s;}g["sourcePoints"]=sources;
    const auto candidate=materialize(original,QJsonArray{g});const auto checked=validate(candidate);need(checked.isEmpty(),checked);return candidate;
}
}
