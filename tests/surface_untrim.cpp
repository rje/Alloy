// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include "SurfaceTrim.h"
#include "SurfaceUntrim.h"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QTemporaryDir>
#include <BRepTools.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <sstream>
#include <cmath>
#include <iostream>
namespace {
void check(bool b,const QString& message){if(!b)throw std::runtime_error(message.toStdString());}
void close(double a,double b){check(std::abs(a-b)<1e-6,QString("%1 != %2").arg(a,0,'g',14).arg(b,0,'g',14));}
QByteArray hash(const TopoDS_Shape& shape){std::ostringstream s;BRepTools::Write(shape,s);return QCryptographicHash::hash(QByteArray::fromStdString(s.str()),QCryptographicHash::Sha256);}
const EvaluatedBody& body(const CadDocument& doc,const QString& id){for(const auto& b:doc.bodies())if(b.bodyId==id)return b;throw std::runtime_error("body unavailable");}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    CadDocument doc;const auto owner=doc.authored().value("documentId").toString();
    auto make=[&](const QString& plane,double width,double reach){
        auto sketch=SketchGeometry::addLine(SketchGeometry::newFeature(owner,plane,"Sheet source"),{{"u1Mm",-width/2},{"v1Mm",0},{"u2Mm",width/2},{"v2Mm",0}});
        check(doc.commit(SketchGeometry::replaceFeature(doc.authored(),sketch),"Sketch").isEmpty(),"sketch commit");
        const auto entity=sketch.value("parameters").toObject().value("entities").toArray().first().toObject().value("id");
        check(doc.commit(CadDocument::surfaceExtrudeCandidate(doc.authored(),{{"sketchId",sketch.value("id")},{"entityIds",QJsonArray{entity}},{"sideOneMm",reach},{"sideTwoMm",reach}}),"Sheet").isEmpty(),"sheet commit");return sketch;
    };
    auto sketch=make("XY",40,10);const auto original=doc.bodies().back();make("XZ",60,20);const auto tool=doc.bodies().back();const auto sourceHash=hash(original.shape),toolHash=hash(tool.shape);
    if(argc>1)check(doc.save(QString::fromLocal8Bit(argv[1])).isEmpty(),"save pretrim crossing-sheet fixture");
    const auto trimId=CadDocument::newId();const QJsonObject trim{{"id",trimId},{"type","surfaceTrim"},{"parameters",QJsonObject{{"targetBodyId",original.bodyId},{"toolBodyId",tool.bodyId},{"projectionMode","surfaceIntersection"},{"removedRegionIds",QJsonArray{}}}}};
    const auto split=SurfaceTrim::split(trim,original,doc.evaluation());check(split.regions.size()==2,"two signed portions");
    check(doc.commit(CadDocument::surfaceTrimCandidate(doc.authored(),{{"newFeatureId",trimId},{"targetBodyId",original.bodyId},{"toolBodyId",tool.bodyId},{"removedRegionIds",QJsonArray{split.regions.front().regionId}}}),"Trim").isEmpty(),"trim commit");
    const auto trimmed=doc.authored();const auto current=body(doc,original.bodyId);close(current.summary.value("areaMm2").toDouble(),400);
    check(doc.evaluation().surfaceTrimInputs.contains(trimId) && hash(doc.evaluation().surfaceTrimInputs.value(trimId).shape)==sourceHash,"exact retained pretrim stage");
    const auto f=current.supportFaces.front();const QJsonObject ref{{"bodyId",current.bodyId},{"producerFeatureId",f.producerFeatureId},{"faceId",f.faceId}};
    const auto candidate=CadDocument::surfaceUntrimCandidate(trimmed,{{"sourceFace",ref},{"trimFeatureId",trimId}});const auto feature=candidate.value("features").toArray().last().toObject();const auto id=feature.value("id").toString();
    const auto preview=CadDocument::evaluateCandidate(candidate);check(preview.ok,preview.error);check(doc.authored()==trimmed && hash(body(doc,original.bodyId).shape)==hash(current.shape) && hash(original.shape)==sourceHash,"isolated preview");
    check(doc.commit(candidate,"Untrim").isEmpty(),"untrim commit");
    auto verify=[&](const EvaluatedBody& expected){const auto& restored=body(doc,original.bodyId);check(doc.bodies().size()==2 && restored.featureId==original.featureId && restored.bodyId==original.bodyId,"in-place body and producer");check(hash(restored.shape)==hash(expected.shape),"exact finite pretrim BRep restored");check(restored.supportFaces.size()==1 && restored.supportFaces.front().faceId==expected.supportFaces.front().faceId && restored.supportFaces.front().producerFeatureId==expected.featureId,"original face identity");check(restored.solidEdges.size()==4,"four original edges");for(size_t i=0;i<4;++i)check(restored.solidEdges[i].edgeId==expected.solidEdges[i].edgeId && hash(restored.solidEdges[i].edge)==hash(expected.solidEdges[i].edge),"original edge geometry and identity");check(restored.summary.value("boundsMm")==expected.summary.value("boundsMm") && restored.summary.value("topology")==expected.summary.value("topology"),"original finite bounds and topology");check(hash(body(doc,tool.bodyId).shape)==toolHash && hash(original.shape)==sourceHash,"tool and held source unchanged");};
    verify(original);close(body(doc,original.bodyId).summary.value("areaMm2").toDouble(),800);
    check(doc.authored().value("features").toArray()[4].toObject().value("id")==trimId && !feature.contains("bodyId"),"Trim retained in authored history");
    doc.history().undo();check(doc.authored()==trimmed,"undo authored trim");close(body(doc,original.bodyId).summary.value("areaMm2").toDouble(),400);doc.history().redo();verify(original);
    check(doc.commit(CadDocument::surfaceUntrimCandidate(doc.authored(),{{"featureId",id},{"name","Restore finite boundary"}}),"Edit Untrim").isEmpty(),"editable retained feature");verify(original);
    // Upstream source changes regenerate the pretrim stage rather than freeze it.
    auto p=sketch.value("parameters").toObject();auto points=p.value("points").toArray();for(int i=0;i<points.size();++i){auto point=points[i].toObject();if(point.value("uMm").toDouble()>0)point["uMm"]=25.;points[i]=point;}p["points"]=points;sketch["parameters"]=p;
    check(doc.commit(SketchGeometry::replaceFeature(doc.authored(),sketch),"Widen source").isEmpty(),"upstream source edit");const auto editedSource=doc.evaluation().surfaceTrimInputs.value(trimId);verify(editedSource);close(body(doc,original.bodyId).summary.value("areaMm2").toDouble(),900);
    const auto accepted=doc.authored();const auto revision=doc.revision();const auto acceptedHash=hash(body(doc,original.bodyId).shape);const int count=doc.history().count(),index=doc.history().index();
    auto atomic=[&]{check(doc.authored()==accepted && doc.revision()==revision && doc.history().count()==count && doc.history().index()==index && hash(body(doc,original.bodyId).shape)==acceptedHash,"atomic refusal");};
    for(const auto& key:{"bodyId","producerFeatureId","faceId"}){auto stale=ref;stale[key]=CadDocument::newId();const auto failed=CadDocument::surfaceUntrimCandidate(accepted,{{"featureId",id},{"sourceFace",stale}});const auto result=CadDocument::evaluateCandidate(failed);check(!result.ok && result.diagnostics.value("sourceFace")==stale,"structured exact-reference refusal");check(!doc.commit(failed,"Stale").isEmpty(),"stale refused");atomic();}
    const auto missing=CadDocument::surfaceUntrimCandidate(accepted,{{"featureId",id},{"trimFeatureId",CadDocument::newId()}});const auto failed=CadDocument::evaluateCandidate(missing);check(!failed.ok && failed.diagnostics.value("code")=="untrim_history_unavailable","missing Trim history refusal");check(!doc.commit(missing,"Missing").isEmpty(),"missing commit refused");atomic();
    auto prior=doc.evaluationBefore(id);auto target=prior.bodies.front();const auto priorHash=hash(target.shape);
    auto rejected=[&](const EvaluatedDocument& stage,const QString& expected){bool refused=false;try{SurfaceUntrim::evaluate(target,feature,stage);}catch(const GeometryKernel::EvaluationError& e){refused=e.diagnostics().value("code")==expected && e.diagnostics().value("sourceFace")==ref;}check(refused && hash(target.shape)==priorHash,"atomic missing/ambiguous evaluated history refusal");};
    auto absent=prior;absent.surfaceTrimInputs.clear();rejected(absent,"untrim_history_unavailable");
    auto ambiguous=prior;ambiguous.surfaceTrimInputs[trimId].supportFaces.push_back(ambiguous.surfaceTrimInputs.value(trimId).supportFaces.front());rejected(ambiguous,"untrim_history_ambiguous");
    auto duplicate=prior;duplicate.bodies.push_back(duplicate.bodies.front());rejected(duplicate,"untrim_history_ambiguous");
    for(const auto& bad:std::vector<QJsonObject>{{{"untrimType","allEdges"}},{{"trimFeatureId",""}},{{"newBodyId",CadDocument::newId()}}}){auto args=bad;args["featureId"]=id;bool refused=false;try{CadDocument::surfaceUntrimCandidate(accepted,args);}catch(const std::invalid_argument&){refused=true;}check(refused,"unsupported mode/schema refusal");atomic();}
    check(doc.commit(CadDocument::surfaceUntrimCandidate(accepted,{{"featureId",id},{"sourceFace",ref},{"trimFeatureId",trimId}}),"Retry").isEmpty(),"valid retry");verify(editedSource);
    // A consumer can use the restored original face, independently of Trim IDs.
    const auto restored=body(doc,original.bodyId);const auto originalFace=restored.supportFaces.front();const auto downstream=CadDocument::surfaceOffsetCandidate(doc.authored(),{{"sourceFace",QJsonObject{{"bodyId",restored.bodyId},{"producerFeatureId",originalFace.producerFeatureId},{"faceId",originalFace.faceId}}},{"distanceMm",3.}});check(CadDocument::evaluateCandidate(downstream).ok,"downstream original-face consumer");
    QTemporaryDir temporary;const auto path=temporary.filePath("untrim.alloy");check(doc.save(path).isEmpty(),"save");CadDocument reopened;check(reopened.open(path).isEmpty() && reopened.authored()==doc.authored(),"reopen authored history");check(hash(body(reopened,original.bodyId).shape)==hash(editedSource.shape) && body(reopened,original.bodyId).supportFaces.front().faceId==original.supportFaces.front().faceId,"reopen exact restoration");
    std::cout<<"PASS finite pretrim Untrim 800->400->800, exact source face/edges/bounds, live source900, immutable stages, strict history/refusals, Undo/Redo/reopen\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
