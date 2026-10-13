# Entity solved coloring candidate

2026-10-13, #259, isolated `fix/entity-solved-colors` from 9455df2. The complete
sketch lesson exposed a defined ellipse colored blue because an unrelated
conic retained seven DOF. This candidate classifies geometry from actual solver
freedom instead of whole-sketch DOF. Independent compatibility acceptance is pending; focused API metadata and
edit/history preservation passed on the repaired candidate.

The pinned PlaneGCS public source provides `getDependentParams`: its Jacobian
QR analysis reports all parameters participating in a remaining nullspace
direction, including coordinates coupled to its independent pivots. After an
accepted solve, Alloy invalidates the cached diagnosis, diagnoses at the solved
coordinates and maps returned parameter pointers to stable authored ownership.
Constraint-free systems mark their declared unknowns free. This is local
linearized solver freedom, not a proof of global nonlinear rigidity.

Structured solver diagnostics expose `pointFreedom` keyed by point UUID
(`uFree`, `vFree`, `determinate`, `fullyConstrained`) and `entityFreedom` keyed
by curve UUID (`determinate`, `fullyConstrained`). Line endpoints, circle/arc
parameters and spline poles supply ownership. Ellipses own their center and
both full construction axes' endpoints. Fit tangent intent and conic Rho remain
free shape parameters outside the current PlaneGCS scope. Derived points/arcs
not directly represented in that solve remain conservatively indeterminate
when solver freedom remains. When the solver certifies zero coordinate/curve
freedom, their analytic dependencies retain the valid fixed certificate. No per-entity DOF count is invented.

OCCT `AIS_ColoredShape` assigns each exact child edge its solved color in the
ordinary, construction and centerline presentations, retaining established
dash styles and selected overlays. Edit markers and finished standalone points
also use point freedom. Fixed entities are black even in a mixed sketch;
ordinary free entities stay blue. Presentation metadata does not enter authored
storage, exact curve/profile geometry or history.

The focused core fixture checks a fixed origin ellipse, fixed construction
axes and a separate seven-DOF conic, then removes an actual axis dimension:
global DOF becomes eight, that axis/ellipse become free and the other axis
remains fixed. Existing ellipse geometry/edit/history/storage core also passes.
Ellipse/arc-slot fixtures held temporary `QJsonValueRef` UUIDs; copying the
strings fixes their test lifetime. A focused app open also exposed the same
production lifetime bug in `validateParameters` arc-tangent contact ownership;
that and directly equivalent retained UUID locals now copy owned strings.
Validation rules and authored geometry are unchanged. The derived arc-slot
fixed-entity assertion and existing straight-slot core also pass.

Candidate app SHA256:
`8f2fa56b361d96b032aec2168d12b5411895a8549c82216d452f22dd6ff271a6`.
No private editor was launched by this implementation lane. Root owns the
separate focused app fixture `tests/mixed_sketch_colors_api.py`. Attempt02
passed full-model reopen, fixed/free entity metadata, dimension removal7→8DOF,
selected and finished states, and exact authored preservation after Cancel.
Attempt01 preserves the concrete validation lifetime crash. Attempt03 passed with readable corrected captures on the same binary; the
previous runner needlessly reduced an already fitted viewport. All four final
captures were inspected: fixed ellipse/axes/center are black, the independent
conic is blue, selected ellipse remains orange, and dimension removal makes the
ellipse free. Finished state retains the mixed solved colors. Evidence remains
external under `deliverables/mixed-sketch-colors-2026-10-13/attempt-01`,
`attempt-02`, and `attempt-03`. The final private editor is closed.
