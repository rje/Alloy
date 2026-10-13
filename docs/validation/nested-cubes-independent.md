# Nested cubes independent review

Reviewed 2026-10-13 from the integrated application, saved model, strict STL
readback, stage captures, public source ledger, and sampled recorded workflow.
The reviewer did not implement the integrated feature changes. Source provenance
and public images remain external in
`knowledge/tutorials/warwick/nested-cubes-ledger.json`.

## Accepted scope

Tutorial #172's complete modeling sequence passes the real application API:
25 × 25 × 12.5 base; inward Shell 2; Mirror across the rim; Join the two halves;
three orthogonal diameter-15 through cuts; radius-1.5 rounding of twelve outside
edges; offset plane 18.5 below the top; centered 12 × 12 sketch; separate
12 mm extrusion. The saved document contains fifteen authored features and two
evaluated solids. The runner checks upstream Shell 1.8 editing, exact undo/redo
restoration, and save/reopen. Independent reopening confirmed two bodies.

The outer feature chain matches the ledger. Different allowed base quadrants
and bore axis ordering do not change the intended geometry. The twelve-edge
fillet follows the written source; its screenshot's eleven-selection count is
an explicitly recorded source inconsistency, not silently resolved evidence.

Independent triangle readback of `nested-cubes-mm.stl` passed the unchanged
strict checker: 5,036 triangles, two closed consistently wound components,
finite unit normals aligned with winding. The inner component has twelve
triangles, volume 1,728 mm³, and exact bounds (6.5,6.5,6.5)..(18.5,18.5,18.5).
The outer bounds are 0..25 mm within numerical precision; its triangulated
volume is 4,105.112657 mm³. This is mesh volume, not an exact BRep-volume claim.

The final actual-app image shows the centered separate cube through all three
visible circular openings and smooth outside rounding. The 53.4-second,
1244 × 902, 10 fps recording was sampled at six-second intervals using an
external contact sheet. Samples show the construction sequence and final
result; this is not a claim of uninterrupted manual playback or OS input.

## Feature acceptance and limitations

- #187: accept the bounded named planar history required by this complete
  Mirror/Join/Cut/Fillet/offset/sketch chain, together with prior focused outward
  frame tests. Arbitrary topology naming is not established.
- #194: accept the detached binary-STL precision-collapse handling for this
  model. Independent strict manifold/winding checks pass without weakening the
  checker. This does not establish every possible tessellation or unit scale.
- #195: fixed independently. With the reopened nested model and Save As Mesh
  modal still open, `app.quit` with `discard:true` exited normally with code zero
  within the existing five-second harness deadline.
- #172: accept the complete API modeling/export/artifact gate. Existing focused
  Qt tests supply native-widget routing evidence for prerequisite controls.
  An uninterrupted physical OS mouse/keyboard tutorial pass is unverified;
  native computer-use startup was unavailable. Exact visual parity is not claimed.

Follow-up on c6ee735: Mirror's corrected dock no longer clips selection counts,
and its source body is visibly blue while the mirrored preview remains gray.
The named input case passed again with inspected new pixels. No bounded Mirror
visual blocker remains. Loft is a separate gate.

## Evidence

- `build-tutorial-polish/evidence/nested-cubes-movie/`: runner receipt, fifteen-
  feature `.alloy`, STL, ten stage images, and application log.
- `build-tutorial-polish/evidence/nested-independent/`: independent reopen
  receipt/image, open export-modal image, and `quit-pass.txt`.
- `build-tutorial-polish/evidence/loft-independent/qt-input.txt`: the targeted
  Mirror and Loft input checks passed, four including setup/cleanup, zero failed.
- External `demos/2026-10-13/nested-cubes/`: source recording and independently
  extracted contact sheet. Recording is explicitly API-driven.
