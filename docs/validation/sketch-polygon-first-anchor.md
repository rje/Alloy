# SKT-103: retained first Polygon anchor

Accepted 2026-10-17 for one actual placement relation per retained Polygon.
The public origin, point and midpoint snap evidence, cue limits and provenance
are external in `../Alloy-reference-records/knowledge/specs/skt-093-polygons-2026-10-17.md`
(first-anchor addendum, references P1 and P7–P10). The center of Inscribed and
Circumscribed modes, or first base endpoint of Edge mode, can acquire an
origin, authored sketch point or retained straight-line midpoint. A transient
cue appears only when that relation can be committed. Grid alignment does
not claim a geometric relation.

The authored Polygon owns one distinct anchor point with a real
Coincident/Midpoint constraint when snapped, or a Fix for an unsnapped
anchor. Each boundary vertex is constrained at its exact relative offset
from the solved anchor; its coordinate is not independently frozen. A source
move therefore translates the same closed profile and downstream solid.
Surviving boundary IDs and external constraints persist through size/count
edits. Legacy fixed groups remain readable and migrate deterministically on
edit. Missing, ambiguous, self-referential or removed sources refuse
transactionally.

Evidence from Release `build-preview-release`, binary SHA-256
`f0e2f1427b64262ba7bb9265408419beb5c93eee14f568d450a788daa652c438`:

- Focused `polygon_anchors`, `sketch_polygon`,
  `polygon_anchor_controls`, `polygon_annotations_controls` and
  `sketch_polygon_controls` tests passed 5/5 in 15.18 s. Core checks include
  free and constrained source motion, point/midpoint/origin and projected
  support, an external Coincident on a surviving vertex, count/size edits,
  stable IDs, legacy migration, refusal, history and reopen. The native Qt
  test acquires real viewport cues and clicks each supported first-anchor
  mode without intervening focus repair; an ambiguous pick is refused.
- The real application gate in
  `../Alloy-reference-records/validation/skt103-polygon-anchor-final/`
  attached a Circumscribed hexagon to a standalone sketch point. Moving that
  point from x=10 to x=25 mm translated the same 5 mm extrusion by exactly
  15 mm without changing body ID or volume. Editing the retained inradius
  from 10 to 12 mm preserved the anchor and changed volume from 1732.0508
  to 2494.1532 mm³. Undo/redo, saved reopen and independent STL readback
  passed; the STL is one closed 20-triangle component, Euler characteristic
  2, with bounds (13,-13.8564,0) to (37,13.8564,5) mm. The saved model,
  export, transcript, final-solid capture and actual acquired-cue capture
  are external evidence beside the receipt.
- The app reported Apple M4 hardware acceleration, OCCT OpenGL and a
  Metal-backed OpenGL 4.1 driver in its 2× framebuffer. Astra independently
  inspected the solver equations, source validation, native test and real-app
  evidence and found no blocker for this bounded first-anchor slice.

The acquired cue and retained relation match the documented first-point
behavior conceptually; exact cue styling/placement in current public views
is unknown. Snapping the second radial vertex/edge midpoint or second base
endpoint, with its size/angle ownership, remains Todo
[SKT-105](https://github.com/rje/Alloy/issues/528). This receipt does not
claim complete Polygon snap parity, physical hand input or Windows/Linux
runtime/GPU qualification.
