#include "CadDocument.h"
#include <QCoreApplication>
#include <QJsonArray>
#include <iostream>
#include <cmath>
#include <stdexcept>
void check(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
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
        document.reset(); check(document.authored()["documentId"].toString() != root && document.history().count() == 0, "new identity lifetime");
        std::cout << "authored ownership, units and identity contracts passed\n"; return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
