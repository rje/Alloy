# Coupled piston workflow checkpoint

ASM-015 remains in progress: coupled authority/API evidence is independently
accepted, while native multi-joint creation and Upper Arm drag/Revert are pending.
The authority increment is integrated at `118df34`; main app build and focused
`assembly_coupled` pass (1/1, 1.27 seconds).

The actual independently authored four-part fixture now follows the source order:
arm Revolute, nested piston Cylindrical, two end-eye Revolutes, then Lower Pin.
Disconnected temporary gauges disappear when the end relationships connect the
parts. Only the arm hinge drives the connected mechanism; passive coordinates
are inspected rather than silently prescribed at zero.

Core and the 32-call real-app API workflow verify sampled -15/0/+15 degree
poses. The API independently calculates the Upper mounting point by rotating
about the authored hinge, checks both transformed piston mounting axes, and
compares measured telescoping travel with `180 - mounting-axis distance`.
At -15/+15 degrees the slide is approximately -21.9717/+25.0127 mm. All twelve
Lower pose entries remain pinned; original joint residuals pass, previews leave
authored state unchanged, Revert restores every leaf, and committed angle
history/save/reopen pass. This establishes sampled kinematics, not a collision
sweep, stroke/contact limits, or general mobility rank.

External evidence is in
`../Alloy-reference-records/deliverables/piston-coupled-2026-10-14/`:
`core-01/` and `api-01/` contain the independently reviewed receipts. The API
capture shows all four named joints and a visibly tilted, telescoping piston.
Public source provenance and declared adapted dimensions remain in the external
piston tutorial knowledge directory. Copied nested components remain ASM-016.

A complete API creation recording is in external `from-blank-01/` and linked
from the gallery's `#piston-coupled` entry. One real application creates the four
parts, converts/nests them, edits shaft radius from 4 to 4.2 mm, reopens that same
model, creates the joints in order and runs the coupled assertions. Both chained
workflow receipts pass. The 188.5-second recording has 2,827 frames, 2,536 repeats
and zero capture failures; creation, moving and reopened frames were inspected.
It preserves computation delays and qualifies small sketch details; it is not
physical pointer evidence, a frame-rate benchmark or the copied-piston gate.
