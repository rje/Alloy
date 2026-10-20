// SPDX-License-Identifier: MIT
#include "SketchGeometry.h"
#include <GCE2d_MakeSegment.hxx>
#include <Geom2d_Circle.hxx>
#include <Geom2dAPI_InterCurveCurve.hxx>
#include <gp_Ax2d.hxx>
#include <QMap>
#include <QUuid>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace {
constexpr double pi=3.14159265358979323846,eps=1e-7;
QString fresh(){return QUuid::createUuid().toString(QUuid::WithoutBraces);}
double positive(double a){a=std::fmod(a,2*pi);if(a<0)a+=2*pi;return 2*pi-a<1e-10?0:a;}
struct Curve {QJsonObject entity;Handle(Geom2d_Curve) curve;gp_Pnt2d a,b,c;double start=0,sweep=0,r=0;
    double parameter(const gp_Pnt2d& p)const{if(entity.value("type")=="line"){gp_Vec2d d(a,b);return gp_Vec2d(a,p).Dot(d)/d.SquareMagnitude();}const auto angle=std::atan2(p.Y()-c.Y(),p.X()-c.X());return (sweep>0?positive(angle-start):positive(start-angle))/std::abs(sweep);}
    bool finite()const{return entity.value("type")!="circle";}
    gp_Vec2d tangent(const gp_Pnt2d& p)const{return entity.value("type")=="line"?gp_Vec2d(a,b):gp_Vec2d(-(p.Y()-c.Y()),p.X()-c.X());}
};
bool mentions(const QJsonValue& v,const QString& id){if(v.isString())return v.toString()==id;if(v.isArray()){for(auto x:v.toArray())if(mentions(x,id))return true;}if(v.isObject()){auto o=v.toObject();for(auto x:o)if(mentions(x,id))return true;}return false;}
}
SketchGeometry::SolveResult SketchGeometry::breakCurve(const QJsonObject& draft,const QJsonObject& arguments,const QJsonObject& context){
 try{
    for(auto it=arguments.begin();it!=arguments.end();++it)if(it.key()!="entityId"&&it.key()!="uMm"&&it.key()!="vMm")throw std::invalid_argument("Unknown Break argument");
    if(!arguments.value("uMm").isDouble()||!arguments.value("vMm").isDouble()||!std::isfinite(arguments.value("uMm").toDouble())||!std::isfinite(arguments.value("vMm").toDouble()))throw std::invalid_argument("Break needs a finite sketch point");
    const auto baseline=solveDraft(draft,context);if(!baseline.ok)return {false,draft,baseline.diagnostics,baseline.error};
    auto p=baseline.feature.value("parameters").toObject();const auto original=p.value("entities").toArray();const auto id=arguments.value("entityId").toString();
    if(!p.value("projections").toArray().isEmpty() || !p.value("circleProjections").toArray().isEmpty())throw std::invalid_argument("Break with linked curve boundaries is unsupported");
    QMap<QString,QJsonObject> points;for(auto v:p.value("points").toArray()){auto o=v.toObject();points.insert(o.value("id").toString(),o);}
    auto xy=[&](const QString& key){const auto o=points.value(key);if(o.isEmpty())throw std::invalid_argument("Break point unavailable");return gp_Pnt2d(o.value("uMm").toDouble(),o.value("vMm").toDouble());};
    auto curve=[&](const QJsonObject& e){Curve c;c.entity=e;if(e.value("type")=="line"){c.a=xy(e.value("startPointId").toString());c.b=xy(e.value("endPointId").toString());c.curve=GCE2d_MakeSegment(c.a,c.b).Value();}else{c.c=xy(e.value("centerPointId").toString());c.r=e.value("radiusMm").toDouble();c.start=e.value("startAngleDeg").toDouble()*pi/180;c.sweep=e.value("type")=="circle"?2*pi:e.value("sweepAngleDeg").toDouble()*pi/180;c.curve=new Geom2d_Circle(gp_Ax2d(c.c,gp_Dir2d(1,0)),c.r);}return c;};
    QJsonObject source;for(auto v:original)if(v.toObject().value("id")==id)source=v.toObject();
    if(!QSet<QString>{"line","arc","circle"}.contains(source.value("type").toString())||source.value("projected").toBool()||source.contains("circleMode"))throw std::invalid_argument("Break requires an independent authored line, arc or circle");
    const QSet<QString> sourceFields=source.value("type")=="line"?QSet<QString>{"id","type","startPointId","endPointId","construction"}:source.value("type")=="arc"?QSet<QString>{"id","type","centerPointId","startPointId","endPointId","radiusMm","startAngleDeg","sweepAngleDeg","construction"}:QSet<QString>{"id","type","centerPointId","radiusMm"};
    for(auto it=source.begin();it!=source.end();++it)if(!sourceFields.contains(it.key()))throw std::invalid_argument("Break of derived or linked geometry is unsupported");
    for(auto it=p.begin();it!=p.end();++it)if(!QSet<QString>{"entities","points","constraints","dimensions","originPointId"}.contains(it.key())&&mentions(it.value(),id))throw std::invalid_argument("Break of retained grouped or derived geometry is unsupported");
    const auto target=curve(source);const gp_Pnt2d picked(arguments.value("uMm").toDouble(),arguments.value("vMm").toDouble());
    const auto pick=target.parameter(picked);if(target.finite()&&(pick<=eps||pick>=1-eps))throw std::invalid_argument("Pick inside the curve, away from its endpoints");
    struct Cut{double t;gp_Pnt2d point;QString boundary;};std::vector<Cut> cuts;
    for(auto value:original){auto e=value.toObject();if(e.value("id")==id||e.value("type")=="point")continue;
      if(!QSet<QString>{"line","arc","circle"}.contains(e.value("type").toString()))throw std::invalid_argument("Break intersection discovery currently requires analytic line, arc and circle boundaries");
      const auto other=curve(e);Geom2dAPI_InterCurveCurve inter(target.curve,other.curve,1e-8);
      if(inter.NbSegments())throw std::invalid_argument("Overlapping curves make Break ambiguous");
      for(int i=1;i<=inter.NbPoints();++i){const auto at=inter.Point(i);const double t=target.parameter(at),u=other.parameter(at);
        if((target.finite()&&t>1+eps)||(other.finite()&&u>1+eps))continue;
        if((target.finite()&&(t<=eps||t>=1-eps))||(other.finite()&&(u<=eps||u>=1-eps)))throw std::invalid_argument("Break intersection meets an existing endpoint");
        const auto a=target.tangent(at),b=other.tangent(at);if(std::abs(a.Crossed(b))<=eps*a.Magnitude()*b.Magnitude())throw std::invalid_argument("Tangent Break intersection is unsupported");
        for(const auto& cut:cuts)if(cut.point.Distance(at)<eps)throw std::invalid_argument("Break intersection ownership is ambiguous");
        cuts.push_back({t,at,e.value("id").toString()});
      }
    }
    const bool circle=source.value("type")=="circle";
    if(cuts.size()!=(circle?2:1)||(circle&&cuts[0].boundary!=cuts[1].boundary))throw std::invalid_argument("Break requires one unique interior crossing, or exactly two circle secant intersections from one curve");
    std::sort(cuts.begin(),cuts.end(),[](const auto& a,const auto& b){return a.t<b.t;});
    QJsonArray allPoints=p.value("points").toArray(),markers;QStringList cutIds;
    for(const auto& cut:cuts){const auto pointId=fresh();cutIds.append(pointId);allPoints.append(QJsonObject{{"id",pointId},{"uMm",cut.point.X()},{"vMm",cut.point.Y()}});markers.append(QJsonObject{{"uMm",cut.point.X()},{"vMm",cut.point.Y()}});}
    auto first=source,second=source;const auto newId=fresh();second["id"]=newId;
    if(source.value("type")=="line"){first["endPointId"]=cutIds[0];second["startPointId"]=cutIds[0];}
    else{
      auto arc=[&](QJsonObject e,double start,double sweep,const QString& a,const QString& b){e["type"]="arc";e["startAngleDeg"]=std::remainder(start*180/pi,360.0);e["sweepAngleDeg"]=sweep*180/pi;e["startPointId"]=a;e["endPointId"]=b;return e;};
      if(circle){const double start=cuts[0].t*2*pi,span=(cuts[1].t-cuts[0].t)*2*pi;
        // Original identity belongs to the piece containing the original circle's zero-angle seam.
        QStringList secondIds;for(const auto& cut:cuts){const auto pointId=fresh();secondIds.append(pointId);allPoints.append(QJsonObject{{"id",pointId},{"uMm",cut.point.X()},{"vMm",cut.point.Y()}});}
        first=arc(first,start+span,2*pi-span,cutIds[1],cutIds[0]);second=arc(second,start,span,secondIds[0],secondIds[1]);
      }else{first=arc(first,target.start,target.sweep*cuts[0].t,source.value("startPointId").toString(),cutIds[0]);second=arc(second,target.start+target.sweep*cuts[0].t,target.sweep*(1-cuts[0].t),cutIds[0],source.value("endPointId").toString());}
    }
    QJsonArray entities;for(auto v:original){if(v.toObject().value("id")==id){entities.append(first);entities.append(second);}else entities.append(v);}
    QJsonObject diagnostics{{"operation","break"},{"status","valid"},{"sourceEntityId",id},{"retainedEntityIds",QJsonArray{id}},{"createdEntityIds",QJsonArray{newId}},{"breakPoints",markers},{"identityPolicy","original parameter-start piece; circle zero-angle seam piece"},{"intersectionRelations","line/arc split point shared; circle endpoint pairs independent; boundary geometry unchanged"}};
    for(const auto& key:{"constraints","dimensions"}){QJsonArray kept,removed,retained,reasons;for(auto v:p.value(key).toArray()){auto o=v.toObject();const bool affected=mentions(o.value("entityIds"),id)||o.value("entityId")==id;
      if(affected){removed.append(o.value("id"));reasons.append(QJsonObject{{"id",o.value("id")},{"reason","Source curve split changes segment meaning"}});}else{kept.append(o);retained.append(o.value("id"));}}
      p[key]=kept;const auto label=QString(key)=="constraints"?"Constraint":"Dimension";diagnostics[QString("removed%1Ids").arg(label)]=removed;diagnostics[QString("retained%1Ids").arg(label)]=retained;diagnostics[QString("removed%1Reasons").arg(label)]=reasons;}
    p["entities"]=entities;p["points"]=allPoints;auto candidate=baseline.feature;candidate["parameters"]=p;auto result=solveDraft(candidate,context);if(!result.ok)return {false,draft,result.diagnostics,result.error};
    // A successful solve must not pull existing geometry away from its original locus.
    const auto solved=result.feature.value("parameters").toObject();if(solved.value("points")!=p.value("points")||solved.value("entities")!=p.value("entities"))throw std::invalid_argument("Break would move sketch geometry");
    for(auto it=diagnostics.begin();it!=diagnostics.end();++it)result.diagnostics[it.key()]=it.value();return result;
 }catch(const std::exception& error){return {false,draft,{{"operation","break"},{"status","invalid"}},QString::fromUtf8(error.what())};}
}
