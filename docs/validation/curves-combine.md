# Analytic curves and Combine: bounded evidence

Initial checkpoint recorded 2026-10-12. Later native arc qualification is recorded
below; Combine native acceptance remains separate. Circle/arc and Combine implementation is available in the
separate `build-next` candidate. API, geometry, storage and bounded visual review
passed; native mouse/keyboard acceptance remains incomplete. This does not close
SKT-006, SOL-002, any tutorial gate or a release gate.

## Implemented workflow

Center Diameter Circle, Center Point Arc and Three Point Arc author exact analytic
geometry. Three Point Arc accepts start, end, then a point on the arc. Circle D
uses diameter; arc D uses radius. Radius/diameter lengths honor document units;
explicit Arc Sweep uses degrees. Solved curves produce actual OCCT edges, with
exact supported single-loop profiles and editable circle-profile extrusion.
Controls include C, canvas gestures, disposable previews and tool-specific numeric
entry. Native gesture code is implemented but has not been physically qualified.
Two-line angular dimensions, broad inference and holes/multiple profile regions
remain open.

Combine explicitly selects target and tool bodies, Join/Cut/Intersect and Keep
Tools. It retains the target body identity and stores an editable history feature.
Editing shows the input history stage, including consumed tools, and previews the
operation stage while validating all downstream history. Cancel restores the final
scene. Intersect operates against the union of the tool group. The initial contract
requires one valid closed solid result; empty/disconnected results fail atomically.
Direct Extrude Join/Cut/Intersect remains SOL-015, separate from Combine.

## Evidence and candidate boundaries

- Core tests passed: new `sketch_curves` and `boolean_document`, plus existing
  `sketch_solver`, `sketch_profiles` and `extrusion_document` after integration.
- `build-next/evidence/curves-combine/report.json` records passing existing
  sketch/extrusion and new curve cases. Its initial Boolean case failed because
  the test compared a full state snapshot from before undo/redo, including an old
  revision. The test snapshot was corrected; no application change was needed.
- `build-next/evidence/combine-final/report.json` then passed explicit operation
  previews, consumed-tool canvas-coordinate picking, cancel restoration, history,
  upstream edits and reopen on candidate `c9acd8de512f24bf5e421eb0bfa35c16552ac472c35ab5b48c85c0864b26e91e`.
- Actual captures exposed a missing diameter font glyph and coarse curved-solid
  tessellation. A presentation-only correction uses supported Ø text and tighter
  AIS deviation/angle settings. The fresh curve case passed in
  `build-next/evidence/curves-visual-final/report.json`, candidate
  `8f5a9f2e74f8c8c5b334410992036c6a2d33b2be03b037262d43082aee732a9b`.
  It verifies diameter/radius/sweep editing, conflicting-input preservation,
  annotation placement, circle extrusion volume 125π→180π, undo/redo and reopen.
- Astra independently reviewed source, matching receipt hashes and actual GPU
  captures. Diameter labels and smooth disk presentation passed on the final
  candidate; Combine preview/result presentation passed on the earlier candidate.
  These are bounded visual observations, not exact reference-style parity.
- Native controller access to the separate test app failed with
  `cgWindowNotFound`, including a desktop-controller launch. No physical input
  claim follows from API-coordinate picking. The temporary test apps were closed.
  In particular, verify C/D/arc gestures and Keep Tools' visible checkbox states
  before closing the implementation issues.

Public-reference records and visuals stay in the external knowledge archive.
The [query guide](../knowledge/README.md) explains access. Actual test artifacts
stay under ignored `build-next/evidence`; no research assets are shipped.

## Native arc completion supplement (2026-10-12)

On the private ExchangeCandidate, native Create → Arc menus selected Center
Point Arc and 3-Point Arc. Center/start/end clicks authored the first quarter arc;
start/end/through clicks authored the second arc. Native D, curve selection and
canvas placement opened the radius editor; typing 12 and Enter retained a driving
12 mm radius on the first arc. It remained an analytic 90° arc, length 6π mm.
Freehand placement is pixel-limited: the initial radius was 9.9291 mm, and the
second arc was approximately radius 10.0181 mm with clockwise sweep 181.014°.
These are placement observations, not claims of exact freehand numeric input.

Native Cmd-Z restored the exact pre-dimension draft; Cmd-Shift-Z restored the
exact dimensioned draft. Native Finish committed the sketch. API save/reopen
retained identical authored features. Blank XY setup, inspection and storage
were API actions; curve creation and radius entry were actual native input.
The receipt and state captures are under `build-exchange/evidence/closure/`:
`native-arcs-receipt.json`, `native-arcs-created.json`, `native-arcs-dimensioned.json`,
`native-arcs-undo.json`, `native-arcs-redo.json`, `native-arcs-finished.json`,
`native-arcs-reopened.json`, `native-arcs-dimensioned.png` and `native-arcs.alloy`.

The private binary contains unchanged qualified sketch code plus preliminary
STL work; this check does not qualify exchange features. Existing circle native
input and placed two-line angular dimension evidence is recorded in
[the bracket workflow](illustrated-bracket.md), [angular sketches](angular-sketch.md)
and [five-circle Cut](five-circle-cut.md). Exact visual parity and broad analytic
constraint families remain separate tracked requirements.

Astra independently reviewed the actual capture and exact state comparisons;
SKT-006 #33 is closed. SOL-002 Combine remains open for its native completion.

## Native Combine completion supplement (2026-10-12)

An API-created 20 × 20 × 10 mm plate and radius-3 through-cylinder were selected
using native Combine controls. Native target/tool selection, Cut, Keep Tools
checked and OK produced two valid closed bodies. Reopening the Timeline feature,
unchecking Keep Tools and accepting produced one valid closed plate with the same
body ID and volume 4000 − 90π = 3717.256661176918 mm³. Actual captures show both
checkbox states and the resulting hole. API save/reopen preserves exact authored
features. No automation substitution was used for the Combine controls.

Evidence: `build-exchange/evidence/closure/native-combine-receipt.json`, matching
`native-combine-*.json` states, `native-combine-keep-true.png`,
`native-combine-keep-false.png`, `native-combine-result.png` and `native-combine.alloy`.
Astra independently inspected captures, topology, volume, identity and storage.
Combined with the earlier API Join/Intersect, hidden-target, atomic-failure and
history checks, SOL-002 #40 is qualified for closure. This does not claim physical
input for every operation or resolve the separate #158 accessibility row-name issue.
