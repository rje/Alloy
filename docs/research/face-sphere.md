# Face-positioned Sphere contract

Bounded SOL-014/#145 candidate, 2026-10-13. Public evidence is recorded externally
in `knowledge/tutorials/warwick/library-die-ledger.md` under source
`tutorial-library-die`; original URLs and public images stay outside the repo.

Select an actual retained planar face, then place a center on its trimmed area.
Authored parameters store the exact body/producer/face triple, face-local u/v,
diameter and Cut or New Body. Cut targets the selected face's body and retains
its identity. Preview is disposable; Cancel leaves authored history untouched.
A missing/split face, placement in an opening or ineffective cut is an explicit
error. No nearest-face substitution or cylindrical recess is permitted.

The source demonstrates automatic Cut for intersecting placement. Our face-based
command therefore defaults to Cut. Repeat Sphere binds the newly selected face,
clears its center and retains the last accepted diameter. Diameter carry-over is
an explicit Alloy choice; public documentation does not establish that default.
The existing owner authority must keep the operation inside the active component.

The deterministic fixture uses a centered20mm cube extruded from z0 to20, then
1mm rounding. Diameter3 pips use counts +Z1/-Z6,+X2/-X5,+Y3/-Y4, symmetric offsets
0 or±5mm around each face center. These numbers are fixture choices, not source
instructions. Verify21 authored cuts, all6 original planar face IDs, volume loss
47.25π from the rounded baseline, upstream rounding edit, history and reopen.
Do not count spherical topology patches as pips; kernel seams can split them.

Core Sphere passes its focused six-face/21-cut checks. Shared command/UI and
Repeat now have a compiled candidate; independent real-app/native qualification
and complete die/component/STL/movie acceptance remain outstanding. Exact pixels, view-cube
interaction and physical OS input are not established by core tests.
