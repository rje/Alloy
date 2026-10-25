// SPDX-License-Identifier: MIT
#include "RecoveryStore.h"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool ok,const char* message) {if(!ok)throw std::runtime_error(message);}
QString snapshotPath(const RecoveryStore& store,const QString& tabId) {
    return QDir(QDir(store.root()).filePath(store.sessionId())).filePath(tabId+".json");
}
void rewrite(const QString& path,const QJsonObject& value) {
    QFile file(path);check(file.open(QIODevice::WriteOnly|QIODevice::Truncate),"open test recovery");
    const auto bytes=QJsonDocument(value).toJson(QJsonDocument::Compact);
    check(file.write(bytes)==bytes.size(),"write test recovery");
}
}
int main(int argc,char** argv) {
    QCoreApplication app(argc,argv);QTemporaryDir temporary;check(temporary.isValid(),"temporary root");
    const auto root=QDir(temporary.path()).filePath("recovery");
    const QString documentId="11111111-1111-4111-8111-111111111111";
    const QString tabId="22222222-2222-4222-8222-222222222222";
    const QString secondTabId="33333333-3333-4333-8333-333333333333";
    const QJsonObject authored{{"schema","alloy-document/1"},{"documentId",documentId},
        {"displayUnits","mm"},{"features",QJsonArray{QJsonObject{{"id","one"}}}}};
    QString staleId,path;
    {
        RecoveryStore owner(root);check(owner.error().isEmpty(),"private owner initialized");
        path=snapshotPath(owner,tabId);staleId=owner.sessionId()+":"+tabId;
        check(owner.write(tabId,authored,{}).isEmpty() && QFile::exists(path),"atomic snapshot written");
        check(owner.write(secondTabId,authored,{}).isEmpty() && QFile::exists(snapshotPath(owner,secondTabId)),
            "same authored document in a second tab has independent snapshot");
        check(owner.removeOwned(secondTabId).isEmpty() && QFile::exists(path),
            "closing duplicate tab retains first tab snapshot");
        // Same /1 checksum survives appended/reordered member encoding and
        // legacy Qt canonical serialization; checksum/member injection cannot.
        QFile encoded(path);check(encoded.open(QIODevice::ReadOnly),"read encoded snapshot");
        const auto originalBytes=encoded.readAll();encoded.close();
        auto legacy=QJsonDocument::fromJson(originalBytes).object();
        const auto hash=legacy.take("sha256").toString();
        check(hash==QString::fromLatin1(QCryptographicHash::hash(QJsonDocument(legacy).toJson(QJsonDocument::Compact),QCryptographicHash::Sha256).toHex()),"canonical legacy checksum equivalence");
        legacy["sha256"]=hash;rewrite(path,legacy);
#ifndef Q_OS_WIN
        const auto parent=QFileInfo(path).absolutePath();const auto permissions=QFile::permissions(parent);
        check(QFile::setPermissions(parent,QFileDevice::ReadOwner|QFileDevice::ExeOwner),"make atomic destination unwritable");
        const auto failed=owner.write(tabId,QJsonObject{{"documentId",documentId},{"changed",true}},{});
        check(QFile::setPermissions(parent,permissions),"restore destination permissions");
        check(!failed.isEmpty(),"atomic replacement failure reported");
        QFile prior(path);check(prior.open(QIODevice::ReadOnly),"prior valid snapshot readable");
        check(QJsonDocument::fromJson(prior.readAll()).object()==legacy,"failed replacement preserves previous valid snapshot");
#endif
        RecoveryStore other(root);check(other.error().isEmpty(),"second owner initialized");
        check(other.list().isEmpty(),"live session hidden");
        check(!other.transfer(staleId,[](const auto&){return QString{};}).isEmpty(),"live transfer refused");
        check(!other.discard(staleId).isEmpty() && QFile::exists(path),"live discard refused");
    }
    RecoveryStore receiver(root);check(receiver.error().isEmpty(),"receiver initialized");
    auto rows=receiver.list();check(rows.size()==1 && rows[0].toObject().value("status")=="ready","stale snapshot listed");
    check(!receiver.transfer(staleId,[](const auto&){return QString{"consumer refused"};}).isEmpty() && QFile::exists(path),
        "failed adoption preserves source");
    bool copied=false;
    check(receiver.transfer(staleId,[&](const RecoveryStore::Record& record){
        copied=record.authored==authored && record.tabId==tabId && record.documentId==documentId && record.originalPath.isEmpty();
        return receiver.write(secondTabId,record.authored,{});
    }).isEmpty() && copied && !QFile::exists(path),"durable owned copy precedes source retirement");
    check(receiver.removeOwned(secondTabId).isEmpty(),"owned snapshot cleanup");
    {
        RecoveryStore producer(root);check(producer.write(tabId,authored,{}).isEmpty(),"second stale source");
        staleId=producer.sessionId()+":"+tabId;path=snapshotPath(producer,tabId);
    }
    QFile file(path);check(file.open(QIODevice::ReadOnly),"read second source");
    auto value=QJsonDocument::fromJson(file.readAll()).object();file.close();
    auto injected=value;injected["unexpected"]=true;rewrite(path,injected);
    check(receiver.list().first().toObject().value("status")=="malformed","extra member injection refused");
    check(!receiver.transfer(staleId,[](const auto&){return QString{};}).isEmpty(),"injected transfer refused");
    value["schema"]="alloy-recovery/2";rewrite(path,value);
    rows=receiver.list();check(rows.size()==1 && rows[0].toObject().value("status")=="unsupported_version","newer version visible");
    check(!receiver.transfer(staleId,[](const auto&){return QString{};}).isEmpty(),"newer transfer refused");
    value["schema"]="alloy-recovery/1";value["authored"]=QJsonObject{};rewrite(path,value);
    rows=receiver.list();check(rows.size()==1 && rows[0].toObject().value("status")=="malformed","corrupt source visible");
    check(!receiver.transfer(staleId,[](const auto&){return QString{};}).isEmpty(),"corrupt transfer refused");
    check(receiver.discard(staleId).isEmpty() && !QFile::exists(path),"explicit corrupt discard");
    std::cout<<"PASS atomic committed store, live ownership, transfer ordering, newer/malformed refusal and discard\n";
}
