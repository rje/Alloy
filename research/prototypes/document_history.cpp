// Permitted Qt/OCCT foundation experiment, not the Alloy app or a product gate.
#include <QCoreApplication>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QProcess>
#include <QSaveFile>
#include <QUndoStack>
#include <QUuid>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <BRepTools.hxx>
#include <BRep_Builder.hxx>
#include <BinDrivers.hxx>
#include <TDocStd_Application.hxx>
#include <TDocStd_Document.hxx>
#include <TDataStd_AsciiString.hxx>
#include <TDataStd_Real.hxx>
#include <TNaming_Builder.hxx>
#include <TNaming_NamedShape.hxx>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <cmath>

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
QByteArray bytes(const QJsonObject& intent) { return QJsonDocument(intent).toJson(QJsonDocument::Indented); }
QByteArray read(const QString& path) {
    QFile file(path); require(file.open(QIODevice::ReadOnly), "read failed"); return file.readAll();
}
bool save(const QString& path, const QJsonObject& intent) {
    QSaveFile file(path);
    const auto data = bytes(intent);
    return file.open(QIODevice::WriteOnly) && file.write(data) == data.size() && file.commit();
}
QJsonObject load(const QString& path) {
    QJsonParseError error;
    const auto json = QJsonDocument::fromJson(read(path), &error);
    require(error.error == QJsonParseError::NoError && json.isObject(), "invalid document");
    const auto intent = json.object();
    require(intent.value("schema").toString() == "alloy-document/1", "unsupported schema");
    return intent;
}
double volume(const TopoDS_Shape& shape) {
    require(!shape.IsNull() && BRepCheck_Analyzer(shape).IsValid(), "invalid BRep");
    GProp_GProps properties; BRepGProp::VolumeProperties(shape, properties); return properties.Mass();
}
class Replace final : public QUndoCommand {
public:
    Replace(QJsonObject& target, QJsonObject after, QString text)
        : target_(target), before_(target), after_(std::move(after)) { setText(text); }
    void undo() override { target_ = before_; }
    void redo() override { target_ = after_; }
private:
    QJsonObject& target_; QJsonObject before_, after_;
};
QJsonObject thin(const QString& dir) {
    const QString documentId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const QString featureId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const QJsonObject box{{"id", featureId}, {"type", "box"}, {"widthMm", 10.0}, {"depthMm", 20.0}, {"heightMm", 30.0}};
    const QJsonObject sketch{{"id", QUuid::createUuid().toString(QUuid::WithoutBraces)}, {"type", "future-sketch"}, {"plane", "XY"}, {"entities", QJsonArray{}}};
    QJsonObject intent{{"schema", "alloy-document/1"}, {"id", documentId}, {"units", "mm"}, {"features", QJsonArray{}}};
    QUndoStack stack;
    auto candidate = intent; candidate["features"] = QJsonArray{box, sketch};
    stack.push(new Replace(intent, candidate, "Create box and sketch intent"));
    require(stack.count() == 1 && !stack.isClean(), "create undo boundary");
    const auto created = intent;
    stack.undo(); require(intent.value("features").toArray().isEmpty(), "undo create");
    stack.redo(); require(intent == created, "redo IDs");
    // Group several authored changes in a single validated replacement, not preview pushes.
    auto features = intent.value("features").toArray();
    auto edit = features[0].toObject(); edit["widthMm"] = 15.0; edit["heightMm"] = 40.0;
    features[0] = edit; candidate = intent; candidate["features"] = features;
    stack.push(new Replace(intent, candidate, "Edit two dimensions"));
    require(stack.count() == 2, "grouped edit");
    const auto edited = intent;
    const QString path = dir + "/thin.alloy";
    require(save(path, intent), "save thin"); stack.setClean();
    stack.undo(); require(intent == created && !stack.isClean(), "undo saved edit");
    stack.redo(); require(intent == edited && stack.isClean(), "redo saved checkpoint");
    const auto beforeCancel = intent; const int count = stack.count();
    candidate = intent; candidate["units"] = "cancelled-preview"; // discard a disposable draft
    require(intent == beforeCancel && stack.count() == count, "cancel mutates document");
    edit["widthMm"] = -1.0;
    const bool valid = edit.value("widthMm").toDouble() > 0;
    if (valid) stack.push(new Replace(intent, candidate, "Invalid edit"));
    require(!valid && intent == beforeCancel && stack.count() == count, "invalid edit mutates document");
    stack.undo(); candidate = intent; candidate["label"] = "Branched edit";
    stack.push(new Replace(intent, candidate, "New branch"));
    require(!stack.canRedo() && !stack.isClean(), "redo branch/clean marker");
    const auto saved = load(path);
    require(saved == edited && saved["id"].toString() == documentId, "roundtrip intent/units/IDs");
    QUndoStack reopened; require(reopened.count() == 0, "session undo is not persisted");
    auto newer = edited; newer["schema"] = "alloy-document/999";
    require(save(dir + "/unknown.alloy", newer), "write unknown fixture");
    bool rejected = false; try { load(dir + "/unknown.alloy"); } catch (const std::runtime_error&) { rejected = true; }
    require(rejected, "unknown schema accepted");
    const auto original = read(path);
    QProcess crash;
    crash.start(QCoreApplication::applicationFilePath(), {"--crash-before-replace", path});
    require(crash.waitForFinished(10000) && crash.exitCode() == 71 && read(path) == original, "crash overwrote previous file");
    require(!save(dir, intent) && read(path) == original, "failed destination corrupted original");
    const auto authoredBox = saved["features"].toArray()[0].toObject();
    const auto shape = BRepPrimAPI_MakeBox(authoredBox["widthMm"].toDouble(),
        authoredBox["depthMm"].toDouble(), authoredBox["heightMm"].toDouble()).Shape();
    require(std::abs(volume(shape) - 12000) < 1e-7, "evaluated shape volume");
    std::ofstream stream((dir + "/evaluated.brep").toStdString());
    BRepTools::Write(shape, stream); stream.close();
    TopoDS_Shape restored; BRep_Builder builder;
    require(BRepTools::Read(restored, (dir + "/evaluated.brep").toStdString().c_str(), builder), "shape readback");
    require(std::abs(volume(restored) - 12000) < 1e-7, "shape roundtrip");
    return {{"result", "passed"}, {"bytes", original.size()}, {"schema", "alloy-document/1"},
        {"volumeMm3", volume(restored)}, {"undoPersisted", false}, {"atomicCrashBeforeReplace", true}};
}
QJsonObject ocaf(const QString& dir) {
    Handle(TDocStd_Application) app = new TDocStd_Application;
    BinDrivers::DefineFormat(app);
    Handle(TDocStd_Document) doc; app->NewDocument("BinOcaf", doc); doc->SetUndoLimit(20);
    const auto root = doc->Main();
    doc->NewCommand();
    const auto box = root.FindChild(1);
    const auto width = box.FindChild(1);
    const auto depth = box.FindChild(2);
    const auto height = box.FindChild(3);
    const auto sketch = root.FindChild(2);
    TDataStd_AsciiString::Set(root, "alloy-document/1;units=mm");
    TDataStd_AsciiString::Set(box, "box:stable-authored-id");
    TDataStd_AsciiString::Set(sketch, "future-sketch:stable-id;plane=XY;entities=[]");
    TDataStd_Real::Set(width, 10);
    TDataStd_Real::Set(depth, 20); TDataStd_Real::Set(height, 30);
    TNaming_Builder(box).Generated(BRepPrimAPI_MakeBox(10,20,30).Shape());
    require(doc->CommitCommand(), "OCAF create commit");
    require(doc->Undo() && !box.IsAttribute(TDataStd_AsciiString::GetID()), "OCAF undo create");
    require(doc->Redo(), "OCAF redo create");
    doc->NewCommand();
    TDataStd_Real::Set(width, 15);
    TDataStd_Real::Set(height, 40);
    Handle(TNaming_NamedShape) named; require(box.FindAttribute(TNaming_NamedShape::GetID(), named), "OCAF named shape");
    TNaming_Builder(box).Modify(named->Get(), BRepPrimAPI_MakeBox(15,20,40).Shape());
    require(doc->CommitCommand(), "OCAF grouped edit");
    require(doc->Undo(), "OCAF undo edit");
    Handle(TDataStd_Real) value; require(width.FindAttribute(TDataStd_Real::GetID(), value) && value->Get() == 10, "OCAF undo value");
    require(doc->Redo() && value->Get() == 15, "OCAF redo edit");
    doc->NewCommand(); TDataStd_Real::Set(width, 999); doc->AbortCommand();
    require(value->Get() == 15, "OCAF abort");
    const QString path = dir + "/ocaf.cbf";
    require(app->SaveAs(doc, TCollection_ExtendedString(path.toStdString().c_str())) == PCDM_SS_OK, "OCAF save");
    require(doc->Undo(), "OCAF branch undo");
    doc->NewCommand(); TDataStd_Real::Set(width, 12); require(doc->CommitCommand(), "OCAF branch commit");
    require(!doc->Redo(), "OCAF discarded redo branch");
    named.Nullify(); value.Nullify(); // Attributes must not outlive document closure.
    app->Close(doc);
    Handle(TDocStd_Document) restored;
    require(app->Open(TCollection_ExtendedString(path.toStdString().c_str()), restored) == PCDM_RS_OK, "OCAF reopen");
    Handle(TDataStd_AsciiString) id, units, sketchIntent;
    require(restored->Main().FindAttribute(TDataStd_AsciiString::GetID(), units) && units->Get() == "alloy-document/1;units=mm", "OCAF units/schema");
    require(restored->Main().FindChild(1).FindAttribute(TDataStd_AsciiString::GetID(), id) && id->Get() == "box:stable-authored-id", "OCAF persisted ID");
    require(restored->Main().FindChild(2).FindAttribute(TDataStd_AsciiString::GetID(), sketchIntent), "OCAF future sketch intent");
    require(restored->Main().FindChild(1).FindChild(1).FindAttribute(TDataStd_Real::GetID(), value) && value->Get() == 15, "OCAF value roundtrip");
    require(restored->Main().FindChild(1).FindChild(2).FindAttribute(TDataStd_Real::GetID(), value) && value->Get() == 20, "OCAF depth roundtrip");
    require(restored->Main().FindChild(1).FindChild(3).FindAttribute(TDataStd_Real::GetID(), value) && value->Get() == 40, "OCAF height roundtrip");
    require(restored->Main().FindChild(1).FindAttribute(TNaming_NamedShape::GetID(), named), "OCAF shape roundtrip");
    require(std::abs(volume(named->Get()) - 12000) < 1e-7, "OCAF shape volume");
    require(restored->GetAvailableUndos() == 0, "OCAF session undo not persisted");
    const QJsonObject result{{"result", "passed"}, {"bytes", read(path).size()}, {"volumeMm3", volume(named->Get())},
        {"undoPersisted", false}, {"atomicCrashBeforeReplace", "not verified for OCAF SaveAs"},
        {"applicationSchemaValidation", "requires application-owned validation"}};
    named.Nullify(); value.Nullify(); id.Nullify(); units.Nullify(); sketchIntent.Nullify();
    app->Close(restored); return result;
}
}
int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    if (app.arguments().size() == 3 && app.arguments()[1] == "--crash-before-replace") {
        QSaveFile file(app.arguments()[2]); require(file.open(QIODevice::WriteOnly), "crash fixture open");
        require(file.write("replacement not committed") == 25, "crash fixture write");
        std::_Exit(71);
    }
    try {
        require(app.arguments().size() == 2, "usage: document_history_spike output-directory");
        const auto dir = app.arguments()[1]; require(QDir().mkpath(dir), "output directory");
        const QJsonObject report{{"kind", "library-prototype-only"}, {"qt", qVersion()}, {"occt", "7.9.3"},
            {"thin", thin(dir)}, {"ocaf", ocaf(dir)}};
        require(save(dir + "/prototype.json", report), "report write");
        std::cout << bytes(report).constData(); return 0;
    } catch (const Standard_Failure& e) { std::cerr << e.GetMessageString() << '\n'; return 1;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
