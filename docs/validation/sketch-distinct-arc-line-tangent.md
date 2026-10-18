# Distinct arc–line endpoint Tangent — SKT-129

Accepted 2026-10-18 after independent source, saved-model, transcript and capture review. One authored circular
arc and finite straight line may retain an endpoint Tangent while their meeting
point IDs remain distinct. Existing shared-ID endpoint and strictly interior
arc–line routes retain their representations.

Activate Tangent and choose **First start/end to Second start/end** in Contact,
then pick the curves in either order. These roles apply to the first and second
picks and also remain available for two arcs. Changing Contact clears pending
picks. The second valid pick commits; Tangent stays active and Escape returns
to Select. The shared `sketch.constraint`, `sketch.constraintTool`, and
`sketch.tangentContact` commands use `endpoint: "start:start"|"start:end"|
"end:start"|"end:end"`. In Automatic mode, exactly one endpoint pair within
0.000001 mm selects this endpoint route. A missing explicitly chosen pair or
multiple candidates without roles causes atomic refusal. This tolerance and unique
pair rule are Alloy policies; reference identity/picker behavior is unknown.

The authored `arcTangent` keeps canonical arc/line IDs, `side`, and `endpoints`
with each stable point ID and endpoint role; the arc also retains `sweepSign`.
The solver jointly enforces point coincidence and radial/line perpendicularity,
avoiding a duplicate positional equation when that pair already has Coincident.
Recompute checks the retained sweep and signed side; it never searches another
endpoint. Profile discovery treats this declared endpoint coincidence as closure
without merging authored point IDs.

Focused `sketch_arc_tangent_test` passes the existing shared-ID and interior
cases plus 32 distinct-ID combinations of pick order, automatic/explicit roles,
start/end roles and signed sweeps. Duplicate and reversed-sweep refusal pass.
The actual-entrypoint API fixture passes 59 calls: one corner of a closed
60×40 R5 plate uses distinct arc-start/right-line-end IDs and the other tangent
uses a shared ID. R5→R7 and width60→70 preserve the relation, distinct identities,
closed region and 5 mm extrusion. Volumes are 11973.174770424683 and
13947.422550032374 mm³. Saved authored geometry independently reads back zero
endpoint gap and zero radial/line dot residual, R7 and a 90° sweep. An R100
edit refuses without changing draft geometry, selection or committed document;
recoverable error diagnostics are expected to change. Undo/Redo and exact
authored save/reopen pass.

The focused Qt native test passes 3/3 slots (one workflow plus setup/cleanup):
actual Contact popup selection and both curve pick orders, retained role/point
IDs, selectable badge, Delete, exact Undo and Redo, and recreation. Only initial
window activation is used; each subsequent focus and popup closure is observed.
The private test editor closes after capture. These are Qt-delivered inputs,
not physical OS input.

Inspected actual captures show a smooth join, endpoint markers and Tangent
badge, readable R7/70/40 dimensions and the edited plate solid. The Apple M4
hardware viewport reports Apple OpenGL 4.1 Metal, with a 1744×1132 native
framebuffer in the API state. Precise reference visual parity, physical OS input,
Windows/Linux runtime and broader pairs remain unknown or separate work.

Evidence: `../Alloy-reference-records/validation/skt-129-2026-10-18/`
(`core.txt`, `api/result.json`, saved `api/rounded-plate.alloy`, native result,
state and screenshots, and `geometry-readback.json`). Source contract and public
provenance: external `knowledge/specs/skt-022-distinct-arc-line-endpoint-2026-10-18.md`.
SKT-022/#163 can close for its declared v1 line/circle/arc analytic scope. Later spline Tangent/Symmetry, one-shot multi-pair Symmetry and branch-choice research remain SKT-130–132/#555–557 and SKT-122/#547.
