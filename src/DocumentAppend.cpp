// SPDX-License-Identifier: MIT
#include "DocumentAppend.h"
#include <QJsonArray>
namespace {
QJsonObject metadata(QJsonObject document){document.remove("features");return document;}
bool equalPrefix(const QJsonArray& before,const QJsonArray& after,const QString& hiddenSketch){
    for(int i=0;i<before.size();++i){
        auto row=after[i].toObject();const auto old=before[i].toObject();
        if(old["id"]==hiddenSketch && !hiddenSketch.isEmpty()){
            if(old["type"]!="sketch" || old["visible"]!=true || row["visible"]!=false)return false;
            row["visible"]=true;
        }
        if(row!=old)return false;
    }
    return true;
}
}
std::optional<DocumentAppend::Plan> DocumentAppend::classify(const QJsonObject& current,
    const QJsonObject& candidate,const QJsonObject& resolvedCurrent,const QJsonObject& resolvedCandidate,
    const std::vector<QString>& evaluatedOrder){
    const auto before=current["features"].toArray(),after=candidate["features"].toArray();
    if(before.isEmpty() || after.size()!=before.size()+1 || evaluatedOrder.size()!=static_cast<size_t>(before.size())
        || metadata(current)!=metadata(candidate) || metadata(resolvedCurrent)!=metadata(resolvedCandidate))return std::nullopt;
    for(int i=0;i<before.size();++i)
        if(evaluatedOrder[static_cast<size_t>(i)]!=before[i].toObject()["id"].toString())return std::nullopt;
    const auto tail=after.last().toObject(),p=tail["parameters"].toObject();
    const bool cut=tail["type"]=="extrude" && p["operation"]=="cut";
    if(tail["type"]!="constructionPlane" && tail["type"]!="sketch" && !cut)return std::nullopt;
    QString hiddenSketch;
    if(cut)for(int i=0;i<before.size();++i){const auto old=before[i].toObject(),row=after[i].toObject();
        if(old["id"]==p["sketchId"] && old["visible"]==true && row["visible"]==false)hiddenSketch=old["id"].toString();}
    if(!equalPrefix(before,after,hiddenSketch)
        || !equalPrefix(resolvedCurrent["features"].toArray(),resolvedCandidate["features"].toArray(),hiddenSketch))return std::nullopt;
    return Plan{static_cast<int>(before.size()),hiddenSketch};
}
