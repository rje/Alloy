# Offset construction planes: bounded acceptance

2026-10-12, GEO-002 #186. Sol implemented; Astra independently reviewed.
Retained origin, prior construction plane, sketch plane and supported planar-face
sources, signed offsets, visible GPU planes, shared API, edits and storage are
accepted. Broader construction types remain #56.

The real-app40-command ledger verifies four circular sections on z0/18/38/58,
relative offsets18/20/20, selected-plane sketch inheritance and explicit planeId
sessions. Editing the first offset propagates to19/39/59; undo/redo and reopen
preserve authored references. Captures show all sections and planes on Apple M4
hardware rendering. Evidence: build-tutorial-polish/evidence/construction-planes.

Astra's focused Qt widget test verifies Browser sketch selection, Offset Plane
numeric entry, unchanged Cancel, commit, plane selection and Create Sketch, and
a second relative offset. Final source-driven checks exercise the real viewport
profile context menu, select Offset Plane, type18 and press Enter once. Amber
preview, legible controls and modal capture route pass; qt-input-final.txt records
3 passed including fixture setup/cleanup. The selected-plane clearing defect was
fixed before acceptance. Test: offsetPlaneSelectedSketchAndPlaneInput.

Input evidence is Qt-dispatched mouse/key events, not OS-level mouse control;
CUA native pipe startup failed. Alloy currently uses a modal numeric dialog
instead of the reference inline value/arrow presentation. Interactive arrow
manipulation remains with #56, and no exact presentation parity is claimed.
The complete egg-cup/nested-cubes tutorials remain open.
