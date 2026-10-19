# Editable Counterbore Hole — SOL-079/#609

Independently accepted bounded slice, 2026-10-19, extending the accepted [Simple Hole](simple-hole.md)
and [Countersink](countersink-hole-controls.md) shared retained point/target path.
Public basis: `hole` documentation, retrieved 2026-10-19, and
`lafavre-counterbore-2017` original public tutorial page8, retrieved 2026-10-13.
URLs/provenance, observed stepped cavity/field labels, explicit defaults policy
and unknowns remain external in `knowledge/specs/sol079-counterbore-2026-10-19.md`.

## Bounded contract

Create → Hole accepts one earlier standalone Point on a retained planar face
sketch and an explicit same-body Objects to Cut target. Hole Type → Counterbore
reveals Counterbore Depth and Counterbore Diameter, alongside Bore Diameter,
Distance/All, fixed Simple tap and Flat drill point. Recess diameter must exceed
bore diameter; recess depth must be positive and smaller than blind Distance.
All recomputes bore reach from current target bounds. Recess dimensions 10/3 mm
are initial Alloy policy, with no public-default claim. All does not enforce a
screw-fit or remaining-stock recess-depth constraint.

UI/API share `CadDocument::holeCandidate`, `SolidHole::cutter` and evaluated
history. The actual result and red removed material preview before OK; Cancel
or Escape discards the session. Timeline edit keeps feature/point/target IDs.
`solid.hole`/`solid.holeDialog` retain `holeType: counterbore`,
`recessDiameterMm`, `recessDepthMm`, `boreDiameterMm`, supported point/target IDs
and `extent: distance` plus `distanceMm`, or `extent: all` without stale depth.
Inactive countersink fields are omitted. Older Simple/Countersink schema and
Countersink 4/10/90 defaults remain unchanged. Counterbore Pattern refuses
explicitly through the existing Countersink-only Pattern authority.

## Verification

Accepted build directory `build-preview-release`: Release, AppleClang17,
macOS arm64, Qt6.11.2, OCCT7.9.3, `-j4`. No CI dispatch.
Artifacts: `../Alloy-reference-records/validation/counterbore-hole-2026-10-19/`.

- Core Counterbore and legacy Simple/Countersink pass 3/3 (1.02 s). Direct
  BRep solid-classification probes check recess radius/depth2 and bore depth5
  material intervals independently of authored fields and volume. Analytic removed
  volume: bore4/recess10×2/Distance5 = 62π mm³; bore6/Distance7 = 95π;
  All at thickness10 = 122π, then thickness12 = 140π. Width75→85 moves the
  edge-associated point to x−32.5. Exact undo/redo/reopen and invalid diameter,
  zero recess depth, recess depth equal to Distance, and lost-point refusals pass.
- Real entrypoint `tests/counterbore_hole_api.py` passes 83 calls: actual
  preview/Cancel, create, atomic invalid inputs, edit all three dimensions and
  Distance, reassign point, switch All, change target thickness10→12 and source
  width75→85, downstream retained outer Chamfer, earlier bore edit,
  exact Undo/Redo and save/reopen. `api/report.json`, transcript and authored
  model retain state; final BRep volume50193.9251116 mm³.
- Independent standard-library triangle readback checks finite aligned normals,
  paired oriented edges, one closed component, bounds and positive volume.
  Blind STL:1740 triangles, Euler2, volume36832.9031651 versus
  BRep36833.0088213 mm³, bounds ±37.5/±25/0..10. All+Chamfer STL:2186 triangles,
  Euler0, volume50193.7829725 mm³, bounds ±42.5/±25/0..12. Both agree within
  0.001 relative tolerance. `api/meshes.json` retains the independent result.
- Native `counterbore_hole_controls_test` uses sole initial activation, actual
  point/target viewport clicks, queued Create menu activation/closure,
  focused native combo popup selection/closure, actual-hit numeric editors
  (`QApplication::widgetAt`), typed bore/recess diameter/recess depth/Distance,
  exact Cancel, fresh Point selection, All commit and actual Timeline edit.
  No inter-step focus/selection repair. Input is Qt-delivered, not physical-device.
  `native.log` records 3/3 passing (5.920 s); focus observations, preview/final state and GPU captures retain
  the route. The final native and API captures are inspected for readable labels,
  smooth red stepped entry and committed cavity. Graphics report Apple / Apple M4 /
  OpenGL4.1 Metal90.5, native framebuffer and ResolutionRatio2.

Public screenshots differ in viewport/theme/scale and show a historical angled
drill point; visual equivalence beyond readable field semantics and stepped
cavity remains unknown. Direct face placement, multiple points, angled tip,
reverse, To, taps, manipulators, cross-body support and Counterbore Pattern
remain outside this slice. SOL-007/#52 stays open. Physical input,
Windows/Linux GPU runtime and packages remain separate gates.

Independent Sol review checked final focused BRep step/depth probes, shared command source, 83-call real-app transcript, native focus/captures, exact saved authored state and independent STL; no blocker found.
