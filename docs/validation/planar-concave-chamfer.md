# Exact planar concave attachment Chamfer

GEO-015 original-side implementation independently accepted. Reflected full
finish-group qualification remains open.

Native OCCT Chamfer remains the first attempt. If it fails for one current
straight edge, a bounded fallback constructs the exact triangular prism and
fuses it through OCCT. It is applicable only to two perpendicular planar
incident faces whose complete finite distance-by-edge-length legs fit inside
the actual trimmed faces. Solid classification must establish the empty
concave quadrant and material behind each leg.

Each endpoint requires an outward perpendicular limiting cap. An additional
incident face is allowed only as an opposite-normal continuation of exactly
one leg support plane. This is the wall-ending-at-stem configuration in the
adapted enclosure. For that transition, a native half-distance chamfer must
agree with the corresponding half-distance construction in both directional
solid differences, with both results valid closed single solids. The smaller
result is only an applicability witness; the returned finish uses the exact
authored distance.

The proposed exact wedge must have no positive-volume source intersection,
no positive-area contact with other source faces, the expected added volume,
and no removed source material. Final geometry must be one valid closed solid.
Surface normals use the plane parameter directions U × V and current face
orientation; reflected OCCT planes may have indirect placements whose nominal
Z axis has the opposite sign. Material-classification guards remain unchanged.
Support and edge references use the actual Boolean history. Unsupported
corners retain the ordinary Chamfer refusal; no closest-edge or reduced-radius
fallback exists. Inspection reports `chamferMethod` as `native` or
`planarConcave`; authored distance and command behavior are unchanged.

## Evidence

The external adapted enclosure prefix retains the first two snap finishes,
then selects the published current wall/stem root. The focused test establishes
exact 2 mm (+11 mm³), rigid rotation/translation and reflection invariance, oversized refusal
without authored mutation, no added positive-volume interference with any
other assembly body, upstream Join 5 → 5.25, undo/redo and native-file
save/reopen. A separate diagnostic establishes zero symmetric-difference volume
between native and constructed 1 mm results. Endpoint rejection is source
reviewed, not independently exercised by a separate synthetic negative case.

External records live under
`Alloy-reference-records/deliverables/enclosure-snap-2026-10-14/root-chamfer-probe-01`.
These contain the diagnostic geometry, incidence normals, finite-leg areas and
comparison logs. The real-app runner is
`tests/planar_concave_chamfer_api.py`; API02 records 23 successful calls covering
preview/Cancel, exact commit, history/reopen and GPU captures through the shared
commands. The Case component is explicitly activated before modeling. The dock
shows a readable 2.000 mm value; the broad assembled capture occludes the root
and does not establish a visible root-detail comparison.
API captures do not establish native mouse/keyboard parity.

## Remaining workflow scope

The inferred source placement and edge mapping are documented externally;
public evidence does not establish every exact source click. This geometric
finish alone does not close the five-feature snap Mirror, final 0.5 mm Fillet,
two-Mirror reorder, assembly interference or complete enclosure video gates.
Those remain SOL-053, SOL-055 and TUT-022. Do not count the earlier technical
elbow Chamfer as source attachment completion.
