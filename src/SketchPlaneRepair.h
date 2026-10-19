// SPDX-License-Identifier: MIT
#pragma once
#include "GeometryKernel.h"
namespace SketchPlaneRepair {
bool ordinary(const QJsonObject& feature);
bool singleInset(const QJsonObject& feature);
struct Candidate { QJsonObject authored,reference,frame; };
Candidate recreateInset(const QJsonObject& failed,const EvaluatedDocument& prefix,const QString& sketchId,
    const QJsonObject& replacement,const QJsonObject& acceptedFrame,const QSet<QString>& oldGeneratedIds);
Candidate candidate(const QJsonObject& failed,const EvaluatedDocument& prefix,const QString& sketchId,
    const QJsonObject& replacement,const QJsonObject& acceptedFrame);
}
