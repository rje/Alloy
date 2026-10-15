// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "RoundedLoopOffset.h"
#include "SketchGeometry.h"
#include <BRepAdaptor_Curve.hxx>
#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace {
EvaluatedSupportFace cap(const CadDocument& document) {
    for(const auto& body:document.bodies())for(const auto& face:body.supportFaces)
        if(face.role=="split.negative.cap")return face;
    throw std::runtime_error("split lower cap unavailable");
}
QJsonArray point(const gp_Pnt& value) {return {value.X(),value.Y(),value.Z()};}
struct Rail {QString id;gp_Pnt a,b;gp_Vec direction;double length;QJsonObject provenance;};
}

int main(int argc,char** argv) {
    QCoreApplication app(argc,argv);
    try {
        if(argc!=3 && argc!=4)throw std::runtime_error("usage: enclosure_rim_continuation_test source.alloy output.json [fillet]");
        CadDocument document;
        if(!document.open(argv[1]).isEmpty())throw std::runtime_error("source did not reopen");
        const auto sourceFeatureCount=document.authored().value("features").toArray().size();
        const auto support=cap(document);
        QString owner;
        for(const auto& body:document.bodies())if(body.bodyId==support.bodyId)owner=body.ownerId;
        const QJsonObject reference{{"producerFeatureId",support.producerFeatureId},
            {"bodyId",support.bodyId},{"faceId",support.faceId}};
        auto sketch=SketchGeometry::newSupportedFeature(owner,reference,"Rim inset",{});
        auto parameters=sketch.value("parameters").toObject();
        parameters["loopOffsets"]=QJsonArray{QJsonObject{{"id",CadDocument::newId()},
            {"kind","planarInsetV1"},{"outerLoopId",RoundedLoopOffset::insetLoopId(support)},
            {"distanceMm",-1.25},{"chain",true}}};
        sketch["parameters"]=parameters;
        if(!document.commit(SketchGeometry::replaceFeature(document.authored(),sketch),"Rim inset").isEmpty())
            throw std::runtime_error("inset refused");
        const auto profile=document.sketches().back().profiles.front();
        const auto join=CadDocument::extrusionCandidate(document.authored(),
            {{"ownerId",owner},{"sketchId",sketch.value("id")},{"profileId",profile.profileId},
             {"distanceMm",2.0},{"operation","join"},{"targetBodyIds",QJsonArray{support.bodyId}}});
        if(!document.commit(join,"Rim Join2").isEmpty())throw std::runtime_error("Join2 refused");
        std::vector<Rail> rails;QJsonArray rows;
        for(const auto& body:document.bodies())if(body.bodyId==support.bodyId)
            for(const auto& edge:body.solidEdges) {
                if(edge.edge.IsNull() || edge.provenance.value("sourceSketchId")!=sketch.value("id")
                    || edge.provenance.value("role")!="endRim")continue;
                const BRepAdaptor_Curve curve(edge.edge);
                if(curve.GetType()!=GeomAbs_Line)continue;
                const auto a=curve.Value(curve.FirstParameter()),b=curve.Value(curve.LastParameter());
                gp_Vec direction(a,b);const double length=direction.Magnitude();
                if(length<1e-8)continue;direction.Normalize();
                rails.push_back({edge.edgeId,a,b,direction,length,edge.provenance});
                rows.append(QJsonObject{{"edgeId",edge.edgeId},{"start",point(a)},{"end",point(b)},
                    {"lengthMm",length},{"provenance",edge.provenance}});
            }
        QJsonArray pairs;
        for(size_t i=0;i<rails.size();++i)for(size_t j=i+1;j<rails.size();++j) {
            const auto& a=rails[i];const auto& b=rails[j];
            const double parallel=std::abs(a.direction.Dot(b.direction));
            if(parallel<0.9999)continue;
            const gp_Vec from(a.a,b.a);const double distance=from.Crossed(a.direction).Magnitude();
            const double b0=from.Dot(a.direction),b1=gp_Vec(a.a,b.b).Dot(a.direction);
            const double overlap=std::max(0.0,std::min(a.length,std::max(b0,b1))-
                std::max(0.0,std::min(b0,b1)));
            if(overlap<1)continue;
            pairs.append(QJsonObject{{"first",a.id},{"second",b.id},{"parallel",parallel},
                {"distanceMm",distance},{"overlapMm",overlap}});
        }
        QFile file(argv[2]);if(!file.open(QIODevice::WriteOnly))throw std::runtime_error("output unavailable");
        QJsonObject result{{"source",argv[1]},{"sourceFeatureCount",sourceFeatureCount},{"supportFaceId",support.faceId},
            {"profileId",profile.profileId},{"rails",rows},{"parallelPairs",pairs}};
        if(argc==4) {
            if(QString::fromUtf8(argv[3])!="fillet")throw std::runtime_error("unsupported diagnostic mode");
            QStringList selected;
            for(const auto& rail:rails) {
                const bool rear=std::abs(rail.length-54)<1e-7 &&
                    (std::abs(rail.a.Y()+45.25)<1e-7 || std::abs(rail.a.Y()+44.25)<1e-7);
                if(rear)selected.append(rail.id);
            }
            if(selected.size()!=2)throw std::runtime_error("rear opposing 54mm rail pair unavailable");
            result["selectedRearPair"]=QJsonArray{selected[0],selected[1]};
            result["attemptedRadiusMm"]=0.5;
            const auto candidate=CadDocument::filletCandidate(document.authored(),
                {{"ownerId",owner},{"targetBodyId",support.bodyId},
                 {"edgeIds",QJsonArray{selected[0],selected[1]}},{"radiusMm",0.5}});
            const auto error=document.commit(candidate,"Two upper rim rails R0.5");
            if(!error.isEmpty()) {
                result["filletAccepted"]=false;result["filletError"]=error;
                file.write(QJsonDocument(result).toJson(QJsonDocument::Indented));
                qCritical("two-edge R0.5 refused: %s",qPrintable(error));return 2;
            }
            result["filletAccepted"]=true;
            for(const auto& body:document.bodies())if(body.bodyId==support.bodyId) {
                result["filletedBody"]=body.summary;
                result["filletedSolidEdgeCount"]=static_cast<int>(body.solidEdges.size());
            }
            const auto save=QString::fromUtf8(argv[2])+".alloy";
            if(!document.save(save).isEmpty())throw std::runtime_error("filleted model save refused");
            result["savedModel"]=save;
        }
        file.write(QJsonDocument(result).toJson(QJsonDocument::Indented));
        return 0;
    } catch(const std::exception& error) {qCritical("%s",error.what());return 1;}
}
