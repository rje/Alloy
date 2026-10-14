// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "AssemblySlider.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <iostream>
#include <stdexcept>
namespace {
void check(bool valid,const char* reason){if(!valid)throw std::runtime_error(reason);}
void commit(CadDocument& document,const QJsonObject& candidate){
    const auto error=document.commit(candidate,"Piston component test");check(error.isEmpty(),error.toUtf8().constData());
}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    CadDocument document;const auto root=document.authored()["documentId"].toString();
    const char* labels[]={"Lower arm","Upper arm","Inner piston","Outer piston"};QJsonArray bodies;
    for(int i=0;i<4;++i){auto candidate=document.authored();auto features=candidate["features"].toArray();
        const auto id=CadDocument::newId();bodies.append(id);
        features.append(QJsonObject{{"id",CadDocument::newId()},{"bodyId",id},{"ownerId",root},{"type","box"},
            {"name",labels[i]},{"visible",true},{"parameters",QJsonObject{{"widthMm",20+i},
                {"depthMm",8},{"heightMm",10},{"xMm",i*30},{"yMm",0},{"zMm",0}}}});
        candidate["features"]=features;commit(document,candidate);
    }
    const auto before=document.authored();QJsonArray originalBounds;
    for(const auto& body:document.bodies())originalBounds.append(body.summary["boundsMm"]);
    auto candidate=CadDocument::componentsFromBodiesCandidate(document.authored(),document.evaluation(),bodies);
    check(candidate["components"].toArray().size()==4,"Four definitions");commit(document,candidate);
    const auto converted=document.authored();check(document.authored()["features"]==before["features"],"Conversion rewrote geometry");
    for(int i=0;i<4;++i)check(document.bodies()[i].bodyId==bodies[i] && document.bodies()[i].summary["boundsMm"]==originalBounds[i],"World solid changed");
    candidate=CadDocument::componentCandidate(document.authored(),"Piston subassembly");
    const auto parent=candidate["components"].toArray().last().toObject()["id"].toString();commit(document,candidate);
    const auto children=document.authored()["occurrences"].toArray();
    const QJsonArray moved{children[2].toObject()["id"],children[3].toObject()["id"]};
    candidate=CadDocument::reparentOccurrenceCandidate(document.authored(),moved,parent);commit(document,candidate);
    const auto grouped=document.authored();
    check(grouped["features"]==before["features"] && document.bodies()[2].summary["boundsMm"]==originalBounds[2],"Grouping changed authored/world geometry");
    const auto revision=document.revision();
    candidate=grouped;auto bad=candidate["occurrences"].toArray();auto row=bad[4].toObject();row["parentId"]=bad[2].toObject()["definitionId"];bad[4]=row;candidate["occurrences"]=bad;
    check(!document.commit(candidate,"Cycle").isEmpty() && document.authored()==grouped && document.revision()==revision,"Cycle refusal is atomic");
    auto flat=grouped;auto flatOccurrences=flat["occurrences"].toArray();for(int i:{2,3}){
        auto row=flatOccurrences[i].toObject();row["parentId"]=root;flatOccurrences[i]=row;
    }flat["occurrences"]=flatOccurrences;
    const QJsonObject jointArguments{{"component1OccurrenceId",flatOccurrences[1].toObject()["id"]},
        {"component2OccurrenceId",flatOccurrences[0].toObject()["id"]},
        {"position",QJsonObject{{"bodyId",bodies[0]},{"edgeId",document.bodies()[0].solidEdges[0].edgeId}}},
        {"axis","Y"},{"positionMm",0}};
    check(CadDocument::evaluateCandidate(AssemblySlider::candidate(flat,jointArguments)).ok,
        "The same Slider is valid in the flat hierarchy");
    candidate=AssemblySlider::candidate(grouped,jointArguments);
    check(CadDocument::validate(candidate).contains("nested motion")
        && !document.commit(candidate,"Nested Slider").isEmpty() && document.authored()==grouped && document.revision()==revision,
        "A later Slider cannot silently pose only the parent body of a nested occurrence");
    bool opacityRefused=false;try{CadDocument::componentOpacityCandidate(grouped,parent,0.5);}catch(const std::invalid_argument&){opacityRefused=true;}
    check(opacityRefused && document.authored()==grouped,"Parent opacity must refuse until nested presentation is supported");
    document.history().undo();document.history().redo();check(document.authored()==grouped,"Grouping undo/redo");
    candidate=document.authored();auto features=candidate["features"].toArray();auto source=features[2].toObject();
    auto parameters=source["parameters"].toObject();parameters["heightMm"]=15;source["parameters"]=parameters;features[2]=source;candidate["features"]=features;commit(document,candidate);
    check(document.bodies()[2].bodyId==bodies[2] && document.bodies()[2].ownerId==converted["components"].toArray()[2].toObject()["id"].toString(),"Source edit lost body identity/owner");
    QTemporaryDir dir;check(dir.isValid(),"Temp directory");check(document.save(dir.filePath("piston.alloy")).isEmpty(),"Save");
    CadDocument reopened;check(reopened.open(dir.filePath("piston.alloy")).isEmpty() && reopened.authored()==document.authored(),"Reopen hierarchy/source edit");
    std::cout<<"piston conversion/nesting/source edit/history/reopen/cycle passed\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
