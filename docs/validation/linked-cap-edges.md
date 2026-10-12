# Linked cap-edge constraint checkpoint

Verified on macOS, 2026-10-12, implementation `c8327ee`.
New positive-cap sketches automatically retain references to supported straight
boundary edges. Actual OCCT region/union/prism history supplies each edge; no
index, nearest-edge or frozen-coordinate substitute is used. References resolve
before solving and contribute fixed virtual geometry. Only stable source IDs are
saved; derived reference coordinates are stripped from authored output.

## Evidence

The combined build, focused face-support regression, sketch solver and legacy
extrusion regression passed. The real-app `sketch-linked-projection` ledger passes
182 commands, including the rounded bracket, two construction lines Parallel to
model edges, horizontal/vertical endpoint-relative distances, parent width/radius
changes that move the guides, retained reference/constraint IDs, undo/redo and exact
save/reopen. Evidence is under the isolated worktree's
`build-face/evidence/linked-projection-ledger`, run `dipqd2ey`.

Native verification used API-seeded construction guides, then the actual Parallel
control and two consecutive guide/model-edge pick pairs, Escape and Finish.
Degrees of freedom fell 8 → 7 → 6. Exactly two retained constraints each join one
authored line and one projected edge. `projection-native-{prepared,parallel,finished}.json`
and `projection-native-parallel.png` record state and presentation; the saved part
is `native-linked-bracket.alloy`, all in `build-face/evidence`.
Astra independently accepted source, state and the four purple references with
two readable Parallel glyph pairs. Contrast remains light; no exact visual parity
or full tutorial pass is claimed.

The tests caught two fixture mistakes, corrected without weakening application
contracts: importing another document requires native open, not a cross-document
commit; constrained local coordinates should change when their source edge moves.
Reference IDs and authored relations, rather than those dependent values, persist.

## Limits

Only straight positive-cap edges are projected. Curved references are reported as
unsupported. These references are excluded from profile boundaries; this is a
bounded reference-only slice, not general Project profile semantics. Every retained
reference must still resolve, including unused ones: losing/splitting/merging an
edge rejects the upstream transaction. Link repair and broader face naming remain
open. The original native guide drawing sequence was not part of this check;
the endpoint follow-up below qualifies additional native actions. The private app was closed and the human app unchanged.


## Native endpoint follow-up

Implementation `f62ebc7`, verified 2026-10-12. Projected endpoints are selectable
and read-only, and native line/circle creation can retain Coincident relations
to them while keeping new authored points separate from derived geometry.
Supported-face circle preview uses the same resolved plane as the accepted circle.

The focused face-support regression covers line/circle snaps, source edits,
history/storage and stale-target rejection. Native checks selected a projected
endpoint, verified `read_only_projection`, then used C and two clicks to create a
circle with a retained center Coincident. The live circle preview was separately
checked through the pointer-move API. Native D, two point picks, label placement,
100 and Enter created a driving horizontal distance from a projected endpoint to
an authored point; the exact separation is 100 mm and DOF fell 7 to 6. Finish and
native-file save succeeded. This check found and fixed a preview lookup that had
incorrectly searched authored points only.

Evidence: `endpoint-native-selected.json`, `endpoint-readonly.json`,
`endpoint-circle-preview.{json,png}`, `endpoint-native-circle.{json,png}`,
`endpoint-native-dimension.{json,png}` and `native-endpoint-dimension.alloy` in
the isolated worktree's `build-face/evidence`. Astra independently accepted the
reference identities, read-only behavior, retained relations and readable
dimension/extension lines. Rounded-edge center snaps and the exact tutorial
construction chain remain next.
