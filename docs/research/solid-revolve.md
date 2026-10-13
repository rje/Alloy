# Retained Revolve foundation — SOL-029

New Body from one earlier solved closed planar sketch profile and one authored
nondegenerate straight line in that same sketch. Store `sketchId`, `profileId`,
`axisEntityId`, `extent` and canonical `angleDeg`. Any authored line can be the
axis: boundary, construction or centerline intent does not alter axis eligibility.
This feature does not implement Centerline semantics or change profile extraction.
The detached axis fixture uses existing Construction conversion; source Centerline
and detached-open profile behavior belong to SKT-038/#215.

`solid.revolve` creates/edits the feature. `command.begin` with command
`solid.revolve` exposes the shared draft, reserved feature/body IDs and input
schema; `command.update`, `command.preview`, `command.cancel`, `command.commit`
and `feature.editDialog` share that authority. Preview evaluates actual OCCT
geometry without authoring history. Native controls are unavailable and the
session states `nativeControlsAvailable:false`; SOL-030/#216 owns those controls.
No interactive dialog or angular manipulator is claimed.

Defaults are Full/360°. Partial accepts 0.001°≤angle<360°. Positive rotation follows
the right-hand rule about the retained line's start→end direction; this is an
explicit Alloy choice. New feature/body reservations must be paired; edits retain
those IDs and ownership. New features inherit active component ownership. Full
requires exactly360°. New Body only: Boolean modes remain in SOL-004/#49.

OCCT rotational face sweep supplies the solid. The profile and axis resolve at the
feature's history position through its current solved sketch frame. Exact kernel
bounds in a transverse frame refuse profiles crossing the axis, avoiding the
bounded self-intersection case. Axis boundary contact is supported. Kernel output
must be one valid closed positive-volume solid. Missing axis/profile, malformed
angle and evaluation failures preserve accepted authored data/history.

Partial start/end planar caps use exact kernel FirstShape/LastShape provenance,
feature-derived IDs and rotated right-handed outward frames. Full has no sweep
caps. No proximity naming, general curved support, retained solid-edge family,
separate-sketch axis or Project Axis behavior is implied. Planar attachment beyond
these cap identities requires its own cumulative evidence.

Public workflow source index: REF-004, external
`tutorial.next-revolve-feature-checkpoint` and steps39–50 ledger, retrieved
2026-10-13. Fixture dimensions are Alloy choices. Native source workflow and
visual fidelity remain pending #216 and the tutorial gate.
