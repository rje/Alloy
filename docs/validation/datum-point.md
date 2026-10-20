# Retained datum Point — GEO-027 / #641

Alloy now creates one editable construction Point from either an exact semantic
Box edge endpoint or an explicit location on an earlier authored straight
sketch Line. The Line supports Proportional `[0,1]` and Physical in-segment
distance modes. Authored source identity and semantic location remain separate
from the evaluated world position; source edits recompute the point or refuse
atomically. The point creates no body or exported mesh.

## Evidence

- Implementation `5b12f97` passed the accepted Release `-j4` build, focused
  C++ analytic/source/history/refusal/reopen checks, and three compiled Qt
  dock tests covering actual widget hits, typed value, mode conversion,
  preview/Cancel/OK, edit restoration and Escape. Qt-delivered input is
  distinct from physical OS input.
- Independent fixture `tests/datum_point_api.py` (`c572303`) passed 186
  real-app calls. A Box endpoint followed width 20→40 mm with exact retained
  identity. On a Line changing from `(35,0)`→`(47,16)` (20 mm) to
  `(35,0)`→`(59,32)` (40 mm), ratio 0.25 moved the point from `(38,4)` to
  `(41,8)`, while Physical 5 mm stayed at `(38,4)`. Exact endpoints,
  same-feature edits, source deletion/shortening, stale/wrong/curved/chain and
  out-of-range refusal, preview/Cancel, visibility, Undo/Redo and save/reopen
  passed. Independent STL readback contained only the resized Box: 12
  triangles, bounds 40×30×15 mm and volume 18,000 mm³.
- Inspected Apple M4 hardware OCCT OpenGL 4.1 / DPR2 / 4× MSAA captures show
  visible blue Plus preview and orange retained markers, readable methods,
  units, values and buttons, and valid resized presentation. With the dock
  open the framebuffer was 1440×1216; reopened full viewport 2056×1216;
  resized viewport 2296×1316. The private editor exited normally.

The permitted public-source observations and unknowns are outside the
repository at `../Alloy-reference-records/knowledge/specs/geo027-datum-point-2026-10-20.md`.
Independent review, transcript, analytic results, saved model, STL files and
captures are under `../Alloy-reference-records/validation/datum-point-2026-10-20/`.
No proprietary runtime or CI was used.

## Remaining acceptance

Physical Box vertex/Line actual-hit picking, canvas distance handle, focus
and 1×/2× native visual qualification remain first-v1 Todo GEO-035/#685 while
the Mac console is locked. Curved/connected paths are GEO-036/#686; other
vertices and snap sources GEO-037/#687. Public help and API text conflict on
past-end placement; GEO-038/#688 owns that resolution and implementation.
Other point constructors stay GEO-001/#56. The authored start→end direction,
new-session ratio 0.5, position-preserving mode conversion and finite-segment
limit are Alloy policies; exact public glyph/default/layout parity and
Windows/Linux runtime remain unknown.
