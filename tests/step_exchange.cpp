#include "CadDocument.h"
#include "StepExchange.h"
#include <BRepPrimAPI_MakeBox.hxx>
#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    try {
        QTemporaryDir temporary;
        check(temporary.isValid(), "Temporary directory");
        const auto sourcePath=temporary.filePath("fixture.step");
        const auto source=StepExchange::write(BRepPrimAPI_MakeBox(25.4,50.8,12.7).Shape(),"in");
        check(!source.bytes.isEmpty(), "STEP writer output");
        QFile fixture(sourcePath);
        check(fixture.open(QIODevice::WriteOnly), "Fixture write open");
        check(fixture.write(source.bytes)==source.bytes.size(), "Fixture write complete");
        fixture.close();

        CadDocument document;
        const auto candidate=CadDocument::stepImportCandidate(document.authored(),{{"path",sourcePath}});
        const auto importError=document.commit(candidate,"Import STEP");
        if (!importError.isEmpty()) std::cerr << importError.toStdString() << ' '
            << QJsonDocument(document.lastEvaluationError()).toJson(QJsonDocument::Compact).toStdString() << '\n';
        check(importError.isEmpty(),"Import candidate commit");
        check(document.bodies().size()==1 && document.history().count()==1,"One imported body and undo entry");
        auto summary=document.bodies().front().summary;
        check(summary["valid"].toBool() && summary["closed"].toBool(),"Imported solid valid and closed");
        check(std::abs(summary["volumeMm3"].toDouble()-25.4*50.8*12.7)<1e-4,"Inch STEP normalizes to mm");
        check(summary["sourcePolicy"]=="embedded","Imported source is embedded");

        const auto nativePath=temporary.filePath("retained.alloy");
        check(document.save(nativePath).isEmpty(),"Native save");
        check(QFile::remove(sourcePath),"Remove original STEP");
        CadDocument reopened;
        check(reopened.open(nativePath).isEmpty() && reopened.bodies().size()==1,
            "Native reopen must not need original STEP path");
        document.history().undo();
        check(document.bodies().empty(),"Import undo removes body");
        document.history().redo();
        check(document.bodies().size()==1,"Import redo restores body");

        const auto before=document.authored();
        auto corrupt=before;
        auto features=corrupt["features"].toArray();
        auto feature=features.first().toObject();
        auto parameters=feature["parameters"].toObject();
        auto sourceMetadata=parameters["source"].toObject();
        sourceMetadata["sha256"]=QString(64,'0');
        parameters["source"]=sourceMetadata;feature["parameters"]=parameters;features[0]=feature;corrupt["features"]=features;
        check(!document.commit(corrupt,"Corrupt import").isEmpty(),"Hash mismatch must reject");
        check(document.authored()==before && document.history().count()==1,"Rejected edit preserves authored history");

        const auto invalidPath=temporary.filePath("invalid.step");
        QFile invalid(invalidPath);
        check(invalid.open(QIODevice::WriteOnly),"Invalid fixture write open");
        check(invalid.write("not a STEP model")>0,"Invalid fixture write");
        invalid.close();
        const auto invalidCandidate=CadDocument::stepImportCandidate(document.authored(),{{"path",invalidPath}});
        check(!document.commit(invalidCandidate,"Invalid import").isEmpty(),"Malformed STEP must reject");
        check(document.authored()==before && document.history().count()==1,"Malformed import is atomic");

        std::cout << "PASS: STEP inch units, embedded reopen, undo/redo, invalid source rejection\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
