# Entity solved coloring candidate

2026-10-13, #259, isolated `fix/entity-solved-colors` from 9455df2. The complete
sketch lesson exposed a defined ellipse colored blue because an unrelated
conic retained seven DOF. This candidate classifies geometry from actual solver
freedom instead of whole-sketch DOF. Independent bounded acceptance is recorded below; focused API metadata and
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
Attempt01 preserves the concrete validation lifetime crash. Attempt03 passed
on the same repaired binary with readable framing. Final evidence is external at
`../Alloy-reference-records/deliverables/mixed-sketch-colors-2026-10-13/attempt-03/`.

## Independent acceptance

Accepted 2026-10-13 against candidate `4f26782` (documentation amendment of reviewed production `f4cfdd7`).
The independent reviewer inspected the final source, pinned solver nullspace
implementation, focused core fixtures and the actual report plus all four PNGs.
The defined ellipse is black beside a blue free conic; removing its full-axis
dimension makes it blue and raises total DOF from seven to eight. Selection
remains orange, and the finished view retains black/blue classification. The
275/100 witnesses and palette are readable at the inspected scale. The focused
application run reports API pass and preserves authored data exactly on Cancel.
No additional editor or unchanged workflow replay was launched for this review.

The derived-geometry regression raised during review is resolved by the zero
solver-freedom certificate, with a focused arc-slot assertion. Explicitly free
fit/conic shape parameters are still excluded from fixed classification; mixed
unresolved derived dependencies remain conservative. This is local Jacobian
freedom, not universal nonlinear rigidity. Late diagnosis can temporarily solve
redundancies inside PlaneGCS; solved geometry and measurements are extracted
before that call, and the added classification reads pointer identity only.
Owned-string copies remove the observed temporary-reference lifetime failure
without changing validation policy. This acceptance establishes the bounded
color correction and clears #257's remaining visual blocker. API captures do
not establish new physical-input coverage.
