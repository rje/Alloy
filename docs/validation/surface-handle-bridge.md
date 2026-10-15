# Retained three-section handle bridge

SRF-015/#359 is independently accepted. Implementation `f3065d5` extends the no-guide surface Loft to actual
retained Trim boundary chains, a middle sketch ellipse and the opposite chain.
The active-owner selection correction `eb34db6` and final native/GPU evidence are accepted.

The two outside sections retain body, producer, face and two edge identities.
The evaluator resolves one closed inner Trim wire and orients its first edge
through the original cutter endpoints. It passes the actual curves to OCCT
ThruSections, preserving source geometry. Section contact is checked against
resulting support surfaces, not merely shared boundary topology. This is sampled
contact evidence, not an analytic or G1 continuity guarantee.

New Body creates a separate surface in the active component; it does not create
a new component. Existing sketch-only no-guide, guided surface and solid modes
retain their dispatch. The native Chain Selection and local API use the same
selection authority. Missing/ambiguous boundaries and unsupported placed inputs
refuse atomically.

## Accepted evidence

The actual exterior→paired Trim→ellipse→bridge core passes source and ellipse
edits, stable identities, history, reopening, owner preservation and refusal.
The existing three-sketch Loft regression also passes. The final bridge is one
valid open shell with two faces, six edges and no solid; the exterior remains.

The actual application API passes 43 calls through boundary-chain selection,
preview/Cancel, commit into the activated source component, ellipse edit,
undo/redo, reopening and feature editing. Bridge area changes from 7872.075 to
8106.866 mm² while face/edge IDs remain stable. Maximum sampled section/support
distance is below 6e−8 mm across the base and edited results. Five bounded rays through the ellipse interior encounter
neither the exterior nor the bridge; this confirms those sampled passages,
not exhaustive clearance.

External evidence is in
`../Alloy-reference-records/deliverables/surface-iron-continuation-2026-10-15/retained-bridge-01/`.
The prepared coordinates remain independently authored; permitted public source
operation observations and uncertainty are in the external knowledge records.

## Remaining qualifications

The focused native check passes all three Qt cases, including real menu/curve
picks, preview/Cancel and commit with a component-owned ellipse. The API separately
covers a root-owned ellipse with output in the activated component. Native clean
front/rear/isometric captures show smooth surfaces and an open passage. Apple M4
hardware rendering is verified; no physical OS input or Windows/Linux GPU claim.
Astra independently reviewed source, receipts and actual captures and accepted
the bounded feature. The first native attempt stopped before opening Loft because
its harness assumed a toolbar button visible at that width; the corrected route
uses the actual Surface Create menu. Both results remain in the evidence.

Measured complete selection/commit/edit/reopen costs about 14 seconds; opening
the feature edit dialog costs 41 seconds. REL-028/#460 tracks redundant history
work in that edit path. Correctness acceptance does not establish responsiveness.
Closing sheets, curved Trim trials and Boundary Fill remain separate TUT-018
work. No finished tutorial, solid export or broader platform runtime is claimed.
