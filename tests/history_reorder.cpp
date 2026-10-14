// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include <BRepAdaptor_Curve.hxx>
#include <QCoreApplication>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
static void check(bool valid,const char* why){if(!valid)throw std::runtime_error(why);}
static const EvaluatedBody& body(const std::vector<EvaluatedBody>& bodies,const QString& id){
    for(const auto& value:bodies)if(value.bodyId==id)return value;
    throw std::runtime_error("Body unavailable");
}
static QString innerRim(const EvaluatedBody& current,double y){
    QString found;
    for(const auto& edge:current.solidEdges){
        if(edge.edge.IsNull())continue;
        BRepAdaptor_Curve curve(edge.edge);if(curve.GetType()!=GeomAbs_Line)continue;
        const auto a=curve.Value(curve.FirstParameter()),b=curve.Value(curve.LastParameter());
        if(std::abs(a.Y()-y)>1e-6 || std::abs(b.Y()-y)>1e-6
            || std::abs(a.Z()-15)>1e-6 || std::abs(b.Z()-15)>1e-6
            || std::abs(a.Distance(b)-83)>1e-6)continue;
        check(found.isEmpty(),"Inner rim identity is ambiguous");found=edge.edgeId;
    }
    check(!found.isEmpty(),"Inner rim edge unavailable");return found;
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    check(argc==2 || argc==4,"Pass post-Split snap fixture and optional before/after output paths");
    CadDocument doc;check(doc.open(QString::fromLocal8Bit(argv[1])).isEmpty(),"Source fixture open failed");
    const auto source=doc.authored()["features"].toArray();check(source.size()>=5,"Snap source group absent");
    const auto join=source[source.size()-5].toObject();
    check(join["type"]=="extrude" && join["parameters"].toObject()["operation"]=="join","Source Join absent");
    const auto top=join["parameters"].toObject()["targetBodyIds"].toArray().first().toString();
    QString bottom;for(const auto& current:doc.bodies())if(current.bodyId!=top)bottom=current.bodyId;
    check(!bottom.isEmpty(),"Separate Bottom body absent");
    const auto firstEdge=innerRim(body(doc.bodies(),bottom),28.5);
    const auto oppositeEdge=innerRim(body(doc.bodies(),bottom),-28.5);
    QJsonArray snapSources;for(int i=source.size()-5;i<source.size();++i)snapSources.append(source[i].toObject()["id"]);
    check(doc.commit(CadDocument::filletCandidate(doc.authored(),{{"targetBodyId",bottom},
        {"edgeIds",QJsonArray{firstEdge}},{"radiusMm",0.5}}),"Bottom inner Fillet").isEmpty(),
        "Source-side Bottom inner Fillet failed");
    const auto bottomFillet=doc.authored()["features"].toArray().last().toObject()["id"].toString();
    check(doc.commit(CadDocument::featureMirrorCandidate(doc.authored(),{{"targetBodyId",top},
        {"sourceFeatureIds",snapSources},{"mirrorPlane",QJsonObject{{"kind","origin"},{"plane","XZ"}}}}),
        "Snap group Mirror").isEmpty(),"Snap Mirror with Bottom interlude failed");
    const auto mirrorId=doc.authored()["features"].toArray().last().toObject()["id"].toString();
    const auto before=doc.authored();const auto beforeRevision=doc.revision();
    if(argc==4)check(doc.save(QString::fromLocal8Bit(argv[2])).isEmpty(),"Pre-reorder save failed");
    const auto reordered=CadDocument::reorderCandidate(before,bottomFillet,mirrorId);
    check(reordered!=before,"Moving Bottom Fillet after snap Mirror was a no-op");
    const auto oldOrder=before["features"].toArray(),newOrder=reordered["features"].toArray();
    check(oldOrder.size()==newOrder.size() && newOrder.last().toObject()["id"]==bottomFillet,
        "Bottom Fillet did not land immediately after Mirror");
    QList<QString> oldIds,newIds;for(const auto& f:oldOrder)oldIds.append(f.toObject()["id"].toString());
    for(const auto& f:newOrder)newIds.append(f.toObject()["id"].toString());
    std::sort(oldIds.begin(),oldIds.end());std::sort(newIds.begin(),newIds.end());
    check(oldIds==newIds,"Reorder changed retained feature identities");
    check(doc.commit(reordered,"Move Bottom Fillet").isEmpty(),"Bottom Fillet reorder failed");
    check(doc.revision()==beforeRevision+1,"Reorder was not one history transaction");
    const auto after=doc.authored();
    check(CadDocument::reorderCandidate(after,bottomFillet,mirrorId)==after,"Already-positioned move was not a no-op");
    const auto noOpRevision=doc.revision();const auto noOpCount=doc.history().count();
    check(doc.commit(after,"No-op reorder").isEmpty() && doc.revision()==noOpRevision
        && doc.history().count()==noOpCount,"No-op polluted revision or history");
    const auto existing=doc.evaluationBefore(bottomFillet);
    check(innerRim(body(existing.bodies,bottom),28.5)==firstEdge
        && innerRim(body(existing.bodies,bottom),-28.5)==oppositeEdge,
        "Retained inner rim IDs changed under reorder");
    check(doc.commit(CadDocument::filletCandidate(doc.authored(),{{"featureId",bottomFillet},
        {"targetBodyId",bottom},{"edgeIds",QJsonArray{firstEdge,oppositeEdge}},{"radiusMm",0.5}}),
        "Fillet both Bottom inner edges").isEmpty(),"Two-edge Bottom Fillet edit failed");
    const auto finished=doc.authored();
    for(const auto& id:{top,bottom}){const auto summary=body(doc.bodies(),id).summary;
        check(summary["valid"].toBool() && summary["closed"].toBool(),"Reordered split body invalid");}
    const auto invalid=CadDocument::reorderCandidate(finished,join["id"].toString(),mirrorId);
    const auto rejected=doc.prepareCandidate(invalid);
    check(!rejected.ok && rejected.diagnostics["code"]=="mirror_source_unavailable",
        "Moving selected Join after dependent Mirror was not refused");
    check(doc.authored()==finished && doc.revision()==noOpRevision+1,"Invalid reorder changed document");
    doc.history().undo();check(doc.authored()==after,"Undo did not restore one-edge Bottom Fillet");
    doc.history().undo();check(doc.authored()==before,"Undo did not restore original order");
    doc.history().redo();doc.history().redo();check(doc.authored()==finished,"Redo did not restore order and edit");
    if(argc==4){const auto path=QString::fromLocal8Bit(argv[3]);check(doc.save(path).isEmpty(),"Reordered save failed");
        CadDocument reopened;check(reopened.open(path).isEmpty() && reopened.authored()==finished,
            "Reordered reopen failed");}
    std::cout<<"PASS Bottom Fillet move after snap Mirror; both retained inner edges; history/reopen\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
