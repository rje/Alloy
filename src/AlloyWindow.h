#pragma once
#include <QMainWindow>
#include <QJsonArray>
#include <QJsonObject>
#include <QMap>
#include <functional>
#include <vector>
#include "CadDocument.h"
#include "SketchGeometry.h"

class QLabel;
class QTreeWidget;
class QDialog;
class ReferenceView;
class QComboBox;
class QLineEdit;
class QDoubleSpinBox;
class QCloseEvent;
class QTreeWidgetItem;
class QHBoxLayout;
class QDockWidget;
class QShortcut;
class QCheckBox;
class QPushButton;
class QScrollArea;
class QListWidget;

// UI and automation dispatch against the same authored document.
class AlloyWindow final : public QMainWindow {
public:
    AlloyWindow();
    QJsonObject execute(const QString& command, const QJsonObject& args = {});
    QJsonObject state() const;
    QJsonArray commands() const;
    QJsonObject identity() const;
protected:
    void closeEvent(QCloseEvent*) override;
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
    void showPrimitive();
    void updatePrimitive(const QJsonObject& inputs);
    QJsonObject beginExtrude(const QJsonObject& inputs);
    QString validateExtrudeInputs(const QJsonObject& inputs) const;
    QJsonObject extrusionCandidate(const QJsonObject& inputs, bool hideSource) const;
    QJsonObject previewExtrude();
    void showExtrude();
    void refreshExtrudeControls();
    void updateExtrude(const QJsonObject& inputs);
    QJsonObject beginCombine(const QJsonObject& inputs);
    QJsonObject combineCandidate(const QJsonObject& inputs) const;
    QString validateCombineInputs(const QJsonObject& inputs) const;
    std::vector<EvaluatedBody> combineInputBodies() const;
    QJsonObject previewCombine();
    void showCombine();
    void refreshCombineControls();
    void updateCombine(const QJsonObject& inputs);
    void selectCombineBody(const QString& bodyId);
    QJsonObject selectFinishedProfile(const QString& sketchId, const QString& profileId);
    QJsonArray dependentBodies(const QString& sketchId) const;
    QJsonObject primitiveCandidate(const QString&, const QJsonObject&) const;
    QJsonObject beginSketch(const QString& plane, const QString& name, const QString& sketchId = {});
    QJsonObject appendSketchEntity(const QString& tool, const QJsonObject& inputs);
    QJsonObject appendSketchCurve(const QString& tool, const QJsonObject& inputs);
    QJsonObject sketchCurveGesture(const QJsonObject& point, bool commit);
    double sketchDimensionMeasure(const QJsonObject& targets, const QString& kind) const;
    void showSketch();
    void showCurveEntry();
    void sketchResponse(const QJsonObject& response);
    QJsonObject publishSketchSolve(const SketchGeometry::SolveResult& solved);
    QJsonObject setSketchSelection(const QJsonObject& selection);
    QJsonObject sketchDimensionTargets(const QJsonObject& args) const;
    void refreshSketchControls();
    void dimensionSelectionChanged();
    QJsonObject previewDimension(const QJsonObject& args);
    QJsonObject acceptDimension(const QJsonObject& args);
    QJsonObject editDimension(const QString& id);
    void cancelDimension();
    void showDimensionEditor();
    void positionDimensionEditor();
    QJsonObject inlineEditorState() const;
    bool confirmDiscard();
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
    QWidget* primitivePanel_ = nullptr;
    QDockWidget* primitiveDock_ = nullptr;
    QMainWindow* canvasWindow_ = nullptr;
    QJsonObject primitiveWidgetInputs() const;
    QMap<QString,QDoubleSpinBox*> primitiveInputs_;
    QLabel* primitiveError_ = nullptr;
    QLineEdit* primitiveName_ = nullptr;
    QWidget* extrudePanel_ = nullptr;
    QDockWidget* extrudeDock_ = nullptr;
    QComboBox* extrudeProfile_ = nullptr;
    QDoubleSpinBox* extrudeDistance_ = nullptr;
    QLabel* extrudeError_ = nullptr;
    QPushButton* extrudeOk_ = nullptr;
    QShortcut* extrudeEscape_ = nullptr;
    QWidget* combinePanel_ = nullptr;
    QDockWidget* combineDock_ = nullptr;
    QComboBox* combineTarget_ = nullptr;
    QListWidget* combineTools_ = nullptr;
    QComboBox* combineOperation_ = nullptr;
    QCheckBox* combineKeepTools_ = nullptr;
    QLabel* combineError_ = nullptr;
    QPushButton* combineOk_ = nullptr;
    QPushButton* combinePickTarget_ = nullptr;
    QPushButton* combinePickTools_ = nullptr;
    QShortcut* combineEscape_ = nullptr;
    QTreeWidgetItem* bodiesNode_ = nullptr;
    QTreeWidgetItem* sketchesNode_ = nullptr;
    QWidget* sketchPanel_ = nullptr;
    QDockWidget* sketchDock_ = nullptr;
    QWidget* sketchToolbar_ = nullptr;
    QShortcut* sketchEscape_ = nullptr;
    QLabel* sketchInfo_ = nullptr;
    QLabel* sketchError_ = nullptr;
    QLineEdit* sketchName_ = nullptr;
    QMap<QString,QDoubleSpinBox*> sketchInputs_;
    QJsonObject displayedSketchDraft_;
    QJsonObject sketchWidgetDraft_, sketchWidgetSelection_;
    QString sketchWidgetUnits_;
    QTreeWidget* sketchSelectionTree_ = nullptr;
    QPushButton* sketchCurveEntry_ = nullptr;
    QComboBox* sketchDimensionKind_ = nullptr;
    QDoubleSpinBox* sketchDimensionValue_ = nullptr;
    QCheckBox* sketchDimensionDriving_ = nullptr;
    QComboBox* sketchDimensionList_ = nullptr;
    QDoubleSpinBox* sketchDimensionEdit_ = nullptr;
    QLabel* sketchDimensionMeasure_ = nullptr;
    QScrollArea* sketchScroll_ = nullptr;
    QWidget* dimensionEditor_ = nullptr;
    QLineEdit* dimensionEditorValue_ = nullptr;
    QLabel* dimensionEditorError_ = nullptr;
    QHBoxLayout* timelineLayout_ = nullptr;
    QLabel* documentTitle_ = nullptr;
    QLabel* browserNote_ = nullptr;
    QString selectedBody_;
    QString selectedSketch_, selectedProfile_;
    quint64 displayedDocumentRevision_ = 0;
    bool authorizedQuit_ = false;
    QJsonObject session_;
    QString instanceId_ = CadDocument::newId();
    bool browserVisible_ = true;
    bool timelineVisible_ = true;
    bool navigationVisible_ = true;
    quint64 revision_ = 0;
    CadDocument document_;
};
