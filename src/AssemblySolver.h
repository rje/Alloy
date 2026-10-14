// SPDX-License-Identifier: MIT
#pragma once
#include <stddef.h>
#include <stdint.h>
#ifdef _WIN32
# ifdef ALLOY_ASSEMBLY_SOLVER_BUILD
#  define ALLOY_ASSEMBLY_API __declspec(dllexport)
# else
#  define ALLOY_ASSEMBLY_API __declspec(dllimport)
# endif
#else
# define ALLOY_ASSEMBLY_API __attribute__((visibility("default")))
#endif
#ifdef __cplusplus
extern "C" {
#endif
// Original POD boundary: no Qt, OCCT or third-party C++ types. Row-major proper
// rotation; translation in authored millimeters, each component within +/-1e9. Joint frames are local to
// their leaf occurrence. A pinned leaf retains its complete supplied world pose.
typedef struct AlloyAssemblyPose { double translationMm[3]; double rotation[9]; } AlloyAssemblyPose;
typedef struct AlloyAssemblyPart { const char* occurrenceId; AlloyAssemblyPose world; int pinned; } AlloyAssemblyPart;
typedef enum AlloyAssemblyJointType { ALLOY_ASSEMBLY_REVOLUTE=1, ALLOY_ASSEMBLY_CYLINDRICAL=2 } AlloyAssemblyJointType;
typedef struct AlloyAssemblyJoint {
    const char* jointId; const char* occurrenceI; const char* occurrenceJ;
    AlloyAssemblyJointType type; AlloyAssemblyPose localI; AlloyAssemblyPose localJ;
} AlloyAssemblyJoint;
// Optional single exact signed principal Revolute coordinate [-pi,pi]; no turn count. Angle is marker-J's X about
// common directed Z, measured from marker-I's X; do not add a baseline twice.
typedef struct AlloyAssemblyDrive { const char* jointId; double angleRadians; } AlloyAssemblyDrive;
typedef enum AlloyAssemblyStatus {
    ALLOY_ASSEMBLY_SUCCESS=0, ALLOY_ASSEMBLY_INVALID_INPUT=1,
    ALLOY_ASSEMBLY_UNSUPPORTED=2, ALLOY_ASSEMBLY_CONFLICT=3,
    ALLOY_ASSEMBLY_SOLVER_ERROR=4
} AlloyAssemblyStatus;
typedef struct AlloyAssemblyReport {
    AlloyAssemblyStatus status; char message[256]; char failedId[128];
    double maxOriginErrorMm; double maxAxisErrorRadians;
    double maxPinErrorMm; double maxPinRotationErrorRadians; double driveErrorRadians;
} AlloyAssemblyReport;
ALLOY_ASSEMBLY_API uint32_t alloy_assembly_solver_abi_version(void);
// Serialized fresh graph, immutable input. Exactly one pinned leaf is supported.
// At most 64 parts/128 joints. On any failure outputPoses is untouched. No solver
// state persists and no general DOF/rank claim is returned. Zero means every
// original joint, full Pin and optional exact target passed independent checks.
ALLOY_ASSEMBLY_API AlloyAssemblyStatus alloy_assembly_solver_solve(
    const AlloyAssemblyPart* parts,size_t partCount,
    const AlloyAssemblyJoint* joints,size_t jointCount,
    const AlloyAssemblyDrive* drive,AlloyAssemblyPose* outputPoses,AlloyAssemblyReport* report);
#ifdef __cplusplus
}
#endif
