# Editable Simple Hole — SOL-078/#608

Independently accepted bounded candidate, 2026-10-19. Extends the accepted
[Countersink ingredient](countersink-hole-controls.md), using the same retained
point, target, command session and OCCT cut authority.

Public basis: source `hole`, retrieved 2026-10-19, plus the previously accepted
`plate-countersink-pattern-20261013` point-placement demonstration. URLs,
creator/provenance and compact public-versus-Alloy contract remain external in
`../Alloy-reference-records/knowledge/specs/sol078-simple-hole-2026-10-19.md`.
Public documentation establishes From Sketch, Simple type, Distance/All, Flat
drill point, Objects to Cut and point preselection. Launch defaults, exact
keyboard sequence, visual scale/theme and field spacing remain unknown.

## Workflow contract

Create → Hole opens the accepted From Sketch workflow. Preselect one visible
standalone Point, or use Select Point and pick its actual marker. It must belong
to an earlier face-supported sketch on the explicit target body. Objects to Cut /
Select Target picks one actual body and never silently replaces the point.
Choose Hole Type → Simple, Extent → Distance or All, and Bore Diameter.
Distance reveals Depth, measured inward from the retained face plane to a flat
cylindrical bottom. All recomputes reach from the current target bounds. Tap
Type Simple and Drill Point Flat are explicit fixed supported options.

The readable GPU preview displays the actual result and red removed material.
OK commits; Cancel/Escape discards the session exactly. Timeline edit reopens
the same Hole identity and point/target references; Select Point can reassign
placement to another supported retained standalone Point. Unsupported placement,
lost/off-face point, invalid diameter/depth and failed cuts refuse atomically.
UI/API use `CadDocument::holeCandidate`, `SolidHole::cutter` and the same
committed evaluation, rather than a separate automation model.

`solid.hole` / `solid.holeDialog` accept `holeType: simple`, `extent: distance`
with positive `distanceMm`, or `extent: all`. Both retain `boreDiameterMm` and
`sourceSketchId` / `sourceEntityId` / `sourcePointId` / `targetBodyId`.
Saved simple features omit inactive countersink dimensions and All omits stale
depth. Existing Countersink/All documents and defaults remain compatible:
4 mm bore, 10 mm sink, 90° angle. Initial native Distance 5 mm is an Alloy
policy, not a reference default. Countersink remains All-only; unsupported
simple-hole Pattern seeds refuse explicitly instead of using a conical cutter.

## Evidence

Accepted build directory `build-preview-release`: Release, AppleClang 17,
macOS arm64, Qt 6.11.2, OCCT 7.9.3; build `-j4`, no CI dispatched.
External artifacts: `../Alloy-reference-records/validation/simple-hole-2026-10-19/`.

- Core `simple_hole` and existing `countersink_hole` pass (2/2, 1.24 s).
  Blind Ø4 × 5 removes 20π mm³; Ø6 × 7 removes 63π; All at thickness10
  removes 90π, then thickness12 removes 108π. Width75→85 follows the retained
  10 mm edge-associated point to x−32.5. Exact history/reopen and atomic invalid
  dimensions/lost-point refusals pass. `core.log` records the run.
- Real application entrypoint `tests/simple_hole_api.py` passes 73 API calls.
  Preview/Cancel preserves authored stock exactly; blind creation, explicit
  retained-point reassignment, diameter/depth/All edits, upstream width follow,
  downstream outer-edge Chamfer, later Hole diameter edit, exact Undo/Redo and
  save/reopen pass. `api/report.json`, `transcript.json`, saved model and both
  STL exports retain the actual commands/state. Final BRep volume is
  42056.502531428705 mm³. API metadata for the Hole is checked before downstream
  Chamfer replaces the final body summary; saved feature intent remains checked.
- Independent standard-library triangle readback (`tests/stl_export_api.py`)
  checks finite aligned normals, positive winding, every oriented edge paired
  once, one connected closed component and expected bounds. Blind Ø6×7 STL:
  1164 triangles, Euler2, volume37087.2914333 mm³ versus BRep37087.4778262.
  Final All + Chamfer STL:1610 triangles, Euler0, volume42056.3081597 mm³.
  Bounds respectively ±37.5/±25/0..10 and ±42.5/±25/0..10 mm.
  Both volumes agree within 0.001 relative tolerance. `api/meshes.json` stores
  this independent readback, performed separately after the successful API run;
  the checked-in runner now also performs those assertions directly.
- GPU API captures `api/blind-preview.png` and `api/final-hole.png` were inspected.
  The blind preview has readable supported controls, smooth red entry and gray
  stock. Reported graphics: Apple / Apple M4 / OpenGL4.1 Metal90.5,
  native framebuffer, ResolutionRatio2. Public images have unmatched viewport,
  scale/theme and state; no pixel-equivalence claim.

Native `simple_hole_controls_test` passes in 5.790 s. Shared API creates the
rounded plate/edge-associated Point fixture; the native sequence then uses its
sole initial activation, actual Point marker and Create menu, explicit target
body click, Simple/Distance popup selection, receiving numeric editors, exact
Cancel, fresh Point selection, Simple/All commit, actual Timeline button and
retained bore edit. Popup keyboard selection goes to the observed focused
popup view after native activation; closure is observed before the next input.
The queued Create menu click also observes native activation and closure.
There is no focus or selection repair between user steps. Native API captures
`native/native-hole-preview.png` and `native/native-edited-hole.png` were
inspected: complete readable controls, smooth red blind entry and actual
through cavity. The input is Qt-delivered, not physical-device input.
`native.log`, `native/native-focus.json` and preview/final states retain evidence.

Earlier harness attempts are retained externally: direct combo key injection
left a visible popup, stationary and hover row-click probes did not select a
Mac popup row. Those attempts are not acceptance evidence. The qualified
route uses the actual focused popup view's keyboard handler. Production code
was unchanged by this harness correction. The final run includes the
independent reviewer's additional Create-menu activation/closure assertions.
`fingerprints.json` and `source.diff` record the working candidate and binaries.
Private editors exited; no user instance was altered.

Independent Sol review accepted the bounded candidate after source, core/API
receipts, saved authored model, independent mesh readback and final native/GPU
capture inspection. No reviewer build or application launch was needed. The
review requested the additional queued Create-menu activation/closure checks,
which are included in the final passing native result.

Physical-device input, direct face-center placement,
multiple points, angled drill tip, reverse, To, counterbore/taps, manipulators,
general cross-body placement and simple-hole Pattern remain unqualified or
unsupported. Wider Hole-family scope stays SOL-007/#52. Windows/Linux runtime,
packaging and print fit remain separate qualification gates.
