// SPDX-License-Identifier: MIT
#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <functional>
#include <memory>

class QLockFile;

// One atomic committed-authored snapshot per dirty tab. The instance lock
// keeps another Alloy process from offering a live editor's snapshots.
class RecoveryStore final {
public:
    struct Record {
        QString id;
        QString tabId;
        QString documentId;
        QString originalPath;
        QString capturedAtUtc;
        QJsonObject authored;
    };

    explicit RecoveryStore(QString root = {});
    ~RecoveryStore();
    RecoveryStore(const RecoveryStore&) = delete;
    RecoveryStore& operator=(const RecoveryStore&) = delete;

    QString error() const { return error_; }
    QString root() const { return root_; }
    QString sessionId() const { return sessionId_; }
    QString write(const QString& tabId, const QJsonObject& authored, const QString& originalPath);
    QString removeOwned(const QString& tabId);
    QStringList ownedTabIds() const;
    QString removeAllOwned();
    QJsonArray list() const;
    // The consumer must validate/evaluate and durably snapshot a new owner
    // before returning success. A failed consumer leaves the old record intact.
    QString transfer(const QString& id, const std::function<QString(const Record&)>& consumer);
    QString discard(const QString& id);

private:
    QString recordPath(const QString& sessionId, const QString& tabId) const;
    QString root_;
    QString sessionId_;
    QString error_;
    std::unique_ptr<QLockFile> lock_;
};
