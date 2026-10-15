// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "RoundedLoopOffset.h"
#include "SketchGeometry.h"
#include <BRepAdaptor_Curve.hxx>
#include <BRepFilletAPI_MakeFillet.hxx>
#include <BRep_Tool.hxx>
#include <TopExp.hxx>
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
        if(argc!=3 && argc!=4)throw std::runtime_error("usage: enclosure_rim_continuation_test source.alloy output.json [fillet|contour]");
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
            const auto mode=QString::fromUtf8(argv[3]);
            if(mode!="fillet" && mode!="contour")throw std::runtime_error("unsupported diagnostic mode");
            QStringList selected;
            for(const auto& rail:rails) {
                const bool rear=std::abs(rail.length-54)<1e-7 &&
                    (std::abs(rail.a.Y()+45.25)<1e-7 || std::abs(rail.a.Y()+44.25)<1e-7);
                if(rear)selected.append(rail.id);
            }
            if(selected.size()!=2)throw std::runtime_error("rear opposing 54mm rail pair unavailable");
            result["selectedRearPair"]=QJsonArray{selected[0],selected[1]};
            result["attemptedRadiusMm"]=0.5;
            if(mode=="contour") {
                for(const auto& body:document.bodies())if(body.bodyId==support.bodyId) {
                    BRepFilletAPI_MakeFillet builder(body.shape);
                    QJsonArray seedContours;
                    for(const auto& id:selected)for(const auto& edge:body.solidEdges)if(edge.edgeId==id) {
                        builder.Add(0.5,edge.edge);
                        seedContours.append(QJsonObject{{"edgeId",id},{"contour",builder.Contour(edge.edge)}});
                    }
                    result["seedContours"]=seedContours;
                    QJsonArray contours;int expandedCount=0;
                    for(int index=1;index<=builder.NbContours();++index) {
                        builder.SetRadius(0.5,index,1);
                        QJsonArray chain;
                        for(int edgeIndex=1;edgeIndex<=builder.NbEdges(index);++edgeIndex) {
                            const auto actual=builder.Edge(index,edgeIndex);
                            const BRepAdaptor_Curve curve(actual);
                            const auto start=curve.Value(curve.FirstParameter()),end=curve.Value(curve.LastParameter());
                            QJsonArray identities;
                            for(const auto& edge:body.solidEdges)if(!edge.edge.IsNull() && edge.edge.IsSame(actual))
                                identities.append(QJsonObject{{"edgeId",edge.edgeId},{"provenance",edge.provenance}});
                            chain.append(QJsonObject{{"index",edgeIndex},{"start",point(start)},{"end",point(end)},
                                {"identities",identities}});
                            ++expandedCount;
                        }
                        contours.append(QJsonObject{{"index",index},{"edges",chain}});
                    }
                    result["contours"]=contours;result["expandedEdgeCount"]=expandedCount;
                    try {builder.Build();result["buildThrew"]=false;}
                    catch(const Standard_Failure& error) {
                        result["buildThrew"]=true;result["kernelException"]=QString::fromUtf8(error.GetMessageString());
                    }
                    result["isDone"]=builder.IsDone();result["hasPartialResult"]=builder.HasResult();
                    QJsonArray statuses,faultyContours;
                    for(int index=1;index<=builder.NbContours();++index)
                        statuses.append(QJsonObject{{"index",index},{"stripeStatus",static_cast<int>(builder.StripeStatus(index))}});
                    for(int index=1;index<=builder.NbFaultyContours();++index)faultyContours.append(builder.FaultyContour(index));
                    result["statuses"]=statuses;result["faultyContours"]=faultyContours;
                    result["faultyVertexCount"]=builder.NbFaultyVertices();
                    QJsonArray faultyVertices;
                    for(int index=1;index<=builder.NbFaultyVertices();++index) {
                        const auto vertex=builder.FaultyVertex(index);
                        QJsonArray incidentEdges,incidentEdgeIds;
                        for(const auto& edge:body.solidEdges)if(!edge.edge.IsNull()) {
                            TopoDS_Vertex first,last;TopExp::Vertices(edge.edge,first,last);
                            if((!first.IsNull() && first.IsSame(vertex)) || (!last.IsNull() && last.IsSame(vertex)))
                                {incidentEdgeIds.append(edge.edgeId);
                                 incidentEdges.append(QJsonObject{{"edgeId",edge.edgeId},{"provenance",edge.provenance}});}
                        }
                        faultyVertices.append(QJsonObject{{"position",point(BRep_Tool::Pnt(vertex))},
                            {"tolerance",BRep_Tool::Tolerance(vertex)},
                            {"incidentEdgeIds",incidentEdgeIds},{"incidentEdges",incidentEdges}});
                    }
                    result["faultyVertices"]=faultyVertices;
                }
                file.write(QJsonDocument(result).toJson(QJsonDocument::Indented));
                return result.value("isDone").toBool() ? 0 : 2;
            }
            try {
                const auto candidate=CadDocument::filletCandidate(document.authored(),
                    {{"ownerId",owner},{"targetBodyId",support.bodyId},
                     {"edgeIds",QJsonArray{selected[0],selected[1]}},{"radiusMm",0.5}});
                const auto error=document.commit(candidate,"Two upper rim rails R0.5");
                if(!error.isEmpty())throw std::runtime_error(error.toStdString());
            } catch(const std::exception& error) {
                result["filletAccepted"]=false;result["filletError"]=QString::fromUtf8(error.what());
                file.write(QJsonDocument(result).toJson(QJsonDocument::Indented));
                qCritical("two-edge R0.5 refused: %s",error.what());return 2;
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
