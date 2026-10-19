// SPDX-License-Identifier: MIT
#include "failed_edit_repair_fixtures.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <iostream>
using MultiFaceFixtures::check;
int main(int argc,char** argv) {
    QCoreApplication app(argc,argv);
    try {
        QTemporaryDir scratch;const auto metadata=FailedEditRepairFixtures::createBoundary(scratch.path());
        const auto ids=metadata.value("failure").toObject();CadDocument doc;
        check(doc.open(scratch.path()+"/boundary-failure.alloy").isEmpty(),"valid saved boundary fixture opens");
        const auto before=doc.state();
        const auto candidate=CadDocument::booleanCandidate(doc.authored(),{{"featureId",ids.value("combineFeatureId")},
            {"targetBodyId",ids.value("targetBodyId")},{"toolBodyIds",QJsonArray{ids.value("splitBodyId")}},
            {"operation","cut"},{"keepTools",true}});
        check(!doc.commit(candidate,"Changed boundary").isEmpty(),"changed boundary was accepted");
        const auto diagnostics=doc.lastEvaluationError();
        check(diagnostics.value("code")=="ambiguous_inset_lineage"
            && diagnostics.value("message")=="Inset source boundary does not have complete retained edges","boundary error changed");
        check(diagnostics.value("featureId")==ids.value("sketchId") && diagnostics.value("featureType")=="sketch","missing authoritative sketch");
        for(const auto& key:{"bodyId","producerFeatureId","faceId"})
            check(diagnostics.value(key)==ids.value("support").toObject().value(key),"missing exact support context");
        check(doc.state()==before,"failed boundary changed committed authored/revision/history/geometry");
        auto prefix=candidate;auto rows=prefix.value("features").toArray();rows.removeLast();prefix["features"]=rows;
        const auto evaluated=CadDocument::evaluateCandidate(prefix);check(evaluated.ok,"failed candidate prefix invalid");
        const auto support=GeometryKernel::resolveSketchSupport(*evaluated.evaluation,ids.value("support").toObject());
        check(support.faceId==ids.value("support").toObject().value("faceId").toString(),"exact face no longer uniquely resolves");
        const auto sketch=SketchGeometry::findFeature(doc.authored(),ids.value("sketchId").toString());
        const auto profile=doc.sketches().back().profiles.front().profileId;
        bool refused=false;
        try {RoundedLoopOffset::deleteProfile(sketch,support,QJsonArray{profile});}
        catch(const GeometryKernel::EvaluationError& error) {refused=error.diagnostics().value("code")=="ambiguous_inset_lineage";}
        check(refused,"existing owner validation did not refuse changed boundary");
        const auto accepted=GeometryKernel::resolveSketchSupport(doc.evaluation(),ids.value("support").toObject());
        check(!RoundedLoopOffset::deleteProfile(sketch,accepted,QJsonArray{profile}).value("parameters").toObject().contains("loopOffsets"),"ordinary valid owner Delete failed");
        std::cout<<"PASS: contextual boundary failure, unique exact support, atomic document and existing owner validation\n";
        return 0;
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
