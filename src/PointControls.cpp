// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "SketchGeometry.h"
#include "ReferenceView.h"
namespace {
QJsonObject failure(const QString& code,const QString& message) {
    return {{"ok",false},{"error",QJsonObject{{"code",code},{"message",message}}}};
}
}
QJsonObject AlloyWindow::appendStandalonePoint(const QJsonObject& inputs) {
    if(session_.value("command")!="sketch.edit")return failure("no_active_sketch","Enter a sketch first");
    if(inputs.contains("session") && inputs["session"]!=session_["id"])return failure("invalid_session","Session is no longer active");
    if(session_.value("baseDocumentRevision").toInteger()!=static_cast<qint64>(document_->revision()))return failure("stale_session","The document changed; cancel and restart this sketch");
    const auto units=inputs.value("units").toString(session_["inputs"].toObject()["units"].toString());
    QJsonObject canonical{{"uMm",CadDocument::millimeters(inputs["x"].toDouble(),units)},
        {"vMm",CadDocument::millimeters(inputs["y"].toDouble(),units)}};
    if(inputs.contains("snapTarget"))canonical["snapTarget"]=inputs["snapTarget"];
    return publishSketchSolve(SketchGeometry::addStandalonePoint(session_["draftSketch"].toObject(),canonical,session_["projectionContext"].toObject()));
}
