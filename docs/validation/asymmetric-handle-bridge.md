# Retained asymmetric handle Loft compatibility

GEO-023/#471, independently accepted 2026-10-17.

The corrected iron's large two-edge side-panel boundaries and smaller high,
tilted ellipse crashed OCCT 7.9.3 automatic polar wire compatibility. The bounded
retained-boundary path now orders the opposite loop by cutter ancestry and splits
the exact analytical ellipse into two corresponding arcs. Only validated 2/2/2
wires bypass automatic compatibility. Authored curves, full ellipse, source
contact checks and output lineage guards remain intact; sketch-only Loft retains
its previous path.

The old retained bridge and sketch-only tests pass. The exact asymmetric fixture
passes both first-boundary picking directions, retained edits/history/reopen,
rigid-frame transformation and atomic refusal. The final focused regression took
141.85s; it is a geometry regression, not a performance pass.

Actual app replay passes51 calls: preview, exact Cancel (including unchanged undo
count), preview/commit, front/top/both obliques and isolated views, app save and
exact authored reopen. The bridge is a valid2-face/6-edge/1-shell open surface;
maximum sampled section deviation is1.3979e-8mm. Independent Astra code/evidence
review found coherent inward surfaces and a correctly placed high tilted opening,
without visible crossing or twisting.

Evidence: external `deliverables/iron-reassessment-2026-10-17/candidate-04-explicit-bridge/`.
It includes source/build records, raw transcript, model, captures and regression
log. The accepted build is `build-preview-release`; stale deleted Ondsel source
override was cleared, restoring the pinned dependency under that build tree.

This accepts the bounded crash fix only. TUT-034 retains visible lower-panel
waviness, final proportions and rear/underside closure; TUT-018 remains reopened.
It establishes neither general Loft coverage nor native gesture parity. Preview
and geometry rebuilds remain slow. REL-031 separately addresses display-only
visibility recomputation; other responsiveness remains under REL-016.
