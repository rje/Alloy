# Centerline and parallel-edge diameter slice

Candidate for SKT-038 / #215, built from `916daff`.

Public workflow evidence is indexed by `tutorial.next-revolve-feature-checkpoint`.
The permitted public tutorial steps 41–45 show conversion to Centerline, a detached
vertical axis and closed stepped profile, and D selecting that axis plus a parallel
profile edge to place a full diameter annotation. The source shows Ø100 and Ø165;
it does not specify all coordinates, height, constraints or remaining DOF. Detailed
ledger and source visuals remain in the external reference records.

## Implementation contract

An authored line may carry optional boolean `centerline`, independently of the
existing Construction intent. Centerline participates in closed profile boundaries;
a detached open axis does not discard a separate closed profile. Conversion retains
point and line IDs, existing relations, feature identity and ownership. Explicit
conversion to either Centerline or Construction clears the other intent. Absent
flags remain legacy normal geometry. The optional schema and mutually exclusive
conversion are Alloy choices, not documented public storage behavior.

`centerlineDiameter` stores ordered `entityIds: [axisLineId, profileLineId]`, retained
`side: -1 | 1`, `driving`, and a positive full `valueMm` for driving dimensions.
Exactly one line must be Centerline, neither Construction. Lines must be distinct,
nondegenerate, parallel and separated. The solver adds one half-distance equation;
it does not add a hidden parallel relation or remove orientation DOF. Both endpoint
signed offsets are checked after solving. An edit that loses parallel orientation,
conflicts or yields zero separation refuses the whole draft transaction. Retain
orientation through explicit Parallel or Horizontal/Vertical relations as needed.
Axis endpoint order and retained side define the branch; they are not silently
reversed. Driven dimensions measure twice the perpendicular distance and remain
read-only with no equation or DOF reduction.

The shared `sketch.centerline` and `sketch.centerlineToggle` commands back the
Normal/Centerline palette control. D recognizes the axis/parallel-edge pair. The
annotation uses a mirrored opposite witness and the full diameter span with Ø,
using existing placement, numeric editor, label editing and Cancel behavior.
Centerline is independently drawn with a dot-dash stroke. The button layout and
stroke styling are Alloy choices; exact source spacing/defaults remain unknown.

## Bounded verification

- `sketch_centerline_test`: detached axis and boundary Centerline preserve the
  closed region, stable refs/side, Ø100→Ø110 edit, one distance DOF, driven read-only,
  nonparallel/lost-orientation refusal, referenced-axis conversion refusal, undo/redo and storage.
- `centerline_controls_test`: real application/GPU with synthetic Qt input for
  palette conversion, D axis/edge selection, placement/value, mirrored 100 mm span,
  label edit, Cancel, driven read-only, undo/redo, Finish and save/reopen.

The affected app/core/native targets built with AppleClang 17, Qt 6.11.2 and
OCCT 7.9.3 on macOS arm64. Focused geometry test passed. The serialized native
Qt run passed 3/3 in 3.371 seconds and exited 0. Synthetic Qt mouse/key input
clicked the visible scrolled palette control, pressed D, picked two real lines,
placed the annotation, typed `100 mm`, edited to `110`, and canceled typed `120`.
Existing shared API activated dimension editing; physical input and canvas label
double-click are not separately claimed.

Evidence is in the isolated build's `build-centerline/evidence/`: `qt-test.log`,
`centerline-converted-palette.png`, `centerline-diameter-100.png`,
`centerline-diameter-edited-driven.png`, and `centerline-state.json`.
Actual captures show the distinct Centerline control, mirrored Ø100/Ø110 span,
and `(Ø165)` driven measurement. The test checks 100 mm full witness span,
authored IDs, Cancel equality, draft/model undo and exact authored save/reopen.
Fixture authoring, initial selection, edit activation, driven creation, history and
reopen use the shared API. Actual conversion click, D/picks/placement and typed
numeric/Return/Escape use Qt input. Native Qt results are distinct from geometry/storage checks. Captures were
inspected; public source and Alloy fixtures/viewports are unmatched, so source
workflow is matched within this slice while exact spacing/style metrics remain
unknown. Remaining profile DOF is truthfully reported; complete constraint is not
claimed. No private editor remains open. Astra independently reviewed source, Qt log and all three PNGs and reported no
bounded blocker. Final integration and issue acceptance remain parent-owned.
No full Revolve workflow qualification, angular controls, automatic axis selection,
or general nonparallel line distance support is claimed by this slice.

## Independent acceptance

Astra accepts bounded #215 candidate `10cac47` on 2026-10-13. Independent review
covered the authored schema, profile participation, solver equation and endpoint
postcheck, shared command and input routing, annotation construction, focused test
sources, persisted core/Qt logs, saved draft and all three actual captures. The
reviewer reused the recorded runs and did not launch another editor.

Centerline conversion preserves line/point identity and boundary participation;
it does not silently substitute Construction. The diameter normalizes the retained
axis/edge pair and side, uses half the full value for its one distance equation,
and rejects loss of parallel orientation before publishing an accepted draft.
The focused free-endpoint case specifically tests this refusal without inventing
a parallel constraint. Driven dimensions add no equation and omit authored value.
The saved draft contains driving 110 and driven outer diameter references, both
with retained side -1; the driven value is evaluated rather than stored.

The captured Ø100 and Ø110 annotations span genuinely mirrored witnesses across
the axis; `(Ø165)` is readable and distinguished as driven. The palette conversion
control is visible after scrolling. The early conversion image is not a fitted
whole-profile view; later annotation captures show the complete fixture. Qt input
and shared-API setup/edit activation boundaries are as documented above, with no
physical OS input or label double-click claim. There is no blocking mismatch in
this child. Exact visual parity, arbitrary nonparallel line dimensions, native
Revolve selection/manipulation and the complete tutorial remain separate gates.
