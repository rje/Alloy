// SPDX-License-Identifier: MIT
#include "RecoveryStore.h"
#include "PrivateDirectory.h"
#include <QCryptographicHash>
#include <QDateTime>
#include <QElapsedTimer>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QLockFile>
#include <QSaveFile>
#include <QStandardPaths>
#include <QUuid>
#include <utility>

namespace {
constexpr auto kSchema = "alloy-recovery/1";

bool canonicalUuid(const QString& id) {
    const QUuid uuid(id);
    return !uuid.isNull() && uuid.toString(QUuid::WithoutBraces) == id;
}
QString digest(const QJsonObject& value) {
    return QString::fromLatin1(QCryptographicHash::hash(
        QJsonDocument(value).toJson(QJsonDocument::Compact),QCryptographicHash::Sha256).toHex());
}
bool splitId(const QString& id, QString* sessionId, QString* tabId) {
    const auto fields=id.split(':');
    if(fields.size()!=2 || !canonicalUuid(fields[0]) || !canonicalUuid(fields[1]))return false;
    *sessionId=fields[0];*tabId=fields[1];return true;
}
struct Parsed {
    QString status;
    QString error;
    RecoveryStore::Record record;
};
Parsed parseRecord(const QString& path,const QString& id) {
    Parsed result;result.record.id=id;
    if(QFileInfo(path).isSymLink()) {result.status="malformed";result.error="Recovery file is a symbolic link";return result;}
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly)) {result.status="malformed";result.error="Cannot read recovery file";return result;}
    QJsonParseError parse;
    const auto json=QJsonDocument::fromJson(file.readAll(),&parse);
    if(parse.error!=QJsonParseError::NoError || !json.isObject()) {
        result.status="malformed";result.error="Malformed recovery JSON";return result;
    }
    auto envelope=json.object();
    if(!envelope.value("schema").isString()) {
        result.status="malformed";result.error="Recovery version is missing";return result;
    }
    if(envelope.value("schema")!=kSchema) {
        result.status="unsupported_version";result.error="Unsupported recovery version";return result;
    }
    const auto suppliedHash=envelope.take("sha256").toString();
    QString sessionId,tabId;
    const auto documentId=envelope.value("documentId").toString();
    if(envelope.size()!=7 || !splitId(id,&sessionId,&tabId) || !canonicalUuid(documentId) ||
       envelope.value("sessionId")!=sessionId || envelope.value("tabId")!=tabId ||
       !envelope.value("originalPath").isString() || !envelope.value("capturedAtUtc").isString() ||
       !envelope.value("authored").isObject() || suppliedHash!=digest(envelope)) {
        result.status="malformed";result.error="Recovery identity or checksum is invalid";return result;
    }
    const auto originalPath=envelope.value("originalPath").toString();
    if((!originalPath.isEmpty() && (!QDir::isAbsolutePath(originalPath) ||
        !originalPath.endsWith(".alloy",Qt::CaseInsensitive))) ||
       !QDateTime::fromString(envelope.value("capturedAtUtc").toString(),Qt::ISODateWithMs).isValid() ||
       envelope.value("authored").toObject().value("documentId")!=documentId) {
        result.status="malformed";result.error="Recovery metadata or authored identity is invalid";return result;
    }
    result.status="ready";
    result.record.tabId=tabId;
    result.record.documentId=documentId;
    result.record.originalPath=originalPath;
    result.record.capturedAtUtc=envelope.value("capturedAtUtc").toString();
    result.record.authored=envelope.value("authored").toObject();
    return result;
}
}

RecoveryStore::RecoveryStore(QString root)
    : root_(root.isEmpty() ? QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)+"/recovery" : std::move(root)),
      sessionId_(QUuid::createUuid().toString(QUuid::WithoutBraces)) {
    if(!alloyPrivateDirectory(root_,true,&error_))return;
    const auto sessionPath=QDir(root_).filePath(sessionId_);
    if(!alloyPrivateDirectory(sessionPath,true,&error_))return;
    lock_=std::make_unique<QLockFile>(QDir(sessionPath).filePath("owner.lock"));
    lock_->setStaleLockTime(0);
    if(!lock_->tryLock(0)) {error_="Cannot lock private recovery session";lock_.reset();}
}
RecoveryStore::~RecoveryStore() {
    const auto sessionPath=QDir(root_).filePath(sessionId_);
    const bool empty=ownedTabIds().isEmpty();
    lock_.reset();
    if(empty)QDir().rmdir(sessionPath);
}

QString RecoveryStore::recordPath(const QString& sessionId,const QString& tabId) const {
    return QDir(QDir(root_).filePath(sessionId)).filePath(tabId+".json");
}

QString RecoveryStore::write(const QString& tabId,const QJsonObject& authored,const QString& originalPath) {
    if(!error_.isEmpty() || !lock_)return error_.isEmpty() ? "Recovery session is unavailable" : error_;
    const auto documentId=authored.value("documentId").toString();
    if(!canonicalUuid(tabId) || !canonicalUuid(documentId) || (!originalPath.isEmpty() &&
        (!QDir::isAbsolutePath(originalPath) || !originalPath.endsWith(".alloy",Qt::CaseInsensitive))))
        return "Recovery needs valid tab/document IDs and native source path";
    const auto path=recordPath(sessionId_,tabId);
    if(QFileInfo(path).isSymLink())return "Recovery destination is a symbolic link";
    QJsonObject envelope{{"schema",kSchema},{"sessionId",sessionId_},{"tabId",tabId},{"documentId",documentId},
        {"originalPath",originalPath},{"capturedAtUtc",QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)},
        {"authored",authored}};
    const bool trace=qEnvironmentVariableIsSet("ALLOY_TRACE_RECOVERY_COST");
    QElapsedTimer timer;if(trace)timer.start();
    auto bytes=QJsonDocument(envelope).toJson(QJsonDocument::Compact);
    const auto encodeDigestMs=trace?timer.nsecsElapsed()/1e6:0.;if(trace)timer.restart();
    const auto hash=QCryptographicHash::hash(bytes,QCryptographicHash::Sha256).toHex();
    const auto hashMs=trace?timer.nsecsElapsed()/1e6:0.;if(trace)timer.restart();
    // Preserve the /1 checksum over Qt's canonical unsigned object. JSON member
    // order is immaterial: append only the fixed member and hex digest instead
    // of serializing the complete (potentially large) authored object twice.
    if(!bytes.endsWith('}'))return "Cannot encode recovery object";
    bytes.chop(1);bytes.append(",\"sha256\":\"");bytes.append(hash);bytes.append("\"}");
    const auto encodeFinalMs=trace?timer.nsecsElapsed()/1e6:0.;if(trace)timer.restart();
    QSaveFile file(path);file.setDirectWriteFallback(false);
    if(!file.open(QIODevice::WriteOnly))return file.errorString();
#ifndef Q_OS_WIN
    if(!file.setPermissions(QFileDevice::ReadOwner|QFileDevice::WriteOwner))return "Cannot protect recovery snapshot";
#endif
    if(file.write(bytes)!=bytes.size())return file.errorString();
    const auto writeMs=trace?timer.nsecsElapsed()/1e6:0.;if(trace)timer.restart();
    if(!file.commit())return file.errorString();
    if(trace)qInfo().noquote()<<"recovery_cost"<<"bytes"<<bytes.size()<<"encodeDigestMs"<<encodeDigestMs
        <<"hashMs"<<hashMs<<"encodeFinalMs"<<encodeFinalMs<<"writeMs"<<writeMs<<"commitMs"<<timer.nsecsElapsed()/1e6;
    return {};
}

QString RecoveryStore::removeOwned(const QString& tabId) {
    if(!error_.isEmpty() || !lock_)return error_.isEmpty() ? "Recovery session is unavailable" : error_;
    if(!canonicalUuid(tabId))return "Invalid recovery tab ID";
    const auto path=recordPath(sessionId_,tabId);
    if(!QFileInfo::exists(path) && !QFileInfo(path).isSymLink())return {};
    if(QFileInfo(path).isSymLink() || !QFile::remove(path))return "Cannot remove owned recovery snapshot";
    return {};
}

QStringList RecoveryStore::ownedTabIds() const {
    QStringList ids;
    if(!error_.isEmpty() || !lock_)return ids;
    const QDir directory(QDir(root_).filePath(sessionId_));
    for(const auto& file:directory.entryInfoList(QStringList{"*.json"},QDir::Files|QDir::NoSymLinks)) {
        const auto id=QFileInfo(file.fileName()).completeBaseName();
        if(canonicalUuid(id))ids.append(id);
    }
    return ids;
}

QString RecoveryStore::removeAllOwned() {
    for(const auto& id:ownedTabIds())if(const auto error=removeOwned(id);!error.isEmpty())return error;
    return {};
}

QJsonArray RecoveryStore::list() const {
    QJsonArray rows;
    if(!error_.isEmpty())return rows;
    const QDir root(root_);
    for(const auto& session:root.entryInfoList(QDir::Dirs|QDir::NoDotAndDotDot|QDir::NoSymLinks)) {
        const auto sessionId=session.fileName();
        if(!canonicalUuid(sessionId) || sessionId==sessionId_)continue;
        const auto path=session.absoluteFilePath();
        const auto lockPath=QDir(path).filePath("owner.lock");
        if(QFileInfo(lockPath).isSymLink())continue;
        QLockFile guard(lockPath);guard.setStaleLockTime(0);
        if(!guard.tryLock(0))continue; // Another editor still owns this session.
        for(const auto& file:QDir(path).entryInfoList(QStringList{"*.json"},QDir::Files|QDir::NoSymLinks)) {
            const auto tabId=QFileInfo(file.fileName()).completeBaseName();
            if(!canonicalUuid(tabId))continue;
            const auto id=sessionId+":"+tabId;
            const auto parsed=parseRecord(file.absoluteFilePath(),id);
            rows.append(QJsonObject{{"id",id},{"status",parsed.status},{"reason",parsed.error},
                {"tabId",tabId},{"documentId",parsed.record.documentId},{"originalPath",parsed.record.originalPath},
                {"capturedAtUtc",parsed.record.capturedAtUtc},
                {"featureCount",parsed.status=="ready" ? parsed.record.authored.value("features").toArray().size() : 0}});
        }
    }
    return rows;
}

QString RecoveryStore::transfer(const QString& id,const std::function<QString(const Record&)>& consumer) {
    if(!error_.isEmpty() || !lock_)return error_.isEmpty() ? "Recovery session is unavailable" : error_;
    QString sessionId,tabId;
    if(!splitId(id,&sessionId,&tabId) || sessionId==sessionId_)return "Invalid stale recovery ID";
    const auto sessionPath=QDir(root_).filePath(sessionId);
    if(!QFileInfo(sessionPath).isDir() || QFileInfo(sessionPath).isSymLink())return "Recovery session is unavailable";
    const auto lockPath=QDir(sessionPath).filePath("owner.lock");
    if(QFileInfo(lockPath).isSymLink())return "Recovery owner lock is a symbolic link";
    QLockFile guard(lockPath);guard.setStaleLockTime(0);
    if(!guard.tryLock(0))return "Recovery belongs to a live editor";
    const auto path=recordPath(sessionId,tabId);
    if(!QFileInfo(path).isFile() || QFileInfo(path).isSymLink())return "Recovery snapshot is unavailable";
    const auto parsed=parseRecord(path,id);
    if(parsed.status!="ready")return parsed.error;
    const auto error=consumer(parsed.record);
    if(!error.isEmpty())return error;
    if(!QFile::remove(path))return "Recovered copy is safe, but old recovery could not be retired";
    return {};
}

QString RecoveryStore::discard(const QString& id) {
    if(!error_.isEmpty() || !lock_)return error_.isEmpty() ? "Recovery session is unavailable" : error_;
    QString sessionId,tabId;
    if(!splitId(id,&sessionId,&tabId) || sessionId==sessionId_)return "Invalid stale recovery ID";
    const auto sessionPath=QDir(root_).filePath(sessionId);
    if(!QFileInfo(sessionPath).isDir() || QFileInfo(sessionPath).isSymLink())return "Recovery session is unavailable";
    const auto lockPath=QDir(sessionPath).filePath("owner.lock");
    if(QFileInfo(lockPath).isSymLink())return "Recovery owner lock is a symbolic link";
    QLockFile guard(lockPath);guard.setStaleLockTime(0);
    if(!guard.tryLock(0))return "Recovery belongs to a live editor";
    const auto path=recordPath(sessionId,tabId);
    if(!QFileInfo(path).isFile() || QFileInfo(path).isSymLink())return "Recovery snapshot is unavailable";
    return QFile::remove(path) ? QString{} : QString{"Cannot discard recovery snapshot"};
}
