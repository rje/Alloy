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
lineage evidence. Exact source/log/BREP and qualifications are in the same
external folder's `exact-boundary-erosion-result.md`.

## Reusable kernel ingredient

`PlanarBoundaryInset::evaluate` now admits one valid planar support face with
one closed outer wire, a complete unique retained edge-ID registry, only finite
straight/circular edges, circular radii greater than the positive inset, and a
stable owning feature ID. Line strips and endpoint disks use the actual support
plane; circular cutters retain the source curve's finite span and plane. Each
Boolean step must keep one valid face. The final face must have one wire,
positive area, source containment and the requested boundary clearance. Any
unsupported/ambiguous input refuses with a structured evaluation error.

Final output edge IDs are derived from the owning feature ID, support face ID,
and propagated stable source-edge or sorted incident-edge vertex ancestry.
Duplicate/unmapped output ancestry refuses. The two short source lip IDs are
reported as vanished from final ancestry rather than inferred from late
Boolean `IsDeleted` calls. Owning-feature namespacing prevents two inset
features on the same support from sharing output IDs; it does not implement
the future authored inset feature itself.

The focused `planar_boundary_inset_test` executable was built and run manually
against the external saved post-Split model. It produced one valid 17-edge
233.8576963 mm² inset. An **upstream authored stock-width edit** from64 to65mm
changed the recomputed inset area to about236.289mm² while retaining all17
output IDs and both vanished-lip IDs. Source undo/redo and exact save/reopen
recomputed the same result. This saves and reopens the **source model**, not an
authored inset feature. The actual fixture exercises XY; arbitrary planar
orientation is source-reviewed in the helper and awaits a dedicated workflow.
The test target is intentionally not a default CTest because its saved fixture
is held outside the repository. Exact command output and before/after BReps are
external under `../Alloy-reference-records/deliverables/enclosure-rim-2026-10-14/production-inset-01/`
with log `production-inset-01.log` beside it. Astra independently accepted the
bounded kernel source and focused evidence. Sketch authoring and visual input
remain SKT-076.
