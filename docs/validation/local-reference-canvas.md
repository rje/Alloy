# Local reference canvas candidate

DOC-024/#294 implements user-local reference images with retained plane placement,
opacity/visibility, two-point distance calibration, editable document history
and portable embedded reopen. Shared native/API authority uses `ReferenceCanvas`
for authoring/calibration and `CadDocument::commitCanvases` for metadata-only
transactions; bodies and sketches remain independent. OCCT presents an actual
plane using in-memory `AIS_TexturedShape`, not a screen overlay.

## Bounded verification, 2026-10-14

- `reference_canvas_test` passes: aspect/default size, rotated pixel/plane inverse,
  calibrated physical distance, atomic refusal, undo/redo, source-file deletion
  and embedded reopen. A retained construction plane5→8mm edit moves the resolved
  canvas plane while preserving authored canvas data; undo returns5mm.
- `tests/reference_canvas_api.py` passes46 calls through the real application
  entrypoint. It exercises placement preview/Cancel, calibration/edit/history,
  visibility, source relocation and portable reopen, and authors a real sketch
  line over the independently generated image marks. The saved document contains
  one real sketch feature; no imported image becomes evaluated CAD geometry.
- `reference_canvas_controls_test` passes3/3 Qt test phases. Native typed width
  and35% opacity update real GPU preview; Cancel preserves blank authored state;
  OK authors the image. Two native viewport clicks select actual image-plane
  points, typed90mm previews calibration, native OK/undo/redo preserve exact
  authored history. One initial harness activation is separate; no focus or
  selection repairs occur between demonstrated command/input steps.
- Actual full-app/native captures inspected: image is upright, colored corner
  marks and text readable, Canvases folder and dock controls readable. API GPU
  diagnostics report hardware acceleration; camera fit includes the textured
  plane, including the narrowed calibration viewport.

Evidence remains external in
`../Alloy-reference-records/deliverables/local-reference-canvas-2026-10-14/`:
`core.log`, `api-06/commands.json`, `api-06/summary.json`,
`api-06/local-reference-traced.alloy`, API captures and
`native-03/test.log` plus native captures/state. Earlier failed runs retain
schema/session/camera/harness-lookup/calibration-fit diagnostics; final runs
qualify only the corrected candidate.

| Compatibility evidence | Result |
| --- | --- |
| Local raster, plane, position/angle, opacity, uniform scale, two-point calibration and commit/Cancel | Matched bounded documented workflow |
| Native dock and Browser controls, real plane texture and readable labels | Inspected Alloy captures; no exact style/metric match claim |
| Published expanded Canvas dialog flags/manipulators/nonuniform controls/flips | Outside bounded implementation; explicit public options remain |
| Current default opacity, exact scaling pivot and layout metrics | Unknown; Alloy choices disclosed |

Native file-picker typing, Browser context-menu gestures, arbitrary face/curve
support, native image-plane dragging and pixel-exact public-reference parity were
not established by these checks. Origin/calibration native inputs and retained
construction-plane edits have separate evidence. A congruent traced line and the
raster guide overlap visually; actual authored/sketch evaluation is separate
proof, not a claim that the capture alone establishes geometry. This does not
accept the complete surface-iron tutorial or a creation movie.

Independent Astra review identified a disconnected Browser visibility handler and
stale calibration validity after an extra point. Both are corrected: Browser
row click/Space toggles authored visibility; a completed pair refuses further
picks without changing the session; incomplete calibration cannot commit. The
focused native-04 run passes 3/3 phases including these assertions, without
inter-step focus repair. The completed-pair hint explains Cancel/restart. Final
independent acceptance and integration remain pending.
See [the bounded source contract](../research/local-reference-canvas-contract.md).
