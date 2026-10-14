# ASM-015 coupled piston UI acceptance

Independently accepted and integrated at `105961e`; main app build passes.

The bounded UI adds new Revolute rows through the existing As-Built Joint dock,
stages a complete solved assembly pose from a Qt-delivered UpperArm pointer drag,
and exposes Revert Position through the same local command authority. The drag
is enabled only for the source-shaped one-island, pinned 3-Revolute/1-Cylindrical
graph with one visible moving-arm body. It sends an exact driver angle to
`joint.previewAngle`; no body transform is authored by the gesture.

The graph authority is the independently reviewed `118df34` integrated increment. Its
focused `assembly_coupled` core test passes the actual four-part fixture,
including signed ±15° drive, coupled piston travel, full Pin, history and
reopen. The [cumulative actual-app API evidence](../../../Alloy-reference-records/deliverables/piston-coupled-2026-10-14/api-01/)
was produced by the root workflow lane and passes source-order 3R+1C+Pin,
analytical mount/travel checks at ±15°, Revert, history and reopen.

The [focused native evidence](../../../Alloy-reference-records/deliverables/piston-coupled-ui-2026-10-14/native-01/)
contains the passing Qt log and inspected capture. The test opens the accepted
two-joint fixture, starts a new Revolute dock by shared command, sets the two
component combo indices and retained circular Position programmatically, and
clicks native OK. It verifies the existing two joint rows remain exact and a
third is added. It then opens the complete saved mechanism and delivers actual
Qt press/move/release to the visible UpperArm body. Three occurrence world poses
change; the authored document remains exact. Shared `position.revert` restores
all occurrence poses, with no history edit. The observed one-event drag command
took 223 ms on this validation machine; this is not a continuous frame-rate
measurement. The final no-motion click fallback was source/build checked after
the capture, so its click selection behavior is not claimed as native evidence.

The native test's initial window activation is harness setup. It does not
repair focus between the demonstrated pointer steps and checks viewport focus
after press. The component selection and Revert in that test are shared-command
or programmatic control actions, not physical dropdown/Revert clicks. General
assembly dragging, playback, linked piston copy, broad joint graphs, and
Windows/Linux runtime behavior remain outside this acceptance. Drag latency is tracked in REL-010/#367.
