// SPDX-License-Identifier: MIT
#pragma once
#if defined(_WIN32)
#define FREECAD_DECL_EXPORT __declspec(dllexport)
#define FREECAD_DECL_IMPORT __declspec(dllimport)
#else
#define FREECAD_DECL_EXPORT __attribute__((visibility("default")))
#define FREECAD_DECL_IMPORT __attribute__((visibility("default")))
#endif
