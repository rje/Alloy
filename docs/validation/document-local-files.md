# Local document file workflow (DOC-013)

The real macOS Alloy editor now supports the bounded local New/Open/Save As and
dirty-close workflow through the same commands used by its native UI. The
current implementation also exposes an open native Save/Discard/Cancel prompt
through `document.pendingDecision` and `document.answerDecision`. Each prompt
has a fresh stable ID, scope (`active-tab` or `all-tabs`), active tab ID,
message and available choices. A stale ID is refused. Native Cmd+N adds a
design tab; API `document.new` replaces the active document after an explicit
dirty-work decision. These are intentionally different operations.

The focused final real-entrypoint run was
`python3 tests/document_file_workflow_api.py --app build-preview-release/alloy.app/Contents/MacOS/alloy --output ../Alloy-reference-records/validation/doc013-files-2026-10-17/api-guard-final`.
It passed 35 recorded API responses: dirty New/Close/Open refusals with exact
state retention, save, failed save to a missing destination retaining the old
path and dirty edits, Save As to a second path, reopening both files, malformed
Open retaining the current document, and explicit discard/new. The successful
files contain different authored states. `CadDocument::save` updates path and
clean state only after `QSaveFile::commit` succeeds.

Native checks used the exact `build-preview-release/alloy.app` via
LaunchServices and the macOS accessibility route, plus Alloy's own local API
for resulting authoritative state. A native NSSavePanel was opened and
cancelled with the one-box dirty document intact. Native Save As wrote
`native-save.alloy` in the external evidence directory, with the correct path,
authored content and clean state. After a second box, native tab Close opened
the Save/Cancel/Don't Save prompt. Cancel retained both boxes and dirty state;
Save committed the second box, closed that tab and activated a blank tab.
A fresh app instance used native Open to load the saved file and reported its
two features, same authored data and clean path. Native Cmd+N then added a
second independent blank tab. Both private editor instances were quit.

The final build additionally opened a native dirty-close prompt on an unsaved
box. The API reported `pending=true`, `scope=active-tab`, the prompt's message
and three choices. `document.answerDecision` with a wrong ID returned
`no_pending_decision`. Answering Cancel through the API closed the prompt and
left the one-tab document dirty and intact. Reopening the native prompt and
answering Discard closed the dirty tab and activated a clean blank one. The
API then reported `pending=false`. This uses the actual Qt dialog and tab
commands, not a parallel automation model.

Independent review found that a second API request could mutate the active
tab while the native prompt was open. The corrected implementation holds a
decision-operation guard through both the prompt and any follow-on native
Save file picker, and retains the target tab, document and revision before
acting. In a focused final native run, `document.tabNew`, explicit-discard
`document.new`, `solid.cylinder`, explicit-discard `document.tabClose` and
`app.quit` all returned `document_decision_pending` during the prompt. The
authored state and tab list remained exact. After choosing Save on the prompt,
the native NSSavePanel stayed open while additional tab/new mutations were
refused. Cancelling that panel left the same one-box document dirty and the
decision guard cleared. The local `guard-verification.json` records the final
state; this bounded check covers the observed reentrant API risk.

External evidence is in
`../Alloy-reference-records/validation/doc013-files-2026-10-17/`:
`api-guard-final/report.json`, `guard-verification.json`, app logs/ready files
and the native saved model.
Final executable SHA-256:
`b7c586647b0fc5f8d93b9345352c29ccb9d1c96bc289b4a8e80b9cf99201ae7e`.
Native saved model SHA-256:
`02985fafc2d1f17db1a0f6600e972bd154cf68b83679129f2b090c2341adbe7c`.
The native observations above are an operator transcript, not a persisted
input movie or a claim of physical hand-operated hardware testing. Geometry
creation was only a fixture; no broader CAD operation, platform or visual
parity is inferred from this file-management gate.
