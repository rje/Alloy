# SKT-105: retained second Polygon driver

Accepted 2026-10-17 for the bounded second-placement relation in all three
Polygon modes. The public snap and input evidence is recorded externally at
`../Alloy-reference-records/knowledge/specs/skt-093-polygons-2026-10-17.md`.
Inscribed retains a radial vertex source, Circumscribed retains an edge-midpoint
source, and Edge retains its second base endpoint source. Circumscribed can
also acquire a retained straight-line midpoint. The two actual source points
own size and orientation; a conflicting typed size or angle is refused
without dropping either relation or changing authored state.

The authored group retains distinct stable anchor and second-driver points
with real Coincident/Midpoint relations. Each polygon vertex is an affine
combination of the solved two points within PlaneGCS, with separate residual
checks. Count and side edits preserve surviving boundary IDs and both source
identities. Existing one-anchor and legacy fixed Polygon groups remain
readable. Second-driven canvas sizes use the reference-value convention
`(R12 mm)` or `(12 mm)` and report `sizeOwner: secondDriver`; the panel explains
that the source owns size and angle. Native acquired cues appear only for
eligible targets compatible with typed locks.

Evidence from Release `build-preview-release`, binary SHA-256
`edcfc884b59875551855f5b109bf642c4fb7137feea3d2e6a6b267d0506018b8`:

- Six existing and new focused source/native tests passed in the first final
  seven-test run; the last native fixture initially chose an ambiguous target.
  After correcting that fixture to an isolated authored point, its focused
  test passed in 5.64 s. Together these cover all three second-driver modes,
  upstream scale/rotation, a Circumscribed midpoint, count/surviving external
  relation behavior, incompatible numeric refusal, distinct-driver refusal,
  native cue/clicks, typed-lock suppression, annotation ownership, Undo/Redo
  and saved reopen. No broad test rerun followed the fixture-only correction.
- The real-app workflow in
  `../Alloy-reference-records/validation/skt105-polygon-two-driver-final/`
  snapped an Inscribed hexagon to the origin and an authored radial point,
  extruded it 5 mm, then moved that point from (10,0) to (0,12) mm. The same
  body ID survived; its size and orientation became 12 mm and 90°, and volume
  changed from 1299.0381 to 1870.6149 mm³. A conflicting 11 mm Polygon size
  update refused with the exact active draft unchanged. Undo/redo and saved
  reopen passed. Independent binary STL readback found one closed 20-triangle
  component, Euler characteristic 2, bounds (-10.3923,-12,0) to
  (10.3923,12,5) mm. The saved model, export, actual acquired-cue/preview
  capture, rotated sketch and solid captures, and command transcript are
  beside the receipt.
- The app reported Apple M4 hardware acceleration, OCCT OpenGL and a
  Metal-backed OpenGL 4.1 driver in its 2× framebuffer. A separate Sol review
  inspected the affine equations, native cue routing, source validation,
  authored state and final captures; no material blocker remained.

The functional second-pick relation and source-driven size behavior are
supported by the public instructions; exact current styling, field focus and
constraint-glyph parity remain unknown. Physical hand input and Windows/Linux
runtime/GPU qualification remain open. This is a bounded retained Polygon
workflow, not a claim of full sketch-tool parity.
