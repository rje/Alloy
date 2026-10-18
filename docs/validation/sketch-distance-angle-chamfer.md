# SKT-111: Distance And Angle sketch Chamfer

Accepted 2026-10-18 for a finite connected line corner. The dated public
workflow and geometry evidence are outside the repository at
`../Alloy-reference-records/knowledge/specs/skt-111-sketch-distance-angle-chamfer-2026-10-18.md`.
The first ordered source line owns the distance. Alloy measures the angle at
that line's contact between the ray toward the old corner and the Chamfer
segment; the second setback follows from those values and the included wedge
angle. For a right angle, it is `distance × tan(angle)`. Alloy accepts only
`0 < angle < 180° - wedge angle` and finite nonoverlapping setbacks. These
range, order and crossing-refusal rules are explicit Alloy policies where the
public native behavior remains unknown.

Sketch > Modify > Chamfer > Distance And Angle, the three-mode Sketch Palette
Feature Options control and `sketch.chamfer` share one retained implementation.
Both driving values have canvas annotations and side-specific handles. The
original source lines and dimensions remain authored; a source edit recomputes
the diagonal, closed profile and downstream solid without changing retained
Chamfer IDs.

Evidence from Release `build-preview-release`, binary SHA-256
`0156d2c29a9bd8d4a79a0ff3422a869502764bf2d2b87f9f1dae71cc67f5a3e7`:

- Focused core/native Qt tests passed 2/2 in 23.24 s. Core checks included
  both source-line orders, a 60° rectangular corner, an oblique wedge with a
  geometry-specific angle limit, invalid span/range and unsupported crossing
  supports. Native input exercised menu/Feature Options, typed and inline
  distance/angle edits, both handles, 100° refusal, Cancel/Escape, source
  dimensions, extrusion, Undo/Redo and save/reopen. Captures, framebuffer,
  model and state are at
  `../Alloy-reference-records/validation/2026-10-18-sketch-distance-angle-chamfer/native/`.
- Independent [real-app runner](../../tests/sketch_distance_angle_chamfer_api.py)
  passed 33 commands. A 150×75 mm rectangle received an ordered 10 mm/60°
  treatment. Editing the angle alone to 30° left the distance at 10 mm; then
  editing distance to 12 mm and source dimensions to 160×80 mm kept Chamfer,
  contact, diagonal and body identities. The final closed region was
  12,758.430780618348 mm² and its 5 mm solid was 63,792.15390309174 mm³.
  A 90° value refused transactionally. Undo/Redo and native save/reopen
  reproduced the authored document and solid. Model, captures and transcript
  are at `../Alloy-reference-records/validation/skt111-distance-angle-candidate1/`.
- Inspected native captures show the 60° preview, 12 mm/30° edited state,
  angle arc, two handles, palette controls and source dimensions. The app
  reported Apple M4 hardware acceleration through OCCT OpenGL 4.1 Metal with
  a 2× framebuffer.

The accepted route reuses the previously checked extrusion/STL pipeline; no
second STL is claimed here. Crossing source lines and ambiguous quadrants are
Todo SKT-114/#538. Native defaults, exact handle styling, physical hand input and
Windows/Linux runtime remain unqualified.
