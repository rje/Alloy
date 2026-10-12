// SPDX-License-Identifier: MIT
#include "SketchSolver.h"
#include <QJsonArray>
#include <cmath>
#include <iostream>
#include <stdexcept>

static void check(bool value,const char* message) { if(!value)throw std::runtime_error(message); }
static QJsonObject point(const char* id,double u,double v) {return {{"id",id},{"uMm",u},{"vMm",v}};}
static QJsonObject line(const char* id,const char* a,const char* b) {return {{"id",id},{"type","line"},{"startPointId",a},{"endPointId",b}};}
static QJsonObject constraint(const char* id,const char* type,const char* entity) {return {{"id",id},{"type",type},{"entityIds",QJsonArray{entity}}};}
static QJsonObject dimension(const char* id,const char* kind,const char* a,const char* b,double value) {
    return {{"id",id},{"kind",kind},{"pointIds",QJsonArray{a,b}},{"driving",true},{"valueMm",value}};
}
static QJsonObject rectangle() {
    return {{"plane","XY"},{"points",QJsonArray{point("a",0,0),point("b",39,0.2),point("c",40,24),point("d",-0.3,25)}},
        {"entities",QJsonArray{line("ab","a","b"),line("bc","b","c"),line("cd","c","d"),line("da","d","a")}},
        {"constraints",QJsonArray{QJsonObject{{"id","anchor"},{"type","fixed"},{"pointIds",QJsonArray{"a"}},{"uMm",0},{"vMm",0}},
            constraint("h1","horizontal","ab"),constraint("v1","vertical","bc"),constraint("h2","horizontal","cd"),constraint("v2","vertical","da")}},
        {"dimensions",QJsonArray{dimension("width","horizontal","a","b",40),dimension("height","vertical","a","d",25),
            QJsonObject{{"id","diagonal"},{"kind","linear"},{"pointIds",QJsonArray{"a","c"}},{"driving",false}}}}};
}
int main() {
    try {
        const auto source=rectangle();const auto solved=SketchSolver::solve(source);
        check(solved.accepted,qPrintable(solved.error));
        check(solved.diagnostics["dof"].toInt()==0,"rectangle must have zero DOF");
        check(source==rectangle(),"solve mutated input");
        const auto points=solved.parameters["points"].toArray();
        check(std::abs(points[2].toObject()["uMm"].toDouble()-40)<1e-6,"width not solved");
        check(std::abs(points[2].toObject()["vMm"].toDouble()-25)<1e-6,"height not solved");
        auto measurements=solved.diagnostics["dimensionValues"].toArray();
        check(std::abs(measurements[2].toObject()["valueMm"].toDouble()-std::hypot(40,25))<1e-6,"driven diagonal incorrect");
        check(!solved.parameters["dimensions"].toArray()[2].toObject().contains("valueMm"),"driven value leaked into authored data");
        auto changed=solved.parameters;auto dims=changed["dimensions"].toArray();auto width=dims[0].toObject();width["valueMm"]=50;dims[0]=width;changed["dimensions"]=dims;
        auto edited=SketchSolver::solve(changed);check(edited.accepted,qPrintable(edited.error));
        check(std::abs(edited.parameters["points"].toArray()[2].toObject()["uMm"].toDouble()-50)<1e-6,"dimension edit not solved");
        auto reversed=solved.parameters;dims=reversed["dimensions"].toArray();width=dims[0].toObject();width["pointIds"]=QJsonArray{"b","a"};dims[0]=width;reversed["dimensions"]=dims;
        auto reverseResult=SketchSolver::solve(reversed);check(reverseResult.accepted,qPrintable(reverseResult.error));
        check(std::abs(reverseResult.parameters["points"].toArray()[1].toObject()["uMm"].toDouble()-40)<1e-6,"reversed positive dimension changed branch");
        auto conflict=solved.parameters;dims=conflict["dimensions"].toArray();dims.append(dimension("impossible","horizontal","a","b",41));conflict["dimensions"]=dims;
        auto rejected=SketchSolver::solve(conflict);check(!rejected.accepted && rejected.parameters==conflict,"conflict changed usable input");
        check(!rejected.diagnostics["conflictingConstraintIds"].toArray().isEmpty(),"missing conflicting IDs");
        auto redundant=solved.parameters;auto cs=redundant["constraints"].toArray();cs.append(constraint("duplicate","horizontal","ab"));redundant["constraints"]=cs;
        rejected=SketchSolver::solve(redundant);check(!rejected.accepted && !rejected.diagnostics["redundantConstraintIds"].toArray().isEmpty(),"redundancy falsely accepted");
        // Independent non-rectangle equations: coincidence, parallel,
        // perpendicular, and positive Euclidean driving length.
        QJsonObject other{{"plane","XY"},{"points",QJsonArray{point("p",0,0),point("q",10,0),point("r",10.1,0.2),point("s",20,0.3),point("t",20,1),point("u",20.2,16)}},
            {"entities",QJsonArray{line("pq","p","q"),line("rs","r","s"),line("tu","t","u")}},
            {"constraints",QJsonArray{QJsonObject{{"id","origin"},{"type","fixed"},{"pointIds",QJsonArray{"p"}},{"uMm",0},{"vMm",0}},
                QJsonObject{{"id","join"},{"type","coincident"},{"pointIds",QJsonArray{"q","r"}}},
                QJsonObject{{"id","parallel"},{"type","parallel"},{"entityIds",QJsonArray{"pq","rs"}}},
                QJsonObject{{"id","perpendicular"},{"type","perpendicular"},{"entityIds",QJsonArray{"pq","tu"}}}}},
            {"dimensions",QJsonArray{dimension("pq-length","linear","p","q",10),dimension("rs-length","linear","r","s",10),dimension("length","linear","t","u",15)}}};
        const auto otherSolved=SketchSolver::solve(other);
        if(!otherSolved.accepted)throw std::runtime_error("anchored line-equations fixture: "+otherSolved.error.toStdString());
        check(otherSolved.diagnostics["dof"].toInt()>0,"free sketch must remain underconstrained");
        check(otherSolved.diagnostics["maxResidual"].toDouble()<1e-6,"independent equation residual too high");
        auto legacy=source;legacy.remove("constraints");legacy.remove("dimensions");
        auto unchanged=SketchSolver::solve(legacy);check(unchanged.accepted && unchanged.parameters==legacy,"legacy sketch schema changed");
        std::cout<<"PASS: real PlaneGCS constraints, driving/driven dimensions, edits, conflicts, redundancy, positive direction and copy isolation\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
