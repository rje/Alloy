// SPDX-License-Identifier: MIT
// Saved-model geometry oracle; no private editor or alternate modeling path.
#include "CadDocument.h"
#include <BRepAlgoAPI_Check.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepTools.hxx>
#include <TopExp_Explorer.hxx>
#include <QCryptographicHash>
#include <sstream>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepClass3d_SolidClassifier.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <QCoreApplication>
#include <QJsonDocument>
#include <cmath>
#include <iostream>
#include <algorithm>
namespace {
void require(bool yes,const char* message){if(!yes)throw std::runtime_error(message);}
void state(const TopoDS_Shape& shape,const gp_Pnt& point,TopAbs_State expected){BRepClass3d_SolidClassifier c(shape);c.Perform(point,1e-7);require(c.State()==expected,"Bore or nearby hub material classification differs");}
void valid(const EvaluatedBody& body){require(body.summary["valid"].toBool()&&body.summary["closed"].toBool()&&body.summary["topology"].toObject()["solids"]==1&&body.summary["volumeMm3"].toDouble()>0,"Expected one valid closed positive solid");BRepAlgoAPI_Check check(body.shape,true,true);require(check.IsValid(),"Saved result fails BRep/Boolean argument validity");}
QString hash(const TopoDS_Shape& shape){std::ostringstream out;BRepTools::Write(shape,out);return QString::fromLatin1(QCryptographicHash::hash(QByteArray::fromStdString(out.str()),QCryptographicHash::Sha256).toHex());}
double missingVolume(const TopoDS_Shape& input,const TopoDS_Shape& result){TopTools_ListOfShape args,tools;args.Append(input);tools.Append(result);BRepAlgoAPI_Cut cut;cut.SetArguments(args);cut.SetTools(tools);cut.SetNonDestructive(true);cut.SetRunParallel(false);cut.SetFuzzyValue(0);cut.Build();require(cut.IsDone()&&!cut.HasErrors()&&!cut.Shape().IsNull(),"Exact participant-minus-Join check failed");require(BRepCheck_Analyzer(cut.Shape()).IsValid(),"Participant-minus-Join result invalid");if(!TopExp_Explorer(cut.Shape(),TopAbs_SOLID).More())return 0;GProp_GProps props;const double error=BRepGProp::VolumePropertiesGK(cut.Shape(),props,1e-9,true);require(std::isfinite(error)&&error>=0&&error<=1e-8&&std::isfinite(props.Mass()),"Adaptive residual volume integration failed");return std::abs(props.Mass());}
QJsonObject verify(const QString& path){CadDocument d;const auto error=d.open(path);require(error.isEmpty(),"Saved model did not reopen");require(d.bodies().size()==1,"Saved final model is not one body");const auto& final=d.bodies()[0];valid(final);
 QJsonObject join,hole;for(const auto& value:d.authored()["features"].toArray()){const auto f=value.toObject(),p=f["parameters"].toObject();if(f["type"]=="extrude"&&p["operation"]=="join")join=f;if(f["type"]=="extrude"&&p["operation"]=="cut")hole=f;}
 require(!join.isEmpty()&&!hole.isEmpty(),"Expected retained hub Join and bore Cut");const auto joinInputs=d.evaluationBefore(join["id"].toString()),beforeHole=d.evaluationBefore(hole["id"].toString());require(beforeHole.bodies.size()==1,"Before-Cut Join is not one body");const auto& joined=beforeHole.bodies[0];valid(joined);require(final.bodyId==joined.bodyId,"Cut replaced retained Join identity");
 const auto p=join["parameters"].toObject();const auto targets=p["targetBodyIds"].toArray();require(targets.size()==8&&joinInputs.bodies.size()==8,"Join does not retain exactly eight member participants");QJsonArray preservation;
 // Baseline only the OCCT container Free flag, as in SOL-041; geometry stays exact.
 auto joinedShape=joined.shape;joinedShape.Free(false);
 auto covered=[&](const QString& id,const TopoDS_Shape& shape,double volume){auto owned=shape;owned.Free(false);const auto before=hash(owned),joinedBefore=hash(joinedShape);const double missing=missingVolume(owned,joinedShape);require(hash(owned)==before&&hash(joinedShape)==joinedBefore,"Preservation oracle mutated input geometry");require(missing<=std::max(1e-6,volume*1e-8),"Join lost participant or hub material");preservation.append(QJsonObject{{"id",id},{"inputVolumeMm3",volume},{"missingVolumeMm3",missing},{"inputBrepSha256Before",before},{"inputBrepSha256After",hash(owned)},{"joinedBrepSha256Before",joinedBefore},{"joinedBrepSha256After",hash(joinedShape)}});};
 for(const auto& id:targets){const auto found=std::find_if(joinInputs.bodies.begin(),joinInputs.bodies.end(),[&](const auto& b){return b.bodyId==id.toString();});require(found!=joinInputs.bodies.end(),"Explicit Join member unavailable");valid(*found);covered(found->bodyId,found->shape,found->summary["volumeMm3"].toDouble());}
 const auto sketch=std::find_if(joinInputs.sketches.begin(),joinInputs.sketches.end(),[&](const auto& s){return s.featureId==p["sketchId"].toString();});require(sketch!=joinInputs.sketches.end(),"Hub sketch unavailable");const auto profile=std::find_if(sketch->profiles.begin(),sketch->profiles.end(),[&](const auto& r){return r.profileId==p["profileId"].toString();});require(profile!=sketch->profiles.end(),"Hub profile unavailable");const auto hub=GeometryKernel::extrusion(*profile,p["distanceMm"].toDouble());covered("hub",hub,GeometryKernel::summarize(hub)["volumeMm3"].toDouble());
 for(double y:{-4.,0.,4.})state(joined.shape,gp_Pnt(0,y,-2.5),TopAbs_IN);
 for(double y:{-6.,-4.9,-2.,0.,2.,4.9,6.})state(final.shape,gp_Pnt(0,y,-2.5),TopAbs_OUT);
 for(double x:{-2.5,2.5})for(double y:{-4.,0.,4.})state(final.shape,gp_Pnt(x,y,-2.5),TopAbs_IN);
 for(double z:{-4.75,-.25})state(final.shape,gp_Pnt(0,0,z),TopAbs_IN);
 return {{"path",path},{"bodyId",final.bodyId},{"volumeMm3",final.summary["volumeMm3"]},{"brepAndBooleanArgumentValid",true},{"preservedJoinInputs",preservation},{"boreAxisSamplesOutside",7},{"adjacentWallSamplesInside",8},{"beforeCutAxisSamplesInside",3}};
}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{require(argc==3,"Usage: ornament_geometry_verify original.alloy edited.alloy");const auto original=verify(QString::fromLocal8Bit(argv[1])),edited=verify(QString::fromLocal8Bit(argv[2]));require(original["bodyId"]==edited["bodyId"],"Upstream edit changed retained final body");std::cout<<QJsonDocument(QJsonObject{{"status","geometry-pass"},{"original",original},{"edited",edited}}).toJson(QJsonDocument::Indented).constData();return 0;}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
