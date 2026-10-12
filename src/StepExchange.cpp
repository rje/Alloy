#include "StepExchange.h"

#include <BRepCheck_Analyzer.hxx>
#include <BRep_Tool.hxx>
#include <DESTEP_Parameters.hxx>
#include <IFSelect_ReturnStatus.hxx>
#include <STEPControl_Reader.hxx>
#include <STEPControl_Writer.hxx>
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
#include <cmath>
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

StepExchange::WriteResult StepExchange::write(const TopoDS_Shape& shape, const QString& outputUnits) {
    if (outputUnits != "mm" && outputUnits != "in")
        reject("step_output_units", "STEP output units must be mm or in");
    const auto solid = onlySolid(shape);
    STEPControl_Writer writer;
    DESTEP_Parameters parameters;
    parameters.WriteUnit = outputUnits == "in" ? UnitsMethods_LengthUnit_Inch
                                                : UnitsMethods_LengthUnit_Millimeter;
    if (writer.Transfer(solid, STEPControl_ManifoldSolidBrep, parameters) != IFSelect_RetDone)
        reject("step_export_transfer_failed", "OCCT could not translate the solid to STEP");
    std::ostringstream stream(std::ios::binary);
    if (writer.WriteStream(stream) != IFSelect_RetDone)
        reject("step_export_write_failed", "OCCT could not write the STEP data");
    const auto data = stream.str();
    if (data.empty()) reject("step_export_empty", "OCCT wrote empty STEP data");
    return {QByteArray(data.data(), static_cast<qsizetype>(data.size())),
        {{"code", "step_written"}, {"outputLengthUnit", outputUnits},
         {"solidCount", 1}, {"translator", "OCCT STEPControl"}}};
}
