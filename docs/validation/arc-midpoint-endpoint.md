# Arc midpoint on an attached path endpoint — SKT-068 candidate

The existing Midpoint relation now accepts an ordinary circular arc and one
retained point. It constrains the point to the arc circle and the perpendicular
bisector of the endpoints, then verifies the **authored half-sweep** result;
the antipodal solution is refused atomically. Line Midpoint remains supported.
Choosing the sketch origin for Midpoint creates its fixed, stable point ID in
the same accepted sketch change. On a sketch attached to a retained endpoint
Plane Along Path, that origin follows the evaluated source spline endpoint.
There is no copied world point or new projection authority.

The native Constraints > Midpoint control accepts a line or arc, then a point
or the visible sketch origin. The real midpoint triangle is placed beside the
solved target. The public source calls for an arc-to-origin relation on the
start section and a 30 mm line midpoint on the end section; the compact
source, observations and unknown plane roll are recorded externally at
`../Alloy-reference-records/knowledge/tutorials/advanced-audit-2026-10-13/arc-midpoint-endpoint-contract.md`.

The focused `arc_midpoint` core test passes a 50 mm front arc with 10 mm rise
at both endpoints, a 30 mm rear line, and source spline edits that carry both
attached section frames. It checks the actual half-sweep point, undoes/reopens
both sections, and refuses an opposite-sweep arc without changing the sketch.
The existing `outlet_constraints` line Midpoint test also passes.

A 44-call real-app API workflow in
`../Alloy-reference-records/deliverables/arc-midpoint-endpoint-2026-10-14/api-01/`
creates both retained endpoint planes and source-size section sketches, edits
the spine, checks attached frame updates, then undoes, saves and reopens.
The Qt-native test in
`../Alloy-reference-records/deliverables/arc-midpoint-endpoint-2026-10-14/native-03/`
uses the visible Midpoint button followed by arc and origin canvas clicks;
its captured relation has the triangle beside the arc midpoint. A later
capture uses shared dimension commands and shows readable 50/10/10 dimensions
and `Fully constrained`. Initial Qt test-host activation occurs before the
native sequence; no focus or selection is repaired between demonstrated steps.
The capture is Alloy runtime evidence, not matched-reference screenshot
parity. Surface Loft and arbitrary cross-sketch point selection remain outside
this child.

Independent Astra review accepted candidate `e163bca`; main integration
`83024c0` passes the application build and focused arc-midpoint core test.
