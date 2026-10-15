// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <stdexcept>
namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
struct Ids {QSet<QString> faces,edges,points;bool operator==(const Ids&)const=default;};
Ids ids(const EvaluatedBody& body){Ids result;for(const auto& face:body.supportFaces)result.faces.insert(face.faceId);
    for(const auto& edge:body.solidEdges){result.edges.insert(edge.edgeId);result.points.insert(edge.provenance.value("startPointId").toString());result.points.insert(edge.provenance.value("endPointId").toString());}return result;}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    check(argc==2,"Pass the accepted corner model absolute path");CadDocument doc;
    auto error=doc.open(QString::fromLocal8Bit(argv[1]));if(!error.isEmpty())throw std::runtime_error(error.toStdString());
    const auto input=doc.bodies().back();check(input.summary.value("construction")=="ruledCornerCut","Expected exact corner source");
    QJsonArray seeds;for(const auto& role:QStringList{"source","offset"})for(const auto& edge:input.solidEdges)
        if(!edge.edge.IsNull() && edge.provenance.value("role")=="roundBoundary" && edge.provenance.value("broadFaceRole")==role){seeds.append(edge.edgeId);break;}
    check(seeds.size()==2,"Both rim seeds unavailable");
    const auto candidate=CadDocument::filletCandidate(doc.authored(),{{"targetBodyId",input.bodyId},{"groups",QJsonArray{QJsonObject{{"id",CadDocument::newId()},{"edgeIds",seeds},{"radiusMm",1.5},{"tangentChain",true}}}}});
    error=doc.commit(candidate,"Full perimeter R1.5");if(!error.isEmpty())throw std::runtime_error(error.toStdString());
    const auto baseline=doc.bodies().back();
    check(baseline.summary.value("construction")=="halfThicknessPerimeter","Perimeter construction unavailable");
    check(baseline.summary.value("valid").toBool() && baseline.summary.value("closed").toBool(),"Perimeter invalid");
    check(baseline.summary.value("topology").toObject().value("faces")==10 && baseline.summary.value("topology").toObject().value("edges")==24,"Unexpected perimeter topology");
    check(baseline.supportFaces.size()==10 && baseline.solidEdges.size()==24,"Perimeter names incomplete");
    int pipeFaces=0;for(const auto& face:baseline.supportFaces)if(face.role=="fillet.perimeter"){++pipeFaces;check(face.boundaryEdges.size()==4,"Pipe boundary names incomplete");}
    check(pipeFaces==8,"Eight named pipe sectors unavailable");
    const auto baselineIds=ids(baseline);check(baselineIds.faces.size()==10 && baselineIds.edges.size()==24 && baselineIds.points.size()==16,"Perimeter identities repeat or absent");
    const double volume=baseline.summary.value("volumeMm3").toDouble();
    std::cerr<<std::setprecision(17)<<"perimeter volume "<<volume<<'\n';check(std::abs(volume-16716.881962)<.01,"Unexpected perimeter volume");
    const EvaluatedSupportFace* support=nullptr;const EvaluatedSupportEdge* boundary=nullptr;double rear=-1e100;
    for(const auto& face:baseline.supportFaces)if(face.role=="fillet.perimeter" && face.provenance.value("sourceWallRole")=="fillet.round")
        for(const auto& edge:face.boundaryEdges)if(edge.provenance.value("broadFaceRole")=="source"){
            const auto p=edge.provenance.value("startMm").toArray();if(p[1].toDouble()>rear){rear=p[1].toDouble();support=&face;boundary=&edge;}}
    check(support && boundary,"Rear generated tangent support unavailable");
    auto withPlane=doc.authored();auto features=withPlane.value("features").toArray();
    features.append(QJsonObject{{"id",CadDocument::newId()},{"ownerId",baseline.ownerId},{"type","constructionPlane"},{"name","Rounded endpoint tangent"},{"visible",true},
        {"parameters",QJsonObject{{"source",QJsonObject{{"tangent",QJsonObject{
            {"face",QJsonObject{{"bodyId",baseline.bodyId},{"faceId",support->faceId},{"producerFeatureId",support->producerFeatureId}}},
            {"point",QJsonObject{{"bodyId",baseline.bodyId},{"edgeId",boundary->edgeId},{"endpoint",0}}}}}}}}}});
    withPlane["features"]=features;error=doc.commit(withPlane,"Rounded tangent");if(!error.isEmpty())throw std::runtime_error(error.toStdString());
    const auto tangentEdgeId=boundary->edgeId;const auto stable=doc.authored();QTemporaryDir temporary;const auto path=temporary.filePath("perimeter.alloy");
    check(doc.save(path).isEmpty(),"Save perimeter");CadDocument reopened;check(reopened.open(path).isEmpty() && reopened.authored()==stable,"Reopen perimeter");
    auto changed=stable;features=changed.value("features").toArray();auto spine=features.first().toObject();QString lengthId;
    for(const auto& value:spine.value("parameters").toObject().value("dimensions").toArray()){
        const auto dimension=value.toObject();if(dimension.value("kind")=="horizontal" && dimension.value("valueMm")==130)lengthId=dimension.value("id").toString();}
    check(!lengthId.isEmpty(),"Length dimension unavailable");const auto edit=SketchGeometry::updateDimension(spine,{{"dimensionId",lengthId},{"valueMm",140.0}});
    check(edit.ok,"Source length solve failed");features[0]=edit.feature;changed["features"]=features;
    error=reopened.commit(changed,"Length 140");if(!error.isEmpty())throw std::runtime_error(error.toStdString());
    const auto edited=reopened.bodies().back();check(ids(edited)==baselineIds,"Generated perimeter identities changed under source edit");
    check(std::abs(edited.summary.value("volumeMm3").toDouble()-17974.711626154)<.01,"Unexpected length140 perimeter volume");
    const auto edge=std::find_if(edited.solidEdges.begin(),edited.solidEdges.end(),[&](const auto& e){return e.edgeId==tangentEdgeId;});
    check(edge!=edited.solidEdges.end(),"Retained tangent edge unavailable after edit");
    const auto origin=reopened.evaluation().constructionPlanes.back().frame.value("origin").toArray(),expected=edge->provenance.value("startMm").toArray();
    for(int i=0;i<3;++i)check(std::abs(origin[i].toDouble()-expected[i].toDouble())<1e-6,"Generated tangent missed edited endpoint");
    const auto editedAuthored=reopened.authored();reopened.history().undo();check(reopened.authored()==stable,"Perimeter source undo failed");
    reopened.history().redo();check(reopened.authored()==editedAuthored,"Perimeter source redo failed");
    check(reopened.save(path).isEmpty(),"Save edited perimeter");CadDocument editedOpen;check(editedOpen.open(path).isEmpty() && editedOpen.authored()==editedAuthored,"Edited tangent reopen failed");
    std::cout<<"SHOEHORN_PERIMETER_OK volume "<<std::setprecision(17)<<volume<<" edited "<<edited.summary.value("volumeMm3").toDouble()<<" faces 10 edges 24\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
