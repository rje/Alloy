# Native mesh chooser investigation

2026-10-19, REL-053/#607. Bounded diagnosis only; no accepted product change.
The accepted REL-052 sheet route remains unchanged. Two experiments were stopped
at chooser Cancel; neither exercised Save or independently read chooser output.

The first experiment replaced static `QFileDialog::getSaveFileName` with an owned
WindowModal `QFileDialog::open()` and a finished continuation. This established
an actual AppKit `NSSavePanel` key window and produced an inspected native chooser
capture. Native Escape closed it; exact document/history equality and absence of
the intended destination passed. The export sheet remained Qt's active modal,
but AppKit had no key window and Qt had no focused widget. Thus presentation is
isolated from the still-broken return ownership.

The second experiment added guarded immediate export-sheet `activateWindow()`
in the chooser's finished/Cancel callback, analogous to the accepted outer
sheet/editor return. It did not fix return ownership. The final observation was:
application active; editor attached sheet `NSKVONotifying_QNSPanel`;
`qtModalClass=QDialog`; `keyWindowMatchesModal=false`;
`nativeKeyWindowClass=none`; `focusClass=none`; `focusObject=none`.
No harness activation or selection repair occurred between demonstrated steps.
This experiment restored the current REL-052 strict prefix before extending it.

Qt 6.11.2 Cocoa source explains that ApplicationModal helper presentation is
deferred until native `exec()` and warns native modal presentation can fail when
NSApp is not running after a Qt event-loop interruption. This motivates the first
experiment; it does not establish why the original route fails under physical
input. Source inspected locally on 2026-10-19:
[QFileDialog](https://code.qt.io/cgit/qt/qtbase.git/tree/src/widgets/dialogs/qfiledialog.cpp?h=v6.11.2),
[Cocoa helper](https://code.qt.io/cgit/qt/qtbase.git/tree/src/plugins/platforms/cocoa/qcocoafiledialoghelper.mm?h=v6.11.2).

Evidence and both draft sources are external under
`../Alloy-reference-records/validation/rel053-chooser-2026-10-19/`:
`hypothesis1-chooser/`, `hypothesis2/`, `hypothesis1-source/`,
`hypothesis2-source/`, `hypothesis1-run.log`, `hypothesis2-run.log`, `build.log`.
The first source uses the prior external diagnostic driver; the second contains
the accepted current prefix plus chooser extension. The second draft patch is
preserved, not applied. Earlier failed probes remain in the REL-052 directory.

A finite restart should investigate native sheet return timing/ownership with
these same fixtures before repeating Save. Required acceptance remains actual
chooser Cancel with naturally usable owning sheet, reopen/Save mm STL,
independent dimensions/closedness, exact authored/history preservation, editor
native return and next pointer/key input, readable native/framebuffer capture,
and independent Sol review. Do not substitute shared API export for this gate.

Build: `build-preview-release`, Release, private Qt 6.11.2, empty SDK override,
`ordinary_bracket_input_controls_test`, maximum `-j4`. Run after
`python3 tools/native_input_preflight.py` passes, with
`ALLOY_ORDINARY_INPUT_REQUIRE_MODAL=1`,
`ALLOY_ORDINARY_INPUT_REQUIRE_CHOOSER=1`,
`ALLOY_ORDINARY_INPUT_MODEL` pointing to the exact saved full-size bracket in
`validation/rel012-ordinary-input-2026-10-18/fullsize-failed-repro.alloy` under
the external records root, and a fresh `ALLOY_ORDINARY_INPUT_EVIDENCE` directory.
Invoke the test's `bracketGate` case. Draft sources need deliberate reapplication
and rebuild; the build directory's last executable is the second experiment,
while repository source was restored to the accepted state. No CI ran. All
private processes exited and the single GUI lease is released. Physical-device,
package and other-platform behavior remain unqualified.
