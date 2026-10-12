#pragma once
#include <QMainWindow>
#include <QJsonArray>
#include <QJsonObject>
#include <functional>
#include <vector>
#include "CadDocument.h"

class QLabel;
class QTreeWidget;
class QDialog;
class ReferenceView;
class QComboBox;
class QLineEdit;

// UI and automation dispatch against the same authored document.
class AlloyWindow final : public QMainWindow {
public:
    AlloyWindow();
    QJsonObject execute(const QString& command, const QJsonObject& args = {});
    QJsonObject state() const;
    QJsonArray commands() const;
    QJsonObject identity() const;
private:
    struct Command {
        QString id, title, description;
        QJsonObject parameters;
        bool available;
        std::function<QJsonObject(const QJsonObject&)> run;
    };
    void registerCommands();
    void refresh();
    void showSearch();
    QWidget* commandButton(const QString& id, const QString& glyph, bool large = false);
    const Command* findCommand(const QString& id) const;
    QString validateArguments(const Command& command, const QJsonObject& args, QString* field = nullptr) const;
    QJsonObject beginSession(const QString& command, const QJsonObject& inputs);
    QJsonObject finishSession(const QString& id, bool commit);
    void showUnits();
    std::vector<Command> commands_;
    ReferenceView* view_ = nullptr;
    QWidget* browser_ = nullptr;
    QWidget* timeline_ = nullptr;
    QWidget* navigation_ = nullptr;
    QTreeWidget* tree_ = nullptr;
    QLabel* status_ = nullptr;
    QDialog* search_ = nullptr;
    QDialog* unitsDialog_ = nullptr;
    QComboBox* unitsInput_ = nullptr;
    QLineEdit* searchInput_ = nullptr;
    QJsonObject session_;
    QString instanceId_ = CadDocument::newId();
    bool browserVisible_ = true;
    bool timelineVisible_ = true;
    bool navigationVisible_ = true;
    quint64 revision_ = 0;
    CadDocument document_;
};
