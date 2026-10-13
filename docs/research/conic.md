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
