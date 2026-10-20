# Circular sketch Pattern canvas candidate

SKT-182/#705, 2026-10-20. Extends the accepted retained
[circular Pattern](sketch-circular-pattern.md) with actual viewport input, reusing
[rectangular canvas](sketch-pattern-canvas.md) roles and transient gesture control.
The retained core, source-only solver, indexed identities and ownership remain
unchanged. Root owns backlog, inventory and continuation-context updates.

Permitted public evidence is external in
`../Alloy-reference-records/knowledge/specs/skt097-sketch-patterns-2026-10-20.md`
(R2/R3) and `skt097-canvas-picking-followup-2026-10-20.md` (P1/P2), retrieved
2026-10-20. It documents Objects then separate Center Point selection,
Full/Angle, angle/Quantity manipulators and center-glyph editing. Exact handle
art, hover colors, snapping and layout are unknown. P2 describes clockwise Angle
in its tutorial setup; Alloy retains explicitly labeled positive sketch-UV
rotation. Matched plane/camera orientation is unavailable, so **rotation sign
parity remains unknown**, rather than reversing the accepted geometry policy.
No proprietary application was operated.

## Shared workflow and policy

Create > Circular Pattern opens the existing embedded dock. Objects, Center Point
and Handles buttons name the active pick role. Picking an ordinary Circle
advances to Center Point; picking a standalone authored Point advances to Handles.
Roles can be reactivated for valid repicks. Curve hover is gold; center-point
hover enlarges a gold point marker; successful picks retain orange selection.
Unsupported/coincident center picks preserve earlier valid picks and preview.
Identity selectors and exact numeric controls remain available.

A GPU arc/ring and radial witnesses connect the source to the retained pivot.
Full shows a fixed 360° ring and one orange square-plus Quantity grip. Angle
adds a blue tangential endpoint arrow labeled A and places the Quantity grip
on the active arc midpoint. Labels sit radially outside the arc. Full's Quantity
grip sits southeast of the source direction. Tangential Quantity drag changes
one total instance per 0.3 times source-to-pivot radius of motion. Angle drag
accumulates signed pointer rotation about the pivot, snapping to 1°. These are
explicit Alloy policies; exact visual/gesture parity is unknown. Quantity includes
the seed, and Full's 360° remains read-only.

The existing `sketch.patternRole`, `sketch.patternHover`, `sketch.patternPick`,
`sketch.patternHandle` and `sketch.patternExit` now serve both families. Circular
roles use center-point IDs, while source and rectangular direction picks use
entity IDs. All geometry passes through the same `sketch.patternPreview` and
retained validation. Handles synchronize numeric Quantity/Total Angle without
publishing authored history. Refused count/angle/collision gestures retain the
last valid preview while held and restore the gesture baseline on release.
Cancel/Escape discard the transaction; OK publishes one sketch-history item.
Actual center-glyph double-click opens the same Handles editor.

## Evidence

Build: `build-preview-release`, Release/private Qt 6.11.2, OCCT 7.9.3, Apple
Clang, `-j4`. External receipts:
`../Alloy-reference-records/validation/skt182-2026-10-20/`.

- `sketch_circular_pattern_canvas_controls_test`: actual Create QToolButton and
  active QMenu action hit, source/center hover and picks, invalid center refusal,
  valid repicks, Full Quantity 6→8→6, Angle120→150→120, colliding 2° rollback,
  Cancel/Escape/OK, distinct Undo/Redo, actual center-glyph edit/Cancel, six-profile
  through Cut, exact authored save/reopen and STL. The final fixture also checks
  below-minimum Quantity drag rollback. Initial gate passed 3/3 in 10.744 s;
  `native-final/` records the final 3/3 gate (11.079 s), including the
  arc-attached Quantity grip.
- Tests deliver Qt pointers to the widget receiving the actual hit; hover uses
  QWindow pointer delivery. They observe popup activation/closure and focus
  produced by the command. Only initial harness activation is used; no interstep
  focus or selection repair is performed. This is Qt native-widget input evidence,
  separate from physical AppKit input qualification.
- `tests/sketch_circular_pattern_canvas_api.py`, `api-initial/`: the real executable
  opens the exact native-produced Full6 model. Source radius 1.5→2 mm, source
  center (37,25)→(38,26), and pivot (25,25)→(26,24) recompute the existing Cut,
  preserving six profile IDs. Authored Undo/Redo/save/reopen are exact. Reducing
  count to 4 refuses because the Cut explicitly references the removed profiles,
  preserving draft/history. Exact volume is 12123.008881569227 mm³.
- Independent STL readback: native and edited meshes each have 3492 triangles,
  one consistently wound watertight component, Euler characteristic −10, and
  bounds (0,0,0)..(50,50,5). Edited mesh volume is 12123.128488755774 mm³
  (within 0.01%). Each expected rotated radius-2 hole has 864 wall-vertex samples
  spanning z=0..5. Geometry, authored state and mesh evidence are independent of
  screenshots.
- Inspected Apple GPU captures show source/center hover, Full ring, 120° arc,
  invalid-angle refusal and six open holes. Diagnostics identify hardware
  accelerated Apple M4 / Apple, OpenGL 4.1 Metal, MSAA4 and DPR2 framebuffer.
  Public screenshots are unmatched in state/scale, so detailed visual parity is
  unknown. Shared rectangular canvas and circular numeric regressions are logged
  separately: rectangular canvas 3/3 (12.118 s) and circular numeric
  3/3 (4.367 s).
- The initial no-evidence rectangular regression completed modeling but stalled
  at the unsaved-document close prompt. Its stack/log are preserved in
  `rectangular-stall-sample.txt` and `rectangular-canvas-regression.log`; this
  was fixture teardown, not a model/pointer stall. Both canvas fixtures now
  explicitly discard their test document before closing, independent of optional
  evidence save/reopen. The no-evidence rerun passes 3/3 (12.118 s) in
  `rectangular-teardown-fixed.log`.

## Limits

One ordinary Circle and standalone authored center Point, Full/Angle,
Quantity 2..16, and retained core collision/association bounds. Symmetric,
suppression and arbitrary seeds remain ADV-005; nonorthogonal rectangular axes
remain SKT-183/#706. Physical AppKit pointer/trackpad activation, native DPR1 and
packaged Windows/Linux runtime are unqualified release gates. No CI, tag, release
or push was run. Private fixture editors closed; user-owned PID 72618 preserved.
Independent acceptance is pending.
