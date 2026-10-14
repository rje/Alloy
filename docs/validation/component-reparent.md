# Local body and sketch organization candidate

ASM-007/#300 adds separate retained organization for finished bodies and sketch
objects in identity-placed local components. `component.reparent` accepts explicit
stable body or sketch IDs and a destination definition ID. The Browser's body
and Sketches-folder move actions use the same candidate transaction. The
original modeling feature owners, feature order, parameters, and source IDs do
not change; evaluated ownership, component inspection, Browser membership,
opacity, and scoped export derive from the retained object-owner map. A new
cross-owner modeling operation is refused clearly, while earlier source
feature edits continue in their original modeling context. Missing object IDs,
pending placement, and nonzero Slider pose are refused atomically.

The focused `component_reparent_test` and existing `local_components_test`
pass. They cover separate body and sketch moves, stable geometry and IDs,
source edit, modeling-prefix preview, undo/redo, save/reopen, scoped export,
and invalid reparent refusal. The real-app API run at
`../Alloy-reference-records/deliverables/component-reparent-2026-10-14/api-01/`
passes body then sketch organization, earlier Extrude edit-dialog preview and
Cancel, source edit, history, reopen, and STL readback. The Qt-native context
actions pass both moves and Escape Cancel; the final Browser capture at
`../Alloy-reference-records/deliverables/component-reparent-2026-10-14/native-03/body-and-sketches-browser.png`
shows the finished body and source sketch under the destination component.

The Browser drag/drop route is implemented, but an actual OS drag is pending
independent validation in ASM-012/#343. Qt-synthesized mouse events started a
drag but did not deliver a drop event in the native test harness; a bounded
direct Qt drop-event probe also did not reach the move dialog. These harness
results do not establish whether a physical gesture succeeds or fails. A
separate computer-use attempt could list the candidate Alloy process, but
`getApp` returned `-10005 cgWindowNotFound` twice before any pointer gesture.
The passed native evidence uses Qt-delivered context actions. The clean-room
source record and gesture contract are external
at `../Alloy-reference-records/knowledge/tutorials/advanced-audit-2026-10-14/asm007-reparent-design.md`.
