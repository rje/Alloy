# Shared As-Built Joint selection checkpoint

ASM-023/#373 was independently accepted at `de69dcc` (source `35e798f`),
then reopened by the cumulative gate: the focused test targeted the tree viewport
beneath an embedded component label, bypassing the actual child widget. That
child currently activates rather than picking a participant. A narrow routing
fix and real-child input check are in progress; the evidence below is qualified.
One Assemble entrypoint opens a shared Type choice for Revolute/Cylindrical and
Pick controls for Component 1/2. Browser and viewport routes dispatch the same
occurrence selection command. Type changes retain both exact occurrence IDs and
the active session, clear incompatible Position/coordinates, and revalidate.
The default Revolute type is an explicit Alloy choice; the public reference
default is unknown. Existing typed APIs and the separate Slider route remain.

The controls are top aligned. Position labels include their axial station;
full coordinates remain available in tooltips. New sessions omit absent optional
values rather than treating a null joint ID as an edit. Position data accepts
both Qt JSON variant representations. Failed previews clear the previous draft
and invalidate the displayed pose.

## Evidence and limits

Native15 passes 3/3 in 4.24 seconds: Qt-delivered Pick buttons and actual Browser
occurrence rows, native Type popup selection (including Revolute to Cylindrical
and back), native OK for both joint types, and API undo restoring the input.
The focused test supplies Position through shared `command.update` from retained
selector data. It does not establish a native Position popup or viewport gesture;
[joint-origin evidence](joint-origin-inference.md) covers the latter separately.
Browser picks are runtime verified; the additional viewport participant-pick
route is source reviewed. Both captures were independently inspected: readable
names, Type and Position, correct blue origin geometry and enabled OK.

Release app/test builds and the integrated main application build pass. The complete native tutorial remains TUT-019;
there is no physical OS input, generic joint catalogue or matched-pixel claim.
External receipt: `deliverables/as-built-joint-native-2026-10-14/receipt.md`.
Exact accepted output/captures: `native-15/` beneath that directory.
