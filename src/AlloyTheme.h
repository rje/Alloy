#pragma once

#include <QPalette>

class QApplication;

namespace AlloyTheme {
// Shared by the native entry point and widget interaction fixtures.
QPalette lightPalette();
void apply(QApplication& application);
}
