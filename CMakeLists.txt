cmake_minimum_required(VERSION 3.24)
project(Alloy VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
find_package(Qt6 6.5 REQUIRED COMPONENTS Widgets Network)

# Discovery is intentionally separate from integration: this shell has no geometry.
option(ALLOY_DISCOVER_OCCT "Discover the proposed Open CASCADE kernel" ON)
if(ALLOY_DISCOVER_OCCT)
    find_package(OpenCASCADE QUIET)
endif()
if(OpenCASCADE_FOUND)
    message(STATUS "OCCT ${OpenCASCADE_MAJOR_VERSION}.${OpenCASCADE_MINOR_VERSION}.${OpenCASCADE_MAINTENANCE_VERSION} discovered; geometry integration pending")
    set(ALLOY_OCCT_VERSION "${OpenCASCADE_MAJOR_VERSION}.${OpenCASCADE_MINOR_VERSION}.${OpenCASCADE_MAINTENANCE_VERSION}")
else()
    message(STATUS "OCCT not discovered; building the independent UI shell")
    set(ALLOY_OCCT_VERSION "not discovered")
endif()

qt_add_executable(alloy MACOSX_BUNDLE
    src/main.cpp src/AlloyWindow.cpp src/AlloyWindow.h
    src/ControlServer.cpp src/ControlServer.h)
target_link_libraries(alloy PRIVATE Qt6::Widgets Qt6::Network)
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
if(BUILD_TESTING)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    add_test(NAME shell_api COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/shell_api.py
        --app $<TARGET_FILE:alloy> --output ${CMAKE_BINARY_DIR}/evidence)
    set_tests_properties(shell_api PROPERTIES TIMEOUT 30)
endif()
