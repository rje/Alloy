// SPDX-License-Identifier: MIT
#include "AssemblyGraph.h"
#include "AssemblyRevolute.h"
#include "CadDocument.h"
#include <BRepAdaptor_Curve.hxx>
#include <QCoreApplication>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
#include <stdexcept>
namespace {
void check(bool v,const char* t){if(!v)throw std::runtime_error(t);}
void near(double a,double b,const char* t,double tolerance=1e-5){check(std::abs(a-b)<tolerance,t);}
void commit(CadDocument& d,const QJsonObject& a){const auto e=d.commit(a,"Cylindrical core");check(e.isEmpty(),qPrintable(e));}
QString definition(const QJsonObject& a,const QString& name){for(const auto& v:a["components"].toArray())if(v.toObject()["name"]==name)return v.toObject()["id"].toString();return {};}
QString occurrence(const QJsonObject& a,const QString& definition){for(const auto& v:a["occurrences"].toArray())if(v.toObject()["definitionId"]==definition)return v.toObject()["id"].toString();return {};}
const EvaluatedBody& body(const std::vector<EvaluatedBody>& bodies,const QString& owner){for(const auto& b:bodies)if(b.ownerId==owner)return b;throw std::runtime_error("Fixture leaf body unavailable");}
QJsonObject origin(const EvaluatedBody& b,bool tube,double x=90,double z=-1){QJsonObject selected;double lowest=1e10;for(const auto& e:b.solidEdges)if(!e.edge.IsNull()){BRepAdaptor_Curve c(e.edge);if(c.GetType()!=GeomAbs_Circle||std::abs(std::abs(c.LastParameter()-c.FirstParameter())-2*std::acos(-1.0))>1e-6)continue;auto v=c.Circle();auto p=v.Location();if(std::abs(p.X()-x)>1e-5||std::abs(tube?v.Axis().Direction().Z():v.Axis().Direction().Y())<1-1e-6)continue;if(tube&&std::abs(p.Y())>1e-5)continue;if(z>=0&&std::abs(p.Z()-z)>1e-5)continue;if(p.Z()<lowest){lowest=p.Z();selected={{"bodyId",b.bodyId},{"edgeId",e.edgeId}};}}return selected;}
QJsonObject joint(const QJsonObject& state,const QString& id){for(const auto& v:state["joints"].toArray())if(v.toObject()["id"]==id)return v.toObject();throw std::runtime_error("Joint unavailable");}
QJsonArray leafPose(const QJsonObject& s,const QString& id){for(const auto& v:s["occurrences"].toArray())if(v.toObject()["occurrenceId"]==id)return v.toObject()["worldPose"].toArray();return {};}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
 check(argc==2,"Pass actual native piston fixture");CadDocument d;check(d.open(QString::fromLocal8Bit(argv[1])).isEmpty(),"Open fixture");
 const auto a=d.authored();const auto inner=definition(a,"Inner piston"),outer=definition(a,"Outer piston"),lower=definition(a,"Lower arm"),upper=definition(a,"Upper arm");
 const auto i=occurrence(a,inner),o=occurrence(a,outer),l=occurrence(a,lower),u=occurrence(a,upper);check(AssemblyGraph::identityLeaf(a,i)&&AssemblyGraph::identityLeaf(a,o),"Nested piston leaves");
 const auto tube=origin(body(d.bodies(),outer),true);check(!tube.isEmpty(),"Actual Outer tube Z circle");
 commit(d,AssemblyGraph::cylindricalCandidate(a,d.evaluation(),{{"component1OccurrenceId",i},{"component2OccurrenceId",o},{"position",tube}}));
 const auto cylindrical=d.authored();const auto id=cylindrical["joints"].toArray()[0].toObject()["id"].toString();
 check(d.assemblyState()["islands"].toArray().size()==1&&!joint(d.assemblyState(),id)["pinAuthored"].toBool(),"Standalone temporary island gauge");
 check(body(d.worldBodies(),inner).shape.IsSame(body(d.bodies(),inner).shape),"Zero as-built leaves original BRep");
 const auto baseline=body(d.bodies(),inner).summary["boundsMm"].toObject();
 for(auto target:std::vector<std::pair<double,double>>{{15,5},{-15,-5}}){const QJsonObject preview{{"jointId",id},{"rotationDeg",target.first},{"slideMm",target.second}};auto state=AssemblyGraph::inspect(d.authored(),d.evaluation(),preview);near(joint(state,id)["coordinateDeg"].toDouble(),target.first,"Signed spin");near(joint(state,id)["coordinateMm"].toDouble(),target.second,"Signed slide");near(leafPose(state,o)[11].toDouble(),0,"Gauge fixed");near(leafPose(state,i)[11].toDouble(),target.second,"Inner actual occurrence translated");check(d.authored()==cylindrical,"Preview immutable");auto placed=AssemblyGraph::worldBodies(d.authored(),d.evaluation(),preview);near(body(placed,inner).summary["boundsMm"].toObject()["min"].toArray()[2].toDouble(),baseline["min"].toArray()[2].toDouble()+target.second,"Actual BRep Z bound follows slide");}
 commit(d,AssemblyGraph::cylindricalCandidate(d.authored(),d.evaluation(),{{"jointId",id},{"rotationDeg",15},{"slideMm",5}}));const auto driven=d.authored();d.history().undo();check(d.authored()==cylindrical,"Coordinate undo");d.history().redo();check(d.authored()==driven,"Coordinate redo");
 QTemporaryDir temp;const auto path=temp.filePath("cylinder.alloy");check(d.save(path).isEmpty(),"Save");CadDocument reopened;check(reopened.open(path).isEmpty()&&reopened.authored()==driven,"Exact reopen");near(joint(reopened.assemblyState(),id)["coordinateMm"].toDouble(),5,"Reopened slide");
 auto edited=reopened.authored();auto fs=edited["features"].toArray();for(int n=0;n<fs.size();++n){auto f=fs[n].toObject();if(f["name"]=="Inner piston"&&f["type"]=="cylinder"){auto p=f["parameters"].toObject();p["xMm"]=p["xMm"].toDouble()+1;f["parameters"]=p;fs[n]=f;}}edited["features"]=fs;commit(reopened,edited);check(reopened.authored()["joints"]==driven["joints"],"Moving source identity retained");near(leafPose(reopened.assemblyState(),i)[3].toDouble(),-1,"Actual moved source axis follows retained moving circle");reopened.history().undo();check(reopened.authored()==driven,"Source edit undo");
 auto bad=driven;auto rows=bad["joints"].toArray();auto record=rows[0].toObject();record["movingPosition"]=QJsonObject{{"bodyId",record["movingPosition"].toObject()["bodyId"]},{"edgeId",CadDocument::newId()}};rows[0]=record;bad["joints"]=rows;const auto revision=reopened.revision();check(!reopened.commit(bad,"Missing source").isEmpty()&&reopened.authored()==driven&&reopened.revision()==revision,"Missing moving origin refuses atomically");
 // Source order: arm hinge, disconnected nested piston, then eye relationships.
 CadDocument graph;check(graph.open(QString::fromLocal8Bit(argv[1])).isEmpty(),"Reopen clean graph fixture");auto hinge=origin(body(graph.bodies(),lower),false,0,80);commit(graph,AssemblyRevolute::candidate(graph.authored(),graph.evaluation(),{{"component1OccurrenceId",u},{"component2OccurrenceId",l},{"position",hinge}}));
 commit(graph,AssemblyGraph::cylindricalCandidate(graph.authored(),graph.evaluation(),{{"component1OccurrenceId",i},{"component2OccurrenceId",o},{"position",tube}}));check(graph.assemblyState()["islands"].toArray().size()==2,"Intermediate source order has two independently gauged islands");const auto cid=graph.authored()["joints"].toArray()[1].toObject()["id"].toString();
 auto lowerEye=origin(body(graph.bodies(),lower),false,90,0);commit(graph,AssemblyRevolute::candidate(graph.authored(),graph.evaluation(),{{"component1OccurrenceId",i},{"component2OccurrenceId",l},{"position",lowerEye}}));check(graph.assemblyState()["islands"].toArray().size()==1,"Eye merges islands and drops temporary piston gauge");check(!joint(graph.assemblyState(),cid)["coordinateEditable"].toBool(),"Merged cylinder becomes passive");
 bool refused=false;try{AssemblyGraph::cylindricalCandidate(graph.authored(),graph.evaluation(),{{"jointId",cid},{"slideMm",1}});}catch(const std::exception&){refused=true;}check(refused,"Passive coordinate edit explicitly refuses");refused=false;try{AssemblyGraph::inspect(graph.authored(),graph.evaluation(),{{"jointId",cid},{"rotationDeg",0},{"slideMm",0}});}catch(const std::exception&){refused=true;}check(refused,"Passive preview explicitly refuses");
 std::cout<<"CYLINDRICAL_OK actual nested circles/signed coordinates/source/history/islands/refusal\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
