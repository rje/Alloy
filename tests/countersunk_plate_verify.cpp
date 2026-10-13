// SPDX-License-Identifier: MIT
// Independent saved-BRep opening and tunnel oracle for the finite #249 plate.
#include "CadDocument.h"
#include <BRepBndLib.hxx>
#include <BRepClass3d_SolidClassifier.hxx>
#include <Bnd_Box.hxx>
#include <QCoreApplication>
#include <TopAbs_State.hxx>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
bool material(const TopoDS_Shape& shape, double x, double y, double z) {
    BRepClass3d_SolidClassifier classifier(shape);
    classifier.Perform(gp_Pnt(x, y, z), 1e-7);
    require(classifier.State() != TopAbs_ON, "Sample landed on an ambiguous boundary");
    return classifier.State() == TopAbs_IN;
}
void near(double actual, double expected, double tolerance = 1e-4) {
    require(std::abs(actual - expected) <= tolerance, "Saved plate bounds or volume differ");
}
}

int main(int argc, char** argv) {
    QCoreApplication application(argc, argv);
    try {
        require(argc == 2, "Usage: countersunk_plate_verify saved-plate.alloy");
        CadDocument document;
        const auto error = document.open(QString::fromLocal8Bit(argv[1]));
        require(error.isEmpty(), "Saved Alloy document did not reopen");
        require(document.bodies().size() == 1, "Expected one evaluated body");
        const auto& body = document.bodies()[0];
        require(body.summary["valid"].toBool() && body.summary["closed"].toBool()
                    && body.summary["topology"].toObject()["solids"] == 1,
                "Saved body must be one valid closed solid");
        near(body.summary["volumeMm3"].toDouble(), 36500 - 353*std::acos(-1.0), 1e-5);
        Bnd_Box bounds;
        BRepBndLib::Add(body.shape, bounds);
        double x0, y0, z0, x1, y1, z1;
        bounds.Get(x0, y0, z0, x1, y1, z1);
        near(x0, -37.5); near(y0, -25); near(z0, 0);
        near(x1, 37.5); near(y1, 25); near(z1, 10);
        int openings = 0;
        for (double x : {-27.5, 0.0, 27.5}) for (double y : {15.0, 0.0, -15.0}) {
            // Top, mid-frustum and bottom radial pairs plus center samples
            // distinguish conical entries from painted circles or blind holes.
            require(!material(body.shape, x, y, 9.95), "Top entry is closed");
            require(!material(body.shape, x+4.8, y, 9.95), "Top Ø10 entry missing");
            require(material(body.shape, x+5.2, y, 9.95), "Top entry oversize");
            require(!material(body.shape, x+3.3, y, 8.5), "Countersink frustum missing");
            require(material(body.shape, x+3.7, y, 8.5), "Countersink frustum oversize");
            require(!material(body.shape, x, y, 6.5), "Upper bore is blocked");
            require(!material(body.shape, x, y, 3.5), "Middle bore is blocked");
            require(!material(body.shape, x, y, .05), "Bottom bore does not exit");
            require(!material(body.shape, x+1.8, y, .05), "Bottom Ø4 opening missing");
            require(material(body.shape, x+2.2, y, .05), "Bottom opening oversize");
            ++openings;
        }
        std::cout << "PASS: saved one-solid BRep, 75x50x10 envelope, " << openings
                  << " distinct Ø10 top entries, Ø4 exits and continuous bores\n";
        return 0;
    } catch (const std::exception& exception) {
        std::cerr << exception.what() << '\n';
        return 1;
    }
}
