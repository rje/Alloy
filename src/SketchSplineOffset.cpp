// SPDX-License-Identifier: MIT
#include "SketchSplineOffset.h"
#include "SketchSpline.h"
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <Geom2dAPI_InterCurveCurve.hxx>
#include <GeomAPI.hxx>
#include <Geom_OffsetCurve.hxx>
#include <GeomConvert_ApproxCurve.hxx>
#include <gp_Ax3.hxx>
#include <gp_Pln.hxx>
#include <QJsonArray>
#include <QMap>
#include <QUuid>
#include <Standard_Failure.hxx>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace {
void require(bool condition,const char* message){if(!condition)throw std::invalid_argument(message);}
double distance(const QJsonObject& entity){
    const auto value=entity.value("signedDistanceMm");
    require(value.isDouble() && std::isfinite(value.toDouble()) &&
            std::abs(value.toDouble())>=0.001 && std::abs(value.toDouble())<=100000,
            "Spline Offset needs a finite nonzero signed distance");
    return value.toDouble();
}
QJsonObject source(const QJsonObject& parameters,const QJsonObject& entity){
    for(const auto& value:parameters.value("entities").toArray()){
        const auto candidate=value.toObject();
        if(candidate.value("id")==entity.value("sourceEntityId") && candidate.value("type")=="fitSpline")return candidate;
    }
    throw std::invalid_argument("Spline Offset lost its retained fit source");
}
QJsonObject frameOrXY(const QJsonObject& frame){
    if(!frame.isEmpty())return frame;
    return {{"origin",QJsonArray{0,0,0}},{"uAxis",QJsonArray{1,0,0}},
            {"vAxis",QJsonArray{0,1,0}},{"normal",QJsonArray{0,0,1}}};
}
gp_Pln plane(const QJsonObject& frame){
    const auto f=frameOrXY(frame);const auto o=f.value("origin").toArray(),u=f.value("uAxis").toArray(),n=f.value("normal").toArray();
    return gp_Pln(gp_Ax3(gp_Pnt(o[0].toDouble(),o[1].toDouble(),o[2].toDouble()),
        gp_Dir(n[0].toDouble(),n[1].toDouble(),n[2].toDouble()),
        gp_Dir(u[0].toDouble(),u[1].toDouble(),u[2].toDouble())));
}
void validateRegular(const Handle(Geom_BSplineCurve)& basis,const Handle(Geom_OffsetCurve)& offset,
                     const double signedDistance){
    const auto first=basis->FirstParameter(),last=basis->LastParameter();
    require(std::isfinite(first) && std::isfinite(last) && last-first>1e-8,
            "Spline Offset source has no usable parameter interval");
    // Finite conservative slice: sample every knot span and refuse near-cusps.
    // OCCT remains the exact curve evaluator; these samples are safety checks.
    for(int knot=1;knot<basis->NbKnots();++knot){
        const auto a=basis->Knot(knot),b=basis->Knot(knot+1);
        if(b-a<=1e-10)continue;
        for(int sample=0;sample<=32;++sample){
            const double t=a+(b-a)*sample/32.0;
            gp_Pnt original,shifted;gp_Vec tangent,second,offsetTangent;
            basis->D2(t,original,tangent,second);
            offset->D1(t,shifted,offsetTangent);
            const double speed=tangent.Magnitude();
            require(std::isfinite(speed) && speed>1e-6 && offsetTangent.Magnitude()>1e-5,
                    "Spline Offset has a zero tangent or cusp");
            const double curvature=tangent.Crossed(second).Magnitude()/std::pow(speed,3);
            require(std::isfinite(curvature) && std::abs(signedDistance)*curvature<0.9,
                    "Spline Offset approaches a cusp at this distance");
            require(std::abs(original.Distance(shifted)-std::abs(signedDistance))<1e-4,
                    "Spline Offset lost its normal distance");
        }
    }
    const auto source2d=GeomAPI::To2d(basis,plane({}));
    const auto offset2d=GeomAPI::To2d(offset,plane({}));
    require(!source2d.IsNull() && !offset2d.IsNull(),"Spline Offset cannot be checked in its plane");
    Geom2dAPI_InterCurveCurve self(offset2d,1e-6);
    require(self.NbPoints()==0 && self.NbSegments()==0,"Spline Offset self-intersects");
    Geom2dAPI_InterCurveCurve sourceSelf(source2d,1e-6);
    require(sourceSelf.NbPoints()==0 && sourceSelf.NbSegments()==0,"Spline Offset source self-intersects");
    Geom2dAPI_InterCurveCurve crosses(source2d,offset2d,1e-6);
    require(crosses.NbPoints()==0 && crosses.NbSegments()==0,"Spline Offset crosses its source");
}
SketchGeometry::SolveResult failed(const QJsonObject& draft,const QString& error){
    return {false,draft,{{"status","invalid"}},error};
}
QString newId(){return QUuid::createUuid().toString(QUuid::WithoutBraces);}
}

SketchSplineOffset::Evaluation SketchSplineOffset::evaluate(const QJsonObject& parameters,
    const QJsonObject& entity,const QJsonObject& frame){
    try{
        const auto original=source(parameters,entity);const auto amount=distance(entity);
        const auto localBasis=SketchSpline::evaluate(parameters,original).curve;
        const auto localOffset=Handle(Geom_OffsetCurve)(new Geom_OffsetCurve(localBasis,amount,gp_Dir(0,0,1)));
        validateRegular(localBasis,localOffset,amount);
        const auto f=frameOrXY(frame);const auto n=f.value("normal").toArray();
        const auto basis=frame.isEmpty()?localBasis:SketchSpline::evaluate(parameters,original,f).curve;
        const auto offset=frame.isEmpty()?localOffset:
            Handle(Geom_OffsetCurve)(new Geom_OffsetCurve(basis,amount,
                gp_Dir(n[0].toDouble(),n[1].toDouble(),n[2].toDouble())));
        const auto start=offset->Value(basis->FirstParameter()),end=offset->Value(basis->LastParameter());
        require(start.Distance(end)>1e-5,"Spline Offset collapsed to a closed curve");
        return {offset,start,end};
    }catch(const Standard_Failure& error){throw std::invalid_argument(error.GetMessageString());}
}

QSet<QString> SketchSplineOffset::derivedPointIds(const QJsonObject& parameters){
    QSet<QString> result;
    for(const auto& value:parameters.value("entities").toArray()){
        const auto entity=value.toObject();if(entity.value("type")!="fitSplineOffset")continue;
        result.insert(entity.value("startPointId").toString());
        result.insert(entity.value("endPointId").toString());
    }
    return result;
}

QJsonObject SketchSplineOffset::refreshEndpoints(const QJsonObject& parameters){
    auto result=parameters;auto points=parameters.value("points").toArray();
    QMap<QString,QJsonObject> computed;
    for(const auto& value:parameters.value("entities").toArray()){
        const auto entity=value.toObject();if(entity.value("type")!="fitSplineOffset")continue;
        const auto evaluated=evaluate(parameters,entity);
        computed[entity.value("startPointId").toString()]={{"uMm",evaluated.start.X()},{"vMm",evaluated.start.Y()}};
        computed[entity.value("endPointId").toString()]={{"uMm",evaluated.end.X()},{"vMm",evaluated.end.Y()}};
    }
    for(int index=0;index<points.size();++index){
        auto point=points[index].toObject();const auto found=computed.find(point.value("id").toString());
        if(found==computed.end())continue;
        point["uMm"]=found.value().value("uMm");point["vMm"]=found.value().value("vMm");points[index]=point;
        computed.erase(found);
    }
    require(computed.isEmpty(),"Spline Offset lost a derived endpoint");
    result["points"]=points;return result;
}

QString SketchSplineOffset::validate(const QJsonObject& parameters,const QJsonObject& entity,bool checkGeometry){
    try{
        require(entity.size()==6 && entity.value("type")=="fitSplineOffset" &&
                entity.value("id").isString() && entity.value("sourceEntityId").isString() &&
                entity.value("startPointId").isString() && entity.value("endPointId").isString() &&
                entity.value("startPointId")!=entity.value("endPointId"),
                "Invalid retained Spline Offset intent");
        source(parameters,entity);distance(entity);
        if(checkGeometry){
            const auto evaluated=evaluate(parameters,entity);
            QMap<QString,QJsonObject> points;
            for(const auto& value:parameters.value("points").toArray()){
                const auto point=value.toObject();points[point.value("id").toString()]=point;
            }
            for(const auto& pair:std::initializer_list<std::pair<QString,gp_Pnt>>{
                    {entity.value("startPointId").toString(),evaluated.start},
                    {entity.value("endPointId").toString(),evaluated.end}}){
                require(points.contains(pair.first),"Spline Offset lost a derived endpoint");
                const auto point=points[pair.first];
                require(std::hypot(point.value("uMm").toDouble()-pair.second.X(),
                                   point.value("vMm").toDouble()-pair.second.Y())<1e-5,
                        "Spline Offset endpoint cache is stale");
            }
        }
        return {};
    }catch(const std::exception& error){return QString::fromUtf8(error.what());}
}

SketchGeometry::SolveResult SketchSplineOffset::add(const QJsonObject& draft,
    const QJsonObject& arguments,const QJsonObject& context){
    try{
        if(arguments.size()!=2 || !arguments.value("sourceEntityId").isString() ||
           !arguments.value("signedDistanceMm").isDouble())throw std::invalid_argument("Spline Offset needs source and signed distance");
        auto candidate=draft,parameters=draft.value("parameters").toObject();
        const auto startId=newId(),endId=newId(),entityId=newId();
        auto entities=parameters.value("entities").toArray(),points=parameters.value("points").toArray();
        const QJsonObject entity{{"id",entityId},{"type","fitSplineOffset"},
            {"sourceEntityId",arguments.value("sourceEntityId")},
            {"signedDistanceMm",arguments.value("signedDistanceMm")},
            {"startPointId",startId},{"endPointId",endId}};
        const auto evaluated=evaluate(parameters,entity);
        points.append(QJsonObject{{"id",startId},{"uMm",evaluated.start.X()},{"vMm",evaluated.start.Y()}});
        points.append(QJsonObject{{"id",endId},{"uMm",evaluated.end.X()},{"vMm",evaluated.end.Y()}});
        entities.append(entity);parameters["entities"]=entities;parameters["points"]=points;
        candidate["parameters"]=parameters;
        auto result=SketchGeometry::solveDraft(candidate,context);
        if(!result.ok)result.feature=draft;
        return result;
    }catch(const std::exception& error){return failed(draft,QString::fromUtf8(error.what()));}
}

SketchGeometry::SolveResult SketchSplineOffset::update(const QJsonObject& draft,
    const QJsonObject& arguments,const QJsonObject& context){
    try{
        if(arguments.size()!=2 || !arguments.value("offsetEntityId").isString() ||
           !arguments.value("signedDistanceMm").isDouble())throw std::invalid_argument("Spline Offset edit needs ID and signed distance");
        auto candidate=draft;auto parameters=draft.value("parameters").toObject();auto entities=parameters.value("entities").toArray();
        bool found=false;
        for(int index=0;index<entities.size();++index){
            auto entity=entities[index].toObject();if(entity.value("id")!=arguments.value("offsetEntityId"))continue;
            require(entity.value("type")=="fitSplineOffset","Selected entity is not a Spline Offset");
            entity["signedDistanceMm"]=arguments.value("signedDistanceMm");entities[index]=entity;found=true;break;
        }
        require(found,"Spline Offset entity is unavailable");
        parameters["entities"]=entities;candidate["parameters"]=parameters;
        auto result=SketchGeometry::solveDraft(candidate,context);
        if(!result.ok)result.feature=draft;
        return result;
    }catch(const std::exception& error){return failed(draft,QString::fromUtf8(error.what()));}
}

// Association removal is an explicit authored conversion, separate from result
// geometry Delete and annotation Delete. Never approximate again on reopen.
SketchGeometry::SolveResult SketchSplineOffset::breakLink(const QJsonObject& draft,
    const QString& entityId,const QJsonObject& context){
    try{
        auto parameters=draft.value("parameters").toObject();auto entities=parameters.value("entities").toArray();
        int index=-1;for(int i=0;i<entities.size();++i)if(entities[i].toObject().value("id")==entityId)index=i;
        require(index>=0,"Spline Offset entity is unavailable");const auto original=entities[index].toObject();
        require(original.value("type")=="fitSplineOffset","Select one retained fit-spline Offset result to Break Link");
        const QSet<QString> protectedIds{entityId,original.value("startPointId").toString(),original.value("endPointId").toString()};
        // Legal straight endpoint connectors keep the same point IDs. Refuse
        // other semantic consumers rather than silently change their meaning.
        const auto references=[&](const auto& self,const QJsonValue& value)->bool{
            if(value.isString())return protectedIds.contains(value.toString());
            if(value.isArray()){for(const auto& child:value.toArray())if(self(self,child))return true;}
            if(value.isObject()){const auto object=value.toObject();for(auto i=object.begin();i!=object.end();++i)if(self(self,i.value()))return true;}
            return false;
        };
        for(auto i=parameters.begin();i!=parameters.end();++i){
            if(i.key()=="points"||i.key()=="entities")continue;
            require(!references(references,i.value()),"Remove dependent relations before breaking this Offset link");
        }
        for(int i=0;i<entities.size();++i){if(i==index)continue;const auto e=entities[i].toObject();
            if(!references(references,e))continue;
            require(e.value("type")=="line"&&!references(references,e.value("sourceEntityId")),
                    "Remove dependent geometry before breaking this Offset link");
        }
        const auto exact=evaluate(parameters,original).curve;
        constexpr double tolerance=1e-3;
        GeomConvert_ApproxCurve approximation(exact,tolerance/4,GeomAbs_C2,32,5);
        require(approximation.IsDone(),"Offset cannot be converted within the Break Link tolerance");
        const auto curve=approximation.Curve();
        require(!curve.IsNull()&&!curve->IsRational()&&!curve->IsPeriodic()&&curve->Degree()==5&&curve->NbPoles()<=128,
                "Offset conversion exceeds the supported editable control-spline representation");
        auto points=parameters.value("points").toArray();QMap<QString,gp_Pnt> endpoints;
        for(const auto& value:points){const auto point=value.toObject();const auto id=point.value("id").toString();
            if(id==original.value("startPointId")||id==original.value("endPointId"))endpoints[id]=gp_Pnt(point.value("uMm").toDouble(),point.value("vMm").toDouble(),0);
        }
        const auto startId=original.value("startPointId").toString(),endId=original.value("endPointId").toString();
        require(endpoints.contains(startId)&&endpoints.contains(endId),"Offset conversion lost its endpoints");
        curve->SetPole(1,endpoints[startId]);curve->SetPole(curve->NbPoles(),endpoints[endId]);
        QJsonArray ids,knots,multiplicities;
        for(int i=1;i<=curve->NbPoles();++i){
            const auto id=i==1?startId:i==curve->NbPoles()?endId:newId();ids.append(id);
            if(i==1||i==curve->NbPoles())continue;const auto point=curve->Pole(i);
            points.append(QJsonObject{{"id",id},{"uMm",point.X()},{"vMm",point.Y()}});
        }
        const double first=curve->FirstParameter(),span=curve->LastParameter()-first;
        for(int i=1;i<=curve->NbKnots();++i){knots.append((curve->Knot(i)-first)/span);multiplicities.append(curve->Multiplicity(i));}
        entities[index]=QJsonObject{{"id",entityId},{"type","controlSpline"},{"version",1},{"pointIds",ids},
            {"startPointId",startId},{"endPointId",endId},{"maximumDegree",5},{"knotPolicy","clampedExplicit"},
            {"knots",knots},{"multiplicities",multiplicities}};
        parameters["entities"]=entities;parameters["points"]=points;auto candidate=draft;candidate["parameters"]=parameters;
        auto result=SketchGeometry::solveDraft(candidate,context);if(!result.ok){result.feature=draft;return result;}
        const auto solvedParameters=result.feature.value("parameters").toObject();
        const auto authored=SketchSpline::evaluate(solvedParameters,entities[index].toObject()).curve;
        double measured=0;for(int i=0;i<=1024;++i){const double t=double(i)/1024;
            measured=std::max(measured,authored->Value(t).Distance(exact->Value(exact->FirstParameter()+t*(exact->LastParameter()-exact->FirstParameter()))));
        }
        require(measured<=tolerance,"Offset conversion changed curve shape beyond the Break Link tolerance");
        require(authored->StartPoint().Distance(endpoints[startId])<1e-10&&authored->EndPoint().Distance(endpoints[endId])<1e-10,
                "Offset conversion changed an endpoint");
        result.diagnostics["convertedEntityId"]=entityId;result.diagnostics["conversionToleranceMm"]=tolerance;
        result.diagnostics["sampledMaximumDeviationMm"]=measured;result.diagnostics["conversionSampleCount"]=1025;
        result.diagnostics["approximationMaximumErrorMm"]=approximation.MaxError();
        return result;
    }catch(const Standard_Failure& error){return failed(draft,QString::fromUtf8(error.GetMessageString()));}
    catch(const std::exception& error){return failed(draft,QString::fromUtf8(error.what()));}
}
