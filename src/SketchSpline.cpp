// SPDX-License-Identifier: MIT
#include "SketchSpline.h"
#include <GeomAPI_Interpolate.hxx>
#include <GCPnts_AbscissaPoint.hxx>
#include <GeomAdaptor_Curve.hxx>
#include <TColgp_HArray1OfPnt.hxx>
#include <TColStd_HArray1OfReal.hxx>
#include <TColgp_Array1OfPnt.hxx>
#include <TColStd_Array1OfInteger.hxx>
#include <QJsonArray>
#include <QMap>
#include <QSet>
#include <QUuid>
#include <Standard_Failure.hxx>
#include <cmath>
#include <array>
#include <algorithm>
#include <stdexcept>
namespace {
void require(bool b,const char* s){if(!b)throw std::invalid_argument(s);}
QString uuid(){return QUuid::createUuid().toString(QUuid::WithoutBraces);}
double number(const QJsonObject& p,const char* k){const auto v=p[k];require(v.isDouble()&&std::isfinite(v.toDouble())&&std::abs(v.toDouble())<=100000,"Spline requires finite coordinates within 100000 mm");return v.toDouble();}
QMap<QString,gp_Pnt> points(const QJsonObject& p){QMap<QString,gp_Pnt> r;for(const auto& v:p["points"].toArray()){const auto q=v.toObject();r[q["id"].toString()]={number(q,"uMm"),number(q,"vMm"),0};}return r;}
gp_Pnt world(const gp_Pnt& p,const QJsonObject& f){if(f.isEmpty())return p;const auto o=f["origin"].toArray(),u=f["uAxis"].toArray(),v=f["vAxis"].toArray();return {o[0].toDouble()+u[0].toDouble()*p.X()+v[0].toDouble()*p.Y(),o[1].toDouble()+u[1].toDouble()*p.X()+v[1].toDouble()*p.Y(),o[2].toDouble()+u[2].toDouble()*p.X()+v[2].toDouble()*p.Y()};}
SketchGeometry::SolveResult failed(const QJsonObject& d,const QString& e){return {false,d,{{"status","invalid"}},e};}
SketchGeometry::SolveResult finish(const QJsonObject& d,const QJsonObject& p,const QJsonObject& c){auto q=d;q["parameters"]=p;auto r=SketchGeometry::solveDraft(q,c);if(!r.ok)r.feature=d;return r;}
}
QString SketchSpline::validate(const QJsonObject& p,const QJsonObject& e){
 try{
 const bool control=e["type"]=="controlSpline";const bool display=e.contains("curvatureComb");
 require(e.size()==8+int(display)&&e["version"]==1&&e["pointIds"].isArray(),"Invalid spline intent");
 if(control)require(e["knotPolicy"]=="clampedUniform"&&e["maximumDegree"].isDouble()&&(e["maximumDegree"].toDouble()==3||e["maximumDegree"].toDouble()==5),"Control spline maximum degree must be 3 or 5");
 else require(e["type"]=="fitSpline"&&e["parameterization"]=="chordLength"&&e["tangentHandles"].isObject(),"Invalid fit spline intent");
 if(display){const auto c=e["curvatureComb"].toObject();require(c.size()==3&&c["enabled"].isBool()&&c["density"].isDouble()&&c["density"].toDouble()==c["density"].toInt()&&c["density"].toInt()>=4&&c["density"].toInt()<=256&&c["scale"].isDouble()&&std::isfinite(c["scale"].toDouble())&&c["scale"].toDouble()>=0.01&&c["scale"].toDouble()<=100,"Comb requires enabled, density4..256 and relative scale0.01..100");}
 const auto ps=points(p);const auto ids=e["pointIds"].toArray();require(ids.size()>=2&&ids.size()<=128,"Fit spline requires 2..128 points");QSet<QString> used;
 for(const auto& id:ids){require(id.isString()&&ps.contains(id.toString())&&!used.contains(id.toString()),"Spline point references must be distinct existing UUIDs");used.insert(id.toString());}
 require(e["startPointId"]==ids.first()&&e["endPointId"]==ids.last(),"Spline endpoint identities must match fit order");
 if(control){require(ps[ids.first().toString()].Distance(ps[ids.last().toString()])>=0.001,"Open control spline endpoints must be distinct");return {};}
 const auto hs=e["tangentHandles"].toObject();for(auto it=hs.begin();it!=hs.end();++it){require(used.contains(it.key())&&it.value().isObject(),"Spline handle must belong to its fit point");const auto h=it.value().toObject();require(h.size()==2&&std::hypot(number(h,"duMm"),number(h,"dvMm"))>=0.001,"Spline tangent must have nonzero direction and weight");}
 for(int i=1;i<ids.size();++i)require(ps[ids[i].toString()].Distance(ps[ids[i-1].toString()])>=0.001,"Spline consecutive fit points must be at least 0.001 mm apart");
 return {};
 }catch(const std::exception& x){return QString::fromUtf8(x.what());}
}
SketchSpline::Evaluation SketchSpline::evaluate(const QJsonObject& p,const QJsonObject& e,const QJsonObject& frame){
 try {
 const auto err=validate(p,e);if(!err.isEmpty())throw std::invalid_argument(err.toStdString());
 const auto ps=points(p);const auto ids=e["pointIds"].toArray();const int count=ids.size();Evaluation result;
 if(e["type"]=="controlSpline"){
  const int degree=std::min(e["maximumDegree"].toInt(),count-1),knotCount=count-degree+1;
  TColgp_Array1OfPnt poles(1,count);TColStd_Array1OfReal knots(1,knotCount);TColStd_Array1OfInteger mults(1,knotCount);
  for(int i=0;i<count;++i)poles.SetValue(i+1,world(ps[ids[i].toString()],frame));
  for(int i=0;i<knotCount;++i){knots.SetValue(i+1,double(i)/(knotCount-1));mults.SetValue(i+1,i==0||i==knotCount-1?degree+1:1);}
  result.curve=new Geom_BSplineCurve(poles,knots,mults,degree,false);return result;
 }
 Handle(TColgp_HArray1OfPnt) fit=new TColgp_HArray1OfPnt(1,count);Handle(TColStd_HArray1OfReal) ts=new TColStd_HArray1OfReal(1,count);
 double length=0;result.parameters.append(0);for(int i=1;i<count;++i){length+=ps[ids[i].toString()].Distance(ps[ids[i-1].toString()]);result.parameters.append(length);}
 for(int i=0;i<count;++i){result.parameters[i]/=length;ts->SetValue(i+1,result.parameters[i]);fit->SetValue(i+1,ps[ids[i].toString()]);}
 GeomAPI_Interpolate baseline(fit,ts,false,1e-8);baseline.Perform();require(baseline.IsDone(),"OCCT could not interpolate fit points");
 QVector<gp_Pnt> positions;QVector<gp_Vec> derivatives,accelerations;const auto handles=e["tangentHandles"].toObject();
 for(int i=0;i<count;++i){gp_Pnt position;gp_Vec d,a;baseline.Curve()->D2(result.parameters[i],position,d,a);const auto id=ids[i].toString();if(handles.contains(id)){const auto h=handles[id].toObject();d={number(h,"duMm"),number(h,"dvMm"),0};}positions.append(position);derivatives.append(d);accelerations.append(a);
 result.handles.append(QJsonObject{{"pointId",id},{"active",handles.contains(id)},{"uMm",position.X()},{"vMm",position.Y()},{"duMm",d.X()},{"dvMm",d.Y()},{"positiveGripUMm",position.X()+d.X()/3},{"positiveGripVMm",position.Y()+d.Y()/3},{"negativeGripUMm",position.X()-d.X()/3},{"negativeGripVMm",position.Y()-d.Y()/3}});}
 // Exact piecewise quintic Hermite. Knots retain global normalized chord spans;
 // OCCT removes only redundant join multiplicity after verifying geometry.
 TColgp_Array1OfPnt poles(1,5*(count-1)+1);TColStd_Array1OfReal knots(1,count);TColStd_Array1OfInteger mults(1,count);
 for(int i=0;i<count;++i){knots.SetValue(i+1,result.parameters[i]);mults.SetValue(i+1,(i==0||i==count-1)?6:5);}
 for(int i=0;i<count-1;++i){const double h=result.parameters[i+1]-result.parameters[i];const auto p0=positions[i],p1=positions[i+1];const auto d0=derivatives[i]*h,d1=derivatives[i+1]*h,a0=accelerations[i]*(h*h),a1=accelerations[i+1]*(h*h);
 const std::array<gp_Pnt,6> span{p0,p0.Translated(d0/5),p0.Translated(d0*(2.0/5)+a0/20),p1.Translated(d1*(-2.0/5)+a1/20),p1.Translated(d1*(-1.0/5)),p1};
 for(int j=0;j<6;++j)poles.SetValue(5*i+j+1,world(span[j],frame));}
 result.curve=new Geom_BSplineCurve(poles,knots,mults,5,false);
 for(int i=count-1;i>=2;--i)require(result.curve->RemoveKnot(i,3,1e-7),"OCCT could not retain C2 quintic joins");
 require(result.curve->Degree()==5&&result.curve->IsCN(2),"Spline evaluation must retain degree 5 and C2 continuity");return result;
 }catch(const Standard_Failure& failure){throw std::invalid_argument(failure.GetMessageString());}
}
SketchGeometry::SolveResult SketchSpline::add(const QJsonObject& d,const QJsonObject& in,const QJsonObject& c){
 try{require(in.size()==1&&in["points"].isArray(),"Fit spline creation requires points array");auto p=d["parameters"].toObject();auto ps=p["points"].toArray(),es=p["entities"].toArray();QJsonArray ids;
 for(const auto& v:in["points"].toArray()){require(v.isObject(),"Fit point must be an object");auto q=v.toObject();require(q.size()==2,"Fit point requires uMm and vMm");const auto id=uuid();ps.append(QJsonObject{{"id",id},{"uMm",number(q,"uMm")},{"vMm",number(q,"vMm")}});ids.append(id);}
 require(ids.size()>=2,"Place at least two fit points");const auto id=uuid();es.append(QJsonObject{{"id",id},{"type","fitSpline"},{"version",1},{"parameterization","chordLength"},{"pointIds",ids},{"startPointId",ids.first()},{"endPointId",ids.last()},{"tangentHandles",QJsonObject{}}});p["points"]=ps;p["entities"]=es;auto r=finish(d,p,c);if(r.ok)r.diagnostics["createdSplineId"]=id;return r;
 }catch(const Standard_Failure& x){return failed(d,QString::fromUtf8(x.GetMessageString()));}catch(const std::exception& x){return failed(d,QString::fromUtf8(x.what()));}
}
SketchGeometry::SolveResult SketchSpline::edit(const QJsonObject& d,const QString& op,const QJsonObject& in,const QJsonObject& c){
 try{auto p=d["parameters"].toObject();auto es=p["entities"].toArray(),ps=p["points"].toArray();int index=-1;for(int i=0;i<es.size();++i)if(es[i].toObject()["id"]==in["entityId"])index=i;require(index>=0,"Spline no longer exists");auto e=es[index].toObject();require(e["type"]=="fitSpline","Select a fit point spline");
 for(const auto& group:p.value("mirrorGroups").toArray())for(const auto& pair:group.toObject()["entityPairs"].toArray()){
  const auto relation=pair.toObject();require(relation["resultEntityId"]!=in["entityId"],"Edit the mirrored spline source tangent or fit points");
  if(relation["sourceEntityId"]==in["entityId"])require(op=="tangent","Changing fit-point count is unavailable while this spline is mirrored");
 }
 auto ids=e["pointIds"].toArray();auto handles=e["tangentHandles"].toObject();const auto id=in["pointId"].toString();QString created;
 if(op=="tangent"){require(in.size()==4&&ids.contains(id),"Tangent edit requires entityId, fit pointId, duMm, dvMm");handles[id]=QJsonObject{{"duMm",number(in,"duMm")},{"dvMm",number(in,"dvMm")}};}
 else if(op=="remove"){require(in.size()==2&&ids.contains(id)&&ids.size()>2,"Removal requires a fit point and at least two remaining points");for(const auto& v:p.value("constraints").toArray())require(!v.toObject()["pointIds"].toArray().contains(id),"Fit point has retained constraints; remove them explicitly first");for(const auto& v:p.value("dimensions").toArray())require(!v.toObject()["pointIds"].toArray().contains(id),"Fit point has retained dimensions");for(const auto& v:es){const auto q=v.toObject();if(q["id"]==e["id"])continue;require(q["startPointId"]!=id&&q["endPointId"]!=id&&q["centerPointId"]!=id&&q["pointId"]!=id&&!q["pointIds"].toArray().contains(id),"Fit point is shared by another entity");}for(int i=ids.size()-1;i>=0;--i)if(ids[i]==id)ids.removeAt(i);for(int i=ps.size()-1;i>=0;--i)if(ps[i].toObject()["id"]==id)ps.removeAt(i);handles.remove(id);}
 else if(op=="insert"){require(in.size()==2,"Insertion requires entityId and parameter");const double t=number(in,"parameter");const auto evaluated=evaluate(p,e);require(t>1e-6&&t<1-1e-6,"Insert inside the curve, away from endpoints");int at=1;while(at<evaluated.parameters.size()&&evaluated.parameters[at]<t)++at;for(double old:evaluated.parameters)require(std::abs(old-t)>1e-6,"A fit point already exists here");const auto pos=evaluated.curve->Value(t);created=uuid();ps.append(QJsonObject{{"id",created},{"uMm",pos.X()},{"vMm",pos.Y()}});ids.insert(at,created);}
 else require(false,"Unsupported fit spline edit");
 e["pointIds"]=ids;e["startPointId"]=ids.first();e["endPointId"]=ids.last();e["tangentHandles"]=handles;es[index]=e;p["entities"]=es;p["points"]=ps;auto r=finish(d,p,c);if(r.ok&&!created.isEmpty())r.diagnostics["createdFitPointId"]=created;return r;
 }catch(const Standard_Failure& x){return failed(d,QString::fromUtf8(x.GetMessageString()));}catch(const std::exception& x){return failed(d,QString::fromUtf8(x.what()));}
}

SketchGeometry::SolveResult SketchSpline::addControl(const QJsonObject& d,const QJsonObject& in,const QJsonObject& context){
 try{
  require(in["points"].isArray()&&in.size()==1+int(in.contains("maximumDegree")),"Control spline requires points and optional maximumDegree");
  const auto maximum=in.value("maximumDegree").toDouble(5);require(maximum==3||maximum==5,"Control spline maximum degree must be 3 or 5");
  auto p=d["parameters"].toObject();auto ps=p["points"].toArray(),es=p["entities"].toArray();QJsonArray ids;
  for(const auto& item:in["points"].toArray()){require(item.isObject(),"Control pole must be an object");const auto q=item.toObject();require(q.size()==2,"Control pole requires uMm and vMm");const auto id=uuid();ps.append(QJsonObject{{"id",id},{"uMm",number(q,"uMm")},{"vMm",number(q,"vMm")}});ids.append(id);}
  require(ids.size()>=2&&ids.size()<=128,"Control spline requires 2..128 poles");const auto id=uuid();es.append(QJsonObject{{"id",id},{"type","controlSpline"},{"version",1},{"pointIds",ids},{"startPointId",ids.first()},{"endPointId",ids.last()},{"maximumDegree",maximum},{"knotPolicy","clampedUniform"}});p["points"]=ps;p["entities"]=es;auto result=finish(d,p,context);if(result.ok)result.diagnostics["createdSplineId"]=id;return result;
 }catch(const std::exception& error){return failed(d,QString::fromUtf8(error.what()));}
}
QJsonArray SketchSpline::curveSamples(const Evaluation& evaluated,int count){
 QJsonArray result;for(int i=0;i<count;++i){const double t=double(i)/(count-1);const auto p=evaluated.curve->Value(t);result.append(QJsonObject{{"parameter",t},{"uMm",p.X()},{"vMm",p.Y()}});}return result;
}
QJsonObject SketchSpline::comb(const QJsonObject& parameters,const QJsonObject& entity){
 try{
  const auto error=validate(parameters,entity);if(!error.isEmpty())throw std::invalid_argument(error.toStdString());
  const auto settings=entity.value("curvatureComb").toObject(QJsonObject{{"enabled",false},{"density",32},{"scale",1.0}});auto result=settings;
  result["samples"]=QJsonArray{};result["undefinedCount"]=0;result["clippedCount"]=0;result["renderedToothCount"]=0;
  if(!settings["enabled"].toBool())return result;
  const auto curve=evaluate(parameters,entity).curve;GeomAdaptor_Curve adaptor(curve);const double length=GCPnts_AbscissaPoint::Length(adaptor,curve->FirstParameter(),curve->LastParameter(),1e-7);
  require(std::isfinite(length)&&length>=0.001,"Comb requires a finite nonzero curve length");const double gain=length*length*settings["scale"].toDouble()/4;
  result["lengthMm"]=length;result["gainMmSquared"]=gain;result["maximumToothLengthMm"]=length/2;QJsonArray samples;int undefined=0,clipped=0,rendered=0;
  for(int i=0;i<settings["density"].toInt();++i){const double t=double(i)/(settings["density"].toInt()-1);gp_Pnt p;gp_Vec d,a;if(curve->Degree()==1)curve->D1(t,p,d);else curve->D2(t,p,d,a);
   const double speed=d.Magnitude();QJsonObject sample{{"parameter",t},{"uMm",p.X()},{"vMm",p.Y()},{"defined",false},{"clipped",false}};
   if(!std::isfinite(speed)||speed<1e-9){++undefined;sample["signedCurvaturePerMm"]=QJsonValue::Null;samples.append(sample);continue;}
   const double k=(d.X()*a.Y()-d.Y()*a.X())/(speed*speed*speed);require(std::isfinite(k),"Curvature evaluation is not finite");const double raw=k*gain,display=std::clamp(raw,-length/2,length/2);const bool clippedSample=std::abs(raw)>length/2;
   sample["defined"]=true;sample["signedCurvaturePerMm"]=k;sample["unclippedToothLengthMm"]=std::abs(raw);sample["clipped"]=clippedSample;sample["tipUMm"]=p.X()+d.Y()/speed*display;sample["tipVMm"]=p.Y()-d.X()/speed*display;samples.append(sample);clipped+=int(clippedSample);rendered+=int(std::abs(display)>1e-9);
  }
  result["samples"]=samples;result["undefinedCount"]=undefined;result["clippedCount"]=clipped;result["renderedToothCount"]=rendered;return result;
 }catch(const Standard_Failure& failure){throw std::invalid_argument(failure.GetMessageString());}
}
SketchGeometry::SolveResult SketchSpline::setComb(const QJsonObject& d,const QJsonObject& in,const QJsonObject& context){
 try{
  for(auto it=in.begin();it!=in.end();++it)require(it.key()=="entityId"||it.key()=="enabled"||it.key()=="density"||it.key()=="scale","Unknown comb option");
  require(in["enabled"].isBool(),"Specify Curvature Comb enabled");auto p=d["parameters"].toObject();auto es=p["entities"].toArray();bool found=false;
  for(int i=0;i<es.size();++i){auto e=es[i].toObject();if(e["id"]!=in["entityId"])continue;require(e["type"]=="fitSpline"||e["type"]=="controlSpline","Select a spline for Curvature Comb");auto settings=e.value("curvatureComb").toObject(QJsonObject{{"enabled",false},{"density",32},{"scale",1.0}});settings["enabled"]=in["enabled"];for(const auto& key:{"density","scale"})if(in.contains(key))settings[key]=in[key];e["curvatureComb"]=settings;es[i]=e;found=true;}
  require(found,"Spline no longer exists");p["entities"]=es;auto candidate=d;candidate["parameters"]=p;auto result=finish(d,p,context);
  if(result.ok){for(const auto& e:es)if(e.toObject()["id"]==in["entityId"])comb(p,e.toObject());result.feature=candidate;}return result;
 }catch(const std::exception& error){return failed(d,QString::fromUtf8(error.what()));}
}
