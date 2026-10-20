# Open-line Surface Revolve — SRF-025 / #628

This accepted bounded core revolves one ordinary open sketch line around a
distinct retained coplanar construction-line axis. The result is a New Body
**surface sheet**, with no implicit closure or caps. It retains sketch,
section and axis identities, Partial One Side angle and Reverse direction;
90° forward is an Alloy default, not a documented reference default.

The current native Surface Create dock has sketch/profile/axis combo selectors
and typed angle/Reverse controls. Public help also describes canvas selection
and an angle manipulator. Those controls are unfinished and tracked under
SRF-036. Joined-Line sections are accepted under [SRF-037](surface-revolve-chain.md);
Line/Arc sections remain later SRF-055, and wider axis classes and extent modes remain SRF-001.

## Evidence

- Implementation `9f6ec7d` with numeric-schema correction `860f3c6` built
  locally in Release at `-j4` with Qt 6.11.2 and OCCT 7.9.3. Focused
  `surface_revolve_test` and unchanged `solid_revolve_test` and
  `surface_extrude_test` pass.
- An independent 66-response real-application fixture creates an XZ sketch
  line at radius 20 mm, height 20 mm, and a distinct construction axis. The
  90° sheet has exact area 200π mm², one face/four boundary edges, zero solids,
  null volume and bounds [0,0,-10]..[20,20,10] mm. The 45° Reverse edit gives
  100π mm² in the opposite Y quadrant; extending the source line height to
  30 mm gives 150π mm². Body, face, boundary and source IDs remain retained.
- Preview/Cancel, a crossing-line invalid draft with same-session retry,
  missing/crossing axis and 0°/360° refusal, angle/direction and source edits,
  Undo/Redo, save/new/reopen and visible dialog input inspection pass. Invalid
  attempts preserve authored state, revision, Undo and evaluation.
- Two actual OCCT/Qt framebuffer captures at DPR 2 (Apple M4 GPU, OpenGL 4.1
  Metal layer, 4× MSAA) were inspected: smooth coherent curved open sheet,
  visible boundaries and no apparent caps. This verifies the current GPU
  prototype for this fixture, not native RHI or matched reference styling.
- The first real-app attempt found that the new angle schema rejected the
  default 90° before evaluation. `860f3c6` added explicit draft bounds;
  the complete corrected fixture passed. Source review also found and fixed
  omitted active-dialog input inspection before that run.

The fixture is `tests/surface_revolve_api.py`. The independent report, model,
transcript, failure trace and images remain outside git under
`../Alloy-reference-records/validation/surface-revolve-2026-10-20/`. Public
source provenance and documented/unknown behavior are separately recorded at
`knowledge/specs/srf025-open-section-surface-revolve-2026-10-20.md` in the
external knowledge tree. No proprietary runtime was operated.

## Remaining acceptance

The Mac console is locked. No actual-hit native selection, angular drag,
physical input, 1× capture or matched public visual comparison is accepted.
SRF-036 is the first-v1 owner for those controls and checks. The joined-Line section
minimum is accepted in [SRF-037](surface-revolve-chain.md); Line/Arc continuation
is later SRF-055. Origin/model axes, Full/Two Sides/Symmetric extents and surface
combinations remain SRF-001. Windows/Linux packaged runtime remains
under release gates. This core/API evidence does not establish complete user
workflow or exact interface parity.
