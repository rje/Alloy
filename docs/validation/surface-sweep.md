# Open-line Surface Sweep — SRF-026 / #629

The accepted bounded core sweeps one retained ordinary open sketch Line along
one retained ordinary analytic Line or Arc path. It creates a Full/100%,
Perpendicular, New Body **surface sheet** with no caps or solid volume. The
feature stores both sketch/entity references and an exact path-start point;
edits recompute from solved sketch geometry. A basic native selector dock
compiles, but public canvas Profile/Path picking remains SRF-038.

## Evidence

- Implementation `eead92f`, edit dispatch correction `1e90a2b`, signed Arc
  orientation correction `a73e03d`. Release application and focused
  `surface_sweep_test` built locally at `-j4`; the focused test passes Line and
  ±90° Arc geometry plus retained source/edit/refusal/history cases.
- Independent 194-response real-app fixture checks one valid open face, four
  boundary edges, zero solids and null volume for each path. Line section
  height 10 mm along 40 mm line yields 400 mm² and bounds
  [0,0,-5]..[40,0,5] mm. The same height along a radius-20 quarter Arc yields
  100π mm² and bounds [0,0,-5]..[20,20,5] mm. Actual B-rep boundary endpoints
  meet the start/end section lines at G0 position.
- Section height 10→14 and line reach 40→50 recompute to 700 mm²; Arc sweep
  90→60° recomputes to 280π/3 mm². Source/path/start IDs and body/face/edge
  identities remain stable on supported edits, Undo/Redo and saved reopen.
  Invalid session and direct edits refuse atomically, permit valid retry and
  leave authored/evaluated/revision/Undo state unchanged.
- Four actual Qt/OCCT framebuffer captures on Apple M4 hardware OpenGL 4.1,
  DPR 2, show readable controls and continuous uncapped planar/curved sheets.
  The home isometric camera views part of the quarter-cylinder edge-on: its
  light/dark bands meet along the curved strip, but this is a poor angle for
  presenting its full shape. No matched public screenshot claim is made.
- The first real-app run exposed `feature.editDialog` dispatching Surface
  Sweep to Surface Revolve. `1e90a2b` corrected it; the final fixture checks
  the reopened edit route. The verifier's own Arc Fix-argument error was
  corrected in its fixture before the complete rerun.

The fixture is `tests/surface_sweep_api.py`. The independent report, both
saved models, transcripts, failed-run traces and captures are outside git at
`../Alloy-reference-records/validation/surface-sweep-2026-10-20/`. Public
source provenance and uncertainties are external at
`knowledge/specs/srf026-open-section-surface-sweep-2026-10-20.md`. No
proprietary runtime was operated.

## Remaining acceptance

The Mac console is locked. Actual-hit native canvas Profile/Path selection,
physical input, 1×/2× interactive controls and matched reference visual
comparison are **not accepted**. SRF-038 owns that first-v1 workflow. Guide
rails/surfaces, multiple sections or path entities, partial extents, alternate
orientation and higher-order continuity remain SRF-002. Windows/Linux
packaged runtime remains a release gate. A passing API fixture is bounded
core evidence, not complete Surface Sweep parity.
