#include "CadDocument.h"
#include "StepExchange.h"
#include <BRepPrimAPI_MakeBox.hxx>
#include <STEPCAFControl_Writer.hxx>
#include <XCAFApp_Application.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <TDataStd_Name.hxx>
#include <TDocStd_Document.hxx>
#include <DESTEP_Parameters.hxx>
#include <UnitsMethods_LengthUnit.hxx>
#include <gp_Ax1.hxx>
#include <gp_Trsf.hxx>
#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSet>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
void repeatedAssembly(const QString& path,bool repeat=true) {
    Handle(TDocStd_Document) source;
    XCAFApp_Application::GetApplication()->NewDocument(TCollection_ExtendedString("BinXCAF"),source);
    const auto tool=XCAFDoc_DocumentTool::ShapeTool(source->Main());
    const auto root=tool->NewShape(),group=tool->NewShape();
    TDataStd_Name::Set(root,TCollection_ExtendedString("Root"));
    TDataStd_Name::Set(group,TCollection_ExtendedString("Rotated group"));
    const auto definition=tool->AddShape(BRepPrimAPI_MakeBox(2,1,1).Shape(),false);
    TDataStd_Name::Set(definition,TCollection_ExtendedString("Repeated block"));
    gp_Trsf groupPlacement;groupPlacement.SetRotation(gp_Ax1(gp_Pnt(0,0,0),gp_Dir(0,0,1)),M_PI/2);
    groupPlacement.SetTranslationPart(gp_Vec(10,0,0));
    const auto groupOccurrence=tool->AddComponent(root,group,TopLoc_Location(groupPlacement));
    TDataStd_Name::Set(groupOccurrence,TCollection_ExtendedString("Rotated group"));
    for(const int x:{1,4}) {
        if(!repeat && x==4)continue;
        gp_Trsf placement;placement.SetTranslation(gp_Vec(x,0,0));
        const auto occurrence=tool->AddComponent(group,definition,TopLoc_Location(placement));
        TDataStd_Name::Set(occurrence,TCollection_ExtendedString(x==1?"First":"Second"));
    }
    tool->UpdateAssemblies();
    STEPCAFControl_Writer writer;writer.SetNameMode(true);
    DESTEP_Parameters parameters;parameters.WriteUnit=UnitsMethods_LengthUnit_Millimeter;
    check(writer.Transfer(source,parameters,STEPControl_AsIs),"Assembly fixture transfer");
    check(writer.Write(path.toUtf8().constData())==IFSelect_RetDone,"Assembly fixture write");
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

        const auto assemblyPath=temporary.filePath("nested-repeated.step");
        repeatedAssembly(assemblyPath);
        if(const auto output=qEnvironmentVariable("ALLOY_SYNTHETIC_ASSEMBLY_OUTPUT");!output.isEmpty()) {
            QFile::remove(output);
            check(QFile::copy(assemblyPath,output),"Persist synthetic assembly fixture");
        }
        QFile assemblyFile(assemblyPath);check(assemblyFile.open(QIODevice::ReadOnly),"Assembly fixture read");
        const auto assemblyBytes=assemblyFile.readAll();assemblyFile.close();
        const auto graph=StepExchange::readAssembly(assemblyBytes,CadDocument::newId());
        check(graph.leaves.size()==2 && graph.graph["definitions"].toArray().size()==3,
            "XDE retains root, group and shared solid definition");
        check(graph.leaves[0].definitionId==graph.leaves[1].definitionId &&
              graph.leaves[0].occurrenceId!=graph.leaves[1].occurrenceId,
            "Repeated components share definition and have separate occurrences");
        CadDocument assemblyDocument;
        const auto assemblyCandidate=CadDocument::stepImportCandidate(assemblyDocument.authored(),{{"path",assemblyPath}});
        check(assemblyCandidate["features"].toArray().first().toObject()["type"]=="stepAssemblyImport",
            "Assembly uses retained XDE feature");
        const auto assemblyError=assemblyDocument.commit(assemblyCandidate,"Import assembly");
        if(!assemblyError.isEmpty())std::cerr<<assemblyError.toStdString()<<' '
            <<QJsonDocument(assemblyDocument.lastEvaluationError()).toJson(QJsonDocument::Compact).toStdString()<<'\n';
        check(assemblyError.isEmpty(),"Assembly commits as one feature");
        check(assemblyDocument.bodies().size()==2 && assemblyDocument.history().count()==1,
            "Assembly evaluates two leaf solids in one transaction");
        const auto first=assemblyDocument.bodies()[0],second=assemblyDocument.bodies()[1];
        check(first.definitionId==second.definitionId && first.occurrenceId!=second.occurrenceId,
            "Evaluated bodies retain XDE identities");
        const auto bounds1=first.summary["boundsMm"].toObject(),bounds2=second.summary["boundsMm"].toObject();
        const auto min1=bounds1["min"].toArray(),max1=bounds1["max"].toArray();
        const auto min2=bounds2["min"].toArray(),max2=bounds2["max"].toArray();
        check(std::abs(min1[0].toDouble()-9)<1e-5 && std::abs(max1[0].toDouble()-10)<1e-5 &&
            std::abs(min1[1].toDouble()-1)<1e-5 && std::abs(max1[1].toDouble()-3)<1e-5,
            "Nested rotated and translated placement applies once");
        check(std::abs(min2[1].toDouble()-4)<1e-5 && std::abs(max2[1].toDouble()-6)<1e-5,
            "Second occurrence has independent placement");
        check(!first.supportFaces.empty() && !first.solidEdges.empty() &&
            first.supportFaces.front().faceId!=second.supportFaces.front().faceId,
            "Imported planar references are occurrence qualified");
        const auto singleAssemblyPath=temporary.filePath("nested-one-leaf.step");
        repeatedAssembly(singleAssemblyPath,false);
        CadDocument oneLeafDocument;
        const auto oneLeaf=CadDocument::stepImportCandidate(oneLeafDocument.authored(),{{"path",singleAssemblyPath}});
        check(oneLeaf["features"].toArray().first().toObject()["type"]=="stepAssemblyImport",
            "One-leaf named XDE assembly retains hierarchy instead of using legacy solid import");
        check(oneLeafDocument.commit(oneLeaf,"Import one-leaf assembly").isEmpty() && oneLeafDocument.bodies().size()==1,
            "One-leaf assembly evaluates its retained hierarchy");
        const auto assemblyNative=temporary.filePath("nested-repeated.alloy");
        check(assemblyDocument.save(assemblyNative).isEmpty(),"Assembly save");
        check(QFile::remove(assemblyPath),"Remove assembly STEP source");
        CadDocument reopenedAssembly;
        check(reopenedAssembly.open(assemblyNative).isEmpty() && reopenedAssembly.bodies().size()==2,
            "Assembly reopens from embedded source");
        check(reopenedAssembly.bodies()[0].bodyId==first.bodyId &&
            reopenedAssembly.bodies()[0].supportFaces.front().faceId==first.supportFaces.front().faceId &&
            reopenedAssembly.bodies()[0].solidEdges.front().edgeId==first.solidEdges.front().edgeId,
            "Imported leaf, face and edge identities survive reopen");
        const auto authoredAssembly=assemblyDocument.authored();auto edited=authoredAssembly;
        auto rows=edited["features"].toArray();auto row=rows.first().toObject();auto p=row["parameters"].toObject();
        auto snapshot=p["assembly"].toObject();auto definitions=snapshot["definitions"].toArray();
        auto renamed=definitions.last().toObject();renamed["name"]="Falsified";definitions[definitions.size()-1]=renamed;
        snapshot["definitions"]=definitions;p["assembly"]=snapshot;row["parameters"]=p;rows[0]=row;edited["features"]=rows;
        check(!assemblyDocument.commit(edited,"Mutate snapshot").isEmpty(),"Snapshot mismatch rejects");
        check(assemblyDocument.authored()==authoredAssembly,"Rejected assembly edit is atomic");
        assemblyDocument.history().undo();check(assemblyDocument.bodies().empty(),"Assembly undo");
        assemblyDocument.history().redo();check(assemblyDocument.bodies().size()==2,"Assembly redo");
        if(const auto boardPath=qEnvironmentVariable("ALLOY_ASSEMBLY_FIXTURE");!boardPath.isEmpty()) {
            CadDocument board;
            const auto candidate=CadDocument::stepImportCandidate(board.authored(),{{"path",boardPath}});
            const auto error=board.commit(candidate,"Import board assembly");
            if(!error.isEmpty())std::cerr<<error.toStdString()<<' '
                <<QJsonDocument(board.lastEvaluationError()).toJson(QJsonDocument::Compact).toStdString()<<'\n';
            check(error.isEmpty() && board.bodies().size()==12,"External board assembly evaluates 12 solid leaves");
            bool port=false;
            for(const auto& body:board.bodies())if(body.name=="SIDE-USB-A-001") {
                const auto bounds=body.summary["boundsMm"].toObject();
                const auto low=bounds["min"].toArray(),high=bounds["max"].toArray();
                port=std::abs(low[0].toDouble()-70)<1e-5 && std::abs(low[1].toDouble()-19)<1e-5 &&
                    std::abs(low[2].toDouble()-1.6)<1e-5 && std::abs(high[0].toDouble()-88)<1e-5 &&
                    std::abs(high[1].toDouble()-35)<1e-5 && std::abs(high[2].toDouble()-17.6)<1e-5;
            }
            check(port,"External named leaf retains real world bounds");
            const auto imported=candidate["features"].toArray().first().toObject()["parameters"].toObject()["assembly"].toObject();
            check(imported["definitions"].toArray().size()==18 && imported["occurrences"].toArray().size()==18,
                "External board retains root, five groups and twelve named parts");
            QSet<QString> groupNames;
            for(const auto& value:imported["occurrences"].toArray())if(value.toObject()["kind"]=="assembly")
                groupNames.insert(value.toObject()["name"].toString());
            check(groupNames.contains("Board") && groupNames.contains("SidePorts") &&
                groupNames.contains("BottomPorts") && groupNames.contains("Underside") &&
                groupNames.contains("Interior"),"Browser uses five authored group names");
        }
        if(const auto plainMulti=qEnvironmentVariable("ALLOY_PLAIN_MULTI_FIXTURE");!plainMulti.isEmpty()) {
            CadDocument plain;
            try {
                const auto candidate=CadDocument::stepImportCandidate(plain.authored(),{{"path",plainMulti}});
                check(candidate["features"].toArray().first().toObject()["type"]=="stepImport",
                    "Unstructured multiple solids must not become a fabricated XDE assembly");
                check(!plain.commit(candidate,"Plain multi").isEmpty(),"Unstructured multiple solids reject atomically");
            } catch(const StepExchange::Error& error) {
                check(error.diagnostics()["code"]=="step_assembly_root" || error.diagnostics()["code"]=="step_assembly_name",
                    "Unstructured multiple solids reject with an assembly diagnostic");
            }
        }

        std::cout << "PASS: single STEP and retained nested repeated XDE assembly, placement, references, persistence, atomicity\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
