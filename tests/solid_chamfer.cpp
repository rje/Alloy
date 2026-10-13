#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
#include <stdexcept>
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try {
    CadDocument doc;auto sketch=SketchGeometry::newFeature(doc.authored()["documentId"].toString(),"XY","Cube");
    sketch=SketchGeometry::addRectangle(sketch,{{"u1Mm",-10},{"v1Mm",-10},{"u2Mm",10},{"v2Mm",10}});
    check(doc.commit(SketchGeometry::replaceFeature(doc.authored(),sketch),"Sketch").isEmpty(),"sketch");
    QJsonObject extrude{{"sketchId",sketch["id"]},{"profileId",doc.sketches()[0].profiles[0].profileId},{"distanceMm",20}};
    check(doc.commit(CadDocument::extrusionCandidate(doc.authored(),extrude),"Cube").isEmpty(),"cube");
    const auto before=doc.authored();const auto body=doc.bodies()[0].bodyId;QJsonArray edges;for(const auto& edge:doc.bodies()[0].solidEdges)edges.append(edge.edgeId);
    check(edges.size()==12,"twelve retained edges");QJsonObject args{{"targetBodyId",body},{"edgeIds",edges},{"distanceMm",1}};
    const auto candidate=CadDocument::chamferCandidate(before,args);const auto revision=doc.revision();const auto count=doc.history().count();
    auto preview=CadDocument::evaluateCandidate(candidate);check(preview.ok,"all-edge equal chamfer preview");const auto summary=preview.evaluation->bodies[0].summary;
    check(summary["valid"].toBool() && summary["closed"].toBool() && summary["topology"].toObject()["solids"]==1,"valid closed solid");
    check(summary["volumeMm3"].toDouble()<8000 && summary["volumeMm3"].toDouble()>7800,"real planar cuts");
    check(doc.authored()==before && doc.revision()==revision && doc.history().count()==count,"disposable preview preserves authority");
    check(doc.commit(candidate,"Chamfer").isEmpty(),"commit");const auto committed=doc.authored();const auto id=committed["features"].toArray().last().toObject()["id"];
    check(doc.bodies()[0].bodyId==body,"retained body");doc.history().undo();check(doc.authored()==before,"one undo");doc.history().redo();check(doc.authored()==committed,"exact redo");
    args["featureId"]=id;args["distanceMm"]=.5;check(doc.commit(CadDocument::chamferCandidate(doc.authored(),args),"Edit chamfer").isEmpty(),"distance edit");doc.history().undo();
    extrude["featureId"]=before["features"].toArray().last().toObject()["id"];extrude["distanceMm"]=24;
    check(doc.commit(CadDocument::extrusionCandidate(doc.authored(),extrude),"Height edit").isEmpty(),"upstream size edit retains all edges");doc.history().undo();
    const auto stable=doc.authored();const auto stableRevision=doc.revision();const auto stableHistory=doc.history().count();args["distanceMm"]=30;
    check(!doc.commit(CadDocument::chamferCandidate(stable,args),"Impossible").isEmpty(),"impossible distance refused");
    check(doc.authored()==stable && doc.revision()==stableRevision && doc.history().count()==stableHistory,"failed geometry atomic");
    args["distanceMm"]=1;args["edgeIds"]=QJsonArray{CadDocument::newId()};const auto missing=CadDocument::evaluateCandidate(CadDocument::chamferCandidate(stable,args));check(!missing.ok && missing.diagnostics["code"]=="chamfer_edge_unavailable","missing edge diagnostic");
    QTemporaryDir tmp;auto path=tmp.filePath("chamfer.alloy");check(doc.save(path).isEmpty(),"save");CadDocument reopened;check(reopened.open(path).isEmpty() && reopened.authored()==stable,"exact reopen");
    check(std::abs(reopened.bodies()[0].summary["volumeMm3"].toDouble()-summary["volumeMm3"].toDouble())<1e-6,"reopen geometry");
    std::cout<<"Equal-distance Chamfer preview/edit/upstream/history/storage/atomic checks passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
