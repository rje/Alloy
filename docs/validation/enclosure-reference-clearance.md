# Final enclosure/reference interference inspection

TUT-029, 2026-10-14. Diagnostic completed; final whole-reference fit does not pass.
Independently accepted by Astra as bounded blocker evidence.
The later user-approved [connector-clearance adaptation](enclosure-connector-clearance.md)
resolves this collision in a new authored variant; the original diagnostic and
saved model here remain unchanged.

The saved TUT-023 stage model was reopened without authored edits. The existing
manual enclosure target inspected both valid closed halves against all twelve
retained imported solids using non-destructive OCCT Common, unchanged tolerances
and no simplification. All 24 pairs completed. Twenty-two have zero intersection
volume. Two contain valid closed positive-volume solids:

| Half / reference | Volume mm³ | Intersection bounds mm |
|---|---:|---|
| Top / SIDE-ETH-001 | 0.732240293455 | X −28…−27, Y 42.618033989…43, Z 15…21.1 |
| Bottom / SIDE-ETH-001 | 0.888291503536 | X −28…−27, Y 42.618033989…43, Z 7.6…15 |

The Top value agrees with the earlier snap-stage inspection. That earlier test
only compared Top against other bodies, so it did not establish Bottom/reference
fit. The final zero-overlap result between Top and Bottom remains valid but does
not establish clearance to the imported board.

The preserved source and measured bounds support a specific geometric explanation:
64 × 93 stock with outer R5 and Shell3.5 leaves an inner R1.5 corner centered at
(−27, 41.5). The nominal rectangular connector reaches X −28 and Y 43. The
circular corner wedge has area 0.120039392370 mm²; multiplying by the respective
6.1 and 7.4 mm heights reproduces both measured volumes. This is an analytical
inference matching the intersection evidence, not a new geometry construction.

Evidence is external under
`deliverables/enclosure-cumulative-2026-10-14/tut029-clearance-01/`:
`clearance.json`, both evaluated half BReps, both overlap BReps and the implicated
reference BReps, authored reference snapshot, input/build hashes, logs and the
analytical note. The focused target built successfully. Exit 2 means positive
intersections, not an incomplete inspection. Initial relative-path invocations
refused before evaluation; the actual inspection used the required absolute path.

No source dimensions, reference envelope, placement, tolerance or radius were
changed. A bounded follow-up must qualify a source-supported fixture or enclosure
adaptation, preserve the original evidence, and verify affected authored
references/history/exports. Silently trimming the imported reference or changing
the source stock is not acceptance. Zero volume also does not establish positive
distance, manufacturing clearance, snap force or physical fit. Full TUT-016
clearance acceptance remains open.
