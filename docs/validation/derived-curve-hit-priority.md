# SKT-151: derived curve selection through source grips

Accepted 2026-10-18 after independent Sol review of the baseline failure,
source, native logs, API evidence and actual-app captures.

A plain click on a clearly drawn retained fit-spline Offset result now selects
that result while its source remains selected and displays tangent grips. The
same press can still start a deliberate source-grip drag. No control, shortcut,
command schema, geometry evaluation or global picking tolerance changes.

Permitted public evidence and the bounded policy are external:
`../Alloy-reference-records/knowledge/specs/skt151-derived-curve-hit-priority-2026-10-18.md`.
S1 documents selecting a spline to display handles and dragging handle endpoints;
S2 documents individual canvas selection and distinct curve/point categories.
Neither establishes overlap priority. This priority is an explicit Alloy policy;
exact reference overlap behavior and styling remain unknown.

## Interaction contract

The existing spline-grip press path retains its source drag. Only a no-modifier
press whose ordinary sketch pick resolves to that source's `fitSplineOffset`
result within 2 logical pixels retains a result-click candidate. On release
without the existing 4-pixel drag threshold being crossed, cancel the pending
grip edit and invoke ordinary `sketch.selectionPick` at the press location.
Dragging still invokes the existing shared `sketch.splineDrag` authority. The
normal picker retains its authored-point precedence. Other curve families,
modifiers, ambiguous/farther hits and deliberate grip drags keep their existing
paths. Selection uses the original stable entity IDs.

## Evidence

External evidence directory:
`../Alloy-reference-records/validation/skt151-derived-curve-hit-priority-2026-10-18/`.

The accepted `build-preview-release` cache was inspected: Release, this repository
as source, Qt 6.11.2 from the private-runtime install. `alloy` and
`widget_interactions_test` build successfully. Adjacent `skt151-build.log` and
`skt151-native-build.log` retain build output.

A bounded baseline rebuild used HEAD's unchanged `ReferenceView.cpp` with the
new native test. `baseline-native.log` reproduces source selection persisting
when the expected result is clicked; `baseline-build.log` records the build.
The candidate source was restored before all final checks.

`widget_interactions_test sketchFitSplineOffsetDelete sketchCircleOffsetDelete`
passes (4 Qt cases including setup/cleanup) in `native.log`. The original
logical (292,185) hit is covered by the source grip at (291,186). The canvas
route now omits blank-canvas deselection: source selected/grips visible → direct
result click → stable result selection → focused Delete. Exact draft parameters
and draft-history index remain unchanged until Delete. Delete/Undo/Redo retain
the existing exact authored-state assertions. Initial macOS activation is
harness setup; no intervening focus or selection repair is used for these steps.
List and context routes continue to pass separately.

A separate deliberate drag starts on the overlapping source grip, delivers a Qt
mouse move with the left button held, and changes source tangent handles in one
history step. Source fit points, entity intent outside tangent handles and all
point IDs remain; the retained result recomputes its endpoints. Exact Undo
restores the original authored parameters. The Circle Offset canvas, badge and
context routes guard an unrelated curve family. This is Qt-delivered native
input, not physical hand input.

`ctest --test-dir build-preview-release -R '^(fit_spline_offset|sketch_constructor_delete)$' --output-on-failure`
passes 2/2 (`core.log`). The existing real-entrypoint runner
`tests/sketch_fit_spline_offset_delete_api.py` passes (`api.log`), with exact
source/sibling preservation, dependency refusal, Undo/Redo and saved reopen.
Its `api/receipt.json` and `api/transcript.json` retain API and GPU evidence.
App SHA-256: `14f3743882f98ab9167a37800919606be2ae11c9e6d86f5cf09c0b2e576cb8bd`.

Inspected native `source-selected.png`, `result-selected.png` and
`source-grip-dragged.png`: two smooth distinct curves, selected result orange
with source grips removed after the direct click; source grips visible again
after deliberate editing. The fixture intentionally retains the original small
camera scale to reproduce the interception. These are actual Qt/native OCCT
framebuffer captures, not matched public screenshots. API GPU evidence reports
Apple / Apple M4, hardware acceleration, OpenGL `4.1 Metal - 90.5`, native
framebuffer 2376 × 1216 at 2× scale. Existing native popup warnings are nonfatal.

Private validation editors closed; user-owned PID 33805 preserved. Physical OS
input, exact public overlap parity and Windows/Linux runtime remain unqualified.
