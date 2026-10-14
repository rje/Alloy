# Invocation-local solved-sketch prefix reuse

REL-009/#356 improves the actual final enclosure source and its last grouped
Fillet edit. It advances one local evaluated prefix over already solved features
before each attached/construction sketch, and continues the final canonical
geometry evaluation from that prefix. The prefix is discarded after the one
candidate evaluation; no persistent cache is introduced. Full recompute still
reports `full_ordered`; the accepted append path reports `exact_prefix_append`.
Structural/dependency checks, final authored validation, assembly inspection,
normalization, original ownership and atomic failure remain authoritative.

## Measured headless evidence, 2026-10-14

Current main baseline218a6ae and candidate, Debug/macOS arm64,
Qt6.11.2/OCCT7.9.3, same kernel and exact saved final enclosure source:

| Operation | Baseline | Candidate |
| --- | ---: | ---: |
| Saved source full evaluation |12.950s|4.701s|
| Final grouped Fillet R0.5→R0.45 |13.596s|4.706s|

The first instrumented run identifies six repeated sketch-prefix evaluations
consuming8.281s,64.2% of its12.906s edit. Exact authored JSON, every final body
summary (including topology/reference descriptors) and every sketch summary are
identical before/after for both original and edited states. Final canonical
geometry checks remain enabled. Existing `document_append` and
`component_reparent` regressions pass2/2. Actual saved-model CadDocument edit/undo/redo/save/reopen
passes; reopened authored/body/sketch summaries equal the edited state exactly.

Detailed logs, comparison JSON, source fixture SHA256 and disposable headless
profiler stay outside Git in
`../Alloy-reference-records/deliverables/enclosure-recompute-profile-2026-10-14/`.
The original tutorial fixture stays in the enclosure workflow deliverables.

Independent source, headless comparison and actual-app review accepted this
bounded change. The 36-call saved-enclosure tail passes against the same source
with actual Fillet edit, history/reopen, GPU clipping and individual STL checks.
Actual app opens took 5.150/5.059 seconds, Fillet edit 4.891 seconds and undo/redo
0.194–0.196 seconds. Evidence is in the external `app-01` subdirectory. These are
current-app measurements, not a measured before/after GUI ratio. Headless
wall time does not establish native-input/GPU command time, explain all movie
waiting, or qualify broader REL-004 budgets. No geometry tolerances, radii or
display quality policy change. This does not accept a new tutorial recording.
