#pragma once
#include <QJsonObject>
#include <QUndoStack>

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
    QString commit(const QJsonObject& candidate, const QString& label);
    static QString validate(const QJsonObject& candidate);
    static double millimeters(double value, const QString& units);
    static QString newId();
private:
    friend class DocumentEdit;
    void apply(const QJsonObject& value);
    QJsonObject authored_;
    QUndoStack history_;
    quint64 revision_ = 0;
};
