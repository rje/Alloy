#include "CadDocument.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
QJsonObject box(CadDocument& document,double width=50){
    auto feature=QJsonObject{{"id",CadDocument::newId()},{"bodyId",CadDocument::newId()},
        {"ownerId",document.authored()["documentId"]},{"type","box"},{"name","Enclosure stock"},{"visible",true},
        {"parameters",QJsonObject{{"widthMm",width},{"depthMm",80},{"heightMm",40},{"xMm",0},{"yMm",0},{"zMm",0}}}};
    auto candidate=document.authored();candidate["features"]=QJsonArray{feature};
    check(document.commit(candidate,"Box").isEmpty(),"Box commit");return feature;
}
QJsonObject group(const QString& id,const QJsonArray& edges,double radius){
    return {{"id",id},{"edgeIds",edges},{"radiusMm",radius},{"tangentChain",true}};
}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    CadDocument document;const auto stock=box(document),source=document.authored();
    const auto body=document.bodies().at(0);QJsonArray vertical,rims;
    for(const auto& edge:body.solidEdges){
        const auto semantic=edge.provenance["boxEdge"].toString();
        if(semantic.contains("z*"))vertical.append(edge.edgeId);else rims.append(edge.edgeId);
    }
    check(vertical.size()==4 && rims.size()==8,"four vertical/eight top-bottom retained edges");
    const auto verticalId=CadDocument::newId(),rimId=CadDocument::newId();
    const auto groups=QJsonArray{group(verticalId,vertical,5),group(rimId,rims,2.5)};
    const auto args=QJsonObject{{"targetBodyId",body.bodyId},{"groups",groups}};
    check(document.commit(CadDocument::filletCandidate(source,args),"Grouped Fillet").isEmpty(),"one grouped Fillet commits");
    auto authored=document.authored();const auto fillet=authored["features"].toArray().last().toObject();
    check(fillet["parameters"].toObject()["groups"].toArray()==groups,"group IDs/seed sets/radii authored exactly");
    const auto rounded=document.bodies().at(0);
    check(rounded.summary["valid"].toBool() && rounded.summary["closed"].toBool(),"one valid closed rounded solid");
    check(rounded.summary["radiusGroups"].toArray().size()==2 && rounded.summary["selectedEdgeIds"].toArray().size()==12,"both retained groups evaluated");
    check(rounded.summary["volumeMm3"].toDouble()<50*80*40 && rounded.summary["volumeMm3"].toDouble()>150000,"material removed by real fillet");
    document.history().undo();check(document.authored()==source,"one undo returns unrounded source");
    document.history().redo();check(document.authored()==authored,"redo restores one feature exactly");
    auto editedGroups=groups;editedGroups[0]=group(verticalId,vertical,4);
    auto edit=args;edit["featureId"]=fillet["id"];edit["groups"]=editedGroups;
    check(document.commit(CadDocument::filletCandidate(authored,edit),"Edit first radius").isEmpty(),"one group radius edits");
    const auto edited=document.authored();
    check(edited["features"].toArray().last().toObject()["parameters"].toObject()["groups"].toArray()[1]==groups[1],"other group unchanged");
    auto resize=edited;auto features=resize["features"].toArray();auto resizedBox=features[0].toObject();auto dimensions=resizedBox["parameters"].toObject();dimensions["widthMm"]=60;resizedBox["parameters"]=dimensions;features[0]=resizedBox;resize["features"]=features;
    check(document.commit(resize,"Resize stock").isEmpty(),"upstream Box width edit retains both selected groups");
    const auto stable=document.authored();const auto revision=document.revision();
    auto overlap=editedGroups;auto duplicate=overlap[1].toObject();auto members=duplicate["edgeIds"].toArray();members.append(vertical[0]);duplicate["edgeIds"]=members;overlap[1]=duplicate;edit["groups"]=overlap;
    bool rejected=false;try{CadDocument::filletCandidate(stable,edit);}catch(const std::exception&){rejected=true;}
    check(rejected && document.authored()==stable && document.revision()==revision,"duplicate cross-group seed refused atomically");
    edit["groups"]=QJsonArray{group(verticalId,vertical,30),group(rimId,rims,2.5)};
    check(!document.commit(CadDocument::filletCandidate(stable,edit),"Impossible radius").isEmpty() &&
        document.authored()==stable && document.revision()==revision,"impossible grouped radius leaves authored source intact");
    QTemporaryDir temp;const auto path=temp.filePath("grouped.alloy");
    check(document.save(path).isEmpty(),"save grouped Fillet");
    CadDocument reopened;check(reopened.open(path).isEmpty() && reopened.authored()==stable,"reopen retains group and edge IDs");
    check(reopened.bodies().at(0).summary["valid"].toBool(),"reopened grouped geometry");
    CadDocument chain;box(chain);const auto chainBody=chain.bodies().at(0);QJsonArray chainVertical;
    for(const auto& edge:chainBody.solidEdges)if(edge.provenance["boxEdge"].toString().contains("z*"))chainVertical.append(edge.edgeId);
    check(chain.commit(CadDocument::filletCandidate(chain.authored(),{{"targetBodyId",chainBody.bodyId},{"edgeIds",chainVertical},{"radiusMm",5}}),"Round corners").isEmpty(),"first Fillet makes tangent rim");
    QString rimSeed;for(const auto& edge:chain.bodies().at(0).solidEdges)
        if(!edge.edge.IsNull() && edge.provenance["boxEdge"]=="x*y0z1")rimSeed=edge.edgeId;
    check(!rimSeed.isEmpty(),"retained top rim seed after vertical rounding");
    check(chain.commit(CadDocument::filletCandidate(chain.authored(),{{"targetBodyId",chainBody.bodyId},{"groups",QJsonArray{group(CadDocument::newId(),QJsonArray{rimSeed},2.5)}}}),"Tangent chain").isEmpty(),"one seed fillets current tangent rim");
    const auto contours=chain.bodies().at(0).summary["tangentContours"].toArray();
    check(contours.size()==1 && contours[0].toArray().size()>1,"tangent chain expands beyond authored seed");
    const auto chainAuthored=chain.authored();auto wider=chainAuthored;auto chainFeatures=wider["features"].toArray();
    auto chainStock=chainFeatures[0].toObject();auto chainSize=chainStock["parameters"].toObject();
    chainSize["widthMm"]=60;chainStock["parameters"]=chainSize;chainFeatures[0]=chainStock;wider["features"]=chainFeatures;
    check(chain.commit(wider,"Widen tangent source").isEmpty(),"tangent contour re-evaluates after upstream width edit");
    check(chain.authored()["features"].toArray().last().toObject()["parameters"]==
        chainAuthored["features"].toArray().last().toObject()["parameters"],"dynamic chain retains only its seed and radius");
    const auto rebuilt=chain.bodies().at(0).summary["tangentContours"].toArray();
    check(rebuilt.size()==1 && rebuilt[0].toArray().size()>1,"tangent contour expands on edited pre-feature topology");
    std::cout<<"PASS: one feature, two radii, actual Box geometry, edit, resize, atomic refusal, history and reopen\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
