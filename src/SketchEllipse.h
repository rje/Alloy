// SPDX-License-Identifier: MIT
#pragma once
#include <QJsonObject>

namespace SketchEllipse {
struct Axes {
    double centerU,centerV;
    double firstU,firstV,secondU,secondV;
    double firstRadius,secondRadius;
};
// The third pick lies on the contour, not necessarily on the second axis.
double secondRadiusFromContour(double centerU,double centerV,double firstU,double firstV,
    double contourU,double contourV);
// The two retained full construction lines are the authored axes. This
// calculation never renames them when the evaluated major axis changes.
Axes axes(const QJsonObject& parameters,const QJsonObject& ellipse);
}
