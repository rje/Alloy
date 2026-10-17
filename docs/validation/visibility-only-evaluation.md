# Display-only visibility reuses evaluated geometry

REL-031/#473, 2026-10-17. Ordinary feature visibility updates previously rebuilt
all downstream geometry on the UI thread. On the corrected iron, hiding Outer
took41.718s and showing it47.911s. A process sample identified Surface Trim's
OCCT Splitter during this presentation-only action.

The document now admits a visibility-only path only when metadata, history,
feature IDs/order and every other field are identical, visibility values are
boolean, and the accepted evaluated history matches. It copies the complete
evaluated snapshot and updates bodies, sketches and planes by producer feature
ID. Shapes, references, ownership, occurrences and immutable prior snapshots are
preserved. Existing document transactions retain normal undo/redo/dirty behavior.
Mixed geometry edits and cold reopen still use ordinary evaluation.

The focused document_append_test passes shape identity/support preservation,
producer-versus-modifier visibility, actual sketch/plane toggles, undo/redo/no-op,
invalid input, mixed visibility/geometry fallback and cold reopen. The application
and focused target built in the accepted build-preview-release tree.

A real-app replay of the exact iron bridge model passes9 calls. Hide took335.8ms
and show345.2ms; both report `evaluation.recompute=visibility_only`. All body
summary fields except visibility and all assembly records remain unchanged;
show restores original authored JSON. Actual GPU captures show Outer hidden and
restored while the Inner surface remains. Astra independently reviewed source,
test assertions, raw ledger and images; no additional repeated replay required.

Evidence is external under
`deliverables/iron-reassessment-2026-10-17/visibility-fastpath-01/`. Earlier slow
calls and the process sample remain in `candidate-04-explicit-bridge/`.

This accepts idle visibility improvement, not asynchronous geometry execution.
Active Combine still explicitly recomputes its preview; geometry-changing edits
and reopening remain slow. Viewport presentation reconstruction also remains a
separate cost. REL-016 retains broader responsiveness work.
