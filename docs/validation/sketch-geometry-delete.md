# SKT-099: ordinary sketch geometry Delete

Accepted 2026-10-17 for authored line, circle, arc and standalone Point
entities. The command removes no more than the selected entities, their
unshared auxiliary points and constraints/dimensions that reference retired
IDs. It solves and publishes once as one draft-history edit. A selected
endpoint or center required by a surviving curve is refused; grouped or
derived references are also refused atomically. Delete on the canvas and
sketch context menu use the same `sketch.deleteSelection` command as the local
control API. The context menu is a nonblocking popup.

The public behavior contract and source provenance are external at
`../Alloy-reference-records/knowledge/specs/skt-099-delete-sketch-entities-2026-10-17.md`.
Reference cascade details and exact selection styling remain unknown; this
transactional dependency policy is Alloy's bounded design.

Evidence from `build-preview-release`, binary SHA-256
`518357e71c4eab19b2ccf0acc6d8b83efec6426f1ef793933455fc483c79f5e5`:

- `widget_interactions_test sketchGeometryDeleteAndUndo` passed in 2.286 s
  after the harness established initial native-window activation.
  Actual Qt canvas clicks selected a line and Shift-added a circle; Delete
  removed both with their points, a geometric relation and a diameter
  dimension, one history increment and exact undo/redo. A separate context
  event and pointer click on the popup Delete action removed a selected
  circle. A sketch-list row click followed by Delete removed the surviving
  line after the list's focused-key route was fixed. The test asserts canvas
  focus after its clicks and tree focus after the row click. No focus/selection
  repair occurred between demonstrated inputs.
- The real application workflow in
  `../Alloy-reference-records/validation/skt099-delete-2026-10-17-candidate3/`
  removed a line while retaining a circle profile, extruded the 30 mm circle
  8 mm into a valid closed 5654.8668 mm³ solid, saved and reopened the same
  authored document/body identity. The saved model and GPU captures are there.
  A second sketch with retained `slotGroups` refused deleting a group side;
  the exact draft, selection and draft history stayed unchanged. Candidate2
  contains the preceding equivalent fixture; candidate3 used the final binary.
- The separately observed macOS native input in
  `../Alloy-reference-records/validation/skt099-delete-2026-10-17/`
  clicked the line on the canvas, pressed Delete without focus repair, undid,
  then right-clicked the line and chose Delete from the popup. The final state
  records one remaining circle, one closed menu with the Delete action result,
  and an Apple M4 hardware-accelerated OpenGL 4.1 Metal-backed framebuffer.
  `native-cua-sequence.json` is the operator-observed step log; the main-window
  screenshot did not capture the popup surface. This native sequence preceded
  the final sketch-list key-route fix; the focused Qt test and candidate3 app
  fixture use the final binary.

Astra independently reviewed the candidate evidence and the final sketch-list
key-route fix, with no remaining blocker for this slice. This does **not** qualify human hand input,
Windows/Linux, reference cascade/visual parity or deletion of ellipses,
conics, splines and retained groups. [SKT-102](https://github.com/rje/Alloy/issues/525)
is the Todo owner for those curve families. Arc and standalone-Point deletion
were code-reviewed but not separately exercised in the bounded real-app gate.
