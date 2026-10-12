#pragma once
#include <QJsonObject>
#include <QUndoStack>
#include "GeometryKernel.h"
#include <memory>

class DocumentEdit;

// Authored intent only. Evaluated BReps and viewport objects are separate caches.
class CadDocument final {
public:
    CadDocument();
    const QJsonObject& authored() const { return authored_; }
    QJsonObject state() const;
    quint64 revision() const { return revision_; }
    QUndoStack& history() { return history_; }
    void reset();
    QString save(const QString& path);
    QString open(const QString& path);
    QString path() const { return path_; }
    const std::vector<EvaluatedBody>& bodies() const { return *bodies_; }
    QString commit(const QJsonObject& candidate, const QString& label);
    static QString validate(const QJsonObject& candidate);
    static double millimeters(double value, const QString& units);
    static QString newId();
private:
    friend class DocumentEdit;
    void apply(const QJsonObject& value, std::shared_ptr<const std::vector<EvaluatedBody>> bodies, const QString& transaction = {});
    QJsonObject authored_;
    QUndoStack history_;
    quint64 revision_ = 0;
    QString lastTransaction_;
    QString path_;
    std::shared_ptr<const std::vector<EvaluatedBody>> bodies_ = std::make_shared<const std::vector<EvaluatedBody>>();
};
