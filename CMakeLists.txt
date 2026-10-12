cmake_minimum_required(VERSION 3.24)
project(Alloy VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
message(STATUS "Alloy toolchain: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}, ${CMAKE_SYSTEM_NAME}/${CMAKE_SYSTEM_PROCESSOR}")
if(DEFINED ALLOY_EXPECTED_COMPILER_ID AND NOT CMAKE_CXX_COMPILER_ID STREQUAL ALLOY_EXPECTED_COMPILER_ID)
    message(FATAL_ERROR "Compiler family drift from the reviewed native baseline")
endif()
if(DEFINED ALLOY_EXPECTED_COMPILER_VERSION AND NOT CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL ALLOY_EXPECTED_COMPILER_VERSION)
    message(FATAL_ERROR "Compiler version drift from the reviewed native baseline")
endif()
find_package(Qt6 6.11.2 EXACT REQUIRED COMPONENTS Widgets Network)

# OCCT is required: the viewport must use its native OpenGL renderer.
option(ALLOY_BUILD_SPIKES "Build disposable foundation research fixtures" OFF)
set(ALLOY_OCCT_COMPONENTS FoundationClasses ModelingData ModelingAlgorithms Visualization)
if(ALLOY_BUILD_SPIKES)
    list(APPEND ALLOY_OCCT_COMPONENTS ApplicationFramework)
endif()
find_package(OpenCASCADE 7.9.3 EXACT REQUIRED COMPONENTS ${ALLOY_OCCT_COMPONENTS})
set(ALLOY_OCCT_VERSION "${OpenCASCADE_MAJOR_VERSION}.${OpenCASCADE_MINOR_VERSION}.${OpenCASCADE_MAINTENANCE_VERSION}")
message(STATUS "OCCT ${ALLOY_OCCT_VERSION}: native OpenGL visualization")
message(STATUS "Qt ${Qt6_VERSION}; spikes=${ALLOY_BUILD_SPIKES}; OCCT components=${ALLOY_OCCT_COMPONENTS}")

include(cmake/PlaneGCS.cmake)
add_library(alloy_sketch_solver STATIC src/SketchSolver.cpp src/SketchSolver.h)
target_include_directories(alloy_sketch_solver PUBLIC src)
target_link_libraries(alloy_sketch_solver PUBLIC Qt6::Core PRIVATE alloy_planegcs)

add_library(alloy_kernel STATIC
    src/GeometryKernel.cpp src/GeometryKernel.h
    src/SketchGeometry.cpp src/SketchGeometry.h
    src/SketchProfiles.cpp src/SketchProfiles.h)
target_include_directories(alloy_kernel PUBLIC src)
target_include_directories(alloy_kernel SYSTEM PUBLIC ${OpenCASCADE_INCLUDE_DIR})
target_link_libraries(alloy_kernel PUBLIC alloy_sketch_solver Qt6::Core TKernel TKMath TKG3d TKBRep TKPrim TKTopAlgo)
target_compile_definitions(alloy_kernel PUBLIC ALLOY_OCCT_VERSION="${ALLOY_OCCT_VERSION}")

qt_add_executable(alloy MACOSX_BUNDLE
    src/main.cpp src/AlloyWindow.cpp src/AlloyWindow.h
    src/CadDocument.cpp src/CadDocument.h
    src/ReferenceView.cpp src/ReferenceView.h
    src/ControlServer.cpp src/ControlServer.h)
target_include_directories(alloy SYSTEM PRIVATE ${OpenCASCADE_INCLUDE_DIR})
target_link_libraries(alloy PRIVATE alloy_kernel Qt6::Widgets Qt6::Network TKService TKV3d TKOpenGl TKMesh)
target_compile_definitions(alloy PRIVATE ALLOY_OCCT_VERSION="${ALLOY_OCCT_VERSION}")
set_target_properties(alloy PROPERTIES
    MACOSX_BUNDLE_BUNDLE_NAME Alloy
    MACOSX_BUNDLE_GUI_IDENTIFIER org.alloycad.Alloy
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION})
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(alloy PRIVATE -Wall -Wextra -Wpedantic)
endif()
include(CTest)
if(ALLOY_BUILD_SPIKES)
    add_executable(topology_reference_spike research/prototypes/topology_references.cpp)
    target_include_directories(topology_reference_spike SYSTEM PRIVATE ${OpenCASCADE_INCLUDE_DIR})
    target_link_libraries(topology_reference_spike PRIVATE Qt6::Core
        TKernel TKMath TKBRep TKPrim TKTopAlgo TKFillet TKBO TKBool TKShHealing)
    add_executable(document_history_spike research/prototypes/document_history.cpp)
    target_include_directories(document_history_spike SYSTEM PRIVATE ${OpenCASCADE_INCLUDE_DIR})
    target_link_libraries(document_history_spike PRIVATE Qt6::Widgets
        TKernel TKMath TKBRep TKPrim TKTopAlgo TKLCAF TKCAF TKBin TKBinL TKCDF)
endif()
if(BUILD_TESTING)
    add_executable(sketch_solver_test tests/sketch_solver.cpp)
    target_link_libraries(sketch_solver_test PRIVATE alloy_sketch_solver)
    add_test(NAME sketch_solver COMMAND sketch_solver_test)
    add_executable(geometry_kernel_test tests/geometry_kernel.cpp)
    target_link_libraries(geometry_kernel_test PRIVATE alloy_kernel)
    add_test(NAME geometry_kernel COMMAND geometry_kernel_test)
    add_executable(document_model_test tests/document_model.cpp src/CadDocument.cpp src/CadDocument.h)
    target_include_directories(document_model_test PRIVATE src)
    target_link_libraries(document_model_test PRIVATE Qt6::Gui alloy_kernel)
    add_test(NAME document_model COMMAND document_model_test)
    add_executable(sketch_document_test tests/sketch_document.cpp src/CadDocument.cpp src/CadDocument.h)
    target_include_directories(sketch_document_test PRIVATE src)
    target_link_libraries(sketch_document_test PRIVATE Qt6::Gui alloy_kernel)
    add_test(NAME sketch_document COMMAND sketch_document_test)
    add_executable(sketch_profiles_test tests/sketch_profiles.cpp)
    target_link_libraries(sketch_profiles_test PRIVATE alloy_kernel)
    add_test(NAME sketch_profiles COMMAND sketch_profiles_test)
    add_executable(extrusion_document_test tests/extrusion_document.cpp src/CadDocument.cpp src/CadDocument.h)
    target_include_directories(extrusion_document_test PRIVATE src)
    target_link_libraries(extrusion_document_test PRIVATE Qt6::Gui alloy_kernel)
    add_test(NAME extrusion_document COMMAND extrusion_document_test)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    add_test(NAME client_selection COMMAND ${Python3_EXECUTABLE} -E ${CMAKE_CURRENT_SOURCE_DIR}/tests/client_selection.py)
    add_test(NAME shell_api COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/shell_api.py
        --app $<TARGET_FILE:alloy> --output ${CMAKE_BINARY_DIR}/evidence)
    set_tests_properties(shell_api PROPERTIES TIMEOUT 30)
endif()
