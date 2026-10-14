# Retained Cylindrical joint

Independently accepted and integrated at `bcc5493`.

ASM-008 adds one retained as-built Cylindrical relationship between identity
component leaves. The selected Position is a current complete circular edge on
Component 2 with a Z-directed axis. The joint preserves both components' current
poses at creation; rotation and axial slide can then be edited independently.
Its source body/edge and moving counterpart are retained across recompute. The
authoritative assembly graph reports each joint's measured coordinates and one
temporary rest-pose gauge per disconnected island until an authored Pin is
present. A passive joint's coordinates are read-only after its island connects
to an earlier driver.

The shared `joint.cylindrical` and `joint.cylindricalDialog` commands create/edit
the same authored joint. `joint.previewCylindrical` stages an exact keyed
rotation and slide; `joint.cylindricalStop` restores all persisted poses. The
native As-Built panel displays readable component names, a retained circular
Position and separate Rotation/Slide fields. Browser edit and a short motion
preview route through the same commands.

The focused core `assembly_cylindrical` test passes on the independently authored
four-part piston fixture, including retained origin/source edits and atomic
refusals. The combined application builds. An actual-app API run passed 24 calls:
it created an Arm Revolute and a disconnected nested-piston Cylindrical, measured
two free coordinates, staged +15° and +5 mm, checked that the Inner piston's
world Z bound moved +5 mm, stopped without changing authored data, then verified
undo/redo and save/reopen. A focused Qt native test passed 1/1: a toolbar action
was triggered programmatically, component combo indices were assigned by the
harness, then a viewport circle was clicked and 15°/5 mm were typed into the
native fields; Cancel left authored data intact. The native preview capture has
a readable panel and highlighted circular origin. The test output was copied
from the completed tool result to the external evidence folder.
A separate focused real-app check opened the saved model and refused a Revolute
ID passed to `joint.previewCylindrical` with `invalid_cylindrical`, leaving
authored data unchanged; its result is in `typed-guard-02/` beside the other
external evidence.

Evidence: `../Alloy-reference-records/deliverables/piston-cylindrical-2026-10-14/api-01/`
and `../Alloy-reference-records/deliverables/piston-cylindrical-2026-10-14/native-01/`.
The public operation contract and independently declared fixture dimensions
remain in `../Alloy-reference-records/knowledge/tutorials/piston-subassemblies-2026-10-14/`.

This bounded acceptance does not claim the coupled three-Revolute/one-Cylindrical
loop, Upper Arm drag, copied piston, stroke limits, arbitrary joint axes, or
general assembly mobility. The piston and arms are intentionally separate
islands in this check. Source face-center selection is adapted to one complete
retained circular edge. Independent compatibility review accepted the stated scope.
