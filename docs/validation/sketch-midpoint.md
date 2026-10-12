# Persistent circle midpoint checkpoint

Bounded SKT-005/SKT-016 tutorial slice, 2026-10-12. Tangent circumference
inference and mixed line/circle profile extraction remain separate blockers.

During the first circle-center click, finite normal or construction line midpoints
are snap candidates. A triangular cue reports the stable target line ID. Hover
and first click do not alter the authored draft or edit journal. The circumference
click atomically creates a distinct center point, circle and midpoint relation.
Explicit coordinate circle creation remains unchanged; radius remains free.

The retained relation references the point and line IDs, resolving current line
endpoints each solve. PlaneGCS symmetry equations enforce the midpoint and an
independent mean-coordinate check verifies it. Stale targets and endpoint self
references reject. Normal/Construction conversion preserves references.

## Evidence

- `build-tutorial/evidence/midpoint-build.txt` and `midpoint-core.txt`: build and
  focused curve regression pass; parent width20→30 moves center10→15, radius/IDs
  retained, DOF1, JSON roundtrip and invalid-target checks.
- `build-tutorial/evidence/sketch-midpoint-ledger/report.json`: real-app pointer
  cue/first-click/commit, parent driving edit, construction toggle, Undo/Redo and
  save/reopen pass. Captures include midpoint-cue and midpoint-after-line-edit.
- Native `C`, center click, circumference click, Escape on an API-opened first-
  activity sketch with its left end converted to construction produced a center
  at(0,50), the correct retained target and DOF1. First click retained6entities;
  completion added the circle. See `midpoint-native-center.json`,
  `midpoint-native-created.json/.png`. Setup and viewport navigation used the API;
  no full native Activity2 claim.
- Public behavior and visual provenance are external at knowledge/specs/
  midpoint-circle-tangent-inference.md and indexed record
  sketch.circles-arcs-dimensions. Triangle cue is source-backed; acquisition radius
  and ambiguity policy are Alloy proposals.

## Limits

Only circle-center midpoint inference is added. Circle circumference is unsnapped;
no automatic tangent relation or three-region profile readiness is claimed.
Astra independently accepted the visible cue and persistent glyph, with placement
parity unclaimed. No CI/release.
