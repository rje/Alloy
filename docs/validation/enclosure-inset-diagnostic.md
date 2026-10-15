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
