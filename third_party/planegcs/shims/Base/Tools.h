// SPDX-License-Identifier: MIT
#pragma once
#include <chrono>
namespace Base {
struct TimeElapsed {
    std::chrono::steady_clock::time_point value = std::chrono::steady_clock::now();
    static double diffTimeF(const TimeElapsed& a, const TimeElapsed& b) {
        return std::chrono::duration<double>(b.value - a.value).count();
    }
};
}
