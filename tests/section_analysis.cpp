#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QFile>
#include <QJsonDocument>
#include <iostream>
#include <stdexcept>
static void check(bool v,const char* m){if(!v)throw std::runtime_error(m);}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    CadDocument doc;auto sketch=SketchGeometry::newFeature(doc.authored()["documentId"].toString(),"XY","Section stock");
    sketch=SketchGeometry::addRectangle(sketch,{{"u1Mm",-20},{"v1Mm",-20},{"u2Mm",20},{"v2Mm",20}});
    check(doc.commit(SketchGeometry::replaceFeature(doc.authored(),sketch),"Sketch").isEmpty(),"sketch");
    check(doc.commit(CadDocument::extrusionCandidate(doc.authored(),{{"sketchId",sketch["id"]},{"profileId",doc.sketches()[0].profiles[0].profileId},{"distanceMm",40}}),"Solid").isEmpty(),"solid");
    const auto end=doc.bodies()[0].supportFaces[0];
    QJsonObject support{{"producerFeatureId",end.producerFeatureId},{"bodyId",end.bodyId},{"faceId",end.faceId}};
    auto faceDraft=CadDocument::sectionCandidate(doc.authored(),{{"plane","face"},{"support",support},{"offsetMm",-2.5},{"flip",true},{"name","Face slice"}});
    check(doc.commitAnalyses(faceDraft["analyses"].toArray(),"Face section").isEmpty(),"face section");
    check(doc.authored()["analyses"].toArray()[0].toObject()["offsetMm"].toDouble()==-2.5,"signed offset retained");
    doc.history().undo();check(doc.authored()["analyses"].toArray().isEmpty(),"face section undo");
    auto malformed=faceDraft;auto malformedRows=malformed["analyses"].toArray();auto malformedRow=malformedRows[0].toObject();malformedRow["offsetMm"]="bad";malformedRows[0]=malformedRow;
    check(!doc.commitAnalyses(malformedRows,"Bad face offset").isEmpty(),"reject malformed face offset");
    const auto before=doc.authored();const auto* evaluated=&doc.evaluation();const auto shape=doc.bodies()[0].shape;
    auto candidate=CadDocument::sectionCandidate(before,{{"plane","XZ"}});auto rows=candidate["analyses"].toArray();const auto id=rows[0].toObject()["id"];
    check(doc.commitAnalyses(rows,"Section").isEmpty(),"create");
    check(doc.authored()["analyses"].toArray()[0].toObject().size()==6,"legacy origin record retained");
    check(doc.authored()["features"]==before["features"] && &doc.evaluation()==evaluated && doc.bodies()[0].shape.IsSame(shape),"inspection preserves exact authoring and evaluation");
    const auto legacy=doc.authored();candidate=CadDocument::sectionCandidate(legacy,{{"analysisId",id},{"offsetMm",32.0},{"flip",true}});
    const auto flipOnly=CadDocument::sectionCandidate(legacy,{{"analysisId",id},{"flip",true}})["analyses"].toArray()[0].toObject();
    check(flipOnly["offsetMm"].toDouble()==0.0 && flipOnly["flip"].toBool(),"flip-only origin defaults zero offset");
    check(doc.commitAnalyses(candidate["analyses"].toArray(),"Move origin section").isEmpty(),"origin offset");
    const auto moved=doc.authored()["analyses"].toArray()[0].toObject();
    check(moved.size()==8 && moved["plane"]=="XZ" && moved["offsetMm"].toDouble()==32.0 && moved["flip"].toBool(),"origin transform retained");
    auto invalidOrigin=moved;invalidOrigin["flip"]="bad";check(!doc.commitAnalyses(QJsonArray{invalidOrigin},"Bad origin flip").isEmpty(),"reject malformed origin flip");
    {QTemporaryDir roundTrip;const auto path=roundTrip.filePath("offset.alloy");check(doc.save(path).isEmpty(),"save origin offset");CadDocument reopened;check(reopened.open(path).isEmpty() && reopened.authored()["analyses"]==doc.authored()["analyses"],"reopen origin offset");}
    doc.history().undo();check(doc.authored()==legacy,"origin offset undo");doc.history().redo();check(doc.authored()["analyses"].toArray()[0].toObject()==moved,"origin offset redo");doc.history().undo();
    const auto first=doc.authored();candidate=CadDocument::sectionCandidate(first,{{"plane","YZ"}});check(doc.commitAnalyses(candidate["analyses"].toArray(),"Replace").isEmpty(),"replace");
    check(!doc.authored()["analyses"].toArray()[0].toObject()["visible"].toBool(),"replacement hides old");const auto second=doc.authored();
    doc.history().undo();check(doc.authored()==first && &doc.evaluation()==evaluated,"undo exact");doc.history().redo();check(doc.authored()==second && &doc.evaluation()==evaluated,"redo exact");
    candidate=CadDocument::sectionCandidate(doc.authored(),{{"analysisId",id},{"visible",true},{"hatch",false},{"plane","XY"}});check(doc.commitAnalyses(candidate["analyses"].toArray(),"Edit").isEmpty(),"edit");
    const auto stable=doc.authored();const auto rev=doc.revision();const auto count=doc.history().count();rows=stable["analyses"].toArray();auto bad=rows[0].toObject();bad["plane"]="bad";rows[0]=bad;
    check(!doc.commitAnalyses(rows,"Bad").isEmpty() && doc.authored()==stable && doc.revision()==rev && doc.history().count()==count,"atomic invalid plane");
    rows=stable["analyses"].toArray();bad=rows[0].toObject();bad["id"]=sketch["id"];rows[0]=bad;check(!doc.commitAnalyses(rows,"Duplicate").isEmpty(),"global identity");
    rows=stable["analyses"].toArray();bad=rows[1].toObject();bad["visible"]=true;rows[1]=bad;check(!doc.commitAnalyses(rows,"Two").isEmpty(),"single active");
    QTemporaryDir dir;const auto path=dir.filePath("section.alloy");check(doc.save(path).isEmpty(),"save");CadDocument reopen;check(reopen.open(path).isEmpty() && reopen.authored()==stable,"reopen exact");
    auto ordinary=doc.authored();ordinary["displayUnits"]="in";check(doc.commit(ordinary,"Units").isEmpty() && doc.authored()["analyses"]==stable["analyses"],"model edit preserves analyses");
    bad=stable;bad["analyses"]="bad";QFile f(dir.filePath("bad.alloy"));check(f.open(QIODevice::WriteOnly),"file");f.write(QJsonDocument(bad).toJson());f.close();const auto reopened=reopen.authored();check(!reopen.open(f.fileName()).isEmpty() && reopen.authored()==reopened,"open atomic");
    std::cout<<"Section analysis contracts passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
