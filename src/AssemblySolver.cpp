// SPDX-License-Identifier: MIT
#include "AssemblySolver.h"
#include <ASMTAssembly.h>
#include <ASMTPart.h>
#include <ASMTMarker.h>
#include <ASMTRevoluteJoint.h>
#include <ASMTCylindricalJoint.h>
#include <ASMTRotationalMotion.h>
#include <ASMTJoint.h>
#include <ASMTFixedJoint.h>
#include <charconv>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <map>
#include <mutex>
#include <string>
#include <vector>
namespace {
using V=std::array<double,3>;
constexpr double pi=3.14159265358979323846;
constexpr double positionToleranceMm=1e-5, angleTolerance=1e-6;
std::mutex solveMutex;
V sub(V a,V b){return {a[0]-b[0],a[1]-b[1],a[2]-b[2]};}
double dot(V a,V b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];}
V cross(V a,V b){return {a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};}
double norm(V a){return std::sqrt(dot(a,a));}
V axis(const AlloyAssemblyPose& p,int column){return {p.rotation[column],p.rotation[3+column],p.rotation[6+column]};}
V position(const AlloyAssemblyPose& p){return {p.translationMm[0],p.translationMm[1],p.translationMm[2]};}
V rotate(const AlloyAssemblyPose& p,V v){return {dot({p.rotation[0],p.rotation[1],p.rotation[2]},v),dot({p.rotation[3],p.rotation[4],p.rotation[5]},v),dot({p.rotation[6],p.rotation[7],p.rotation[8]},v)};}
AlloyAssemblyPose compose(const AlloyAssemblyPose& a,const AlloyAssemblyPose& b){AlloyAssemblyPose p{};auto t=rotate(a,position(b));for(int i=0;i<3;++i){p.translationMm[i]=a.translationMm[i]+t[i];for(int j=0;j<3;++j)for(int k=0;k<3;++k)p.rotation[3*i+j]+=a.rotation[3*i+k]*b.rotation[3*k+j];}return p;}
bool rigid(const AlloyAssemblyPose& p){for(double d:p.translationMm)if(!std::isfinite(d)||std::abs(d)>1e9)return false;for(double d:p.rotation)if(!std::isfinite(d))return false;for(int i=0;i<3;++i)for(int j=0;j<3;++j){const auto d=dot(axis(p,i),axis(p,j));if(!std::isfinite(d)||std::abs(d-(i==j?1.:0.))>1e-8)return false;}return std::abs(dot(axis(p,0),cross(axis(p,1),axis(p,2)))-1.)<=1e-8;}
double axisAngle(V a,V b){return std::atan2(norm(cross(a,b)),dot(a,b));}
double rotationAngle(const AlloyAssemblyPose& a,const AlloyAssemblyPose& b){double r[9]{};for(int i=0;i<3;++i)for(int j=0;j<3;++j)r[3*i+j]=dot(axis(a,i),axis(b,j));return std::atan2(.5*norm({r[7]-r[5],r[2]-r[6],r[3]-r[1]}),std::clamp((r[0]+r[4]+r[8]-1.)*.5,-1.,1.));}
AlloyAssemblyStatus reject(AlloyAssemblyReport& r,AlloyAssemblyStatus status,const char* text,const char* id=""){r.status=status;std::snprintf(r.message,sizeof(r.message),"%s",text);std::snprintf(r.failedId,sizeof(r.failedId),"%s",id);return status;}
bool validId(const char* id){return id && *id && std::char_traits<char>::length(id)<=127;}
void setPose(const std::shared_ptr<MbD::ASMTSpatialItem>& item,const AlloyAssemblyPose& p){item->setPosition3D(p.translationMm[0]*.001,p.translationMm[1]*.001,p.translationMm[2]*.001);item->setRotationMatrix(p.rotation[0],p.rotation[1],p.rotation[2],p.rotation[3],p.rotation[4],p.rotation[5],p.rotation[6],p.rotation[7],p.rotation[8]);}
}
extern "C" uint32_t alloy_assembly_solver_abi_version(void){return 1;}
extern "C" AlloyAssemblyStatus alloy_assembly_solver_solve(const AlloyAssemblyPart* parts,size_t partCount,const AlloyAssemblyJoint* joints,size_t jointCount,const AlloyAssemblyDrive* drive,AlloyAssemblyPose* output,AlloyAssemblyReport* report){
 if(!report)return ALLOY_ASSEMBLY_INVALID_INPUT;
 *report={};auto& r=*report;
 if(!parts||!output||partCount==0||(jointCount&&!joints))return reject(r,ALLOY_ASSEMBLY_INVALID_INPUT,"Missing input or output array");
 if(partCount>64||jointCount>128)return reject(r,ALLOY_ASSEMBLY_UNSUPPORTED,"Bounded adapter supports at most 64 leaves/128 joints");
 try{
  std::map<std::string,size_t> partIds,jointIds;size_t pinCount=0;
  for(size_t i=0;i<partCount;++i){if(!validId(parts[i].occurrenceId)||!rigid(parts[i].world))return reject(r,ALLOY_ASSEMBLY_INVALID_INPUT,"Expected stable occurrence ID and finite proper rigid world pose");if(!partIds.emplace(parts[i].occurrenceId,i).second)return reject(r,ALLOY_ASSEMBLY_INVALID_INPUT,"Duplicate occurrence ID",parts[i].occurrenceId);if(parts[i].pinned!=0&&parts[i].pinned!=1)return reject(r,ALLOY_ASSEMBLY_INVALID_INPUT,"Pin flag must be 0 or 1",parts[i].occurrenceId);pinCount+=parts[i].pinned;}
  if(pinCount!=1)return reject(r,ALLOY_ASSEMBLY_UNSUPPORTED,"Exactly one pinned leaf is required");
  for(size_t i=0;i<jointCount;++i){auto& j=joints[i];if(!validId(j.jointId)||!validId(j.occurrenceI)||!validId(j.occurrenceJ)||!rigid(j.localI)||!rigid(j.localJ))return reject(r,ALLOY_ASSEMBLY_INVALID_INPUT,"Expected stable joint IDs and finite proper local frames");if(!jointIds.emplace(j.jointId,i).second)return reject(r,ALLOY_ASSEMBLY_INVALID_INPUT,"Duplicate joint ID",j.jointId);if(!partIds.contains(j.occurrenceI)||!partIds.contains(j.occurrenceJ)||std::string(j.occurrenceI)==j.occurrenceJ)return reject(r,ALLOY_ASSEMBLY_INVALID_INPUT,"Joint needs two distinct current leaf occurrences",j.jointId);if(j.type!=ALLOY_ASSEMBLY_REVOLUTE&&j.type!=ALLOY_ASSEMBLY_CYLINDRICAL)return reject(r,ALLOY_ASSEMBLY_UNSUPPORTED,"Only Revolute and Cylindrical are admitted",j.jointId);}
  if(drive&&(!validId(drive->jointId)||!std::isfinite(drive->angleRadians)||std::abs(drive->angleRadians)>pi||!jointIds.contains(drive->jointId)))return reject(r,ALLOY_ASSEMBLY_INVALID_INPUT,"Prescribed angle needs a current joint and principal angle in [-pi, pi] radians");
  if(drive&&joints[jointIds.at(drive->jointId)].type!=ALLOY_ASSEMBLY_REVOLUTE)return reject(r,ALLOY_ASSEMBLY_UNSUPPORTED,"Only one Revolute angle can be prescribed",drive->jointId);
  std::lock_guard<std::mutex> lock(solveMutex);
  auto assembly=MbD::ASMTAssembly::With();assembly->setName("Alloy");std::vector<std::shared_ptr<MbD::ASMTPart>> solverParts;
  for(size_t i=0;i<partCount;++i){auto p=MbD::ASMTPart::With();p->setName("p"+std::to_string(i));setPose(p,parts[i].world);assembly->addPart(p);solverParts.push_back(p);}
  // Upstream isFixed constrains absolute coordinates to zero, not the current
  // placement. A mature FixedJoint to a root marker preserves arbitrary Pin.
  for(size_t i=0;i<partCount;++i)if(parts[i].pinned){auto rootMarker=MbD::ASMTMarker::With();rootMarker->setName("rootPin");setPose(rootMarker,parts[i].world);assembly->addMarker(rootMarker);auto localMarker=MbD::ASMTMarker::With();localMarker->setName("pinAnchor");solverParts[i]->addMarker(localMarker);auto fixed=MbD::ASMTFixedJoint::With();fixed->setName("pin");fixed->setMarkerI("/Alloy/rootPin");fixed->setMarkerJ("/Alloy/p"+std::to_string(i)+"/pinAnchor");assembly->addJoint(fixed);}
  for(size_t i=0;i<jointCount;++i){auto& j=joints[i];std::string paths[2];int k=0;for(auto pair:{std::pair{j.occurrenceI,&j.localI},std::pair{j.occurrenceJ,&j.localJ}}){const auto index=partIds.at(pair.first);auto marker=MbD::ASMTMarker::With();const auto name="m"+std::to_string(i)+"_"+std::to_string(k);marker->setName(name);setPose(marker,*pair.second);solverParts[index]->addMarker(marker);paths[k++]="/Alloy/p"+std::to_string(index)+"/"+name;}
   std::shared_ptr<MbD::ASMTJoint> q=j.type==ALLOY_ASSEMBLY_REVOLUTE?std::static_pointer_cast<MbD::ASMTJoint>(MbD::ASMTRevoluteJoint::With()):std::static_pointer_cast<MbD::ASMTJoint>(MbD::ASMTCylindricalJoint::With());q->setName("j"+std::to_string(i));q->setMarkerI(paths[0]);q->setMarkerJ(paths[1]);assembly->addJoint(q);}
  if(drive){auto m=MbD::ASMTRotationalMotion::With();m->setName("drive");m->setMotionJoint("/Alloy/j"+std::to_string(jointIds.at(drive->jointId)));char value[64];auto converted=std::to_chars(value,value+sizeof(value),drive->angleRadians);m->setRotationZ(std::string(value,converted.ptr));assembly->addMotion(m);}
  assembly->runPreDrag();std::vector<AlloyAssemblyPose> solved(partCount);
  for(size_t i=0;i<partCount;++i){auto& p=solved[i];solverParts[i]->getPosition3D(p.translationMm[0],p.translationMm[1],p.translationMm[2]);for(double& d:p.translationMm)d*=1000.;for(int row=0;row<3;++row)for(int col=0;col<3;++col)p.rotation[3*row+col]=solverParts[i]->rotationMatrix->at(row)->at(col);if(!rigid(p))return reject(r,ALLOY_ASSEMBLY_SOLVER_ERROR,"Solver produced a nonfinite or improper rigid pose",parts[i].occurrenceId);if(parts[i].pinned){const auto t=norm(sub(position(p),position(parts[i].world))),angle=rotationAngle(p,parts[i].world);if(!std::isfinite(t)||!std::isfinite(angle))return reject(r,ALLOY_ASSEMBLY_SOLVER_ERROR,"Nonfinite Pin metric",parts[i].occurrenceId);r.maxPinErrorMm=std::max(r.maxPinErrorMm,t);r.maxPinRotationErrorRadians=std::max(r.maxPinRotationErrorRadians,angle);if(t>positionToleranceMm||angle>angleTolerance)return reject(r,ALLOY_ASSEMBLY_CONFLICT,"Solved pose violated the full Pin",parts[i].occurrenceId);}}
  for(size_t i=0;i<jointCount;++i){auto& j=joints[i];const auto a=compose(solved[partIds.at(j.occurrenceI)],j.localI),b=compose(solved[partIds.at(j.occurrenceJ)],j.localJ);if(!rigid(a)||!rigid(b))return reject(r,ALLOY_ASSEMBLY_SOLVER_ERROR,"Nonfinite or out-of-range composed marker frame",j.jointId);const auto delta=sub(position(b),position(a));const auto distance=j.type==ALLOY_ASSEMBLY_REVOLUTE?norm(delta):norm(cross(delta,axis(a,2)));const auto angle=axisAngle(axis(a,2),axis(b,2));if(!std::isfinite(distance)||!std::isfinite(angle))return reject(r,ALLOY_ASSEMBLY_SOLVER_ERROR,"Nonfinite original joint metric",j.jointId);r.maxOriginErrorMm=std::max(r.maxOriginErrorMm,distance);r.maxAxisErrorRadians=std::max(r.maxAxisErrorRadians,angle);if(distance>positionToleranceMm||angle>angleTolerance)return reject(r,ALLOY_ASSEMBLY_CONFLICT,"Solved pose violated an original joint",j.jointId);if(drive&&std::string(j.jointId)==drive->jointId){const auto measured=std::atan2(dot(axis(a,2),cross(axis(a,0),axis(b,0))),dot(axis(a,0),axis(b,0)));r.driveErrorRadians=std::abs(std::remainder(measured-drive->angleRadians,2*pi));if(!std::isfinite(measured)||!std::isfinite(r.driveErrorRadians))return reject(r,ALLOY_ASSEMBLY_SOLVER_ERROR,"Nonfinite prescribed angle metric",j.jointId);if(r.driveErrorRadians>angleTolerance)return reject(r,ALLOY_ASSEMBLY_CONFLICT,"Solved pose missed the prescribed signed angle",j.jointId);}}
  std::copy(solved.begin(),solved.end(),output);r.status=ALLOY_ASSEMBLY_SUCCESS;std::snprintf(r.message,sizeof(r.message),"Converged; every original joint, full Pin and prescribed angle checked");return r.status;
 }catch(const std::exception& e){return reject(r,ALLOY_ASSEMBLY_SOLVER_ERROR,e.what());}catch(...){return reject(r,ALLOY_ASSEMBLY_SOLVER_ERROR,"Unknown assembly solver failure");}
}
