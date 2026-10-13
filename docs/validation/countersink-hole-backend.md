# Retained Countersink Hole — backend and API candidate (#247)

Status: backend and real-entrypoint API evidence only, 2026-10-13. Native
selection, preview and dialog acceptance remain pending. The public source is
`plate-countersink-pattern-20261013`, page 4, retrieved 2026-10-13. Its
provenance, source screenshots and uncertain gestures remain external in
`knowledge/tutorials/next-after-outlet/independent-countersink-contract.md`.
This is one Hole ingredient, not the nine-hole Pattern or complete plate lesson.

An authored `hole` feature modifies one same-owner body and retains the earlier
supported `sourceSketchId`, explicit standalone `sourceEntityId` and its
`sourcePointId`, plus `targetBodyId`. The finite placement/type/extent are
`fromSketch`/`countersink`/`all`. Bore diameter, sink diameter and included
angle are editable numeric parameters. The evaluated sketch point and its
stable planar support frame determine entry and inward direction. Validation
refuses a lost source point, a stale or off-face entry, an unavailable support,
incorrect direction, invalid dimensions and a failed or ineffective cut without
publishing a partial document. Failures attribute the feature and all four
retained source/target IDs. The existing Boolean authority requires one valid
closed connected solid and transfers surviving face/edge lineage.

The reusable `SolidHole::tool(currentTarget, entry, outward, bore, sink,
angle)` recomputes All reach from the current target bounds for each call.
Its countersink depth is `(sink − bore)/(2 tan(angle/2))`; no authored fixed
depth or precomputed seed cutter is stored. This operation is available for a
later translated occurrence evaluator; no Pattern behavior is implemented here.

Isolated worktree: `../Alloy-worktrees/countersink-hole`, branch
`feat/countersink-hole`, base `3ed8105`. Backend commits `caca678` and
`7288408`; CMake controls wiring `6ff47ef`. The Qt 6.11.2 / OCCT 7.9.3 Debug
app and affected controls target built in `build-hole`. The API run used that
real executable, SHA-256
`c91baca162705c3762db35fc3c6912dfa8f4f169be34ae54edc2b1647decd5e7`.
At build time the branch was `7288408` with uncommitted native control source
and CMake wiring; this binary is evidence for that candidate snapshot, not a
claim that backend commits alone reproduce the native UI. The later `6ff47ef`
commit records the same CMake wiring. A final combined clean build belongs to
the native candidate review.

Focused `countersink_hole` core test passed 1/1 in 0.77 s after the diagnostic
fix. On the source 75 × 50 × 10 mm R5 rounded plate, point
`(−27.5,+15,10)`, Ø4 bore, Ø10 sink and 90° angle give a 3 mm sink and remove
`67π mm³`; the resulting volume is `36500 + 183π mm³`. The test checks the
retained target identity, bore edit, thickness 10→12 All reach, centered width
75→85 moving the dimensioned point to x−32.5, exact undo/reopen, invalid
dimensions and missing-point atomic refusal with attributed diagnostics. An
XZ cap control proves drilling follows the actual support normal rather than
world Z.

`tests/countersink_hole_api.py` passed through the actual app entrypoint and
closed its editor. The external evidence is
`../Alloy-reference-records/deliverables/countersink-hole-2026-10-13/api/`:
`report.json`, `transcript.json`, `seed-hole.alloy`, `seed-hole.png` and app log.
The 2880 × 1800 top capture was inspected: the rounded plate and conical entry
are visible and readable, with no overlay. The transcript confirms direct
`solid.hole`, exact `67π` removal and 3 mm sink, undo/redo, width-driven point
movement, save and exact authored reopen. Final post-width-edit volume is
42074.9114556 mm³. This is shared-command operation and one GPU capture;
native Hole field/pointer behavior and source-matched styling are separate.

The native lane's first focused run stopped at an asserted viewport focus state
after Select Target, before its target click. Its editor closed. That failure
is under investigation and is not counted as a native pass. Neither the
feature Pattern nor the cumulative named-parameter plate gate has run here.
