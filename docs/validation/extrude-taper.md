# Polygon solid Extrude taper

SOL-081/#619 bounded candidate, 2026-10-19. [Command contract](../research/commands/extrude-taper.md).
Retained `taperAngleDeg` uses negative inward/positive outward degrees, default
zero, finite domain strictly between −90° and +90°. Nonzero supports one straight
polygon without holes, positive Distance, One Side, Profile Plane and New Body.
Zero preserves the previous prism paths. OCCT draft failures refuse atomically.

Preview, direct API and native edit share the authoritative feature evaluator.
The mapped far cap retains its existing feature-derived identity, current actual
boundary descriptors and support frame. No tapered side-face/solid-edge lineage
is asserted. `analyticVolumeMm3=area*distance` is restricted to zero taper;
nonzero volume comes from the actual evaluated B-rep.

Evidence root: `../Alloy-reference-records/validation/sol081-taper-2026-10-19/`.

- **Build:** accepted `build-preview-release`, Release AppleClang17 arm64,
  Qt6.11.2, OCCT7.9.3; app, `extrude_taper_test` and
  `extrude_taper_controls_test` built with `-j4`. `build.log` and
  `core-build.log` and `build-hint.log` retain results. No CI/tag/push.
- **Core:** `extrude_taper`, prior `extrude_taper_kernel`,
  `extrude_symmetric_two_sides` and `extrude_offset_start` pass. Exact signed
  volume, stable IDs/cap support, unsupported circular profile and invalid/collapsed
  angle atomicity, history and authored reopen; `core.log`, `prior-core.log`.
- **Real entrypoint/API:** `python3 tests/extrude_taper_api.py --app
  build-preview-release/alloy.app/Contents/MacOS/alloy --output <root>/api`
  passes convex 40×20×10 at 0/−5/+5°, concave L×8 at −5° with an edit to −3°,
  shared preview/Cancel, retained edit and omitted-angle preservation. A positive
  cap circle and its 2mm New Body solid recompute successfully after the source
  taper changes −5→−3°, with unchanged downstream authored support/feature IDs.
  Failed −80° collapse and ±90° domain edits preserve the last valid document;
  Undo/Redo and exact authored save/reopen pass. `api/receipt.json` records calls.
- **Interchange:** independent binary STL triangle bounds/volume checks pass
  rectangle −5° (7485.2736mm³), +5° (8535.1378mm³), zero (8000mm³),
  concave −5° and edited −3°. Saved models and mesh artifacts remain external.
- **Native:** actual-hit numeric line-edit child entry, −5° preview/Cancel,
  +5°/OK, retained +5° edit → −80° refusal → −3°/OK pass; 3 Qt slots,
  2.304 seconds (`native/controls.log`). Initial activation only, no inter-step
  focus repair. Native captures use Top view and qualify the fields/buttons;
  separate oblique API captures qualify the tapered solid presentation.
- **GPU/visual:** Apple M4, Apple vendor, OpenGL4.1 Metal90.5, hardware
  acceleration, 4× MSAA, 2× resolution and native framebuffer details recorded
  in the API receipt. Actual inward/concave oblique previews and native edit
  capture inspected: readable degree field/OK/Cancel and smooth translucent
  tapered solids. The final native hint states signed taper direction in two
  fully visible lines above the footer. All private validation editors exited.

The initial API probe exposed a missing numeric schema range and stopped before
nonzero taper; explicit −90..90 schema bounds now accompany the shared strict
endpoint validation. This was corrected before the passing final evidence.

Independent compatibility review is required before acceptance. The public-source evidence is external in
`knowledge/specs/sol081-taper-extrude-2026-10-19.md`; no matched screenshot exists,
so precise layout/pixel parity is unknown.

Thin walls, curved/holed/multiple profiles, negative distances, alternate starts,
extents, operations and symmetric/two-side taper remain under SOL-010. Physical
OS input, Windows/Linux and packaged runtime qualification remain separate.
