// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "SketchConic.h"
#include <QJsonArray>

namespace {
QJsonObject failure(const QString& code,const QString& message) {
    return {{"ok",false},{"error",QJsonObject{{"code",code},{"message",message}}}};
}
}
QJsonObject AlloyWindow::conicCommand(const QString& operation,const QJsonObject& inputs) {
    if(session_.value("command")!="sketch.edit")
        return failure("no_active_sketch","Enter a sketch first");
    if(session_.value("baseDocumentRevision").toInteger()!=static_cast<qint64>(document_->revision()))
        return failure("stale_session","Restart the sketch after a document change");
    const auto draft=session_.value("draftSketch").toObject();
    const auto context=session_.value("projectionContext").toObject();
    SketchGeometry::SolveResult solved;
    if(operation=="add") solved=SketchConic::add(draft,inputs,context);
    else if(operation=="rho") solved=SketchConic::setRho(draft,inputs["entityId"].toString(),inputs["rho"].toDouble(),context);
    else return failure("unsupported_conic_operation","Unsupported conic command");
    auto result=publishSketchSolve(solved);
    if(solved.ok) {
        const auto id=operation=="add" ? solved.diagnostics["createdConicId"] : inputs["entityId"];
        setSketchSelection({{"entityIds",QJsonArray{id}}});
    }
    return result;
}
