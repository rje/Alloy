# Thin solid Extrude from one open Line

SOL-082/#620 bounded Mac candidate, 2026-10-19.
[Command contract](../research/commands/extrude-thin.md). Public-source evidence is
external in `knowledge/specs/sol082-thin-extrude-2026-10-19.md`, R1–R8.
Factory defaults and precise visual parity remain unknown.

The authored Extrude retains exclusive `sourceEntityId`, `wallThicknessMm` and
`wallLocation` alongside source sketch and distance. A closed strip is derived
from the current solved Line solely for evaluation; no synthetic sketch/profile
is authored or serialized. Both preview and commit reuse that profile adapter
and the same prism evaluator. The supported combination is New Body, One Side,
Profile Plane, positive Distance and zero taper. Side 1 follows sketch normal ×
directed tangent (+Y for a +X Line on XY), Side 2 the opposite, Center half each
side. That mapping is an Alloy contract, not reference-documented orientation.

Evidence root: `../Alloy-reference-records/validation/sol082-thin-extrude-2026-10-19/`.

- **Build:** `build-preview-release`, Release arm64, AppleClang17, Qt6.11.2,
  OCCT7.9.3, `-j4` maximum. No CI, tag, push or external publication.
- **Core:** `extrude_thin_test` passes the 40 mm directed XY Line and all three
  placements. Center thickness 4 and distance 20 gives bounds
  (0,−2,0)..(40,2,20), volume 3200 mm³. Side 1 gives y=0..4; Side 2 y=−4..0.
  Thickness 6 gives 4800 mm³. Zero/negative/null thickness, invalid location,
  taper/symmetric/negative-distance combinations refuse atomically. A far-cap
  circle and inward −0.5 mm Cut remove π/2 mm³ and recompute through thickness
  editing with the same supported cap. Undo/Redo and authored reopen pass.
  Existing taper and symmetric/two-side core regressions pass separately.
- **Real entrypoint/API:** `tests/extrude_thin_api.py` passes 79 calls: all three
  disposable previews and Cancel; one retained commit; creation Undo/Redo;
  cap-supported radius-1 circle and Cut; thickness 4→6→4 with unchanged feature,
  source and cap IDs; edit Undo/Redo; save/reopen; explicit invalid-input
  refusals. `api/receipt.json` retains commands, outputs and GPU information.
- **Interchange:** independent binary STL parsing checks triangle bounds and
  volume (0.02 mm³ tolerance). The rectangular wall is 12 triangles / 3200 mm³;
  edited cut is 588 triangles / 4798.429701877 mm³, restored cut
  3198.429701877 mm³. Corresponding models and meshes remain external.
- **Native:** actual-hit Type popup switching from Symmetric Whole Length to
  Thin normalizes supported settings; actual canvas hit selects the open Line.
  Actual numeric child entry and Wall Location popup produce Center 4 preview;
  Cancel preserves the document; native OK commits; edit thickness 6/Side 1
  preserves feature/source IDs. Only initial harness activation is applied;
  no focus or selection repair between demonstrated steps. Final native Thin
  passes 3 Qt slots in 2.481 seconds; neighboring taper and symmetric/two-side
  actual-hit regressions pass 2/2 (5.44 seconds). The source is also
  selectable in the native Profiles dropdown.
- **GPU/visual:** Apple M4 / Apple / OpenGL4.1 Metal90.5, hardware acceleration,
  4× MSAA and 2× resolution/native framebuffer data are in the API receipt.
  Native panel and oblique Center preview captures were inspected: readable
  Type/Thickness/Location/Operation controls, declared side mapping, smooth
  translucent closed-wall preview and distance arrow. Native scale is 2×;
  no matched public viewport/theme/scale reference exists. Hidden form rows
  collapse without reserved gaps; Operation, helper and footer fit in the
  inspected full native dock without scrolling.

The fixture is intentionally an ordinary underconstrained open Line (4 DOF),
not the research recipe's fully constrained variant. Its solved coordinates
are fixed for this bounded evaluator test; this does not qualify a constraint
creation workflow. Only the far planar cap is published as qualified support;
no thin side-face, solid-edge or synthetic cap-boundary lineage is asserted.

Native Type switching originally retained unsupported ordinary settings and
would have trapped the user if controls were disabled. The final mode change
normalizes conflicting inputs through the shared command path; an existing
modifying feature cannot change its operation. Native verification includes
this specific transition. Independent review and final text-only capture
verification are recorded by the integrator before acceptance. The Thin and
symmetric native hit helpers now observe that the requested widget receives
the real hit after asynchronous form reflow before delivering a pointer event;
they do not alter focus, selection, scroll position or geometry.

Chains, arcs, closed-profile thin walls, broader start/extent/direction/operation
combinations, taper and side-wall lineage remain under SOL-010. Physical OS
input, Windows/Linux, other display scales and packaged runtime remain separate.
