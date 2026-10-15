# Alignment rim two-rail diagnostic (TUT-023)

The public rim sequence and the remaining source uncertainties are recorded
outside this repository at
`../Alloy-reference-records/knowledge/tutorials/advanced-audit-2026-10-14/enclosure-rim-handoff.md`.
The source calls for an inward 1.25 mm offset, Join 2 mm, R0.5 rounding on two
edges, then a retained-tool Cut. The source does not identify those two edges
at the available storyboard resolution. Alloy's accepted trimmed cap has one
ported U boundary, so its single retained interior inset is a disclosed
adaptation, not a claim of exact source topology.

The focused manual diagnostic `enclosure_rim_continuation_test` opened the
accepted 29-feature snap model, evaluated the retained `-1.25 mm` U inset and
committed `Join +2 mm` to Bottom. Eight straight upper `endRim` rails were
present. The chosen adapted pair is the rear outer rail at y=-45.25 mm and
inner rail at y=-44.25 mm, each spanning x=-27..27 mm at z=17 mm. Their
distinct retained edge IDs on the final snap source are
`33cb1fbe-7390-57d4-8fde-ed23f0dd670f` and
`b89ae6af-9182-559f-94cd-dbae8dbf8c64`. Both have generated inset
source-entity/contact ancestry; they bound the same nominal 1 mm cross-section.
The corresponding split-prefix IDs differ, so that prefix cannot substitute
for this cumulative gate. Parallel side pairs also exist, but were not tried.

One exact `R0.5 mm` two-edge Fillet candidate was refused atomically by the
existing OCCT fillet builder with `fillet_failed`: “The selected edges cannot
be filleted at this radius.” This is a concrete unresolved geometry gate.
There was no radius reduction, second-pair search, Cut, intersection, export,
upstream-edit or footage acceptance. The 0.5 mm radius consumes the nominal
1 mm top width, but the failure alone does not prove mathematical
impossibility. The reproducible source target is
`tests/enclosure_rim_continuation.cpp`; current rail enumeration and refusal
are at `../Alloy-reference-records/deliverables/enclosure-rim-2026-10-14/tut023-preflight-01/`.
TUT-023 remains pending GEO-020, the bounded two-rail Fillet child, and subsequent
retained-tool Cut/export workflow.

A later public 1080p frame audit resolves the chain-selection uncertainty:
the source UI shows two tangent chains and 14 edges total, Tangent Chain enabled,
and a typed 0.5 radius. This supersedes the earlier low-resolution limitation;
it does not identify Alloy edge UUIDs, establish per-chain counts or resolve the
other Offset value. GEO-020 must inspect actual tangent expansion, not merely
count its two seed IDs. Reference frames and provenance remain external.

The first refusal occurred during candidate construction, before commit. Its
`two-rear-rails.json` was empty; use `final-snap-rails.json`, the source and the
refusal log as evidence. The diagnostic now preserves inputs and caught errors
before returning, but that source change was only rebuilt, not rerun against the
unchanged failing case. Atomicity follows the throw-before-commit path; this run
did not assert before/after document snapshots.
