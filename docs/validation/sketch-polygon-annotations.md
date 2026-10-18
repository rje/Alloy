# SKT-104: editable Polygon canvas annotations

Accepted 2026-10-17 for the bounded three-mode annotation workflow.
The dated public measurements and edit gesture are recorded outside the repo
in `../Alloy-reference-records/knowledge/specs/skt-093-polygons-2026-10-17.md`
(annotation addendum and P6). Older public frames show a center-to-edge
midpoint callout, a center-to-vertex callout, and a measured base edge;
general current sketch guidance documents double-click value editing. The
frames do not establish a dedicated center glyph away from the origin or an
exact current label style.

Alloy now draws each annotation from the retained Polygon geometry:
Circumscribed measures the true edge midpoint/inradius, Inscribed the
vertex/circumradius, and Edge the fixed base endpoints/edge length. The
annotation has the stable Polygon ID, visible units, truthful anchor witness
and the existing sketch-dimension hit target. Native double-click opens the
inline editor, and acceptance invokes authoritative `sketch.polygonUpdate`;
no separate solver dimension is authored. Dimension visibility hides/shows
these labels with other sketch dimensions.

Evidence from Release `build-preview-release`, binary SHA-256
`56607d2e9b9bba0cee6e2dbf18d23c9893056bc732b5fec46756232bb835e630`:

- Focused `sketch_polygon`, `polygon_annotations_controls` and
  `sketch_polygon_controls` tests passed 3/3 in 10.63 s. The native Qt test
  hit and double-clicked each of the three actual canvas labels, edited them
  through the shared command, refused zero while keeping the editor active,
  checked three closed profiles and exact draft Undo/Redo, toggled dimension
  visibility, and converted a 0.5 in entry to 12.7 mm. No synthetic authored
  dimension appeared.
- The real-app gate in
  `../Alloy-reference-records/validation/skt104-polygon-annotations-final3/`
  inspected all three annotation records and captured their GPU presentation.
  Editing the circumscribed inradius from 10 to 12 mm changed its on-canvas
  label from R10 to R12 and propagated through a 5 mm extrusion from
  1732.0508 to 2494.1532 mm³. Undo/redo, saved reopen and independent binary
  STL readback passed: one closed 20-triangle component with Euler
  characteristic 2. The model, STL, captures and command transcript are in
  that external evidence directory.
- The app reported Apple M4 hardware acceleration, OCCT OpenGL and a
  Metal-backed OpenGL 4.1 driver in its 2× framebuffer. Astra independently
  inspected the final capture and native/API paths and found no blocker for
  this slice.

The visible radial and base-edge callouts are conceptually compatible with
the public examples. Their exact style, offset and current-platform parity
remain unknown because the frames are unmatched. Labels are automatically
placed; dragging one currently refuses explicitly. Snapped, moving anchors
remain under [SKT-103](https://github.com/rje/Alloy/issues/526) and second
drivers under [SKT-105](https://github.com/rje/Alloy/issues/528). Physical
hand input and Windows/Linux runtime/GPU qualification remain open.
