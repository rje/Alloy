# SKT-093: retained regular Polygon tools

Accepted 2026-10-17 for the bounded Inscribed, Circumscribed and Edge Polygon
foundation. The public command and visual record is external at
`../Alloy-reference-records/knowledge/specs/skt-093-polygons-2026-10-17.md`;
it distinguishes documented inputs from the tutorial-frame observations and
unknown modern defaults. Alloy's explicit 3–128 side bound and six-side
initial value are implementation choices, not exact-reference claims.

Sketch > Create > Polygon exposes the three modes. Inscribed uses a
center-to-vertex circumradius, Circumscribed a center-to-edge-midpoint
inradius, and Edge a fixed first edge followed by a side choice. Typed count,
size and angle, preview, repeat, Escape and retained edit use the shared
`sketch.polygon`, `sketch.polygonInput`, `sketch.polygonTypedInput`,
`sketch.polygonEditSelect` and `sketch.polygonUpdate` commands. The saved
polygon group owns ordinary closed sketch edges and stable IDs. A count edit
that would retire referenced geometry refuses without changing the draft.
Invalid native count/size text stays visible, blocks both a canvas click and
Return, and can be corrected without abandoning the tool.

Evidence from the Release `build-preview-release` app (binary SHA-256
`afc763a2e3184940ba2d75dcf477b6154845da1e861f2298c41c51f36f8f2953`):

- Focused `sketch_polygon` and `sketch_polygon_controls` tests passed 2/2 in
  5.87 s. Core checks cover three construction geometries, retained size and
  count edits, surviving IDs, reference refusal, undo/redo and reopen. Native
  Qt input checks the actual focused fields and viewport clicks, Tab/Return,
  first-edge side choice, retained Apply, Escape, and invalid count 2/size 0
  refusal followed by correction. No step repairs focus or selection between
  user inputs.
- The real application script [polygon_print_api.py](../../tests/polygon_print_api.py)
  passed in `../Alloy-reference-records/validation/skt093-polygon-final/`.
  Three modes produced three closed regions. A 5 mm extrusion of the
  circumscribed hexagon had volume 1732.0508 mm³; editing its inradius from
  10 to 12 mm propagated to 2494.1532 mm³. Invalid count 2 left the draft
  exact, undo/redo and saved reopen retained the authored model, and the
  independently read binary STL was one closed component with 20 triangles,
  Euler characteristic 2 and bounds (-12,-13.8564,0) to (12,13.8564,5) mm.
  The saved model, STL, transcript and sketch/isometric captures are beside
  the receipt.
- The captured app reported Apple M4 hardware acceleration, OCCT OpenGL,
  OpenGL 4.1 Metal-backed driver, a 1744×1132 native framebuffer and 2×
  display scale. The actual captures show readable filled profiles and the
  extruded solid; they do not establish cross-platform presentation.

Public examples and Alloy both show filled closed polygon regions and the
three basic placements. Exact styling/default parity remains unknown because
the available public frames are not matched in size, theme or state. Actual
placement snaps and retained relations are Todo
[SKT-103](https://github.com/rje/Alloy/issues/526) for a first anchor and
[SKT-105](https://github.com/rje/Alloy/issues/528) for the second driver.
Canvas size annotations and center/anchor markers are Todo
[SKT-104](https://github.com/rje/Alloy/issues/527). The fixed solver status
and its edge color are reported honestly rather than recolored to resemble
an underconstrained public example. Physical hand input and Windows/Linux
runtime/GPU qualification remain open.
