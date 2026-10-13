# Exact accepted-prefix append candidate

2026-10-13, SOL-042/#268. Actual cumulative-03 creation passes, but its unchanged
Join is rebuilt for every subsequent append: Join23.67s, construction plane129.86s,
sketch finish126.39s and bore Cut134.5s. Recording fails its frame-gap limit; no
valid complete movie, source edit or strict-STL acceptance is established.

The source-profile/strip sketches stay byte-identical through four consecutive
solver passes. Independently rebuilding the pre-Join history twice takes899/893ms
and yields byte-identical eight input BReps and the same body order. First Join
and later appends both use full ordered evaluation. The slow sample is in Fuse
Build, not the removed single-result Common checks. This establishes unchanged
inputs, not a precise OCCT cause for variable repeated cost.

The bounded current-document path admits exactly one appended construction plane,
sketch or Extrude Cut. All non-feature metadata, complete authored and resolved
prefixes, and the evaluated feature order must match. The only prefix exception
is automatic Cut source-sketch visibility true→false; every other field stays
exact, and only the copied evaluated sketch visibility changes. Unsupported tails,
upstream edits, removals/reordering, metadata/binding changes and open/static
preflight retain full evaluation. Empty histories do not use reuse.

The copied complete EvaluatedDocument preserves bodies, profiles, support/edge
references, planes and Join ownership. The same ordered GeometryKernel loop starts
at the one new feature, reconstructing the earlier sketch map. Only the new sketch
is solved, with its authoritative seeded support context. Document commit and
Extrude preview share the member preparation path. Inspection reports
`exact_prefix_append`; undo retains the old immutable evaluated snapshot.

App build and focused document_append, construction_plane, profile_join and
successive_join core checks pass. The new fixture uses a real through bore and
checks its independent analytic volume against full evaluation, exact support
identity, preview isolation, failed-Cut atomicity, old snapshot geometry, source
hide+Cut in one undo entry, redo, upstream fallback and full save/reopen. Its BRep
comparison normalizes only OCCT's container Free flag on the test side; production
performs no shape normalization. Existing non-destructive Cut may add support
representations, qualified by the prior independent Join review.

Evidence is external under `validation/ornament-feasibility/`:
`sketch-solve-drift.json/.cpp`, `repeated-prefix-inputs.json/.cpp`,
`document-prefix-append-core-final.log`. Root owns the actual cumulative-04 API,
GPU, recording, edit/reopen and strict-STL gate. This receipt does not close the
full ornament or claim a general document checkpoint/cache architecture.

## Independent bounded acceptance

Astra independently reviewed final candidate `c9597d7`, the guard and shared
ordered evaluator, transaction integration and focused fixture. Accepted for
source/core scope: reuse requires exactly one permitted append and exact current
authored/resolved prefix plus metadata and evaluated order. The sole automatic
Cut visibility exception modifies only the copied source sketch. The current
binding resolver preserves feature count/order/IDs; no ID-only admission is used.
Static/open and changed-prefix paths continue through full evaluation.

The focused 4/4 final log passes in 1.09 s. The actual plane/sketch/through-Cut
fixture establishes analytic volume and full-evaluator agreement, original
support identity and geometry, preview isolation, failed transaction atomicity,
one hide+Cut undo entry, redo, upstream fallback and full reopen. Its test-only
Free flag baseline is qualified above. Production geometry is not normalized.
No additional test replay was needed for this review.

This accepts the bounded evaluation change, not measured speed or complete
workflow success on the ornament. Root's cumulative-04 actual API/GPU timing,
recording, source edit/reopen and strict-STL evidence remain pending. No new
native-input or broad checkpoint/cache compatibility claim is made.
