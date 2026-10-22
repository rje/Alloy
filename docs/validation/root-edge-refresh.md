# Root edge selection survives refresh

DOC-044/#725, 2026-10-21. The existing `selection_priority_api.py` fixture
selected a root solid edge, then lost it after the next inspection/capture.
The original code read `selectedEdge_["occurrenceId"]` during highlight; this
mutated the JSON object by adding a null key. A later presence check then
compared the body's empty root occurrence with null and cleared the edge.
The mismatch and preexisting source comparison are recorded in
[the body-set receipt](body-selection-set.md#separate-retained-edge-mismatch).

The fix reads body, edge and occurrence identifiers with `.value().toString()`
into owned strings and compares exact body/occurrence identities. Root empty
occurrences remain empty; a linked occurrence still requires its matching ID.
No authored document or command contract changes.

`cmake --build build-preview-release -j4 --target alloy` passes using the
accepted private Qt/OCCT cache. The unchanged focused real-entrypoint
`tests/selection_priority_api.py` now passes its complete edge → face → body →
command override → Auto → Browser replacement → tab sequence. The external
receipt is `../Alloy-reference-records/validation/doc044-edge-refresh-2026-10-21/`
with `report.json`, full transcript, app log and native GPU screenshot. The
screenshot shows a blue selected root edge; the transcript confirms the edge
remains in state after screenshot and a priority miss, and stale edge refusal
preserves selection. The fixture confirms authored state remains unchanged.

A second original linked-occurrence fixture in
`../Alloy-reference-records/validation/doc044-linked-edge-2026-10-21/`
passes 21 real-app responses. It selects the same retained edge separately on
two occurrences, inspects each twice and after GPU capture, refuses an
ambiguous occurrence-free edge request without changing selection, and checks
the authored/evaluated state plus byte-identical native Save. The inspected
second-occurrence capture shows the blue edge cue on the intended instance.

This is targeted root/linked-edge refresh evidence, not physical input or
three-platform qualification. Independent Sol source and final evidence review
accept the bounded repair. No paid CI or release ran; both private test editors
exited.
