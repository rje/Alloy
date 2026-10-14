#include "StepExchange.h"

#include <BRepCheck_Analyzer.hxx>
#include <BRep_Tool.hxx>
#include <DESTEP_Parameters.hxx>
#include <IFSelect_ReturnStatus.hxx>
#include <STEPControl_Reader.hxx>
#include <STEPControl_Writer.hxx>
#include <STEPCAFControl_Reader.hxx>
#include <STEPCAFControl_Writer.hxx>
#include <TCollection_ExtendedString.hxx>
#include <TDataStd_Name.hxx>
#include <TDF_Tool.hxx>
#include <TDocStd_Document.hxx>
#include <XCAFApp_Application.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <TopLoc_Location.hxx>
#include <TColStd_SequenceOfAsciiString.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <UnitsMethods_LengthUnit.hxx>
#include <UnitsMethods.hxx>
#include <QJsonArray>
#include <QHash>
#include <QRegularExpression>
#include <QSet>
#include <QUuid>
#include <cmath>
#include <functional>
#include <sstream>

namespace {
[[noreturn]] void reject(const QString& code, const QString& message, QJsonObject details = {}) {
    details["code"] = code;
    details["message"] = message;
    throw StepExchange::Error(details);
}

TopTools_IndexedMapOfShape shapes(const TopoDS_Shape& shape, TopAbs_ShapeEnum kind) {
    TopTools_IndexedMapOfShape result;
    TopExp::MapShapes(shape, kind, result);
    return result;
}

TopoDS_Shape onlySolid(const TopoDS_Shape& shape) {
    if (shape.IsNull()) reject("step_no_shape", "STEP translation produced no shape");
    const auto solids = shapes(shape, TopAbs_SOLID);
    if (solids.Extent() != 1)
        reject("step_solid_count", "This STEP slice requires exactly one solid", {{"solidCount", solids.Extent()}});
    const auto solid = solids(1);
    // A wrapping compound is normal; extra free faces, edges or vertices are not.
    for (const auto kind : {TopAbs_SHELL, TopAbs_FACE, TopAbs_WIRE, TopAbs_EDGE, TopAbs_VERTEX}) {
        if (shapes(shape, kind).Extent() != shapes(solid, kind).Extent())
            reject("step_extra_geometry", "STEP contains geometry outside its single solid");
    }
    if (!BRepCheck_Analyzer(solid).IsValid())
        reject("step_invalid_solid", "Translated STEP solid is invalid");
    const auto shells = shapes(solid, TopAbs_SHELL);
    for (Standard_Integer i = 1; i <= shells.Extent(); ++i) {
        if (!BRep_Tool::IsClosed(shells(i)))
            reject("step_open_solid", "Translated STEP solid is not closed");
    }
    return solid;
}

QString labelEntry(const TDF_Label& label) {
    TCollection_AsciiString entry;
    TDF_Tool::Entry(label,entry);
    return QString::fromLatin1(entry.ToCString());
}
QString labelName(const TDF_Label& label) {
    Handle(TDataStd_Name) attribute;
    if (!label.FindAttribute(TDataStd_Name::GetID(),attribute)) return {};
    const auto& wide=attribute->Get();
    return QString::fromUtf16(reinterpret_cast<const char16_t*>(wide.ToExtString()),wide.Length()).trimmed();
}
QString stableId(const QString& featureId,const QString& kind,const QString& key) {
    return QUuid::createUuidV5(QUuid(featureId),(kind+":"+key).toUtf8()).toString(QUuid::WithoutBraces);
}
QJsonArray placement(const gp_Trsf& trsf) {
    double columns[3][3]{};
    for(int column=1;column<=3;++column)for(int row=1;row<=3;++row) {
        const auto value=trsf.Value(row,column);
        if(!std::isfinite(value))reject("step_assembly_placement","STEP component has non-finite placement");
        columns[column-1][row-1]=value;
    }
    for(int column=0;column<3;++column)for(int other=0;other<3;++other) {
        double dot=0;for(int row=0;row<3;++row)dot+=columns[column][row]*columns[other][row];
        if(std::abs(dot-(column==other?1.0:0.0))>1e-8)
            reject("step_assembly_placement","STEP component placement must be rigid with unit scale");
    }
    const auto determinant=columns[0][0]*(columns[1][1]*columns[2][2]-columns[1][2]*columns[2][1])
        -columns[1][0]*(columns[0][1]*columns[2][2]-columns[0][2]*columns[2][1])
        +columns[2][0]*(columns[0][1]*columns[1][2]-columns[0][2]*columns[1][1]);
    if(std::abs(determinant-1)>1e-8)
        reject("step_assembly_placement","STEP component placement must preserve orientation");
    QJsonArray result;
    for(int row=1;row<=3;++row)for(int column=1;column<=4;++column) {
        const auto value=trsf.Value(row,column);
        if(!std::isfinite(value))reject("step_assembly_placement","STEP component has non-finite placement");
        result.append(value);
    }
    return result;
}

QJsonArray checkedUnits(STEPControl_Reader& reader) {
    TColStd_SequenceOfAsciiString lengthUnits,angleUnits,solidAngleUnits;
    reader.FileUnits(lengthUnits,angleUnits,solidAngleUnits);
    QJsonArray observedUnits;
    QSet<int> distinctUnits;
    static const QHash<QString,UnitsMethods_LengthUnit> units{
        {"MM",UnitsMethods_LengthUnit_Millimeter},{"MILLIMETRE",UnitsMethods_LengthUnit_Millimeter},
        {"MILLIMETER",UnitsMethods_LengthUnit_Millimeter},{"CM",UnitsMethods_LengthUnit_Centimeter},
        {"CENTIMETRE",UnitsMethods_LengthUnit_Centimeter},{"CENTIMETER",UnitsMethods_LengthUnit_Centimeter},
        {"M",UnitsMethods_LengthUnit_Meter},{"METRE",UnitsMethods_LengthUnit_Meter},{"METER",UnitsMethods_LengthUnit_Meter},
        {"IN",UnitsMethods_LengthUnit_Inch},{"INCH",UnitsMethods_LengthUnit_Inch},
        {"FT",UnitsMethods_LengthUnit_Foot},{"FOOT",UnitsMethods_LengthUnit_Foot},
        {"MICRON",UnitsMethods_LengthUnit_Micron},{"MICROMETRE",UnitsMethods_LengthUnit_Micron},
        {"MICROMETER",UnitsMethods_LengthUnit_Micron}};
    for(Standard_Integer i=1;i<=lengthUnits.Length();++i) {
        const auto unit=QString::fromUtf8(lengthUnits.Value(i).ToCString()).trimmed();
        observedUnits.append(unit);
        const auto utf8=unit.toUtf8();
        auto normalized=unit.toUpper();normalized.remove(QRegularExpression("[^A-Z0-9]"));
        const auto recognized=units.value(normalized,UnitsMethods::LengthUnitFromString(utf8.constData(),Standard_False));
        if(recognized==UnitsMethods_LengthUnit_Undefined)
            reject("step_units_ambiguous","STEP length unit is absent or unrecognized",{{"fileLengthUnits",observedUnits}});
        distinctUnits.insert(static_cast<int>(recognized));
    }
    if(distinctUnits.size()!=1)
        reject("step_units_ambiguous","STEP must declare one unambiguous length unit",{{"fileLengthUnits",observedUnits}});
    return observedUnits;
}
}

StepExchange::Error::Error(const QJsonObject& diagnostics)
    : std::runtime_error(diagnostics.value("message").toString().toStdString()), diagnostics_(diagnostics) {}

StepExchange::ReadResult StepExchange::read(const QByteArray& source) {
    if (source.isEmpty() || source.size() > maximumSourceBytes)
        reject("step_source_size", "STEP source must contain 1 byte to 32 MiB of data");
    const std::string input(source.constData(), static_cast<size_t>(source.size()));
    std::istringstream stream(input, std::ios::binary);
    STEPControl_Reader reader;
    DESTEP_Parameters parameters;
    reader.SetShapeFixParameters(DESTEP_Parameters::GetDefaultShapeFixParameters());
    if (reader.ReadStream("embedded.step", parameters, stream) != IFSelect_RetDone)
        reject("step_read_failed", "OCCT could not read the STEP source");

    TColStd_SequenceOfAsciiString lengthUnits, angleUnits, solidAngleUnits;
    reader.FileUnits(lengthUnits, angleUnits, solidAngleUnits);
    QJsonArray observedUnits;
    QSet<int> distinctUnits;
    static const QHash<QString, UnitsMethods_LengthUnit> stepLengthUnits{
        {"MM",UnitsMethods_LengthUnit_Millimeter},{"MILLIMETRE",UnitsMethods_LengthUnit_Millimeter},
        {"MILLIMETER",UnitsMethods_LengthUnit_Millimeter},
        {"CM",UnitsMethods_LengthUnit_Centimeter},{"CENTIMETRE",UnitsMethods_LengthUnit_Centimeter},
        {"CENTIMETER",UnitsMethods_LengthUnit_Centimeter},
        {"M",UnitsMethods_LengthUnit_Meter},{"METRE",UnitsMethods_LengthUnit_Meter},
        {"METER",UnitsMethods_LengthUnit_Meter},
        {"IN",UnitsMethods_LengthUnit_Inch},{"INCH",UnitsMethods_LengthUnit_Inch},
        {"FT",UnitsMethods_LengthUnit_Foot},{"FOOT",UnitsMethods_LengthUnit_Foot},
        {"MICRON",UnitsMethods_LengthUnit_Micron},{"MICROMETRE",UnitsMethods_LengthUnit_Micron},
        {"MICROMETER",UnitsMethods_LengthUnit_Micron}};
    for (Standard_Integer i = 1; i <= lengthUnits.Length(); ++i) {
        const auto unit = QString::fromUtf8(lengthUnits.Value(i).ToCString()).trimmed();
        observedUnits.append(unit);
        const auto utf8=unit.toUtf8();
        auto normalized=unit.toUpper();normalized.remove(QRegularExpression("[^A-Z0-9]"));
        const auto recognized=stepLengthUnits.value(normalized,
            UnitsMethods::LengthUnitFromString(utf8.constData(),Standard_False));
        if (recognized==UnitsMethods_LengthUnit_Undefined)
            reject("step_units_ambiguous", "STEP length unit is absent or unrecognized",
                {{"fileLengthUnits", observedUnits}});
        distinctUnits.insert(static_cast<int>(recognized));
    }
    if (distinctUnits.size() != 1)
        reject("step_units_ambiguous", "STEP must declare one unambiguous length unit",
            {{"fileLengthUnits", observedUnits}});
    reader.SetSystemLengthUnit(1.0); // Alloy's OCCT geometry is in millimeters.
    const auto roots = reader.NbRootsForTransfer();
    if (roots < 1)
        reject("step_transfer_failed", "STEP contains no transferable solid", {{"rootCount", roots}});
    const auto transferred=reader.TransferRoots();
    if (transferred!=roots)
        reject("step_transfer_failed", "STEP root translation was incomplete",
            {{"rootCount", roots}, {"transferredRoots", transferred}});
    const auto solid = onlySolid(reader.OneShape());
    return {solid, {{"code", "step_translated"}, {"fileLengthUnits", observedUnits},
        {"canonicalLengthUnit", "mm"}, {"rootCount", roots}, {"transferredRoots", transferred},
        {"shapeCount", reader.NbShapes()},
        {"solidCount", 1}, {"translator", "OCCT STEPControl"}}};
}

StepExchange::AssemblyResult StepExchange::readAssembly(const QByteArray& source,const QString& featureId) {
    if(source.isEmpty() || source.size()>maximumSourceBytes)
        reject("step_source_size","STEP source must contain 1 byte to 32 MiB of data");
    if(QUuid(featureId).isNull())reject("step_feature_id","Assembly import requires a valid feature UUID");
    std::istringstream stream(std::string(source.constData(),static_cast<size_t>(source.size())),std::ios::binary);
    STEPCAFControl_Reader reader;
    reader.SetNameMode(true);
    if(reader.ReadStream("embedded.step",stream)!=IFSelect_RetDone)
        reject("step_read_failed","OCCT could not read the STEP assembly source");
    auto& underlying=reader.ChangeReader();
    const auto units=checkedUnits(underlying);
    // All authored and evaluated coordinates use millimeters.
    underlying.SetSystemLengthUnit(1.0);
    Handle(TDocStd_Document) document;
    XCAFApp_Application::GetApplication()->NewDocument(TCollection_ExtendedString("BinXCAF"),document);
    if(document.IsNull() || !reader.Transfer(document))
        reject("step_transfer_failed","OCCT could not translate the complete STEP assembly");
    const auto tool=XCAFDoc_DocumentTool::ShapeTool(document->Main());
    TDF_LabelSequence roots;tool->GetFreeShapes(roots);
    if(roots.Length()!=1 || !XCAFDoc_ShapeTool::IsAssembly(roots.Value(1)))
        reject("step_assembly_root","STEP assembly requires one named free assembly root",{{"rootCount",roots.Length()}});
    for(const auto& value:units) {
        auto normalized=value.toString().toUpper();normalized.remove(QRegularExpression("[^A-Z0-9]"));
        if(normalized!="MM" && normalized!="MILLIMETRE" && normalized!="MILLIMETER")
            reject("step_assembly_units","This assembly import slice requires millimeter STEP input",{{"fileLengthUnits",units}});
    }
    AssemblyResult result;
    QJsonArray definitions,occurrences;
    QSet<QString> seenDefinitions;
    const auto root=roots.Value(1);
    const auto rootEntry=labelEntry(root);
    const auto rootId=stableId(featureId,"occurrence",rootEntry);
    std::function<void(const TDF_Label&,const TDF_Label&,const QString&,const QString&,const TopLoc_Location&,QSet<QString>)> walk;
    walk=[&](const TDF_Label& component,const TDF_Label& definition,const QString& path,const QString& parentId,
             const TopLoc_Location& parentLocation,QSet<QString> ancestry) {
        const auto definitionEntry=labelEntry(definition);
        if(ancestry.contains(definitionEntry))reject("step_assembly_cycle","STEP assembly contains a cyclic component reference");
        ancestry.insert(definitionEntry);
        const auto occurrenceId=stableId(featureId,"occurrence",path);
        const auto definitionId=stableId(featureId,"definition",definitionEntry);
        const bool isAssembly=XCAFDoc_ShapeTool::IsAssembly(definition);
        if(!seenDefinitions.contains(definitionEntry)) {
            const auto name=labelName(definition);
            if(name.isEmpty())reject("step_assembly_name","STEP definition has no name",{{"label",definitionEntry}});
            definitions.append(QJsonObject{{"id",definitionId},{"sourceLabel",definitionEntry},{"name",name},
                {"kind",isAssembly?"assembly":"solid"}});
            seenDefinitions.insert(definitionEntry);
        }
        const auto local=XCAFDoc_ShapeTool::GetLocation(component);
        const auto world=parentLocation*local;
        const auto componentName=labelName(component);
        // XDE supplies a reference arrow as a placeholder on some component
        // labels. The referred definition carries the authored part/group name.
        const auto name=componentName.isEmpty() || componentName.startsWith("=>[")
            ? labelName(definition) : componentName;
        if(name.isEmpty())reject("step_assembly_name","STEP component has no name",{{"path",path}});
        QJsonObject occurrence{{"id",occurrenceId},{"definitionId",definitionId},{"sourcePath",path},
            {"parentId",parentId},{"name",name},{"localPlacement",placement(local.Transformation())},
            {"kind",isAssembly?"assembly":"solid"}};
        if(isAssembly) {
            TDF_LabelSequence children;
            if(!tool->GetComponents(definition,children) || children.Length()==0)
                reject("step_assembly_empty","STEP assembly contains an empty group",{{"path",path}});
            occurrences.append(occurrence);
            for(Standard_Integer i=1;i<=children.Length();++i) {
                const auto child=children.Value(i);
                TDF_Label referred;
                if(!XCAFDoc_ShapeTool::GetReferredShape(child,referred))
                    reject("step_assembly_reference","STEP component has no definition",{{"path",path}});
                walk(child,referred,path+"/"+labelEntry(child),occurrenceId,world,ancestry);
            }
        } else {
            const auto original=tool->GetShape(definition);
            const auto solid=onlySolid(original);
            const auto worldSolid=solid.Moved(world);
            const auto bodyId=stableId(featureId,"body",path);
            occurrence["bodyId"]=bodyId;
            occurrences.append(occurrence);
            result.leaves.push_back({bodyId,occurrenceId,definitionId,name,worldSolid});
        }
    };
    walk(root,root,rootEntry,QString{},TopLoc_Location(),{});
    if(result.leaves.empty())reject("step_assembly_empty","STEP assembly has no solid leaves");
    result.graph={{"rootOccurrenceId",rootId},{"definitions",definitions},{"occurrences",occurrences}};
    result.diagnostics={{"code","step_assembly_translated"},{"canonicalLengthUnit","mm"},
        {"fileLengthUnits",units},{"definitionCount",definitions.size()},
        {"occurrenceCount",occurrences.size()},{"solidCount",static_cast<int>(result.leaves.size())},
        {"translator","OCCT STEPCAF/XDE"}};
    return result;
}

StepExchange::WriteResult StepExchange::write(const TopoDS_Shape& shape, const QString& outputUnits) {
    return write(std::vector<TopoDS_Shape>{shape},outputUnits);
}

StepExchange::WriteResult StepExchange::write(const std::vector<TopoDS_Shape>& inputShapes, const QString& outputUnits) {
    if (outputUnits != "mm" && outputUnits != "in")
        reject("step_output_units", "STEP output units must be mm or in");
    if(inputShapes.empty())reject("step_solid_count","Select at least one solid");
    if(inputShapes.size()>1){
        Handle(TDocStd_Document) document;
        XCAFApp_Application::GetApplication()->NewDocument(TCollection_ExtendedString("BinXCAF"),document);
        if(document.IsNull())reject("step_export_transfer_failed","OCCT could not create a STEP assembly document");
        const auto tool=XCAFDoc_DocumentTool::ShapeTool(document->Main());
        const auto root=tool->NewShape();TDataStd_Name::Set(root,TCollection_ExtendedString("Alloy export"));
        for(size_t index=0;index<inputShapes.size();++index){
            const auto solid=onlySolid(inputShapes[index]);
            const auto definition=tool->AddShape(solid,false);
            const auto name=QString("Solid %1").arg(index+1).toUtf8();
            TDataStd_Name::Set(definition,TCollection_ExtendedString(name.constData()));
            const auto instance=tool->AddComponent(root,definition,TopLoc_Location());
            TDataStd_Name::Set(instance,TCollection_ExtendedString(name.constData()));
        }
        tool->UpdateAssemblies();
        STEPCAFControl_Writer writer;writer.SetNameMode(true);
        DESTEP_Parameters parameters;parameters.WriteUnit=outputUnits=="in" ? UnitsMethods_LengthUnit_Inch : UnitsMethods_LengthUnit_Millimeter;
        if(!writer.Transfer(document,parameters,STEPControl_AsIs))
            reject("step_export_transfer_failed","OCCT could not translate the solid assembly to STEP");
        std::ostringstream stream(std::ios::binary);
        if(writer.WriteStream(stream)!=IFSelect_RetDone)
            reject("step_export_write_failed","OCCT could not write the STEP assembly");
        const auto data=stream.str();
        if(data.empty())reject("step_export_empty","OCCT wrote empty STEP data");
        return {QByteArray(data.data(),static_cast<qsizetype>(data.size())),
            {{"code","step_written"},{"outputLengthUnit",outputUnits},{"solidCount",int(inputShapes.size())},{"translator","OCCT STEPCAF/XDE"}}};
    }
    STEPControl_Writer writer;
    DESTEP_Parameters parameters;
    parameters.WriteUnit = outputUnits == "in" ? UnitsMethods_LengthUnit_Inch
                                                : UnitsMethods_LengthUnit_Millimeter;
    for(const auto& shape:inputShapes){
        const auto solid=onlySolid(shape);
        if (writer.Transfer(solid, STEPControl_ManifoldSolidBrep, parameters) != IFSelect_RetDone)
            reject("step_export_transfer_failed", "OCCT could not translate the solid to STEP");
    }
    std::ostringstream stream(std::ios::binary);
    if (writer.WriteStream(stream) != IFSelect_RetDone)
        reject("step_export_write_failed", "OCCT could not write the STEP data");
    const auto data = stream.str();
    if (data.empty()) reject("step_export_empty", "OCCT wrote empty STEP data");
    return {QByteArray(data.data(), static_cast<qsizetype>(data.size())),
        {{"code", "step_written"}, {"outputLengthUnit", outputUnits},
         {"solidCount", int(inputShapes.size())}, {"translator", "OCCT STEPControl"}}};
}
