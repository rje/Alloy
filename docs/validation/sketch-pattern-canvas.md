# Rectangular sketch Pattern canvas candidate

SKT-178/#701, 2026-10-20. Extends the accepted retained rectangular
[Pattern foundation](sketch-rectangular-pattern.md) with viewport input for one
ordinary Circle and two perpendicular authored construction Lines. The retained
model, source-only solve, deterministic result IDs and owner guards are unchanged.

Public evidence is external in
`../Alloy-reference-records/knowledge/specs/skt097-sketch-patterns-2026-10-20.md`
(R1) and `skt097-canvas-picking-followup-2026-10-20.md` (P2/P3 for explicit
Objects/Direction field selection and sketch direction Lines; P4 is lower-weight
arrow-drag evidence). Retrieved 2026-10-20. No proprietary application was
operated. Public references establish role selection and interactive values;
exact handle art, snapping, hover colors and untouched defaults remain unknown.

## Shared workflow

Sketch > Create > Rectangular Pattern opens the existing embedded command dock.
Objects, Direction 1, Direction 2 and Handles buttons expose the active viewport
role. A valid Circle pick advances to Direction 1, then valid construction-Line
picks advance to Direction 2 and Handles. The visible prompt names the next pick.
Hover paints eligible authored curves gold; successful picks retain selection
feedback. A refused parallel Direction 2 preserves earlier valid picks and the
last valid preview. Role buttons allow re-picking without knowing identifiers;
the identity selectors and numeric fields remain available.

The four original GPU grips use blue D1/N1 and orange D2/N2 labels: arrows change
Spacing, square-plus grips change total Quantity. Drag follows the authored
line's positive direction in the sketch plane. Distance snaps to 0.1 mm; Quantity
changes by one for each existing spacing of drag. Label alignment/offsets keep
text outside the neighboring circle. This geometry, role autoadvance and snapping
are explicit Alloy policies. They do not claim exact reference graphics.

`sketch.patternRole`, `sketch.patternHover`, `sketch.patternPick`,
`sketch.patternHandle` (begin/move/end/cancel), and `sketch.patternExit` expose the
same controller. All candidate geometry goes through `sketch.patternPreview`.
The numeric fields synchronize with valid handle previews. Invalid overlap or
out-of-range gestures show refusal, preserve the last valid preview while held,
and restore the gesture baseline on release. Draft/history changes only on OK.
Cancel and Escape preserve authored intent. The actual retained grid-glyph
hit/double-click opens the same editor in Handles mode.

## Candidate evidence

Accepted build cache: `build-preview-release`, Release, private Qt 6.11.2,
OCCT 7.9.3, Apple Clang, `-j4`. External evidence is under
`../Alloy-reference-records/validation/skt178-2026-10-20/`.

- `sketch_pattern_canvas_controls_test`: actual Create popup action hit, native
  source/axis hover and picks, parallel-direction refusal preserving prior picks,
  all four handle drags and numeric synchronization, overlap refusal/rollback,
  Cancel/Escape/OK, distinct Undo/Redo, actual glyph edit/Cancel, six-profile
  through Cut, save/exact authored reopen and STL. Initial run: 3/3 functions,
  12.692 s. The label-placement follow-up repeats the complete gate: 3/3,
  12.690 s, with evidence in `native-labels/`. A final text-only D2 offset
  clears the vertical construction Line; `label-clearance/handles-clearance.png`
  captures the reopened real-app editor after that adjustment.
- `native-initial/` holds source-hover, four-handle, invalid-drag and printable
  six-hole solid captures, native-picked model/STL and full report/transcript.
  Input is delivered to the widget receiving the real hit. The fixture verifies
  popup activation/closure and command-produced viewport focus. Its sole initial
  activation is harness setup; it performs no interstep focus/selection repair.
  Hover uses Qt's QWindow pointer delivery rather than OS cursor repositioning.
- `tests/sketch_pattern_canvas_api.py`, `api-initial/`: the real executable
  reopens the exact native-produced model, edits source radius 1.5→2 mm and
  center (8,7)→(9,8) through the existing Cut, preserves six profile IDs, checks
  exact authored Undo/Redo/save/reopen, and exports the changed plate. Exact
  volume is 8023.008881569229 mm³. Independent stdlib mesh readback finds one
  consistently wound watertight component, Euler characteristic −10, 3492
  triangles, volume 8023.128476897367 mm³ (within 0.01%). Each hole has 864 sampled
  wall vertices at radius 2 mm about the six expected centers, spanning z=0..5.
  The native STL independently has the same component/Euler/winding properties.
- Existing rectangular and circular native shared-controls regressions each
  pass 3/3 functions (3.101 s and 3.779 s). No solver/kernel changes are present.
- Captures were inspected for prompt/hover/grip readability and six open holes.
  GPU diagnostics identify Apple M4 / Apple, OpenGL 4.1 Metal, hardware accelerated,
  MSAA4 and DPR2 native framebuffers. These are real GPU captures, separate from
  authored, exact-geometry and mesh assertions.

## Limits

This remains one ordinary Circle, Spacing / One Direction, two perpendicular
construction Lines, and positive distances/total quantities under the foundation's
bounds. Nonorthogonal directions are documented public behavior and remain
SKT-183/#706 Todo. Circular canvas picking/manipulators remain SKT-182/#705 Todo;
broader seeds, modes and suppression remain ADV-005. Physical AppKit pointer and
trackpad activation, DPR1 and packaged Windows/Linux runtime are unqualified;
Qt pointer delivery does not establish those gates. Public screenshots are not
matched in scale/state, so exact visual parity is unknown. No CI, release, push
or proprietary-app operation was performed. Private fixture editors closed;
preexisting user-owned PID 72618 was preserved. Independent acceptance is pending.
