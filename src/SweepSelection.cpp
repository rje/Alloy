// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "ReferenceView.h"
#include "SketchGeometry.h"
#include <QSet>
#include <limits>
#include <cmath>
#include <algorithm>
namespace {
QJsonObject failure(const QString& code,const QString& message){return {{"ok",false},{"error",QJsonObject{{"code",code},{"message",message}}}};}
gp_Pnt point(const QJsonArray& a){return {a[0].toDouble(),a[1].toDouble(),a[2].toDouble()};}
}
EvaluatedDocument AlloyWindow::sweepInputDocument() const {const auto id=session_.value("inputs").toObject().value("featureId").toString();return id.isEmpty()?document_->evaluation():document_->evaluationBefore(id);}
QJsonObject AlloyWindow::sweepPathSelection(const EvaluatedSketch& sketch,const QJsonArray& seeds,bool expand) const {
    QMap<QString,gp_Pnt> positions;for(const auto& q:sketch.summary["worldPoints"].toArray()){const auto p=q.toObject();positions[p["id"].toString()]=point(p["positionMm"].toArray());}
    QMap<QString,QJsonObject> entities;for(const auto& q:sketch.summary["entities"].toArray()){const auto e=q.toObject();if((e["type"]=="line" || e["type"]=="arc") && !e["construction"].toBool())entities[e["id"].toString()]=e;}
    QMap<QString,QString> roots;for(auto i=positions.begin();i!=positions.end();++i)roots[i.key()]=i.key();
    auto merge=[&](const QString& a,const QString& b){const auto keep=std::min(roots.value(a),roots.value(b)),remove=std::max(roots.value(a),roots.value(b));if(keep.isEmpty())return;for(auto i=roots.begin();i!=roots.end();++i)if(i.value()==remove)i.value()=keep;};
    const auto source=SketchGeometry::findFeature(document_->authored(),sketch.featureId);for(const auto& q:source["parameters"].toObject()["constraints"].toArray()){const auto c=q.toObject();const auto ids=c["pointIds"].toArray();if(c["type"]=="coincident" && ids.size()==2)merge(ids[0].toString(),ids[1].toString());}
    QSet<QString> endpointSet;for(const auto& e:entities){endpointSet.insert(e["startPointId"].toString());endpointSet.insert(e["endPointId"].toString());}const auto endpoints=endpointSet.values();
    for(int i=0;i<endpoints.size();++i)for(int j=i+1;j<endpoints.size();++j){const auto a=positions.value(endpoints[i]),b=positions.value(endpoints[j]);const double scale=std::max({1.,std::abs(a.X()),std::abs(a.Y()),std::abs(a.Z()),std::abs(b.X()),std::abs(b.Y()),std::abs(b.Z())});if(a.Distance(b)<=64*std::numeric_limits<double>::epsilon()*scale)merge(endpoints[i],endpoints[j]);}
    QMap<QString,QStringList> incident;for(auto i=entities.begin();i!=entities.end();++i){incident[roots.value(i.value()["startPointId"].toString())].append(i.key());incident[roots.value(i.value()["endPointId"].toString())].append(i.key());}
    QSet<QString> selected;QStringList queue;for(const auto& q:seeds){const auto id=q.toString();if(!entities.contains(id))return failure("unsupported_path_entity","Select ordinary finished-sketch lines or arcs for Sweep Path");selected.insert(id);queue.append(id);}
    if(expand)for(int i=0;i<queue.size();++i){const auto e=entities[queue[i]];for(const auto& field:{"startPointId","endPointId"})for(const auto& next:incident.value(roots.value(e[field].toString())))if(!selected.contains(next)){selected.insert(next);queue.append(next);}}
    QMap<QString,QStringList> chosen;for(const auto& id:selected){const auto e=entities[id];chosen[roots.value(e["startPointId"].toString())].append(id);chosen[roots.value(e["endPointId"].toString())].append(id);}
    QStringList open;for(auto i=chosen.begin();i!=chosen.end();++i){if(i.value().size()>2)return failure("branched_path","Chain Selection cannot choose a route through a branch. Select an unbranched source or disable Chain Selection and choose a path explicitly.");if(i.value().size()==1)open.append(i.key());}
    if(open.size()!=2)return failure("disconnected_path","Select one connected open chain; closed loops and gaps are not automatically repaired");
    const auto inputs=session_.value("inputs").toObject();QJsonObject circle;for(const auto& profileSketch:sweepInputDocument().sketches)if(profileSketch.featureId==inputs["profileSketchId"])for(const auto& profile:profileSketch.profiles)if(profile.profileId==inputs["profileId"]){const auto ids=profile.summary["sourceEntityIds"].toArray();if(ids.size()==1)for(const auto& q:profileSketch.summary["entities"].toArray())if(q.toObject()["id"]==ids.first())circle=q.toObject();}
    if(circle["type"]!="circle")return failure("profile_required","Select a circular disk Profile before its Path");const auto center=point(circle["centerMm"].toArray());QString start;int matches=0;
    for(const auto& endpoint:open){QString actual;for(const auto& id:selected){const auto e=entities[id];for(const auto& field:{"startPointId","endPointId"})if(roots.value(e[field].toString())==endpoint){actual=e[field].toString();break;}if(!actual.isEmpty())break;}if(!actual.isEmpty() && positions[actual].Distance(center)<=1e-7){start=actual;++matches;}}
    if(matches!=1)return failure("profile_start_misaligned","Selected Path must have exactly one endpoint at the circular Profile center; select the starting section or correct the source");
    QJsonArray ordered;QSet<QString> walked;QString current=roots.value(start);while(walked.size()<selected.size()){QString next;for(const auto& id:chosen.value(current))if(!walked.contains(id)){if(!next.isEmpty())return failure("branched_path","Path traversal is ambiguous");next=id;}if(next.isEmpty())return failure("disconnected_path","Selected sections do not make one continuous Path");const auto e=entities[next];ordered.append(next);walked.insert(next);current=roots.value(e[roots.value(e["startPointId"].toString())==current?"endPointId":"startPointId"].toString());}
    return {{"pathSketchId",sketch.featureId},{"pathEntityIds",ordered},{"pathStartPointId",start}};
}
QJsonObject AlloyWindow::selectSweep(const QJsonObject& a){
    if(session_.value("command")!="solid.sweep")return failure("invalid_session","Start Sweep before selecting references");
    if(a.value("clear").toBool() && !a.contains("role"))return failure("invalid_arguments","Choose the role to clear");
    const auto role=a.value("role").toString(session_.value("selectionRole").toString());auto inputs=session_.value("inputs").toObject();
    if(role=="path" && !inputs.contains("profileId"))return failure("profile_required","Select the circular Profile first");
    if(a.contains("chainSelection"))session_["chainSelection"]=a["chainSelection"];
    if(a.value("clear").toBool()){if(role=="profile"){inputs.remove("profileSketchId");inputs.remove("profileId");}inputs.remove("pathSketchId");inputs.remove("pathEntityIds");inputs.remove("pathStartPointId");session_["inputs"]=inputs;session_.remove("pathSeedIds");}
    session_["selectionRole"]=role;session_.remove("selectionError");if(a.value("clear").toBool())previewSweep();displaySweepSelection();refreshSweepControls();view_->setFocus();return session_;
}
QJsonObject AlloyWindow::pickSweep(double x,double y){
    auto reject=[this](const QString& code,const QString& message){auto r=failure(code,message);session_["selectionError"]=r["error"];refreshSweepControls();return r;};
    auto inputs=session_.value("inputs").toObject();const auto available=sweepInputDocument();
    if(session_.value("selectionRole")=="profile"){
        const auto hit=view_->pickFinishedProfile(x,y);if(hit.isEmpty())return reject("profile_required","Click inside a circular finished-sketch Profile");bool circle=false;
        for(const auto& sketch:available.sketches)if(sketch.featureId==hit["sketchId"] && sketch.ownerId==document_->activeOwnerId())for(const auto& profile:sketch.profiles)if(profile.profileId==hit["profileId"]){const auto ids=profile.summary["sourceEntityIds"].toArray();if(ids.size()==1)for(const auto& q:sketch.summary["entities"].toArray())if(q.toObject()["id"]==ids.first() && q.toObject()["type"]=="circle")circle=true;}
        if(!circle)return reject("unsupported_profile","This Sweep supports a circular disk Profile in the active component");
        if(inputs["profileId"]!=hit["profileId"]){inputs.remove("pathSketchId");inputs.remove("pathEntityIds");inputs.remove("pathStartPointId");session_.remove("pathSeedIds");}inputs["profileSketchId"]=hit["sketchId"];inputs["profileId"]=hit["profileId"];session_["inputs"]=inputs;session_["selectionRole"]="path";session_.remove("selectionError");previewSweep();auto result=hit;result["role"]="profile";result["inputs"]=inputs;return result;
    }
    QJsonObject hit;const EvaluatedSketch* selected=nullptr;double nearest=7;
    for(const auto& sketch:available.sketches)if(sketch.ownerId==document_->activeOwnerId()){const auto source=SketchGeometry::findFeature(document_->authored(),sketch.featureId);const auto q=view_->pickSketch(source,x,y,"entity",sketch.resolvedFrame);if(!q.isEmpty() && !q["projected"].toBool() && q["distancePixels"].toDouble()<nearest){nearest=q["distancePixels"].toDouble();hit=q;selected=&sketch;}}
    if(!selected)return reject("path_required","Click an ordinary finished-sketch line or arc for Path");
    auto seeds=QJsonArray{hit["entityId"]};if(!session_.value("chainSelection").toBool() && inputs["pathSketchId"]==selected->featureId){seeds=inputs["pathEntityIds"].toArray();if(!seeds.contains(hit["entityId"]))seeds.append(hit["entityId"]);}
    const auto refs=sweepPathSelection(*selected,seeds,session_.value("chainSelection").toBool());if(refs.value("ok").isBool() && !refs.value("ok").toBool())return reject(refs["error"].toObject()["code"].toString(),refs["error"].toObject()["message"].toString());
    for(auto it=refs.begin();it!=refs.end();++it)inputs[it.key()]=it.value();session_["inputs"]=inputs;session_["pathSeedIds"]=seeds;session_.remove("selectionError");const auto preview=previewSweep();auto result=hit;result["sketchId"]=selected->featureId;result["role"]="path";result["inputs"]=inputs;result["chainSelection"]=session_.value("chainSelection");result["previewed"]=preview.value("previewed");return result;
}
