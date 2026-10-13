# Fit Point Spline candidate

2026-10-13, bounded #253. Implementation and evidence are ready for independent
acceptance. #254 control-point splines/combs and #257 full source-order lesson are
not passed by this receipt. See [the finite contract](../research/fit-point-spline.md).

## Evidence

- Release application/core build passed with Qt 6.11.2, OCCT 7.9.3 and existing
  PlaneGCS. Core `fit_point_spline` passed 1/1 in 0.10 seconds. It checks actual
  degree5 C2, through points, unequal global chord spans, retained 3→2 with an
  active endpoint tangent→3 insertion, interior tangent direction and magnitude,
  duplicate/zero-vector refusal, point versus handle freedom, one real spline +
  line face, independent Simpson boundary-area comparison within0.01mm² and exact
  authored undo/redo/save/reopen.
- Root's real-entrypoint [API fixture](../../tests/fit_point_spline_api.py), added
  separately on main4cba444, passed38calls. It checks shared creation/editing,
  active endpoint retention through removal/insertion, zero-tangent atomicity,
  actual closed profile area2319.839323mm², saved meaningful handle edit, exact
  undo/redo/reopen and a real app capture. Its earlier application binary predates
  the final native drag/cancel and presentation fixes; unchanged geometry and
  storage evidence is reused, not presented as final input qualification.
- Native `fit_point_spline_controls_test` passed3/3 in3930ms, default Cocoa plugin.
  Initial harness window activation is separate. Tool activation uses the real
  QAction's `trigger()`; three picks, Check button, Escape, fit-point selection,
  Backspace, the actual context menu option/click, insertion click and handle
  drags use delivered Qt widget/key events. Every delivered stage checks its
  actual focus/selection; no harness focus/selection repair occurs between steps.
  A click below the drag threshold preserves exact authoring/history. Grip
  movement previews without authoring; release creates one history entry. Escape
  cancels and late release preserves the draft. Undo during preview cancels its
  token; late release does not overwrite Undo, and Redo restores the edit.
- Final single real-app capture reopens the exact authored native draft snapshot,
  fits Top and selects its curve without changing authored geometry. All six
  green grips are visible with the actual orange curve. This is a framed
  inspection capture, not a fresh native replay or a source screenshot match.
  The same app checks the final stale-draft guard: direct API editing during a
  pending drag makes a later drag end refuse, preserving the newer authoritative
  draft and clearing both pending state and the native token.

External evidence roots (outside Git):

- `../Alloy-reference-records/validation/fit-point-spline/core.log`
- `../Alloy-reference-records/validation/fit-point-spline/native/native-final.log`
- `../Alloy-reference-records/validation/fit-point-spline/native/native-fit-state.json`
- `../Alloy-reference-records/validation/fit-point-spline/native/native-fit-snapshot.alloy`
- `../Alloy-reference-records/validation/fit-point-spline/framed-final/report.json`
- `../Alloy-reference-records/validation/fit-point-spline/framed-final/native-authored-framed-handles.png`
- `../Alloy-reference-records/deliverables/fit-point-spline-2026-10-13/api-attempt-01/`

Final capture/stale-check application SHA256:
`9223c496a89bc17ebe850c1ab7327c2a9390326ca89f65523da23623a9a75e9d`.
Native fixture SHA256:
`e949304738b946bd215dac8f03d1d3de5f76aefb77502d8fbc1fdc9869b0847e`.
The native binary predates only the final direct-API stale-draft guard; the
unchanged native paths retain their separate evidence. Final guard has its own
real-entrypoint proof. GPU inspection reports Apple M4, Apple vendor, OpenGL4.1
Metal90.5, 4× MSAA capability, Retina ratio2 and native framebuffer inspection.

## Qualifications and prior failures

Earlier native attempts remain in external `native/prior-attempts.txt` and logs.
They exposed a nonexistent test setup command, incomplete initial window
activation, oversized viewport fixture coordinates, an unsupported product
selection argument and a test's dangling QJsonValueRef. The last crash stack was
QTest comparison/QCbor, not an inferred Cocoa/kernel fault. These were corrected.
The first passing capture followed Undo/Redo's deliberate selection clear and
showed no handles; the next selected capture clipped outer grip circles. The
final framed capture resolves that evidence limitation without geometry edits.

Public reference `sketch-slots-splines-20261013` establishes fit points,
removal/insertion and tangent direction/weight, with supplemental public degree
and handle evidence externally indexed. Coordinates, normalized chord policy,
baseline acceleration and grip multiplier are disclosed Alloy choices. Through
points and smooth native curve/handles are implemented; exact intermediate shape,
source interpolation/continuity beyond the bounded C2 contract, matching camera
and full visual/input parity remain unknown. Closed/periodic creation, independent
curvature handles, spline constraints, trim/offset and control/combs are outside
this child. Actual profile support is implemented and tested, not deferred to a
polyline surrogate. This source stage does not require extrusion or STL.

Native events are Qt events, not a physical OS input claim. macOS is the verified
runtime; Windows/Linux runtime and release packaging remain separate foundations.

## Independent acceptance

Accepted bounded #253 candidate `e9ea9263634c1fc88f2ba670dd2adad0e5ba7efa`
after source/core/native/API/artifact review. No additional editor or unchanged
workflow replay was used. This acceptance excludes #254 and the full #257 lesson.

Independently reviewed the exact quintic Hermite endpoint derivatives, global
nonuniform chord-span scaling and OCCT multiplicity reduction. The focused core
checks actual C2 derivatives, through-point residuals, retained active handles
through3→2→3 edits, independent boundary-area integration and atomic invalid
input. OCCT exceptions are normalized before ordinary sketch edits can escape
the transactional solve path. Explicit point-solver/handle freedom avoids a false
fully-constrained claim. The disclosed interpolation and grip policies remain
Alloy choices; this is not exact reference-algorithm equivalence.

The original38-call API saved file independently compares exactly with its
report's `sourceState.authored`: three retained fit points, two active handle
records and a real closing line. Core and shared-command profile/history evidence
is sufficient without repeating the same geometry run on the final input-only
changes. The API binary provenance limitation above is preserved.

Reviewed the final3930ms native log and test: existing QAction activation is
programmatic; subsequent curve picks, Check, point selection/Backspace, actual
context-menu insertion and tangent grip events are Qt-dispatched input with no
intermediate focus repair. The identified unsupported selection argument,
no-motion mutation and pending-drag history hazards are corrected. Escape, tool
changes and history clear the shared preview and view token; release cannot
restore an obsolete draft. The final exact-draft guard also rejects a direct-API
edit during a pending drag, with its separate13-call real-app evidence.

Inspected the final framed GPU image: all six green grips are visible, the orange
curve and handle association are distinguishable, and the palette is readable.
The actual framed draft independently compares exactly with the earlier native
state, so framing did not manufacture different geometry. This resolves the
earlier edge-clipped evidence limitation. The shallow source-stage curve is
an arbitrary fixture, not a matched reference screenshot; open-profile status
is truthful. Final app SHA and runtime qualifications remain as recorded above.
