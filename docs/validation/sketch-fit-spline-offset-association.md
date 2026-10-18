# Fit-spline Offset association removal

SKT-149/#575 accepted 2026-10-18 after independent Astra geometry/command
review and Sol review of the final native layout, logs and captures.
The bounded command keeps the visible source and result, removes their retained
association, and makes the result independently editable through control poles.

The dated public evidence and actionable workflow contract are outside the
repository in `../Alloy-reference-records/knowledge/specs/skt149-fit-spline-offset-association-2026-10-18.md`.
The public help documents Offset glyph Delete as association removal and
separately documents distance-dimension Delete. It does not establish the exact
spline representation, explicit action name, conversion tolerance or styling.

## Command and representation

In Select, right-click one retained fit-spline Offset result and choose **Break
Link**. The shared authority is `sketch.splineOffsetBreakLink`, with optional
`offsetEntityId`; omission uses exactly one selected entity. This explicit action
is separate from geometry Delete (`sketch.deleteSelection`) and annotation
Delete. There is no synthetic glyph/constraint row and no new keyboard shortcut.
The native action and API publish the same solved draft in one history step.

The result becomes a degree-five `controlSpline` with `knotPolicy:
clampedExplicit`, normalized `[0,1]` `knots`, integer `multiplicities`, and authored
pole `pointIds`. Existing uniform control splines retain their schema. The result
entity, start/end IDs and endpoint coordinates remain unchanged; interior pole
IDs are new. The source fit spline and its points/handles remain authored.

OCCT performs one conversion using a 0.00025 mm approximation target and at most
32 segments. The result must be nonrational, nonperiodic and contain at most 128
poles. Endpoint poles are pinned to the original authored coordinates before
solving, then 1,025 corresponding-parameter samples must deviate by at most
**0.001 mm**. Diagnostics expose the tolerance, sample count, sampled maximum and
OCCT reported error. This is a bounded approximation, not an exact curve identity
claim. Save/reopen evaluates the stored poles/knots and never reapproximates.

Straight Lines sharing the unchanged endpoints remain legal. Other retained
entity/endpoint consumers in geometry, relations or parameter metadata refuse
atomically with dependent-removal guidance. Wrong/stale targets also refuse.
After conversion, ordinary authored geometry Delete can remove the curve while
retaining endpoint records shared with surviving Lines; Undo restores it.
Historical downstream-feature preflight remains the separate SKT-150 scope.

## Evidence

Accepted build directory was inspected before building: `build-preview-release`,
Release, this repository, Qt 6.11.2 private runtime. Local macOS evidence is under
`../Alloy-reference-records/validation/skt149-fit-spline-offset-association-2026-10-18/`.

- `fit_spline_offset_test` passes conversion, exact entity/endpoints, source-edit
  independence, independent control-pole edit, preserved closed connector
  profile, invalid target/dependency refusal, malformed knot validation and
  authored history/reopen, plus the existing exact Offset regression.
- `tests/sketch_fit_spline_offset_association_api.py` passes 69 actual-app calls:
  legal connector retention, exact single-step draft Undo/Redo, source move with
  unchanged result samples, exact authored save/reopen, and a control-pole edit
  after reopening. The saved model is `api/independent-offset.alloy`.
- The 120 mm source fixture converts to 33 poles; measured maximum deviation is
  0.0001620417303 mm. The shared real-app conversion call took 51.5 ms on this
  run (command execution and publication included).
- `fit_spline_offset_controls_test breakLinkContextKeepsCurvesAndEditablePoles`
  passes Qt-delivered curve selection, actual context popup activation and
  closure, Break Link, a direct converted-pole drag without inter-step focus
  repair, and Undo/Redo. The final `native/native-break-link-menu.png` shows
  separated warning and selection-guidance text with both curves inside the
  viewport at 125% zoom; the popup overlays part of the lower curve span.
  `native/native-independent-control-cage.png` shows the complete source/result
  and editable cage after popup closure. The test checks wrapped warning height,
  nonoverlapping label rectangles and sampled curve bounds within the viewport.
- API captures show the connected profile, retained source/result and reopened
  editable cage. The native framebuffer reports Apple M4 / Apple, OpenGL 4.1
  Metal - 90.5, hardware acceleration, scale ratio 2. The private editors closed;
  the existing user-owned PID 33805 was preserved.

The bounded visual follow-up applies the final profile-label style before
measuring wrapped text, reserves its font-metric height and top-aligns it. The
previous overlapping/cropped captures are retained as superseded evidence under
`superseded-before-layout/`; they do not establish current visual acceptance.
Only the affected native test and the 69-call API workflow were rerun after this
layout change; geometry/core source was unchanged.

Matched: bounded documented association-removal outcome and separate Delete
semantics, implemented with the explicit action. Unknown: reference glyph input
parity, exact styling and matched public screenshot comparison. Physical OS
input, other-platform runtime/GPU and arbitrary dependent workflows remain
unqualified. No proprietary application was used.
