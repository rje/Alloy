# SPDX-License-Identifier: MIT
# Official released lib3mf 2.5.0, retrieved 2026-10-17. BSD-2-Clause;
# bundled libzip/zlib/cpp-base64/fast_float keep their original licenses.
include(FetchContent)
FetchContent_Declare(alloy_lib3mf
    URL https://codeload.github.com/3MFConsortium/lib3mf/tar.gz/refs/tags/v2.5.0
    URL_HASH SHA256=66772c95359d70012a3e97c3aadfb40564cc2cbcf8a5d4bc64d8144c4cb20fb2
    DOWNLOAD_EXTRACT_TIMESTAMP FALSE SOURCE_SUBDIR alloy-source-only)
# The release source archive includes compiled dependency sources, but libzip's
# configuration probe and fast_float headers refer to pinned git submodules.
FetchContent_Declare(alloy_3mf_libzip
    URL https://codeload.github.com/nih-at/libzip/tar.gz/3091d0424e04f98e051fce5e94bb3a6461a33b91
    URL_HASH SHA256=d16ea93ee8ccc6bc5923cdd51beccb5c961ddaf13236a667a15b81f149fe15b8
    DOWNLOAD_EXTRACT_TIMESTAMP FALSE SOURCE_SUBDIR alloy-source-only)
FetchContent_Declare(alloy_3mf_fastfloat
    URL https://codeload.github.com/fastfloat/fast_float/tar.gz/f3ff46fd40d28b5eb272727812e5bd8dc5a6d876
    URL_HASH SHA256=0519249347f999b4b73f6de5052b2a7e3c3946b352502119b92d058b2cacf236
    DOWNLOAD_EXTRACT_TIMESTAMP FALSE SOURCE_SUBDIR alloy-source-only)
FetchContent_MakeAvailable(alloy_lib3mf alloy_3mf_libzip alloy_3mf_fastfloat)
file(COPY "${alloy_3mf_libzip_SOURCE_DIR}/" DESTINATION "${alloy_lib3mf_SOURCE_DIR}/submodules/libzip")
file(COPY "${alloy_3mf_fastfloat_SOURCE_DIR}/" DESTINATION "${alloy_lib3mf_SOURCE_DIR}/submodules/fast_float")
# Upstream's libzip configure only needs zlib headers, provided in the archive.
file(COPY "${alloy_lib3mf_SOURCE_DIR}/Libraries/zlib/Include/" DESTINATION "${alloy_lib3mf_SOURCE_DIR}/submodules/zlib")
set(LIB3MF_BUILD_SHARED OFF CACHE BOOL "Static MIT-compatible 3MF dependency" FORCE)
set(LIB3MF_TESTS OFF CACHE BOOL "Only Alloy's bounded 3MF tests" FORCE)
set(STRIP_BINARIES OFF CACHE BOOL "Keep dependency symbols" FORCE)
add_subdirectory("${alloy_lib3mf_SOURCE_DIR}" "${alloy_lib3mf_BINARY_DIR}" EXCLUDE_FROM_ALL)
set_target_properties(lib3mf PROPERTIES AUTOMOC OFF)
# Available to package scripts as well as standard cmake install.
set(ALLOY_LIB3MF_NOTICES_DIR "${CMAKE_BINARY_DIR}/third-party-notices/lib3mf")
file(MAKE_DIRECTORY "${ALLOY_LIB3MF_NOTICES_DIR}")
configure_file("${alloy_lib3mf_SOURCE_DIR}/LICENSE" "${ALLOY_LIB3MF_NOTICES_DIR}/lib3mf-BSD-2-Clause.txt" COPYONLY)
configure_file("${alloy_3mf_libzip_SOURCE_DIR}/LICENSE" "${ALLOY_LIB3MF_NOTICES_DIR}/libzip-BSD-3-Clause.txt" COPYONLY)
# The bundled compiled libzip files carry later copyright years than the
# submodule's top-level LICENSE. Ship those exact source headers as well.
file(COPY "${alloy_lib3mf_SOURCE_DIR}/Libraries/libzip/Source/"
    DESTINATION "${ALLOY_LIB3MF_NOTICES_DIR}/libzip-compiled-source")
configure_file("${alloy_3mf_fastfloat_SOURCE_DIR}/LICENSE-MIT" "${ALLOY_LIB3MF_NOTICES_DIR}/fast_float-MIT.txt" COPYONLY)
# These original source headers contain the complete corresponding notices.
configure_file("${alloy_lib3mf_SOURCE_DIR}/Libraries/zlib/Include/zlib.h" "${ALLOY_LIB3MF_NOTICES_DIR}/zlib-notice.h" COPYONLY)
configure_file("${alloy_lib3mf_SOURCE_DIR}/Libraries/cpp-base64/Source/base64.cpp" "${ALLOY_LIB3MF_NOTICES_DIR}/cpp-base64-notice.cpp" COPYONLY)
install(DIRECTORY "${ALLOY_LIB3MF_NOTICES_DIR}/" DESTINATION share/Alloy/licenses/lib3mf)
message(STATUS "lib3mf 2.5.0: pinned static Core 3MF writer; notices=${ALLOY_LIB3MF_NOTICES_DIR}")
