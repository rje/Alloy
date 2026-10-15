# Reuse exact Loft edit evaluation

REL-028/#460 is independently accepted at `91add7c`; `c3a6f1e` corrects only the
new API test's use of the existing explicit-preview contract.

A command session retains its exact source prefix, keyed by session, document,
revision, edited feature and active/effective owner. Preview reuse additionally
requires complete input and candidate equality. Opening an unchanged final Loft
uses the accepted current result; changing it evaluates the candidate once.
Editing an earlier Loft retains both downstream validation and the through-feature
stage. Commit still uses the normal authoritative document command.

On the same saved iron bridge, edit opening measured 40.319 seconds before and
5.226 seconds after. A second session after owner activation took 8.481 seconds.
Unchanged previews took 118–247 ms. Initial tail editing performs one source-prefix
evaluation and zero candidate/through-feature evaluations. Changed inputs perform
one full evaluation; commit and reopening still take about 9–10 seconds. These
are bounded recorded timings, not a universal latency guarantee.

Focused native tail and non-tail/stale-source checks each pass 3/3. The 25-call
actual-app replay passes input invalidation, repeated preview, owner-context
change, commit, exact undo/redo and reopening. The first new API test incorrectly
assumed updating a Loft automatically previews; the corrected test requests
preview explicitly, preserving that failed transcript. No kernel code changed
and no unchanged geometry suite was repeated.

Astra independently reviewed source, receipts and smooth actual Apple M4 GPU
captures. Evidence and the original saved input are external under
`../Alloy-reference-records/deliverables/loft-edit-prefix-reuse-2026-10-15/`.
Post-review `3af41a1` also clears stale geometry inspection metadata when Loft
inputs change. The focused contract check passes 3/3; no unchanged API/geometry
workflow was repeated for this one-line invalidation correction.

Remaining cold source-prefix cost and broader responsiveness stay outside this
acceptance. No Windows/Linux GPU or physical-device input claim is made.
