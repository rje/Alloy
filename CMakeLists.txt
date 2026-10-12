cmake_minimum_required(VERSION 3.24)
project(Alloy VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
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

qt_add_executable(alloy MACOSX_BUNDLE
    src/main.cpp src/AlloyWindow.cpp src/AlloyWindow.h
    src/CadDocument.cpp src/CadDocument.h
    src/ReferenceView.cpp src/ReferenceView.h
    src/ControlServer.cpp src/ControlServer.h)
target_include_directories(alloy SYSTEM PRIVATE ${OpenCASCADE_INCLUDE_DIR})
target_link_libraries(alloy PRIVATE Qt6::Widgets Qt6::Network TKernel TKMath TKG3d TKService TKV3d TKOpenGl)
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
    add_executable(document_history_spike research/prototypes/document_history.cpp)
    target_include_directories(document_history_spike SYSTEM PRIVATE ${OpenCASCADE_INCLUDE_DIR})
    target_link_libraries(document_history_spike PRIVATE Qt6::Widgets
        TKernel TKMath TKBRep TKPrim TKTopAlgo TKLCAF TKCAF TKBin TKBinL TKCDF)
endif()
if(BUILD_TESTING)
    add_executable(document_model_test tests/document_model.cpp src/CadDocument.cpp src/CadDocument.h)
    target_include_directories(document_model_test PRIVATE src)
    target_link_libraries(document_model_test PRIVATE Qt6::Gui)
    add_test(NAME document_model COMMAND document_model_test)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    add_test(NAME shell_api COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/shell_api.py
        --app $<TARGET_FILE:alloy> --output ${CMAKE_BINARY_DIR}/evidence)
    set_tests_properties(shell_api PROPERTIES TIMEOUT 30)
endif()
