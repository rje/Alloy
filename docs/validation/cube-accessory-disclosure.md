# Orientation-cube accessory disclosure

NAV-021/#452 addresses the 1040×640 sketch-canvas occlusion recorded in the
[modeling-panel readability audit](modeling-panel-readability-audit.md). Public
reference evidence did not establish the target application's accessory hover
or focus behavior, so this is an explicit Alloy disclosure policy, not an
exact-parity claim. The compact source record is stored outside the repository
at [the NAV-021 reference record](../../../Alloy-reference-records/knowledge/specs/orientation-cube-accessory-disclosure-2026-10-14.md).

The orientation cube stays visible. Its Home, adjacent-view, roll and up-axis
controls stay collapsed at rest and appear while the pointer is over the cube
or the controls, or while keyboard focus is in that group. A persistent,
keyboard-focusable `cube.accessories.toggle` disclosure opens the group and
moves focus to its first command. Escape closes the group and returns focus to
the disclosure control. The hidden accessory grid does not itself trigger
disclosure, so sketch work in its former footprint remains unobstructed.

The focused Qt gate is `NavigationCubeAccessoriesControls::accessoryDisclosureSmallSketch`.
It uses the authored dimensioned-rectangle fixture, records collapsed-state
captures at 1360×850 and 1040×640, checks cube hover and the actual Qt hit owners
for the disclosure and Home button, checks Shift+Tab/Space/Escape, and verifies
the active sketch session, authored data and undo state remain unchanged.
The final no-trace Release run passed. Its [test log](../../../Alloy-reference-records/deliverables/nav021-accessory-disclosure-2026-10-14/audit-04/nav021-qt-run.log),
[1360×850 capture](../../../Alloy-reference-records/deliverables/nav021-accessory-disclosure-2026-10-14/audit-04/collapsed-1360x850.png),
and [1040×640 capture](../../../Alloy-reference-records/deliverables/nav021-accessory-disclosure-2026-10-14/audit-04/collapsed-1040x640.png)
are stored with the external evidence. The final run includes the stale-hover
guard and relocated disclosure. Earlier diagnostic and inconsistent candidate
logs remain preserved in the same deliverables folder. These checks establish
local Qt presentation and command routing only; they do not establish physical
input or other-platform behavior.
