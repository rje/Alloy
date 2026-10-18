# SKT-094: Three-Point Rectangle

Accepted 2026-10-17 for the documented rotated Rectangle workflow. The
public input contract, provenance and video frames are in the external
`../Alloy-reference-records/knowledge/specs/skt-094-rectangles-2026-10-17.md`.
P1 selects a corner, P2 defines the first edge and its direction, and P3
chooses the perpendicular side. The native tool offers rough pointer sizes
or typed first/second distances locked with Tab, then repeats after a completed
rectangle; Escape exits or discards the current incomplete placement.

The shared `sketch.threePointRectangle` command creates four ordinary stable
line edges with two Parallel and one Perpendicular solver relations. Typed
distances become editable driving linear dimensions; rough placement retains
its free size and rotation. A first-corner origin, authored point or line
midpoint snap retains an actual relation. A collinear side, zero baseline or
conflicting typed length refuses without altering the draft. The new edges
form a closed selectable profile and follow the normal sketch history,
dimension-edit and save/reopen routes.

Evidence from Release `build-preview-release`, binary SHA-256
`5a23cff75620b3afebdfd214491c37ce6e1a3d26b29e8616f2d8dc8674d2bec5`:

- Focused core and native tests passed. Core checks include an oblique 30×15 mm
  profile, both perpendicular-side choices, editable 36 mm baseline, exact
  downstream Extrude volume, transactional refusal, Undo/Redo and native-file
  reopen. The Qt native test delivered actual menu activation, three clicks,
  live preview, first/second Tab locks, invalid input, repeat, Escape and
  native dimension edit without inter-step focus repair. Its final 3-check
  run took 5.089 s. Captures are in
  `../Alloy-reference-records/deliverables/three-point-rectangle-2026-10-17/native/`.
- The separate real-app control workflow in
  `../Alloy-reference-records/validation/skt094-three-point-rectangle-candidate1/`
  created a 50×50 mm rotated rectangle from an authored first point and two
  driving lengths, extruded it 5 mm, changed the first length to 60 mm and
  moved the source point to (5,7) mm. The same body ID remained; its volume
  changed from 12,500 to 15,000 mm³ and its bounds moved. A collinear third
  point refused with the exact draft unchanged. Undo/Redo and saved reopen
  passed. Independent binary STL readback found one closed 12-triangle,
  8-vertex component with Euler characteristic 2 and volume 14,999.99996 mm³;
  mesh bounds matched the evaluated body within 0.001 mm. The model, STL,
  captures, 33-call transcript and receipt are beside the gate.
- Actual captures were inspected: a rotated filled profile with visible
  Parallel/Perpendicular glyphs and two length annotations, readable palette
  and inline field; and a smooth shaded GPU extrusion. The app reported
  Apple M4, OCCT OpenGL, Apple vendor, OpenGL 4.1 Metal driver and a 2×
  framebuffer. A separate Sol source review checked native-to-control command
  routing, solver relations, target validation, transactional publication and
  test evidence; no material blocker remained.

The public demonstration is older and has unmatched viewport/theme/scale, so
exact modern styling, snap priority and glyph placement remain unknown.
Second/third-point retained snap behavior and physical hand input were not
qualified. Windows/Linux runtime and GPU presentation remain release gates.
A separate Parallelogram command is not established by current public sources;
SKT-106/#530 retains that disposition rather than being claimed here.
