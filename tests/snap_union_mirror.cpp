// SPDX-License-Identifier: MIT
// Technical replay gate for a connected selected-profile Join source.
#include "CadDocument.h"
#include <QCoreApplication>
#include <QJsonDocument>
#include <BRepAdaptor_Curve.hxx>
#include <BRepAlgoAPI_Common.hxx>
#include <BRepGProp.hxx>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <GProp_GProps.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <cmath>
#include <iostream>
#include <stdexcept>
static void check(bool yes,const char* why){if(!yes)throw std::runtime_error(why);}
static double overlap(const TopoDS_Shape& a,const TopoDS_Shape& b){
    BRepAlgoAPI_Common shared(a,b);shared.SetNonDestructive(true);shared.Build();
    check(shared.IsDone() && !shared.HasErrors(),"Snap interference Boolean failed");
    GProp_GProps volume;BRepGProp::VolumeProperties(shared.Shape(),volume);return volume.Mass();
}
static QString reflectedRoot(const CadDocument& doc,const QString& bodyId,const QString& mirrorId){
    const auto& bodies=doc.bodies();const auto body=std::find_if(bodies.begin(),bodies.end(),
        [&](const auto& value){return value.bodyId==bodyId;});
    check(body!=bodies.end(),"Mirrored snap body unavailable");
    TopTools_IndexedMapOfShape current;TopExp::MapShapes(body->shape,TopAbs_EDGE,current);
    QString found;int count=0;
    for(const auto& edge:body->solidEdges){
        if(edge.provenance["role"]!="join.root" || edge.provenance["mirrorFeatureId"]!=mirrorId)continue;
        check(current.Contains(edge.edge) && !edge.provenance["sourceToolEdgeId"].toString().isEmpty()
            && !edge.provenance["targetFaceId"].toString().isEmpty(),"Reflected root lacks exact current ancestry");
        BRepAdaptor_Curve line(edge.edge);const auto a=line.Value(line.FirstParameter()),b=line.Value(line.LastParameter());
        if(std::abs(a.X()-28.5)<1e-6 && std::abs(b.X()-28.5)<1e-6
            && std::abs(a.Y()-17.5)<1e-6 && std::abs(b.Y()-17.5)<1e-6
            && ((std::abs(a.Z()-15)<1e-6 && std::abs(b.Z()-20.5)<1e-6)
                || (std::abs(b.Z()-15)<1e-6 && std::abs(a.Z()-20.5)<1e-6))){found=edge.edgeId;++count;}
    }
    check(count==1,"Reflected wall/stem attachment root is not unique");return found;
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    check(argc==2 || argc==3,"Pass actual five-feature source and optional Mirror output");
    CadDocument doc;const auto openError=doc.open(QString::fromLocal8Bit(argv[1]));
    if(!openError.isEmpty())throw std::runtime_error(("Open source failed: "+openError).toStdString());
    const auto features=doc.authored()["features"].toArray();check(features.size()>=5,"Source feature count");
    const auto join=features[features.size()-5].toObject();
    check(join["type"]=="extrude" && join["parameters"].toObject()["profileIds"].toArray().size()==3,
        "Expected retained three-region Join");
    const auto joinId=join["id"].toString();
    check(doc.evaluation().joinFeatureTools.value(joinId).size()==1 &&
        doc.evaluation().joinFeatureCaps.value(joinId).size()==1,"Connected source union is unavailable");
    QJsonArray ids;for(int i=features.size()-5;i<features.size();++i)ids.append(features[i].toObject()["id"]);
    const auto rootInputs=doc.evaluation().finishFeatureInputs.value(ids[3].toString());
    const bool sourceSizeRoot=rootInputs.size()==1 && rootInputs.front().provenance["role"]=="join.root";
    if(sourceSizeRoot)check(features[features.size()-2].toObject()["parameters"].toObject()["distanceMm"]==2,
        "Source attachment finish is not authored at 2 mm");
    const auto target=join["parameters"].toObject()["targetBodyIds"].toArray().first();
    QString bottomId;
    if(sourceSizeRoot){
        const auto split=std::find_if(features.begin(),features.end(),[](const auto& value){
            return value.toObject()["type"]=="splitBody";});
        check(split!=features.end(),"Source-sized snap lacks Case Split");
        const auto bottom=split->toObject()["parameters"].toObject()["targetBodyId"].toString();
        bottomId=bottom;
        QString catchCut;
        for(auto it=split+1;it!=features.end();++it){const auto f=it->toObject(),p=f["parameters"].toObject();
            if(f["type"]=="extrude" && p["operation"]=="cut"
                && p["targetBodyIds"].toArray().contains(bottom))catchCut=f["id"].toString();}
        check(!catchCut.isEmpty(),"Source-sized Bottom catch Cut missing");
        const auto cutMirror=CadDocument::featureMirrorCandidate(doc.authored(),{{"ownerId",join["ownerId"]},
            {"targetBodyId",bottom},{"sourceFeatureIds",QJsonArray{catchCut}},
            {"mirrorPlane",QJsonObject{{"kind","origin"},{"plane","YZ"}}}});
        check(doc.commit(cutMirror,"Opposite catch Cut Mirror").isEmpty(),"Opposite catch Mirror failed");
    }
    const auto beforeBodies=doc.bodies();
    const auto candidate=CadDocument::featureMirrorCandidate(doc.authored(),{{"ownerId",join["ownerId"]},
        {"targetBodyId",target},{"sourceFeatureIds",ids},
        {"mirrorPlane",QJsonObject{{"kind","origin"},{"plane","YZ"}}}});
    const auto preview=doc.prepareCandidate(candidate);
    std::cout<<"MIRROR_PREVIEW "<<preview.ok<<' '<<preview.error.toStdString()<<' '
        <<QString::fromUtf8(QJsonDocument(preview.diagnostics).toJson(QJsonDocument::Compact)).toStdString()<<'\n';
    if(!preview.ok)return 2;
    check(doc.commit(candidate,"Union snap Mirror").isEmpty(),"Mirror commit failed");
    if(sourceSizeRoot){
        const auto targetBefore=std::find_if(beforeBodies.begin(),beforeBodies.end(),
            [&](const auto& b){return b.bodyId==target.toString();});
        const auto targetAfter=std::find_if(doc.bodies().begin(),doc.bodies().end(),
            [&](const auto& b){return b.bodyId==target.toString();});
        check(targetBefore!=beforeBodies.end() && targetAfter!=doc.bodies().end(),"Snap target body absent");
        check(targetAfter->summary["valid"].toBool() && targetAfter->summary["closed"].toBool()
            && targetAfter->summary["topology"].toObject()["solids"].toInt()==1,
            "Source-sized Snap Mirror must leave one valid closed Top solid");
        for(const auto& other:beforeBodies)if(other.bodyId!=target.toString()){
            const double previous=overlap(targetBefore->shape,other.shape);
            const double current=overlap(targetAfter->shape,other.shape);
            const double added=current-previous;
            if(std::abs(added)>1e-7){
                BRepAlgoAPI_Common common(targetAfter->shape,other.shape);common.SetNonDestructive(true);common.Build();
                Bnd_Box box;BRepBndLib::AddOptimal(common.Shape(),box);
                double x0,y0,z0,x1,y1,z1;box.Get(x0,y0,z0,x1,y1,z1);
                std::cout<<"SNAP_ADDED_INTERFERENCE "<<other.bodyId.toStdString()<<' '
                    <<other.name.toStdString()<<" before="<<previous<<" after="<<current
                    <<" added="<<added<<" bounds="<<x0<<','<<y0<<','<<z0<<".."
                    <<x1<<','<<y1<<','<<z1<<'\n'<<std::flush;
            }
            if(other.bodyId==bottomId){
                // The paired Bottom inner-rim Fillet belongs to the later
                // reorder step; both otherwise identical snap elbows have
                // the same temporary interference before that finish.
                check(previous>0 && previous<0.3 && std::abs(added-previous)<1e-6,
                    "Opposed snap overlap differs from the unrounded source catch");
            }else check(added<1e-6,"Reflected snap adds material intersection with another body");
        }
    }
    const auto accepted=doc.authored();
    const auto mirrorId=accepted["features"].toArray().last().toObject()["id"].toString();
    const auto rootId=sourceSizeRoot ? QString{} : reflectedRoot(doc,target.toString(),mirrorId);
    const auto p=join["parameters"].toObject();
    const auto edited=CadDocument::extrusionCandidate(doc.authored(),{{"featureId",joinId},
        {"sketchId",p["sketchId"]},{"profileIds",p["profileIds"]},{"distanceMm",5.25}});
    const auto editPreview=doc.prepareCandidate(edited);
    std::cout<<"JOIN_EDIT_PREVIEW "<<editPreview.ok<<' '<<editPreview.error.toStdString()<<' '
        <<QString::fromUtf8(QJsonDocument(editPreview.diagnostics).toJson(QJsonDocument::Compact)).toStdString()<<'\n';
    check(editPreview.ok,"Connected Join edit broke retained Mirror");
    check(doc.commit(edited,"Edit selected Join").isEmpty(),"Join edit commit failed");
    if(!sourceSizeRoot)check(reflectedRoot(doc,target.toString(),mirrorId)==rootId,"Join edit changed reflected root identity");
    const auto final=doc.authored();
    doc.history().undo();check(doc.authored()==accepted && (sourceSizeRoot || reflectedRoot(doc,target.toString(),mirrorId)==rootId),"Undo lost accepted Mirror root");
    doc.history().redo();check(doc.authored()==final && (sourceSizeRoot || reflectedRoot(doc,target.toString(),mirrorId)==rootId),"Redo lost edited Mirror root");
    if(argc==3)check(doc.save(QString::fromLocal8Bit(argv[2])).isEmpty(),"Mirror save failed");
    if(argc==3){CadDocument reopened;check(reopened.open(QString::fromLocal8Bit(argv[2])).isEmpty()
        && reopened.authored()==final && (sourceSizeRoot || reflectedRoot(reopened,target.toString(),mirrorId)==rootId),
        "Mirror reopen changed retained root identity");}
    std::cout<<"MIRROR_COMMITTED "<<doc.authored()["features"].toArray().size()
        <<" exactSourceRoot="<<sourceSizeRoot<<'\n';
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
