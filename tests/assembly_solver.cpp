// SPDX-License-Identifier: MIT
#include "AssemblySolver.h"
#include <array>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <stdexcept>
void check(bool value,const char* text){if(!value)throw std::runtime_error(text);}
void near(double a,double b,double tolerance=1e-5){check(std::abs(a-b)<tolerance,"numeric contract mismatch");}
AlloyAssemblyPose pose(double x=0,double y=0,double z=0){return {{x,y,z},{1,0,0,0,1,0,0,0,1}};}
AlloyAssemblyPose pinFrame(double x=0,double z=0){return {{x,0,z},{1,0,0,0,0,1,0,-1,0}};}
void fullPose(const AlloyAssemblyPose& a,const AlloyAssemblyPose& b){for(int i=0;i<3;++i)near(a.translationMm[i],b.translationMm[i]);for(int i=0;i<9;++i)near(a.rotation[i],b.rotation[i],1e-7);}
void proper(const AlloyAssemblyPose& p){for(double v:p.translationMm)check(std::isfinite(v),"nonfinite translation");for(double v:p.rotation)check(std::isfinite(v),"nonfinite rotation");for(int a=0;a<3;++a)for(int b=0;b<3;++b){double d=0;for(int i=0;i<3;++i)d+=p.rotation[3*i+a]*p.rotation[3*i+b];near(d,a==b?1.:0.,1e-8);}const auto& r=p.rotation;near(r[0]*(r[4]*r[8]-r[5]*r[7])-r[1]*(r[3]*r[8]-r[5]*r[6])+r[2]*(r[3]*r[7]-r[4]*r[6]),1.,1e-8);}
int main(){try{
 check(alloy_assembly_solver_abi_version()==1,"ABI version");
 // A nonidentity full root Pin and Y-axis hinge exercise world/local frames,
 // exact signed angle and mm→SI→mm conversion through the actual shared ABI.
 auto world=pose(100,-200,300);double rotateZ[9]={0,-1,0,1,0,0,0,0,1};std::copy(rotateZ,rotateZ+9,world.rotation);
 std::array<AlloyAssemblyPart,2> seed{{{"lower",world,1},{"upper",world,0}}};
 AlloyAssemblyJoint hinge{"hinge","lower","upper",ALLOY_ASSEMBLY_REVOLUTE,pinFrame(0,80),pinFrame(0,80)};
 std::array<AlloyAssemblyPose,2> solved;AlloyAssemblyReport report;
 auto inputs=seed;check(alloy_assembly_solver_solve(seed.data(),2,&hinge,1,nullptr,solved.data(),&report)==ALLOY_ASSEMBLY_SUCCESS,report.message);
 for(int i=0;i<2;++i){fullPose(solved[i],seed[i].world);proper(solved[i]);}check(std::memcmp(inputs.data(),seed.data(),sizeof(seed))==0,"input changed");
 AlloyAssemblyDrive drive{"hinge",.15};check(alloy_assembly_solver_solve(seed.data(),2,&hinge,1,&drive,solved.data(),&report)==ALLOY_ASSEMBLY_SUCCESS,report.message);
 fullPose(solved[0],world);const double c=std::cos(.15),s=std::sin(.15);AlloyAssemblyPose expected{{100,-200-80*s,300+80*(1-c)},{0,-1,0,c,0,s,-s,0,c}};fullPose(solved[1],expected);for(auto& p:solved)proper(p);near(report.driveErrorRadians,0.,1e-7);
 // Negative drive demonstrates directed-axis sign with the same nonidentity Pin.
 drive.angleRadians=-.15;check(alloy_assembly_solver_solve(seed.data(),2,&hinge,1,&drive,solved.data(),&report)==ALLOY_ASSEMBLY_SUCCESS,report.message);fullPose(solved[0],world);
 const double negativeSin=std::sin(-.15);AlloyAssemblyPose negative{{100,-200-80*negativeSin,300+80*(1-c)},{0,-1,0,c,0,negativeSin,-negativeSin,0,c}};fullPose(solved[1],negative);drive.angleRadians=.15;
 // The independently specified source-sized four-part closed loop, no rigid
 // piston parent: three directed Y-pin Revolutes and axial Z Cylindrical.
 std::array<AlloyAssemblyPart,4> loop{{{"lower",pose(),1},{"upper",pose(),0},{"inner",pose(90),0},{"outer",pose(90,0,180),0}}};
 std::array<AlloyAssemblyJoint,4> joints{{
  {"hinge","lower","upper",ALLOY_ASSEMBLY_REVOLUTE,pinFrame(0,80),pinFrame(0,80)},
  {"lowerEye","lower","inner",ALLOY_ASSEMBLY_REVOLUTE,pinFrame(90),pinFrame()},
  {"upperEye","upper","outer",ALLOY_ASSEMBLY_REVOLUTE,pinFrame(90,180),pinFrame()},
  {"axis","inner","outer",ALLOY_ASSEMBLY_CYLINDRICAL,pose(),pose(0,0,-180)}
 }};
 std::array<AlloyAssemblyPose,4> loopOutput;
 check(alloy_assembly_solver_solve(loop.data(),4,joints.data(),4,nullptr,loopOutput.data(),&report)==ALLOY_ASSEMBLY_SUCCESS,report.message);
 for(int i=0;i<4;++i){fullPose(loopOutput[i],loop[i].world);proper(loopOutput[i]);}
 check(alloy_assembly_solver_solve(loop.data(),4,joints.data(),4,&drive,loopOutput.data(),&report)==ALLOY_ASSEMBLY_SUCCESS,report.message);
 fullPose(loopOutput[0],loop[0].world);for(auto& p:loopOutput)proper(p);
 auto upperExpected=pose(-80*s,0,80*(1-c));double rY[9]={c,0,s,0,1,0,-s,0,c};std::copy(rY,rY+9,upperExpected.rotation);fullPose(loopOutput[1],upperExpected);
 near(loopOutput[3].translationMm[0],90*c+100*s);near(loopOutput[3].translationMm[2],80-90*s+100*c);
 check(report.maxOriginErrorMm<1e-5&&report.maxAxisErrorRadians<1e-6&&report.maxPinErrorMm<1e-5&&report.maxPinRotationErrorRadians<1e-6,"independent original constraints");
 // Invalid/unsupported/refused results cannot modify caller-owned outputs.
 std::array<AlloyAssemblyPose,2> untouched{{pose(999),pose(999)}};auto sentinel=untouched;
 auto invalid=seed;invalid[1].world.rotation[0]=2.;check(alloy_assembly_solver_solve(invalid.data(),2,&hinge,1,nullptr,untouched.data(),&report)==ALLOY_ASSEMBLY_INVALID_INPUT,"reject nonrigid input");check(std::memcmp(untouched.data(),sentinel.data(),sizeof(sentinel))==0,"invalid output mutation");
 auto huge=seed;huge[1].world.translationMm[0]=1e308;check(alloy_assembly_solver_solve(huge.data(),2,&hinge,1,nullptr,untouched.data(),&report)==ALLOY_ASSEMBLY_INVALID_INPUT,"reject unsupported coordinate magnitude");check(std::memcmp(untouched.data(),sentinel.data(),sizeof(sentinel))==0,"large input output mutation");
 AlloyAssemblyDrive turns{"hinge",7.};check(alloy_assembly_solver_solve(seed.data(),2,&hinge,1,&turns,untouched.data(),&report)==ALLOY_ASSEMBLY_INVALID_INPUT,"reject turn-count drive");check(std::memcmp(untouched.data(),sentinel.data(),sizeof(sentinel))==0,"invalid drive output mutation");
 auto unsupported=hinge;unsupported.type=static_cast<AlloyAssemblyJointType>(99);check(alloy_assembly_solver_solve(seed.data(),2,&unsupported,1,nullptr,untouched.data(),&report)==ALLOY_ASSEMBLY_UNSUPPORTED,"unsupported type");check(std::memcmp(untouched.data(),sentinel.data(),sizeof(sentinel))==0,"unsupported output mutation");
 // Two distinct origins cannot be the same Revolute pivot between these leaves.
 std::array<AlloyAssemblyJoint,2> contradictory{{hinge,{"conflicting","lower","upper",ALLOY_ASSEMBLY_REVOLUTE,pinFrame(0,80),pinFrame(0,90)}}};
 check(alloy_assembly_solver_solve(seed.data(),2,contradictory.data(),2,nullptr,untouched.data(),&report)!=ALLOY_ASSEMBLY_SUCCESS,"original conflicting joint must refuse");check(std::memcmp(untouched.data(),sentinel.data(),sizeof(sentinel))==0,"conflict output mutation");
 std::cout<<"ASSEMBLY_SOLVER_OK full poses/original joints/Pin/signed drive/refusal\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}}
