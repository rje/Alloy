# Alloy orientation-cube drag (NAV-011)

Public navigation evidence is indexed at
`../Alloy-reference-records/knowledge/specs/viewport-navigation-compatibility-2026-10-14.md`.
The source documents cube dragging to orbit and excludes the custom orbit center
from cube navigation. Its exact drag threshold, sensitivity, Free/Constrained
mode rule and cancel pose are not specified. Alloy uses Qt's drag distance,
the currently selected orbit mode and the ordinary view target as the cube
drag pivot. The separate stored custom orbit center remains available to
Shift-Orbit. Escape ends a cube drag at the current camera pose.

The cube holds the freshly hit face, edge or corner at left press. Release
before the drag threshold sends that region to the shared `view.cubeOrient`
command. After crossing the threshold, pointer deltas go through shared
`view.orbit` with `pivot: "viewCenter"`; release does not apply a final
orientation click. Matching move/release and double-click events are consumed
before sketch/model handlers. Losing the button or window clears the transient
gesture. No authored geometry, selection or history is changed by orbit.

The focused production-theme Qt fixture
[`navigation_view_cube_controls.cpp`](../../tests/navigation_view_cube_controls.cpp)
passes 3/3 in
`../Alloy-reference-records/deliverables/viewport-navigation-2026-10-14/nav011-qt-03.log`.
It uses actual delivered Qt press/move/release on the detected cube, verifies
click versus drag, motion before release and no release snap, preserved custom
pivot/ordinary view target, camera refresh and tab restoration, plus unchanged
authored data, undo and selection. A subsequent ordinary shared orbit moves
around the retained custom pivot, demonstrating the two pivot policies. The
GPU capture `nav011-qt-03/cube-dragged.png` shows the asymmetric solid and
oriented cube after drag. The companion `viewport-geometry.json` records the
792×520 logical/native ReferenceView at DPR 2 within a 1360×850 window;
broader layout investigation remains REL-013.

An attempted Escape key-input assertion in qt02 stopped because the test
window had no active focus widget after tab switching. The fixture did not
repair focus between steps. Escape and interrupted-release handling are
source-reviewed, not accepted as naturally focused native key input. Physical
mouse/trackpad and packaged-runtime qualification remain NAV-008/NAV-009;
cube accessories remain NAV-012.
