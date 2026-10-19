# Thin solid Extrude — one open Line

SOL-082/#620 bounded implementation contract, 2026-10-19. Public evidence and
provenance: external `knowledge/specs/sol082-thin-extrude-2026-10-19.md`, R1–R8.
Reference defaults and exact styling are unknown; fixture values are explicit.

- Route: Solid > Create > Extrude, Type **Thin Extrude**, select one open straight
  nonconstruction Line in canvas or Profiles. Connected-chain selection, arcs,
  closed curves and multiple curves are outside this slice. Selecting one Line
  does not imply whole-chain support.
- Settings: New Body, One Side, Profile Plane, positive Distance, zero taper.
  Wall Thickness is positive, canonical 0.001–100000 mm. Wall Location offers
  Side 1, Side 2 and Center. The Alloy initial wall is 1 active unit and Side 1;
  this is a local choice, not a reference-default claim. Switching Type to Thin
  normalizes conflicting ordinary settings; an existing modifying feature
  cannot migrate its operation to New Body.
- Side 1 is sketch normal cross directed authored Line tangent, Side 2 its
  opposite, Center half on each side. Thus +X in XY maps Side 1 to +Y. This is
  Alloy's declared mapping; public evidence does not establish the exact rule.
- Shared API: `solid.extrude` / `solid.extrudeDialog` take exclusive
  `sketchId` + `sourceEntityId`, `wallThickness` in `units`, and `wallLocation`
  (`side1`, `side2`, `center`) alongside ordinary distance settings. The native
  controls and canvas picks update the same command session. `command.preview`,
  `command.commit`, `command.cancel` retain existing completion semantics.
- Authored feature: `sourceEntityId`, `wallThicknessMm`, `wallLocation`, source
  sketch and distance. A closed strip is derived transiently from solved Line
  endpoints, then passed to the existing solid prism evaluator for both preview
  and commit. It is never authored or serialized as a second sketch/profile.
- Preview changes thickness and side placement without a history change. OK
  creates one Extrude; Cancel discards the draft. Edit reopens retained inputs;
  Undo/Redo and save/reopen retain the original source/feature IDs. Escape follows
  Alloy's existing command cancellation policy, whose reference parity is unknown.
- Only the far planar cap is qualified for downstream support, with the existing
  feature-derived cap ID. Thin side faces, cap boundary projections and solid
  edges have no qualified lineage in this slice. No strip edge is represented
  as an authored source edge.
- Invalid thickness/source/topology, unsupported option combinations and failed
  geometry refuse atomically, retaining the last committed document.

Verification: XY Line (0,0)→(40,0), Distance 20, Center 4 gives one valid closed
solid, bounds (0,−2,0)..(40,2,20), volume 3200 mm³. Side 1 gives y=0..4; Side 2
gives y=−4..0. Center thickness 6 gives 4800 mm³. A radius-1 far-cap circle and
−0.5 mm Cut removes π/2 mm³, follows the same cap through thickness edits, Undo
and reopen. Native actual-hit tests and independent STL readback supplement
core/API geometry. See [validation](../../validation/extrude-thin.md) for actual
results and remaining qualifications.
