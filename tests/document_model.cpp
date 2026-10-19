#include "CadDocument.h"
#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>
#include <QTemporaryDir>
#include <iostream>
#include <cmath>
#include <stdexcept>
void check(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }

void failedReplacementPreservesDocument(CadDocument& document) {
#ifdef Q_OS_UNIX
    QTemporaryDir directory;
    check(directory.isValid(), "save fixture directory");
    const auto path = directory.filePath("previous.alloy");
    check(document.save(path).isEmpty(), "save original document");
    const auto original = document.authored();
    auto read = [&] { QFile file(path); check(file.open(QIODevice::ReadOnly), "read native file"); return file.readAll(); };
    const auto originalBytes = read();
    auto edited = original;
    auto features = edited.value("features").toArray();
    auto feature = features[0].toObject();
    auto parameters = feature.value("parameters").toObject();
    parameters["widthMm"] = 30.4; feature["parameters"] = parameters;
    features[0] = feature; edited["features"] = features;
    check(document.commit(edited, "Widen box").isEmpty(), "edit saved geometry");
    auto extra = edited; extra["displayUnits"] = "in";
    check(document.commit(extra, "Display inches").isEmpty(), "create redo branch");
    document.history().undo();
    const auto before = document.state();
    check(document.isDirty() && document.history().canUndo() && document.history().canRedo(), "dirty fixture with undo and redo");

    // Keep the destination writable: a direct-write fallback would truncate it.
    // Refuse creation of the temporary replacement in its containing directory.
    const auto permissions = QFile::permissions(directory.path());
    struct RestorePermissions {
        QString path; QFileDevice::Permissions permissions;
        ~RestorePermissions() { QFile::setPermissions(path, permissions); }
    } restore{directory.path(), permissions};
    check(QFile::setPermissions(directory.path(), QFileDevice::ReadOwner | QFileDevice::ExeOwner), "restrict replacement directory");
    QFile probe(directory.filePath("creation-probe"));
    check(!probe.open(QIODevice::WriteOnly), "fixture requires enforced directory permissions; do not run as root");
    const auto error = document.save(path);
    check(!error.isEmpty(), "failed replacement reports failure");
    check(document.state() == before && document.authored() == edited, "failed save preserves dirty authored state, IDs, path and undo/redo");
    check(read() == originalBytes, "failed replacement preserves exact previous bytes");
    CadDocument reopened;
    check(reopened.open(path).isEmpty() && reopened.authored() == original, "previous document still opens with original committed state");
    check(std::abs(reopened.bodies()[0].summary.value("volumeMm3").toDouble() - 5080.0) < 1e-7, "reopened original geometry");

    check(QFile::setPermissions(directory.path(), permissions), "restore replacement directory");
    check(document.save(path).isEmpty() && !document.isDirty(), "retry saves dirty document after failure removed");
    check(document.authored() == edited && document.state().value("undo") == before.value("undo"), "successful retry preserves authored IDs and undo/redo");
    check(QJsonDocument::fromJson(read()).object() == edited, "retry writes complete edited authored document");
    check(reopened.open(path).isEmpty() && reopened.authored() == edited, "retry reopens edited state");
    document.history().undo();
    check(document.authored() == original && document.isDirty(), "undo remains usable after retry");
    document.history().redo();
    check(document.authored() == edited && !document.isDirty(), "redo restores saved checkpoint");
    std::cout << "failed replacement preserves prior file, dirty state, IDs and undo/redo; retry passes\n";
#else
    Q_UNUSED(document);
    std::cout << "failed replacement permission fixture is Unix-only; Windows qualification remains open\n";
#endif
}
int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    try {
        CadDocument document;
        const auto root = document.authored()["documentId"].toString();
        check(std::abs(CadDocument::millimeters(1, "in") - CadDocument::millimeters(25.4, "mm")) < 1e-12, "inch/mm conversion");
        auto candidate = document.authored();
        const QJsonObject feature{{"id", CadDocument::newId()}, {"bodyId", CadDocument::newId()},
            {"ownerId", root}, {"type", "box"}, {"name", "Box"}, {"visible", true},
            {"parameters", QJsonObject{{"widthMm", 25.4}, {"depthMm", 20.0}, {"heightMm", 10.0}, {"xMm", 0}, {"yMm", 0}, {"zMm", 0}}}};
        candidate["features"] = QJsonArray{feature};
        check(document.commit(candidate, "Create authored intent").isEmpty(), "valid authored feature");
        auto duplicate = candidate; duplicate["features"] = QJsonArray{feature, feature};
        check(!document.commit(duplicate, "Duplicate").isEmpty() && document.authored() == candidate, "duplicate identity refusal");
        auto foreign = feature; foreign["ownerId"] = CadDocument::newId();
        auto bad = candidate; bad["features"] = QJsonArray{foreign};
        check(!document.commit(bad, "Foreign owner").isEmpty(), "ownership refusal");
        auto units = candidate; units["displayUnits"] = "cm";
        check(!document.commit(units, "Unsupported units").isEmpty(), "unsupported units refusal");
        units["displayUnits"] = "in"; check(document.commit(units, "Display inches").isEmpty(), "display unit edit");
        check(document.authored()["features"] == candidate["features"], "display units rescaled geometry");
        document.history().undo(); check(document.authored() == candidate, "undo preserves IDs and intent");
        failedReplacementPreservesDocument(document);
        document.reset(); check(document.authored()["documentId"].toString() != root && document.history().count() == 0, "new identity lifetime");
        std::cout << "authored ownership, units and identity contracts passed\n"; return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
