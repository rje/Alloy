# Fit-point spline shared-API candidate evidence (#253)

2026-10-13. **API ingredient passed; feature not independently accepted.**
Native input fixes and visual review remain pending. This is not the full sketch
lesson #257, control-point splines, or curvature-comb acceptance.

`tests/fit_point_spline_api.py` passed its first real-app run with38 calls against
`../Alloy-worktrees/fit-point-spline/build-fit/alloy.app/Contents/MacOS/alloy`,
SHA256 `a68e717acd9c4ce7ea9ccee6efbdb9a5222db6685fbad045300cf24616e4fc30`.
The private editor exited normally. External evidence:
`deliverables/fit-point-spline-2026-10-13/api-attempt-01/` under the reference
archive, including report, full transcript, saved model, app log and GPU capture.
The binary predates pending native drag/history and no-motion fixes; those
require separate native evidence and do not inherit an input pass from this run.

The shared API creates three fit points, activates an endpoint tangent, removes
the middle point, inserts a new curve point, edits its tangent direction/weight,
and rejects a zero tangent without changing the authored draft. Stable retained
endpoint identities and distinct inserted identity pass. Evaluation reports real
degree5/C2 and explicitly separates editable handle freedom from point-solver
freedom. Numerical continuity/derivative checks belong to the author's focused
core fixture and independent review, not this API metadata assertion alone.

Closing with a real line yields one valid OCCT profile, area2319.839323043385mm².
A tangent-weight edit changes its evaluated area. Undo/redo compares exact authored
snapshots; reopening the saved file restores identity and area. This closure is
an Alloy integration check, not a source-required extra line in the tutorial.

Root inspected the capture: actual spline, three fit points and paired handles
are present. Their faint lines/knob presentation still needs deliberate native
visual review. The palette exposes an internal evaluator message as an orange
warning; author was asked to remove this non-actionable implementation detail.
The original capture is retained. No visual-parity or native acceptance follows
from the API pass.
