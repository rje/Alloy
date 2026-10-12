// SPDX-License-Identifier: MIT
#pragma once
#include <iostream>
#include <mutex>
namespace Base {
// Standalone diagnostic sink. Preserve the format and arguments without depending
// on a separate formatting library. Solver status/IDs use the structured adapter.
struct Sink {
    template<class... T> void log(const char* format, const T&... values) {
        std::lock_guard<std::mutex> lock(mutex);
        std::clog << "PlaneGCS: " << format;
        ((std::clog << ' ' << values), ...);
        std::clog << '\n';
    }
    template<class... T> void warning(const char* format, const T&... values) { log(format, values...); }
    std::mutex mutex;
};
inline Sink& Console() { static Sink sink; return sink; }
}
