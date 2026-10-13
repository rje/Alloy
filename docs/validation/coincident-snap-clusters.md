# Coincident snap clusters candidate — 2026-10-13

SKT-055/#243 fixes circle-center picking at a rough Center Rectangle origin.
The snap picker now groups authored point targets through retained Coincident
relations, and groups line midpoint inferences only when a retained Midpoint
relation names that authored point. The origin is the deterministic target for
its linked group. An incidental line midpoint at the authored point yields to
that point; distinct unlinked points at equal coordinates remain ambiguous.
The picker changes no point IDs, relations, or solver state.

The focused real-window Qt test started a rough 66 × 108 Center Rectangle on
the origin, added the horizontal midpoint axis, checked the shared pointer
preview, then clicked the origin and circumference through the GPU view. The
circle was authored, its first click targeted the origin, and all prior points
remained. A second same-coordinate rectangle with no origin Coincident relation
returned `ambiguous_snap` and left the sketch unchanged. Both cases passed in
`coincident_snap_controls_test` (4 Qt cases including setup/cleanup). The
inspection image shows the new circle at the origin; it is a focused gesture
record, not a complete outlet lesson rerun.

Evidence outside the repository:
`../Alloy-reference-records/deliverables/wall-outlet-sketch-2026-10-13/coincident-snap-candidate/origin-circle.png`
and `origin-circle-state.json`. The first failed cumulative attempt and its
rough rectangle state remain under `attempt-01` beside this evidence.

The cumulative blank-to-final tutorial gate and broader compatibility review
remain pending integration of this candidate.
