# Fit Point Spline contract

Reference `sketch-slots-splines-20261013`, retrieved 2026-10-13. Detailed public
sources, degree/handle evidence and independent mathematical review remain in
`../Alloy-reference-records/knowledge/tutorials/after-countersunk-plate/spline-design.md`.
This is the bounded #253 implementation contract; #254 control cages/curvature
combs and the complete source-order lesson remain separate gates.

Native Create → Spline → Fit Point Spline collects planar picks. The Check button
or Enter commits one curve; further picks create another. Escape discards pending
picks and leaves the tool. Select a fit point and Delete removes it if at least
two remain and no other authored reference owns it. Select the curve, right-click
Insert Spline Fit Point, then click its actual curve. Selection exposes green
handles; dragging a grip previews direction and weight, release records one
history entry, and Escape cancels. A simple fit-point click selects without
moving or recording a drag. Undo/Redo/tool changes cancel a pending drag before
changing authoritative state.

Authored entity `fitSpline` retains a UUID, ordered point UUIDs, endpoint UUIDs,
version 1, `parameterization=chordLength`, and `tangentHandles` keyed by fit-point
UUID. Coordinates and derivative components are canonical millimeters. Insertion
creates a new UUID; remaining fit identities and edited handles survive removal,
point movement and insertion. Insertion reinterpolates and may change curve shape.
Constrained/shared fit-point removal refuses atomically rather than removing
unrelated user intent.

The actual evaluator is degree 5, nonrational and C2. It derives baseline first
and second derivatives from OCCT unflagged through-point interpolation at
normalized cumulative chord parameters, then creates exact quintic Hermite spans.
Edited handles replace shared first derivatives; baseline second derivatives
remain. OCCT removes redundant knot multiplicity only within the recorded
geometry tolerance, preserving the normalized span domains. The green grip offset
is one-third the first derivative; its direction and distance determine the
stored vector. These knot/boundary/handle scaling choices are Alloy policies;
public evidence establishes fit-point/handle behavior and degree, without a
unique interpolation policy or exact intermediate curve shape. C2 is verified
in the same global parameter across unequal spans; no G4 parity is claimed.

PlaneGCS constrains the ordinary fit points. Tangent vectors remain editable
parameters outside this bounded solver relation set. Inspection distinguishes
`pointSolverDof` and `splineHandleDof`; the latter contributes to the displayed
DOF so fixed fit positions do not claim a fully defined editable curve. Other
curve relations, spline trimming/offsets and closed/periodic spline creation are
outside this child. Exact OCCT edges participate in ordinary region evaluation;
a spline plus a retained closing line produces a real selectable face.

Shared commands: `sketch.splineAdd`, `sketch.splineInsert`, `sketch.splineRemove`,
`sketch.splineTangent`, `sketch.splineComplete`, `sketch.splineInsertMode`,
`sketch.splineDrag`, and existing `sketch.movePoint`. Discovery includes canonical
inputs; geometry inspection includes degree, continuity, normalized fit
parameters and actual handle vectors/grip positions. The drag authority previews
a disposable draft and commits only on completion. Invalid numerical or kernel
evaluation preserves the prior draft and stable identifiers.

Verification remains split: the core fixture checks through points, degree/C2,
nonuniform spans, 3→2 with an active retained endpoint handle→3 reinsertion,
interior direction/weight changes, refusal, independent profile-area integration
and exact authored undo/redo/storage. The real application API fixture and native
Qt input/GPU captures have separate evidence and acceptance. No cross-platform
runtime, physical OS input or broad spline parity follows from these fixtures.
