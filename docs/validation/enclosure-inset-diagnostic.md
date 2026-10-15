# Enclosure seam inset feasibility

GEO-017 remains an implementation blocker; this is independently reviewed
headless measurement, not an accepted offset feature. The saved post-Split
model's actual seam has 18 edges: ten lines and eight circular arcs. The outer
radii are 5 mm and inner radii 1.5 mm. For the required material inset of 1.25 mm,
these become 3.75 and 2.75 mm respectively. The inner radii do not collapse.

An exact connected line/arc path through the U has measured boundary clearance
of at least 1.75 mm at samples spaced no more than 0.25 mm. By the distance
function's Lipschitz bound, continuous clearance is at least 1.625 mm (subject
to kernel tolerance), exceeding the requested inset. This establishes a
connected positive-width interior witness, not a complete single-component
offset contour or persistent topology references.

The nominal offset of the right 1 mm lip is outside the source material and
closer than 1.25 mm to the boundary. Its neighboring offset line and expanded
inner arc meet at a valid 1.25 mm clearance point. Correct inset topology must
therefore eliminate that short offset segment and resolve the local cusp.
Preserving every input edge is not geometrically correct. Any disappearance
must carry explicit provenance; arbitrary omitted edges remain prohibited.

Exact source, compile script, log, fixture identity and analysis are external:
`../Alloy-reference-records/deliverables/enclosure-rim-2026-10-14/seam-probe/exact-inset-diagnostic.md`.
Astra independently checked the path connectivity, sampling bound and local
intersection inference. No product code or accepting UI was added. The next
step is one exact full-contour construction on this same face, followed by
validity/containment/clearance and retained-source checks. Repeating the failed
high-level offset at either sign is not a justified next step.

## Exact whole-contour prototype

A subsequent finite-boundary-neighborhood Boolean prototype is independently
accepted as headless feasibility evidence. It preserves finite line/circle spans
and endpoint disks, subtracting their clearance regions from the actual seam.
The result is one valid face, one wire and17edges, area233.8576962999306mm².
Result-minus-source area is zero; measured final boundary clearance is1.25mm.
Propagated final cutter ancestry covers all17edges; both short-lip labels have no
final descendants. This is a legitimate geometry-driven topology change.

The prototype is XY/single-wire and its endpoint-disk labels are not yet stable
vertex identifiers. Raw per-step IsDeleted on old originals is not sufficient
lineage evidence. Production admission, deterministic edge/vertex references and
a meaningful source edit/history/reopen remain GEO-017 work; authoring controls
remain SKT-076. Exact source/log/BREP and qualifications are in the same external
folder's `exact-boundary-erosion-result.md`. No further unchanged prototype run
is needed.
