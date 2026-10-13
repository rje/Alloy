# Multi-body STL independent review

Reviewed 2026-10-13 against the integrated candidate containing 8026f86 and
11245c4. Scope: #193 explicit body sets, all visible bodies, and export controls.
Reviewer did not implement the feature.

The focused Qt case `meshExportFileScopeAndCheckedBodies` passed (3 including
setup/cleanup, 0 failures, 1.724 seconds). Actual File menu input opened Save As
Mesh; with no selected body and two solids its default was All visible bodies.
Keyboard selection switched to Selected bodies; mouse row selection plus Space
checked both bodies. Cancel left authored state unchanged. Initial test-driver
failures from asynchronous menu timing and checkbox hit coordinates were fixed;
the final receipt is `build-tutorial-polish/evidence/multibody-independent/qt-input-final.txt`.
Inspected `qt-all-visible.png` and `qt-selected-bodies.png`: labels, both checks,
refinement values, Save, and Cancel are readable. Existing selected-body context
may intentionally default a single body; this is not an unconditional all-body
export default.

Real application exports using explicit `bodyIds` and `allVisible:true` each
passed independent STL triangle readback: two closed, consistently oriented
components, bounds (-5,2,3)..(15,12,18) and (40,-5,7)..(52,7,19), positive volumes
3000 and 1728 mm³, and unchanged authored state and undo history. See
`readback.json`, `all-visible.stl`, and `explicit-two.stl` in the same directory.
`capture.dialog` also captured the actual mesh dialog as `api-dialog.png`.

Bounded export acceptance passes. One separate shutdown defect remains:
`app.quit` with `discard:true` returned while Save As Mesh remained open, but the
process did not exit within the harness's five-second deadline. The harness then
terminated that private process. Root was notified; no private editor remained.

This receipt separates Qt-dispatched widget input, API execution, and independent
mesh evidence. Native OS input and full tutorial #172 assembly/export acceptance
remain separate. The native file chooser Save action was not exercised here.
