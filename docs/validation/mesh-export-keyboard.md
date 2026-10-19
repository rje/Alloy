# Save As Mesh sheet keyboard candidate

2026-10-19, REL-052/#587. Bounded implementation candidate pending independent
review. Actual OS chooser Save/Cancel and its native mesh readback remain a
separate required remainder, REL-053/#607; this receipt does not accept that path.

The unchanged strict bracket driver reproduced Tab-to-Format failure in an
unlocked, active Aqua session. The sheet was the Qt/native focus window; after
Tab the focused widget was Normal Deviation. Qt 6.11.2 initializes noneditable
macOS combo boxes with TabFocus, which the standard text-controls-only traversal
skips. Bodies, Format and Unit Type now use StrongFocus locally. Qt's ordinary
initial focus and traversal then reach Bodies → Format → Unit Type. No global
keyboard setting, interstep test focus repair or popup activation patch is used.

Direct AppKit checks additionally found sheet Cancel could leave an active
application with no key window. After the sheet finishes, Alloy now requests
editor activation if the application is still active and no modal remains.
It does not request activation after switching to another application.

The real-entrypoint Qt-delivered bracket route passes 3/3 checks in 9.323 seconds:
accepted sketch/Extrude/Undo prefix; actual Browser popup action hit; popup
closure; initial Bodies focus; actual AppKit sheet key-window identity before
keys; Tab/Down/Up Format; Escape; actual native editor return; reopen; Bodies
focus; Tab through Format and Units; mm/in keys; Shift-Tab; actual-hit Cancel;
exact document/history preservation; native editor return; next Browser popup
and Escape. The separate shared-API STL export and model save still run after
this route. They are not native chooser evidence.

The Release target `ordinary_bracket_input_controls_test` builds incrementally
at `-j4` in `build-preview-release`, with private Qt 6.11.2 and no explicit SDK
override. The inspected actual-app capture has readable fields and full button
labels. GPU receipt: Apple M4 / Apple / OpenGL 4.1 Metal, Retina ratio 2,
2496 × 1316 framebuffer. The fixture is the exact retained full-size bracket.
The API-produced binary STL independently parses as 3640 triangles with mm
bounds X −50..868.1951, Y 0..438.9058, Z 0..50; chooser output is unverified.

External evidence:
`../Alloy-reference-records/validation/rel052-modal-2026-10-19/` contains
`unchanged/` (original failure and preserved driver), `focus-policy/` (first
passing field traversal), `final-native-sheet/` (pre-fix native return failure),
`final/` and `final-run.log` (final candidate), `build.log`, and retained
`chooser01..05/` probes. The latter reached a QFileDialog Qt modal but did not
establish an AppKit NSSavePanel key window; the observed key window remained
QNSPanel. This does not prove native-panel availability or a product chooser
defect. Their diagnostic driver stays external. No mesh was saved by the OS
chooser. The remaining work must establish the actual OS chooser, Cancel/Save,
return input and independently read its output. Physical OS input, packaged
runtime, other platforms and matched reference screenshot parity are unknown.
All private test processes exited; no user-owned editor was modified. No CI ran.

Reproduce after `python3 tools/native_input_preflight.py` exits zero, holding
the single private GUI lease:

```sh
ALLOY_ORDINARY_INPUT_REQUIRE_MODAL=1 \
ALLOY_ORDINARY_INPUT_MODEL="$PWD/../Alloy-reference-records/validation/rel012-ordinary-input-2026-10-18/fullsize-failed-repro.alloy" \
ALLOY_ORDINARY_INPUT_EVIDENCE="/tmp/alloy-mesh-keyboard-review" \
./build-preview-release/ordinary_bracket_input_controls_test bracketGate
```

Qt library implementation evidence inspected 2026-10-19:
[QComboBox initialization](https://code.qt.io/cgit/qt/qtbase.git/tree/src/widgets/widgets/qcombobox.cpp?h=v6.11.2)
and [QApplication traversal](https://code.qt.io/cgit/qt/qtbase.git/tree/src/widgets/kernel/qapplication.cpp?h=v6.11.2),
using the matching locally retained source. This is an Alloy keyboard contract;
exact reference-product field-focus compatibility remains unknown.
