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
open. Native projected-endpoint picking/snapping is not yet implemented; endpoint
dimensions are API-qualified only. The native guide drawing sequence itself was
not part of this check. The private app was closed and the human app unchanged.
