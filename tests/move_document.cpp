#include "CadDocument.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
#include <stdexcept>
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    CadDocument doc;auto authored=doc.authored();const auto tool=CadDocument::newId(),target=CadDocument::newId();
    authored["features"]=QJsonArray{
      QJsonObject{{"id",CadDocument::newId()},{"ownerId",authored["documentId"]},{"bodyId",target},{"type","box"},{"name","stock"},{"visible",true},{"parameters",QJsonObject{{"widthMm",20},{"depthMm",20},{"heightMm",10},{"xMm",-10},{"yMm",-10},{"zMm",0}}}},
      QJsonObject{{"id",CadDocument::newId()},{"ownerId",authored["documentId"]},{"bodyId",tool},{"type","cylinder"},{"name","tool"},{"visible",true},{"parameters",QJsonObject{{"radiusMm",2},{"heightMm",40},{"xMm",0},{"yMm",0},{"zMm",10}}}}};
    check(doc.commit(authored,"stock/tool").isEmpty(),"source commit");const auto source=doc.authored();
    QJsonObject rotate{{"xMm",0},{"yMm",0},{"zMm",0},{"xDeg",0},{"yDeg",10},{"zDeg",0}};
    QJsonObject down{{"xMm",0},{"yMm",0},{"zMm",-20},{"xDeg",0},{"yDeg",0},{"zDeg",0}};
    auto candidate=CadDocument::moveCandidate(source,{{"targetBodyId",tool},{"pivotXMm",0},{"pivotYMm",0},{"pivotZMm",10},{"steps",QJsonArray{rotate,down}}});
    const auto evaluation=CadDocument::evaluateCandidate(candidate);check(evaluation.ok,"move evaluates");check(doc.authored()==source,"preview disposable");
    const auto p=candidate["features"].toArray().last().toObject()["parameters"].toObject();auto transform=GeometryKernel::moveTransform(p);gp_Pnt foot(0,0,10);foot.Transform(transform);
    check(std::abs(foot.X()+20*std::sin(std::acos(-1.0)/18))<1e-9,"local translation rotates with triad");
    check(std::abs(foot.Z()-(10-20*std::cos(std::acos(-1.0)/18)))<1e-9,"pivot semantics");
    check(std::abs(evaluation.evaluation->bodies[1].summary["volumeMm3"].toDouble()-160*std::acos(-1.0))<1e-6,"rigid volume retained");
    check(doc.commit(candidate,"Move").isEmpty(),"commit move");doc.history().undo();check(doc.authored()==source,"undo full move");doc.history().redo();check(doc.authored()==candidate,"redo exact intent");
    auto cut=CadDocument::booleanCandidate(candidate,{{"targetBodyId",target},{"toolBodyIds",QJsonArray{tool}},{"operation","cut"},{"keepTools",false}});check(doc.commit(cut,"Cut").isEmpty(),"cut sees transformed tool");check(doc.bodies().size()==1 && doc.bodies()[0].summary["volumeMm3"].toDouble()<4000,"real cut result");
    QTemporaryDir directory;auto path=directory.filePath("cut.alloy");check(doc.save(path).isEmpty(),"save");CadDocument reopened;check(reopened.open(path).isEmpty() && reopened.authored()==cut,"exact reopen");
    auto invalid=CadDocument::moveCandidate(cut,{{"targetBodyId",tool},{"zMm",1}});check(!CadDocument::evaluateCandidate(invalid).ok,"consumed tool rejects");check(doc.authored()==cut,"failure atomic");
    std::cout<<"Move rigid/local/history/Cut/reopen passed\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
