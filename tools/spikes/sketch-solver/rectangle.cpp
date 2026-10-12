// SPDX-License-Identifier: MIT
// Independent bounded adapter fixture; upstream solver remains separately licensed.
#include "GCS.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <iostream>
#include <stdexcept>

static void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Fixture {
    std::array<double,11> p;
    double zero=0, width, height, radius, centerX;
    GCS::Point a,b,c,d;
    GCS::Line bottom;
    GCS::Circle circle;
    GCS::VEC_pD unknowns;
    GCS::System solver;
    explicit Fixture(double scale, bool dimensionWidth=true, bool withCircle=true)
        : p{0.2*scale,-0.1*scale,39*scale,0.3*scale,41*scale,24*scale,-0.2*scale,26*scale,
            19*scale,6*scale,4.5*scale},
          width(40*scale),height(25*scale),radius(5*scale),centerX(20*scale),
          a(&p[0],&p[1]),b(&p[2],&p[3]),c(&p[4],&p[5]),d(&p[6],&p[7]) {
        bottom.p1=a;bottom.p2=b;circle.center=GCS::Point(&p[8],&p[9]);circle.rad=&p[10];
        for(int i=0;i<(withCircle?11:8);++i) unknowns.push_back(&p[i]);
        solver.addConstraintEqual(a.x,&zero,1);solver.addConstraintEqual(a.y,&zero,2);
        solver.addConstraintHorizontal(a,b,3);solver.addConstraintVertical(b,c,4);
        solver.addConstraintHorizontal(c,d,5);solver.addConstraintVertical(d,a,6);
        if(dimensionWidth)solver.addConstraintDifference(a.x,b.x,&width,7);
        solver.addConstraintDifference(a.y,d.y,&height,8);
        if(withCircle) {
            solver.addConstraintEqual(circle.center.x,&centerX,9);
            solver.addConstraintCircleRadius(circle,&radius,10);
            solver.addConstraintTangent(bottom,circle,true,11);
        }
        solver.declareUnknowns(unknowns);
    }
    GCS::SolveStatus solve() {solver.initSolution();auto result=solver.solve();solver.applySolution();return result;}
    double residual(int last=11) {
        double r=0;for(int tag=1;tag<=last;++tag)r=std::max(r,std::abs(solver.calculateConstraintErrorByTag(tag)));return r;
    }
};

int main() {
    try {
        std::cout.precision(17);
        std::cout<<"{\"schema\":\"alloy-solver-spike/1\",\"cases\":[";
        bool first=true;
        for(double scale:{1.0,1.0/25.4}) {
            Fixture f(scale);
            const auto start=std::chrono::steady_clock::now();
            auto status=f.solve();
            const auto us=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count();
            auto r=f.residual();
            require(status==GCS::SolveStatus::Success,"rectangle/circle did not solve");
            require(f.solver.dofsNumber()==0,"unexpected constrained DOF");
            require(r<1e-7,"constraint residual above tolerance");
            require(std::abs(f.p[2]/scale-40)<1e-7 && std::abs(f.p[5]/scale-25)<1e-7,"rectangle coordinates wrong");
            require(std::abs(std::abs(f.p[9])/scale-5)<1e-7,"circle not tangent");
            if(!first)std::cout<<",";first=false;
            std::cout<<"{\"case\":\"rectangle_tangent_circle\",\"numericScale\":"<<scale
                <<",\"status\":"<<static_cast<int>(status)<<",\"dof\":"<<f.solver.dofsNumber()
                <<",\"maxSolverConstraintError\":"<<r<<",\"circleCenterYCanonicalMm\":"<<f.p[9]/scale
                <<",\"solveMicroseconds\":"<<us<<"}";
            f.width=50*scale;status=f.solve();
            require(status==GCS::SolveStatus::Success && std::abs(f.p[2]/scale-50)<1e-7,"dimension edit failed");
            std::cout<<",{\"case\":\"dimension_edit\",\"numericScale\":"<<scale
                <<",\"widthCanonicalMm\":"<<f.p[2]/scale<<",\"heightCanonicalMm\":"<<f.p[5]/scale
                <<",\"maxSolverConstraintError\":"<<f.residual()<<"}";
        }
        Fixture conflict(1);
        double impossibleWidth=41;
        conflict.solver.addConstraintDifference(conflict.a.x,conflict.b.x,&impossibleWidth,12);
        auto conflictStatus=conflict.solve();GCS::VEC_I conflicting,redundant;
        conflict.solver.getConflicting(conflicting);conflict.solver.getRedundant(redundant);
        require(!conflicting.empty(),"contradictory widths not diagnosed");
        std::cout<<",{\"case\":\"contradictory_dimensions\",\"status\":"<<static_cast<int>(conflictStatus)<<",\"conflictingTags\":[";
        for(size_t i=0;i<conflicting.size();++i){if(i)std::cout<<",";std::cout<<conflicting[i];}
        std::cout<<"]}";
        Fixture drag(1,false,false);require(drag.solve()==GCS::SolveStatus::Success,"underconstrained solve failed");
        const int freeDof=drag.solver.dofsNumber();require(freeDof==1,"expected one free rectangle width");
        double target=60;drag.solver.addConstraintEqual(drag.c.x,&target,-1);
        require(drag.solve()==GCS::SolveStatus::Success,"temporary drag solve failed");
        require(std::abs(drag.p[4]-60)<1e-7 && drag.p[2]>0 && std::abs(drag.p[5]-25)<1e-7,"drag branch or target wrong");
        drag.solver.clearByTag(-1);require(drag.solve()==GCS::SolveStatus::Success,"temporary drag removal failed");
        require(drag.solver.dofsNumber()==1,"temporary drag became a permanent constraint");
        std::cout<<",{\"case\":\"temporary_drag_positive_branch\",\"dofBefore\":"<<freeDof<<",\"dofAfter\":"<<drag.solver.dofsNumber()<<",\"finalWidth\":"<<drag.p[4]<<"}],\"result\":\"passed\"}\n";
    } catch(const std::exception& e) {std::cerr<<e.what()<<"\n";return 1;}
}
