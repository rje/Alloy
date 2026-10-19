// SPDX-License-Identifier: MIT
#pragma once
#include "GeometryKernel.h"
namespace SketchPlaneRepair {
bool ordinary(const QJsonObject& feature);
struct Candidate { QJsonObject authored,reference,frame; };
Candidate candidate(const QJsonObject& failed,const EvaluatedDocument& prefix,const QString& sketchId,
    const QJsonObject& replacement,const QJsonObject& acceptedFrame);
}
