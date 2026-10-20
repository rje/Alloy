# Retained datum Axis — GEO-026 / #640

Alloy now creates an editable datum Axis from either two earlier authored
standalone sketch Points or one earlier authored straight sketch Line. Exact
point or ordered Line IDs are retained; solved source edits recompute the
axis, and stale, coincident, wrong-class or changed-order references refuse
without rebinding. The evaluated axis is an infinite reference line with
origin, direction and source span. Its finite GPU line extends 15 mm beyond
the sources as an explicit Alloy display policy; it creates no body.

## Evidence

- Implementation `e396340` passed the accepted Release `-j4` build, focused
  C++ analytic/source/history/visibility/refusal/reopen checks, and three
  compiled Qt dock tests for actual widget hits, both constructors,
  preview/Cancel/OK and exact edit restoration. Qt-delivered input is separate
  from physical OS input.
- Independent fixture `tests/datum_axis_api.py` (`06aca80`) passed 133
  real-app responses (126 primary calls and seven completion polls). Points
  `(4,3,7)` and `(16,12,7)` give unit direction `(0.8,0.6,0)` and 15 mm
  span; a retained Line from `(-10,-10,7)` to `(-10,10,7)` gives `(0,1,0)`
  and 20 mm. Independent checks cover source/order edits, seven atomic
  direct refusals, source-removal/coincidence refusal, Undo/Redo, visibility,
  authored identities and measured save/reopen.
- Inspected Apple M4 hardware OCCT OpenGL 4.1 / DPR2 / 4× MSAA captures show
  readable Type, source and result controls, oblique preview/committed axes,
  hidden-axis state and valid resize. Reopened framebuffer was 2056×1216;
  resized framebuffer was 2296×1316. The private validation editor closed.

Public-source observations and unknowns are recorded outside the repository at
`../Alloy-reference-records/knowledge/specs/geo026-datum-axis-2026-10-20.md`.
The independent review, transcript, measured results, saved model and captures
are under `../Alloy-reference-records/validation/datum-axis-2026-10-20/`.
No proprietary runtime or CI was used.

## Remaining acceptance

The blue preview Axis merges with its blue source Line; the committed orange
Axis partly covers that Line, so selected-source feedback is not yet distinct.
Physical actual-hit source picking, this cue, and 1×/2× native visual
qualification remain first-v1 Todo GEO-031/#681 while the Mac console is
locked. Retained model edges are GEO-032/#682; documented Extent Type and
Resize controls GEO-033/#683; vertices/snap and other point classes
GEO-034/#684. Other Axis methods remain GEO-001/#56. Positive direction and
15 mm display extension are Alloy policies; exact public color, pattern,
default and layout parity and Windows/Linux runtime remain unknown.
