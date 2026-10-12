#include "AlloyTheme.h"

#include <QApplication>
#include <QColor>
#include <QStyleFactory>

namespace AlloyTheme {

QPalette lightPalette() {
    QPalette palette;
    for (const auto group : {QPalette::Active, QPalette::Inactive, QPalette::Disabled}) {
        const bool disabled = group == QPalette::Disabled;
        const QColor text(disabled ? "#939ca3" : "#323c44");
        palette.setColor(group, QPalette::Window, QColor("#f7f8f9"));
        palette.setColor(group, QPalette::WindowText, text);
        palette.setColor(group, QPalette::Base, QColor(disabled ? "#f0f2f3" : "#ffffff"));
        palette.setColor(group, QPalette::AlternateBase, QColor("#fafbfc"));
        palette.setColor(group, QPalette::Text, text);
        palette.setColor(group, QPalette::Button, QColor("#f4f5f6"));
        palette.setColor(group, QPalette::ButtonText, text);
        palette.setColor(group, QPalette::Light, QColor("#ffffff"));
        palette.setColor(group, QPalette::Midlight, QColor("#e8ebed"));
        palette.setColor(group, QPalette::Mid, QColor("#b9c1c7"));
        palette.setColor(group, QPalette::Dark, QColor("#87929a"));
        palette.setColor(group, QPalette::Shadow, QColor("#67757e"));
        palette.setColor(group, QPalette::BrightText, QColor("#ffffff"));
        palette.setColor(group, QPalette::Highlight, QColor(disabled ? "#e8ebed" : "#deedf5"));
        palette.setColor(group, QPalette::HighlightedText, disabled ? text : QColor("#263944"));
        palette.setColor(group, QPalette::ToolTipBase, QColor("#ffffff"));
        palette.setColor(group, QPalette::ToolTipText, text);
        palette.setColor(group, QPalette::Link, QColor(disabled ? "#939ca3" : "#235d79"));
        palette.setColor(group, QPalette::LinkVisited, QColor(disabled ? "#939ca3" : "#536c88"));
        palette.setColor(group, QPalette::PlaceholderText, QColor("#87929a"));
        palette.setColor(group, QPalette::Accent, QColor(disabled ? "#b9c1c7" : "#2b83ae"));
    }
    return palette;
}

void apply(QApplication& application) {
    // Pair the existing public Qt style with an explicit palette so unstyled
    // descendants do not inherit dark operating-system colors.
    if (auto* style = QStyleFactory::create("Windows")) application.setStyle(style);
    application.setPalette(lightPalette());
}

}
