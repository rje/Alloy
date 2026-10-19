# Retained feature suppression — HIS-011

2026-10-19 bounded candidate on macOS arm64. Qualifies independent Box sources
and retained planar Offset Face chains. General feature-family dependency
suppression and broader history recovery remain HIS-003.

## Command/workflow contract

Timeline **Suppress Features** / **Unsuppress Features** calls the same
`history.suppression {featureId, suppressed}` command as local automation. The
single-feature action commits immediately, without a preview. The authored
optional boolean is explicit intent; `evaluation.featureStates` derives
`explicitSuppressed`, `effectiveSuppressed` and `dependencyFeatureId`. The
result also reports `changed` and `affectedDependentIds`. Existing state is a
no-op; missing IDs refuse. Timeline tiles expose both suppression booleans and
use reduced opacity for unavailable features; tooltips identify an explicit
suppression or its dependency. Both context and arrow menus use the same action.

The evaluator processes all retained feature IDs in order, skips unavailable
operations and preserves dormant source body names/organization. The source
Box and dependent Offset disappear from evaluated geometry; a later independent
Box remains. Suppressing a modifier leaves its earlier source stage available.
Inputs, body IDs and feature IDs remain authored. Ordinary visibility retains
evaluated geometry and leaves suppression/tile states unchanged.

One document transaction owns the explicit state and its derived result.
Undo/Redo, suppressed save/reopen, restoration and restored save/reopen use the
same document authority. A history containing unsupported feature families,
joints or Pin refuses suppression atomically; structured diagnostics identify
the blocking feature when applicable. Suppression-bearing records on unsupported
families and nonboolean flags also refuse on load. No generic dependency
propagation is claimed. Editing unavailable features is refused until restored;
after restoration ordinary retained editing and Cancel apply. Transient history
rollback remains read-only for suppression.

## Evidence and compatibility

Public source IDs HIS-R1 through HIS-R5, retrieved 2026-10-19, are indexed in the
external compact specification
`../Alloy-reference-records/knowledge/specs/his011-suppression-2026-10-19.md`.
The documented menu labels, dependent unavailability, canvas absence, Timeline
dimming and visibility distinction are the bounded behavior targets. The exact
Box fixture propagation, authored explicit/derived storage, atomic transition,
Undo/save contract and refusal policy operationalize HIS-011 as Alloy decisions.
The exact color/opacity, suppressed-feature edit semantics and matched styling
remain unknown. No proprietary application was operated or inspected.

The fixture is Box A 20 × 15 × 10 mm, top Offset Face +5 mm, then independent
Box B 8 × 6 × 4 mm at x=40 mm. A evaluates to 4500 mm³ and B to 192 mm³.
The focused core check passes source/dependent/independent availability, stable
feature order, names/ownership preservation, atomic unsupported-family refusal,
malformed fields, Undo/Redo and both persistence states. The real-entrypoint API
fixture passes distinct visibility, explicit/dependent state, no-op/refusal
identity, unavailable edit refusal, retained source edit to 5400 mm³ after
restoration, edit Cancel, history and save/reopen, and rollback read-only refusal.

```sh
cmake --build build-preview-release \
  --target alloy history_suppression_test history_suppression_controls_test -j4
build-preview-release/history_suppression_test
python3 tests/history_suppression_api.py \
  --app build-preview-release/alloy.app/Contents/MacOS/alloy \
  --output ../Alloy-reference-records/validation/history-suppression-2026-10-19/api
ALLOY_SUPPRESSION_EVIDENCE="$PWD/../Alloy-reference-records/validation/history-suppression-2026-10-19/native" \
  build-preview-release/history_suppression_controls_test
```

Build cache is Ninja/Release, AppleClang 17 arm64, accepted private Qt 6.11.2 and
OCCT 7.9.3. Native build and GPU qualification on Windows/Linux, physical device
input and matched public screenshot parity remain separate gates. The existing
Ninja dependency cache repeatedly reports a premature-end recovery warning and
recompiles unchanged targets; builds complete. Investigation is deferred outside
this feature scope.

Actual API captures `api/suppressed.png` and `api/restored.png` were inspected:
only B remains while the source and dependent tiles are dimmed; restoration
shows both solids with normal tiles and readable controls. There is no stale
selected-face overlay. Runtime GPU details, authored models, call transcript and
structured full/suppressed/restored states are in the external dated evidence
directory. Private validation editors exit after each fixture.

## Native right-click qualification

The final actual-hit Qt driver passes both Timeline right-click actions. It
checks the widget under the tile center and menu item, observes the active
popup and its closure, then checks activation/focus before continuing. Initial
harness activation is separate; no focus or selection repair occurs between
steps. The native fixture verifies the same explicit/dependent tile states,
remaining independent body and exact restored geometry. Menu and suppressed
GPU captures were inspected and are readable; the source and Offset are dimmed.

An initial QWindow-delivered right-click attempt failed before an action. The
corrected driver delivers to the actual hit tile QWidget; the menu action is
queued after that nonblocking click returns, so its subsequent Timeline rebuild
cannot invalidate the outer click target. The first attempt and final log are
retained externally. Qt still emits an embedded `timelineWindow` top-level
warning during popup creation; observed popup activation, actual item hits,
closure and both authoritative changes pass despite it. This warning remains
a bounded native popup follow-up, not a claimed physical-device result.

The API fixture records 55 calls before its final quit. Its runtime identifies
Apple M4, Apple vendor, OpenGL 4.1 Metal, 4× MSAA and a 2296 × 1216 native
framebuffer for the 1148 × 608 logical GPU viewport at 2× display scale.
`manifest.json` in the dated external directory records source, executable,
model and capture hashes. Independent review is recorded by the integrator.
