// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <iostream>
#include <stdexcept>
static void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    CadDocument doc;const auto owner=doc.authored().value("documentId").toString();
    auto draft=doc.authored();QJsonArray features;
    for(int i=0;i<2;++i)features.append(QJsonObject{{"id",CadDocument::newId()},{"bodyId",CadDocument::newId()},
        {"ownerId",owner},{"type","box"},{"name",QString("Box %1").arg(i+1)},{"visible",true},
        {"parameters",QJsonObject{{"widthMm",10.0+i},{"depthMm",12.0},{"heightMm",15.0},{"xMm",0.0},{"yMm",0.0},{"zMm",0.0}}}});
    draft["features"]=features;check(doc.commit(draft,"Two boxes").isEmpty(),"Fixture commit");
    const auto authored=doc.authored();const auto full=doc.state().value("evaluation").toObject();
    const auto revision=doc.revision();const auto index=doc.history().index();
    const auto second=features[1].toObject().value("id").toString();
    check(doc.rollbackBefore(second).isEmpty(),"Prefix evaluation");
    check(doc.bodies().size()==1 && doc.evaluation().featureOrder.size()==1,"Prefix geometry");
    check(doc.worldBodies().size()==1,"World prefix geometry");
    check(doc.authored()==authored && doc.revision()==revision && doc.history().index()==index,"Transient mutation");
    check(!doc.rollbackBefore("missing").isEmpty() && doc.bodies().size()==1,"Atomic invalid marker");
    QTemporaryDir temp;const auto path=temp.path()+"/rollback.alloy";
    check(doc.save(path).isEmpty() && doc.isRolledBack(),"Save while rolled");
    CadDocument reopened;check(reopened.open(path).isEmpty() && !reopened.isRolledBack(),"Reopen latest");
    check(reopened.bodies().size()==2 && reopened.authored()==authored,"Reopen full history");
    doc.returnToLatest();check(doc.state().value("evaluation").toObject()==full,"Full exact restoration");
    check(doc.rollbackBefore(features[0].toObject().value("id").toString()).isEmpty() && doc.bodies().empty(),"Empty prefix");
    doc.history().undo();check(!doc.isRolledBack() && doc.bodies().empty(),"Undo latest history");
    doc.history().redo();check(!doc.isRolledBack() && doc.authored()==authored && doc.bodies().size()==2,"Redo full history");
    std::cout<<"PASS transient prefix geometry, atomic marker, full restore, authored preservation, save/reopen, Undo/Redo\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
