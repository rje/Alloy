# Retained conic contract

2026-10-13, #255 implementation in progress. External source/design evidence is
`knowledge/tutorials/after-countersunk-plate/conic-design.md`, independently
reviewed before implementation. This document does not establish native or
reference visual acceptance.

A conic owns stable start, end and tangent-intersection vertex point references,
ordered `pointIds`, version 1 and a strict `0 < rho < 1`. Its exact OCCT rational
quadratic has poles start/vertex/end and weights `[1-rho,rho,1-rho]`. The midpoint
of the curve lies at `M + rho*(V-M)` where M is the endpoint chord midpoint.
This shoulder-ratio mapping is a disclosed mathematical policy corroborated by
public terminology; the reference's exact parameterization is not proven.
The tangent vertex is generally not on the curve.

`sketch.conicAdd` takes `{start,end,vertex,rho}` with canonical millimeter
`{uMm,vMm,pointId?}` points. Optional point IDs must name an existing point at
the supplied position and preserve that identity. `sketch.conicRho` takes
`{entityId,rho}`. Both use the same sketch solve/publish authority. Points are
solver unknowns; Rho is separately reported as one editable evaluator parameter,
so fixing the three points does not falsely report the curve fully constrained.
`shoulderLocalMm` is explicitly a sketch-local two-component position.

Creation/edit refuses collinear controls, degenerate point separations and Rho
outside the strict domain. Actual kernel weight-resolution limits are disclosed
rather than clamped. Invalid results retain the original draft. Broader conic
relations and tangent constraints are not claimed by this finite source step.

The focused core passes asymmetric endpoint/tangent/shoulder checks at Rho .5,
.541 and .683, invalid-domain/collinear refusal, explicit shared-point identity,
actual evaluated edge, meaningful retained Rho edit and exact undo/redo/reopen.
Latest focused run: 1/1, 0.42 seconds. App/API and native source sequence remain
pending: three placements, visible Rho handle/field, context-menu OK, Cancel and
invalid pending numeric input must be exercised without focus repair. Invalid
text must prevent OK even when retaining the previous valid preview. Do not
close #255 until those controls and actual GPU evidence are independently reviewed.

Shared-app creation preview and numeric Rho controls are now implemented on the
isolated branch. A 39-call real-app check passed using binary
`442fe94af1f14d73d299465c99c66ae3de959beafa361a980c31a45161b15636`,
with three projected-pointer placements, disposable preview, invalid Rho refusal,
valid acceptance, meaningful edit and exact history/reopen. External evidence:
`deliverables/conic-2026-10-13/api-attempt-04`. Earlier attempts preserve a pending
JSON initialization defect and missing numeric schema range, both corrected.
The actual capture shows a readable upper-left Rho/OK/Cancel panel, selected
analytic conic and honest seven-DOF preview. These are shared API calls, not native
mouse/keyboard evidence. The on-canvas Rho arrow and its drag, native context OK,
preview cancellation/stale-input checks and independent final acceptance remain
required. No source workflow completion is claimed.

The candidate now adds the independently authored on-canvas shoulder arrow,
continuous Rho drag, context-menu OK and existing-curve Rho editing. Focused
native evidence passes; see [the candidate receipt](../validation/conic.md).
Independent final acceptance remains pending; earlier pending statements above
record the implementation sequence rather than the current controls.
