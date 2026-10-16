// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SurfaceExtrude.h"
#include "SurfaceTrim.h"
#include <BRepTools.hxx>
#include <BRep_Tool.hxx>
#include <BRepClass_FaceClassifier.hxx>
#include <BRepGProp.hxx>
#include <GeomAPI_ProjectPointOnSurf.hxx>
#include <GProp_GProps.hxx>
#include <QFile>
#include <QCoreApplication>
#include <QDir>
#include <QJsonDocument>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool ok,const QString& message){if(!ok)throw std::runtime_error(message.toStdString());}
void commit(CadDocument& d,const QJsonObject& c,const QString& label){const auto error=d.commit(c,label);
    check(error.isEmpty(),label+": "+error+" "+QString::fromUtf8(QJsonDocument(d.lastEvaluationError()).toJson(QJsonDocument::Compact)));}
const EvaluatedBody& body(const CadDocument& d,const QString& id){for(const auto& b:d.bodies())if(b.bodyId==id)return b;throw std::runtime_error("Missing body");}
QSet<QString> faceIds(const EvaluatedBody& b){QSet<QString> result;for(const auto& f:b.supportFaces)result.insert(f.faceId);return result;}
QSet<QString> edgeIds(const EvaluatedBody& b){QSet<QString> result;for(const auto& e:b.solidEdges)result.insert(e.edgeId);return result;}
QJsonObject feature(const QString& id,const QString& target,const QString& tool){return {{"id",id},{"type","surfaceTrim"},
    {"parameters",QJsonObject{{"targetBodyId",target},{"toolBodyId",tool}}}};}
QJsonArray witness(const TopoDS_Face& face){
    GProp_GProps props;BRepGProp::SurfaceProperties(face,props);const auto surface=BRep_Tool::Surface(face);
    GeomAPI_ProjectPointOnSurf project(props.CentreOfMass(),surface);
    for(int i=1;i<=project.NbPoints();++i){double u,v;project.Parameters(i,u,v);BRepClass_FaceClassifier classify(face,gp_Pnt2d(u,v),1e-7);
        if(classify.State()==TopAbs_IN){const auto p=surface->Value(u,v);return {p.X(),p.Y(),p.Z()};}}
    double u0,u1,v0,v1;BRepTools::UVBounds(face,u0,u1,v0,v1);
    for(double u:{.5,.25,.75,.125,.875})for(double v:{.5,.25,.75,.125,.875}){const gp_Pnt2d uv(u0+(u1-u0)*u,v0+(v1-v0)*v);
        BRepClass_FaceClassifier classify(face,uv,1e-7);if(classify.State()==TopAbs_IN){const auto p=surface->Value(uv.X(),uv.Y());return {p.X(),p.Y(),p.Z()};}}
    throw std::runtime_error("No interior native witness found");
}
void iron(const QString& path,const QString& out){
    CadDocument d;check(d.open(path).isEmpty(),"Open accepted native four-body source");check(d.bodies().size()==4,"Four independent source bodies");
    const auto before=d.authored();QString targetId,upperId,lowerId,extrudeId,rearSeam,frontSeam,toolSketch;
    for(const auto& b:d.bodies())if(b.supportFaces.size()==2 && b.supportFaces[0].role=="loft.closedSection")targetId=b.bodyId;
    for(const auto& v:before["features"].toArray()){const auto f=v.toObject();if(f["type"]!="surfaceExtrude")continue;extrudeId=f["id"].toString();toolSketch=f["parameters"].toObject()["sketchId"].toString();
        for(const auto& id:SurfaceExtrude::resultBodyIds(f)){if(body(d,id).supportFaces.size()==2)upperId=id;else lowerId=id;}}
    check(!targetId.isEmpty() && !upperId.isEmpty() && !lowerId.isEmpty(),"Actual bridge target and explicit tool outputs");
    for(const auto& e:body(d,targetId).solidEdges){if(e.provenance["role"]=="seam.end")rearSeam=e.edgeId;if(e.provenance["role"]=="seam.start")frontSeam=e.edgeId;}
    check(!rearSeam.isEmpty() && !frontSeam.isEmpty(),"Authored rear/front bridge seam ancestry");
    const auto source=body(d,targetId),upper=body(d,upperId),lower=body(d,lowerId);QDir().mkpath(out);
    QJsonArray operations;
    auto run=[&](const QString& tool,const QString& label,int expectedRegions,int expectedRemoved){
        const auto id=CadDocument::newId();auto f=feature(id,targetId,tool);const auto current=body(d,targetId);
        const auto split=SurfaceTrim::split(f,current,d.evaluation());check(int(split.regions.size())==expectedRegions,label+" exact region count");
        QJsonArray removed,points;QSet<QString> signatures;
        // Declared rear/excess portions: current descendants touching the rear
        // seam but not the front seam. This is an explicit fixture choice,
        // independent of area, enumeration or a default keep-largest rule.
        for(const auto& r:split.regions){check(!signatures.contains(r.signature),"Unique exact region signatures");signatures.insert(r.signature);
            if(r.removable() && r.signature.contains(rearSeam) && !r.signature.contains(frontSeam)){removed.append(r.regionId);points.append(witness(r.face));}}
        operations.append(QJsonObject{{"label",label},{"toolBodyId",tool},{"removedRegionIds",removed},{"interiorPointsMm",points}});
        check(removed.size()==expectedRemoved,label+" uniquely authored rear/excess selection");
        auto p=f["parameters"].toObject();p["removedRegionIds"]=removed;f["parameters"]=p;auto preview=current;SurfaceTrim::applySelection(preview,f,split);
        check(current.shape.IsSame(body(d,targetId).shape) && SurfaceTrim::descriptors(split).size()==expectedRegions,"Immutable partition/source");
        auto args=p;args["newFeatureId"]=id;args["ownerId"]=current.ownerId;args["name"]=label;
        commit(d,CadDocument::surfaceTrimCandidate(d.authored(),args),label);
        const auto& result=body(d,targetId);check(result.summary["valid"].toBool() && result.summary["topology"].toObject()["solids"]==0
            && result.supportFaces.size()==2 && faceIds(result)==faceIds(preview) && edgeIds(result)==edgeIds(preview),label+" authoritative selection matches preview");
        check(faceIds(body(d,upperId))==faceIds(upper) && edgeIds(body(d,upperId))==edgeIds(upper)
            && faceIds(body(d,lowerId))==faceIds(lower) && edgeIds(body(d,lowerId))==edgeIds(lower)
            && std::abs(body(d,upperId).summary["areaMm2"].toDouble()-upper.summary["areaMm2"].toDouble())<1e-8
            && std::abs(body(d,lowerId).summary["areaMm2"].toDouble()-lower.summary["areaMm2"].toDouble())<1e-8,"Tools unchanged by Trim");
        check(d.save(out+"/"+label+".alloy").isEmpty(),"Save actual trial");BRepTools::Write(result.shape,(out+"/"+label+".brep").toUtf8().constData());
        return id;
    };
    const auto upperTrim=run(upperId,"upper-trim",3,1);const auto lowerTrim=run(lowerId,"lower-trim",4,2);
    QFile receipt(out+"/selection.json");check(receipt.open(QIODevice::WriteOnly),"Write selection receipt");
    receipt.write(QJsonDocument(QJsonObject{{"targetBodyId",targetId},{"upperToolId",upperId},{"lowerToolId",lowerId},
        {"rearSeamId",rearSeam},{"frontSeamId",frontSeam},{"operations",operations}}).toJson());receipt.close();
    const auto trimmed=d.authored();const auto baseline=body(d,targetId);const auto stableFaces=faceIds(baseline),stableEdges=edgeIds(baseline);
    // Meaningful solved source edit, with authored spline membership unchanged.
    auto edited=trimmed;auto fs=edited["features"].toArray();for(int i=0;i<fs.size();++i){auto f=fs[i].toObject();if(f["id"]!=toolSketch)continue;
        auto p=f["parameters"].toObject();const auto middle=p["entities"].toArray()[0].toObject()["pointIds"].toArray()[1].toString();auto pts=p["points"].toArray();
        for(int j=0;j<pts.size();++j){auto q=pts[j].toObject();if(q["id"]==middle){q["vMm"]=q["vMm"].toDouble()+.5;pts[j]=q;}}
        p["points"]=pts;f["parameters"]=p;fs[i]=f;}edited["features"]=fs;commit(d,edited,"Edit actual upper tool spline");
    check(faceIds(body(d,targetId))==stableFaces && edgeIds(body(d,targetId))==stableEdges
        && std::abs(body(d,targetId).summary["areaMm2"].toDouble()-baseline.summary["areaMm2"].toDouble())>.01,"Source edit preserves qualified identities and changes geometry");
    d.history().undo();check(d.authored()==trimmed,"Undo source edit");d.history().redo();check(d.authored()==edited,"Redo source edit");
    check(d.save(out+"/edited-trials.alloy").isEmpty(),"Save edited trials");CadDocument reopened;
    check(reopened.open(out+"/edited-trials.alloy").isEmpty() && reopened.authored()==edited && faceIds(body(reopened,targetId))==stableFaces
        && edgeIds(body(reopened,targetId))==stableEdges,"Exact reopening");
    auto bad=CadDocument::surfaceTrimCandidate(d.authored(),{{"featureId",lowerTrim},{"removedRegionIds",QJsonArray{CadDocument::newId()}}});
    check(!d.commit(bad,"Unavailable region").isEmpty() && d.authored()==edited,"Unavailable region atomic refusal");
    // The unchanged exterior only receives a slit from these finite tools.
    const auto& exterior=d.bodies()[0];bool refused=false;try{SurfaceTrim::split(feature(CadDocument::newId(),exterior.bodyId,upperId),exterior,d.evaluation());}
    catch(const GeometryKernel::EvaluationError& e){refused=e.diagnostics()["code"]=="trim_region_ambiguous";}check(refused,"Nonseparating slit refuses rather than fake cut");
    auto duplicate=source;duplicate.supportFaces[1].faceId=duplicate.supportFaces[0].faceId;refused=false;
    try{SurfaceTrim::split(feature(CadDocument::newId(),targetId,upperId),duplicate,d.evaluation());}catch(const GeometryKernel::EvaluationError& e){refused=e.diagnostics()["code"]=="trim_lineage_ambiguous";}
    check(refused,"Duplicate source ancestry refuses");
    commit(d,CadDocument::removeBodyCandidate(d.authored(),{{"bodyId",upperId},{"ownerId",upper.ownerId}}),"Remove upper tool");
    commit(d,CadDocument::removeBodyCandidate(d.authored(),{{"bodyId",lowerId},{"ownerId",lower.ownerId}}),"Remove lower tool");
    check(d.bodies().size()==2,"Only tools removed");d.history().undo();d.history().undo();check(d.authored()==edited,"Undo Removes restores exact trials");
    d.history().undo();check(d.authored()==trimmed,"Undo source edit before trial reversion");d.history().undo();d.history().undo();
    check(d.authored()==before && d.bodies().size()==4 && faceIds(body(d,targetId))==faceIds(source),"Undo lower then upper restores exact pre-trial state");
    std::cout<<"SURFACE_CURVED_TRIM_OK unchangedActualTools InnerTarget upperThenLower multipleRearPortions sourceEdit stableIDs immutablePreview slitDuplicateUnavailableRefusal removeUndo exactRestoration reopen\n";
}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{check(argc==3,"Source/output required");iron(QString::fromLocal8Bit(argv[1]),QString::fromLocal8Bit(argv[2]));return 0;}
catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
