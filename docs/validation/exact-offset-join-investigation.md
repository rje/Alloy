# Exact-offset array Join investigation

SOL-039/#265, 2026-10-13. Headless diagnostic findings only; no production
correction or cumulative acceptance is claimed. #261 and #263 remain required
inputs for retained-command reproduction.

## Exact fixture and authority

External `validation/ornament-feasibility/authoritative-profile.cpp` builds the
80 mm / 5 mm / R50 revolved source and inward Shell 5 mm. Candidate #261 actual
SketchRegions evaluates an open fitted spline through learner-chosen points
(-4,-1), (6,19), (-5,45), (3,89), retained signed Offset -5 mm and two explicit
Line connectors. GeometryKernel extrusion and Intersect produce one valid
positive-volume member. Eight rigid rotations at 45° increments and the source
Ø10 × 5 mm hub at z=-5..0 fail their cumulative Join. These fit points are
fixture choices, not source-prescribed dimensions. Source alias:
ornament-pattern-20261013; public evidence remains external.

Diagnostic linkage uses the built candidate #261 kernel/solver and its current
CadDocument object, with Qt 6.11.2 and OCCT 7.9.3 on macOS arm64. This extends
the authoritative profile fixture; the shell and array remain external kernel
construction rather than retained application commands.

## Material findings

- The exact profile face, prism, Intersect member, hub and first-member/hub Join
  pass BRepCheck and OCCT Boolean argument checks including self-interference.
- The profile/prism maximum edge tolerance is 5.19e-7 mm. The Intersect member
  reaches 0.00426379 mm under OCCT's existing exact-surface processing.
- Two members plus the hub fail with BuilderFailed, BuildingPCurveFailed,
  AcquiredSelfIntersection and TooSmallEdge. Eight members return an invalid
  fifteen-solid result; multiple faces report BadOrientationOfSubshape.
  Simplification does not cure this: the unsimplified output is already invalid.
- Hypothesis 1: rigid transforms sharing geometry (`copy=false`) rather than
  deep copies reproduce the same two-member failure. Pattern transform policy
  remains unchanged.
- Hypothesis 2: forced OCCT SameParameter through immutable BRepTools_ReShape
  reconstruction, at 1e-7 mm kernel precision, lowers the worst seed tolerance to
  0.00193433 mm but does not cure two-member Join. Exact 3D curves are retained.
- OCCT's documented gluing modes require no real subshape intersections. This
  fixture has them, so a gluing shortcut was not tested.

Hypothesis 3 also fails: OCCT GeomConvert_ApproxCurve replaces only the evaluated
offset boundary at a requested 1e-6 mm tolerance. Reported MaxError is
9.72417e-7 mm; 1,001 parameter samples reach 9.57125e-7 mm point error and
independent nearest-source normal-distance error 6.80858e-7 mm. The replacement
planar face is valid. Actual GeometryKernel extrusion initially refuses its
nonadaptive area/volume integration mismatch; that refusal is preserved. An
isolated equivalent prism with adaptive area and Gauss–Kronrod volume agrees with
area × 60 mm within the existing relative 1e-9 bound. Kernel Intersect returns
one valid member (~2,274.07 mm³), but the same eight-member Join still yields an
invalid fifteen-solid result (~16,672.4 mm³). No production fallback is warranted.

The evidence establishes a Boolean intersection representation problem on valid
inputs, but does not isolate its precise face/curve pair. Replacing the exact
offset boundary alone is not a demonstrated cure. Rotated shell-derived support
faces and their repeated coincidence/intersections remain a structural suspect,
not a confirmed cause. No tolerance search, relaxed validity gate, dimension
change, simplified source solid or discarded result is used.

## Retained reproduction plan and remaining decision

After #261/#263 are accepted, reproduce from a fresh design through the shared
application commands. Reuse the accepted #260 source fixture to author the
constrained XZ radial profile, Full Revolve and retained generated bottom-face
Shell 5 mm. Create the disclosed four-point XZ spline, retained signed Offset
-5 mm and two explicitly snapped Line connectors, then Intersect one 60 mm prism
with the shell through the authoritative feature evaluator. Save the pre-array
state and exact geometry summaries.

Create the retained full Circular Body Pattern on origin Z with total quantity
8. Save the eight-body design and collect stable IDs. On XY author the Ø10 hub
and extrude -5 mm with all eight IDs as Join participants. Record either its
structured refusal and exact untouched candidate/undo state, or one valid
connected output containing every complete participant and hub. This exact
retained reproduction is the first acceptance gate for any correction.

A possible next structural experiment would partition the original shell once
with all rotated cutting strips, avoiding repeated Boolean intersections among
independently trimmed coincident shell faces. Its geometric equivalence and
preservation of the source Intersect → Body Pattern → hub Join sequence are
unproven. Independent Astra diagnosis precedes any further experiment. No
approximation fallback or tolerance policy is proposed after the failed variants.

If a correction succeeds, verify meaningful upstream source edits, stable result
ownership, exact undo/redo/reopen, and one strict closed STL. Follow with the
transverse Ø4 hub bore and independently verify both exits. API/native/GPU/source
comparison evidence remains separate; the full tutorial gate is still required.

External evidence directories: `topology-diagnostic/`, `shared-transform/` and
`same-parameter/` and `approx-offset/`, with source/build helper alongside them. These are headless
geometry diagnostics; no editor was opened. Actual retained Pattern/Join, source
edit, history, reopen, strict closed STL and transverse bore checks remain required
before #265 or the full ornament can be accepted.

Library evidence (retrieved 2026-10-13):
[OCCT 7.9.3 gluing contract](https://raw.githubusercontent.com/Open-Cascade-SAS/OCCT/V7_9_3/src/BOPAlgo/BOPAlgo_GlueEnum.hxx),
[SameParameter declarations](https://raw.githubusercontent.com/Open-Cascade-SAS/OCCT/V7_9_3/src/BRepLib/BRepLib.hxx).
