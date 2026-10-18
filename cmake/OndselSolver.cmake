# SPDX-License-Identifier: MIT
# Exact public-source snapshot remains under the build directory. Never use
# upstream's Windows static default: both kernel and original ABI are shared.
set(ALLOY_ONDSEL_REVISION 4be80eef02a3486cda0d78f3ccbb308d207a9639)
include(FetchContent)
FetchContent_Declare(alloy_ondsel
    URL https://codeload.github.com/FreeCAD/OndselSolver/tar.gz/${ALLOY_ONDSEL_REVISION}
    URL_HASH SHA256=701a5b6814114d3bd358700b4308e0c75c8fe86d24400d5accde0f3c11e3580a
    DOWNLOAD_EXTRACT_TIMESTAMP FALSE SOURCE_SUBDIR alloy-source-only)
FetchContent_MakeAvailable(alloy_ondsel)
set(ONDSELSOLVER_BUILD_SHARED ON)
add_subdirectory("${alloy_ondsel_SOURCE_DIR}/OndselSolver" "${alloy_ondsel_BINARY_DIR}/library")
set_target_properties(OndselSolver PROPERTIES AUTOMOC OFF CXX_STANDARD 17
    VERSION 1.0.1 SOVERSION 1 WINDOWS_EXPORT_ALL_SYMBOLS ON)
target_compile_definitions(OndselSolver PRIVATE TEST_DATA_PATH="${alloy_ondsel_SOURCE_DIR}/testapp")
add_library(alloy_assembly_solver SHARED "${CMAKE_CURRENT_LIST_DIR}/../src/AssemblySolver.cpp")
set_target_properties(alloy_assembly_solver PROPERTIES AUTOMOC OFF VERSION 0.1.0 SOVERSION 0)
target_compile_definitions(alloy_assembly_solver PRIVATE ALLOY_ASSEMBLY_SOLVER_BUILD)
if(MSVC)
    # Public Ondsel headers use M_PI; MSVC exposes it only when requested
    # before the header's math includes.
    target_compile_definitions(alloy_assembly_solver PRIVATE _USE_MATH_DEFINES)
endif()
target_include_directories(alloy_assembly_solver PUBLIC "${CMAKE_CURRENT_LIST_DIR}/../src")
target_include_directories(alloy_assembly_solver SYSTEM PRIVATE "${alloy_ondsel_SOURCE_DIR}/OndselSolver")
target_link_libraries(alloy_assembly_solver PRIVATE OndselSolver)
message(STATUS "OndselSolver ${ALLOY_ONDSEL_REVISION}: replaceable shared library + original POD/C ABI")
