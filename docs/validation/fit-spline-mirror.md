# Retained open fit-spline Mirror

SKT-092/#457 independently accepted2026-10-15 at `53ec4e6`. Sketch Mirror now
accepts ordinary open fit splines alongside its existing lines/arcs. All fit
points retain source/result identity, including reused on-axis points. Supported
tangent vectors reflect about the final solved axis and remain derived rather
than becoming independent result-handle degrees of freedom. Evaluation exposes
the authoritative Mirror relation for downstream consumers.

Existing Objects/Mirror Line/Preview/Cancel/OK controls and shared commands are
used. Curve and window selection accept splines; window bounds include actual
curve overshoot. Source point/tangent and axis edits rebuild the reflected curve.
Direct result-handle edits and fit-point-count changes while mirrored refuse
explicitly. Closed, control and offset splines remain outside this increment.

The focused core checks41 samples of evaluated source/result curves after
source, axis and tangent edits, retained identity, exact history/reopen and
existing line/arc behavior. Qt-delivered native selection, preview/Cancel and
accept pass3/3 with actual curve and axis hits, without focus/selection repair.
The39-call actual-app API flow passes preview/Cancel/commit, source/axis edits,
history and saved-model reopen. Astra independently reviewed source, receipts
and readable native/Apple M4 captures; root inspected the edited-axis capture.
No physical OS-input or exact public-layout parity claim follows.

External evidence: `deliverables/fit-spline-mirror-2026-10-15/` contains the
source contract, `core-final.log`, `native-02.log`, native captures,
`api-01/report.json`, transcript, baseline/edited models and manifest.
The public workflow contract is independently researched in the external
knowledge records, with source URLs/provenance kept outside Git. Point allocation
and handle serialization are Alloy choices. This supplies the mirrored-outline
authoring prerequisite for TUT-018; the full iron tutorial remains open.

## Actual iron shared-tip correction

The first actual iron fixture exposed an on-axis endpoint omission in Mirror's
spline point array. SKT-092 was reopened; correction `c205138` preserves reused
point IDs while still appending every point in order. Both on-axis endpoints,
reflected handles/curve, exact reopen, the actual iron core fixture and a focused
22-call API run now pass. Astra independently accepted the correction. Evidence
is `core-shared-endpoints.log` and `api-shared-endpoints/` beside the earlier
receipts. Native controls were unchanged; their accepted evidence was reused.
