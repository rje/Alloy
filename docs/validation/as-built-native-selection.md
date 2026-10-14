# Shared As-Built Joint selection checkpoint

ASM-023/#373 is independently accepted through `16d6e9c` (source `7057725`),
which corrects actual embedded Browser-label selection after the cumulative gate
found the first focused test bypassed that child. The initial shared dialog is
`de69dcc` (source `35e798f`).
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

Native17 passes 3/3 in 6.61 seconds: Qt-delivered Pick buttons and actual embedded
component-label controls, exact occurrence selection with unchanged active owner,
and native Type popup selection (including Revolute to Cylindrical
and back), native OK for both joint types, and API undo restoring the input.
The focused test supplies Position through shared `command.update` from retained
selector data. It does not establish a native Position popup or viewport gesture;
[joint-origin evidence](joint-origin-inference.md) covers the latter separately.
Label clicks queue the participant command until the widget release finishes,
checking the original tab/session/command/role/revision before applying it. This
avoids refreshing away the radio inside its own callback. Native17 replaces
Native15 Browser-pick proof, which targeted the underlying tree viewport.
Browser picks are runtime verified; the additional viewport participant-pick
route is source reviewed. Both captures were independently inspected: readable
names, Type and Position, correct blue origin geometry and enabled OK.

Release app/test builds and the integrated main application build pass. The complete native tutorial remains TUT-019;
there is no physical OS input, generic joint catalogue or matched-pixel claim.
External receipt: `deliverables/as-built-joint-native-2026-10-14/receipt.md`.
Exact accepted output/captures: `native-17/` beneath that directory.
