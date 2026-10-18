# Browser document workflow

DOC-009/#21, 2026-10-17. The two-body Browser workflow passes through the
real macOS Alloy entrypoint and the shared local command API. This is a bounded
identity, visibility and body-selection gate; it does not qualify every Browser
node or cross-platform pointer delivery.

`python3 tests/browser_document_workflow_api.py --app
build-preview-release/alloy.app/Contents/MacOS/alloy --output
../Alloy-reference-records/validation/doc009-browser-2026-10-17/api-01`
created two separate solid boxes. Renaming the left body preserved both body
and feature IDs and exact evaluated volumes/bounds. Selecting the right body by
ID, then hiding the left one, retained the right selection. The hidden left
body remained in evaluated geometry with `visible:false`; attempting to select
it returned the explicit `stale_entity` policy error. Undo/redo restored exact
authored states. Save, New and Open preserved both IDs, the name, visibility,
and authored feature history. The 23-command transcript, model and captures are
in the external `api-01/` evidence directory.

Native Alloy UI delivery used the saved two-body model in one private editor.
Clicking the Browser's right body row selected that body and highlighted it in
the GPU viewport. Clicking the left visibility checkbox showed the previously
hidden body; clicking its Browser row selected and highlighted it. After setting
body selection priority, clicking the right body in the viewport selected its
same ID and highlighted the corresponding Browser row. The matched captures
are external `native-both-selected.png` and
`native-viewport-body-right.png`. With default selection priority, a canvas
click selected a face and did not highlight the body row; this is a distinct
face selection, not a body-selection mismatch.

The tested app binary SHA-256 is
`b7c586647b0fc5f8d93b9345352c29ccb9d1c96bc289b4a8e80b9cf99201ae7e`;
the saved model SHA-256 is
`eb7a575bfd3d2abf92a684be185c715e7b846de21ef092b968dbd877c4dd608d`.
The private editor was closed after capture. Export, physical hand input,
Windows/Linux runtime and full reference-product visual parity were outside
this document-organization gate.

An independent Astra review accepted this bounded evidence without a material
blocker. Native rename and visibility gestures beyond the checked checkbox,
and broader assembly Browser parity, require their own gates.
