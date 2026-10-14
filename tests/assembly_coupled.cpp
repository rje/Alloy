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
 check(argc==2,"Pass actual native piston fixture");CadDocument graph;check(graph.open(QString::fromLocal8Bit(argv[1])).isEmpty(),"Open actual fixture");const auto a=graph.authored();
 const auto inner=definition(a,"Inner piston"),outer=definition(a,"Outer piston"),lower=definition(a,"Lower arm"),upper=definition(a,"Upper arm");const auto i=occurrence(a,inner),o=occurrence(a,outer),l=occurrence(a,lower),u=occurrence(a,upper);
 auto addRevolute=[&](QString moving,QString fixed,QString owner,double x,double z){auto ref=origin(body(graph.bodies(),owner),false,x,z);check(!ref.isEmpty(),"Actual named eye circular origin");commit(graph,AssemblyGraph::revoluteCandidate(graph.authored(),graph.evaluation(),{{"component1OccurrenceId",moving},{"component2OccurrenceId",fixed},{"position",ref}}));};
 addRevolute(u,l,lower,0,80);const auto driver=graph.authored()["joints"].toArray()[0].toObject()["id"].toString();
 commit(graph,AssemblyGraph::cylindricalCandidate(graph.authored(),graph.evaluation(),{{"component1OccurrenceId",i},{"component2OccurrenceId",o},{"position",origin(body(graph.bodies(),outer),true)}}));
 check(graph.assemblyState()["islands"].toArray().size()==2,"Source-order disconnected piston island");const auto cylinder=graph.authored()["joints"].toArray()[1].toObject()["id"].toString();
 addRevolute(i,l,lower,90,0);addRevolute(o,u,upper,90,180);commit(graph,CadDocument::pinOccurrenceCandidate(graph.authored(),l));const auto rest=graph.authored();const auto restState=graph.assemblyState();check(restState["islands"].toArray().size()==1,"Complete connected mechanism");
 const auto baseline=joint(restState,cylinder)["coordinateMm"].toDouble();double low=1e10,high=-1e10;
 for(double angle:{-15.0,0.0,15.0}){const QJsonObject draft{{"jointId",driver},{"angleDeg",angle}};const auto state=AssemblyGraph::inspect(rest,graph.evaluation(),draft);near(joint(state,driver)["coordinateDeg"].toDouble(),angle,"Exact whole-mechanism driver");const auto pin=leafPose(state,l);for(int n=0;n<12;++n)near(pin[n].toDouble(),n==0||n==5||n==10?1:0,"Complete Lower Pin pose");
 for(const auto& v:state["joints"].toArray()){const auto j=v.toObject();check(j["pinAuthored"].toBool()&&j["maxOriginErrorMm"].toDouble()<1e-5&&j["maxAxisErrorRadians"].toDouble()<1e-6,"All original relationships independently checked");}
 const auto slide=joint(state,cylinder)["coordinateMm"].toDouble();low=std::min(low,slide);high=std::max(high,slide);const auto placed=AssemblyGraph::worldBodies(rest,graph.evaluation(),draft);check(placed.size()==graph.bodies().size(),"Whole-graph presentation covers leaves");check(graph.authored()==rest,"Whole-mechanism preview immutable");std::cout<<"angleDeg="<<angle<<" pistonSlideMm="<<slide<<" baselineMm="<<baseline<<'\n';}
 check(high-low>10,"Meaningful coupled piston travel");
 bool refused=false;try{AssemblyGraph::inspect(rest,graph.evaluation(),{{"jointId",driver},{"rotationDeg",5}});}catch(const std::exception&){refused=true;}check(refused,"Wrong typed driver coordinate refuses");
 commit(graph,AssemblyGraph::revoluteCandidate(rest,graph.evaluation(),{{"jointId",driver},{"angleDeg",15}}));const auto driven=graph.authored();graph.history().undo();check(graph.authored()==rest,"Coupled coordinate undo");graph.history().redo();check(graph.authored()==driven,"Coupled coordinate redo");QTemporaryDir temp;const auto path=temp.filePath("mechanism.alloy");check(graph.save(path).isEmpty(),"Save complete mechanism");CadDocument reopened;check(reopened.open(path).isEmpty()&&reopened.authored()==driven,"Exact full mechanism reopen");near(joint(reopened.assemblyState(),driver)["coordinateDeg"].toDouble(),15,"Reopened full mechanism drive");
 std::cout<<"COUPLED_OK actual3R1C/Pin/signedrange/travel/history/reopen\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
