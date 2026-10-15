# Alignment rim two-rail diagnostic (TUT-023)

The public rim sequence and the remaining source uncertainties are recorded
outside this repository at
`../Alloy-reference-records/knowledge/tutorials/advanced-audit-2026-10-14/enclosure-rim-handoff.md`.
The source calls for an inward 1.25 mm offset, Join 2 mm, R0.5 rounding on two
tangent chains, then a retained-tool Cut. The public frame does not identify
the selected source curves. Alloy's accepted trimmed cap has one
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

## Exact contour localization (GEO-020)

The one bounded OCCT contour probe uses the same saved 29-feature source,
`-1.25 mm` inset, Join `+2 mm`, adapted rear pair and `R0.5 mm`. The source
model SHA-256 is `cab0d6254ad9ec6067237bfb1df009cf315a222fe33668c8f1caa0a85a615e86`.
The diagnostic source SHA-256 is
`8940cd8787e67d72db934340a13db178fffefac61c7cf11f721d2c3228154628`.
Its exact result and log are under
`../Alloy-reference-records/deliverables/enclosure-rim-2026-10-14/geo020-contour-01/`.
The result JSON SHA-256 is
`3bae851b35ea762848d8bd58b932e1bd156a109e4988dcacd9d110052c3d2b2c`.
An initial relative-path invocation refused to open the source; the recorded
result uses an absolute source path.

OCCT forms **two different tangent contours**, seven named upper `endRim`
edges in each (14 total in this Alloy model). Both contour stripe statuses are
`Ok`, with zero faulty contours. `Build()` returns no completed or partial
solid and reports a faulty vertex at approximately
`(29.25, 44.50, 17) mm`, incident on two `endRim` edges and one `sideCorner`
at the U/port-mouth termination. A preceding observation of the same geometry
reported two faulty inner mouth vertices instead; its JSON was overwritten by
the provenance-enriched result, so those coordinates are not preserved as a
separate reproducible artifact. The retained result localizes a corner/junction
failure; it does not establish a single numerical cause or prove R0.5 impossible.
The public 14-edge count corroborates only the chain-selection form, not exact
topology or geometry parity. No alternate radius, seed pair, kernel algorithm,
Cut or export was attempted. GEO-020 and TUT-023 remain open.

The next source-backed diagnostic is recorded externally in
`knowledge/specs/geo020-occt-mouth-corner-diagnostic-2026-10-14.md`: inspect the
fillet/limiting-face intersection at the reported endpoint against the trimmed
face boundary and identify the applicable corner path. Containment and the exact
cause remain unknown. The observed three-edge junction does not establish a
four-or-more-edge kernel limitation. No additional kernel replay was made for
that research.
