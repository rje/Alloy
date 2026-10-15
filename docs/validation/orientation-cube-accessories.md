# Orientation cube accessories (NAV-012)

Public navigation evidence is indexed at
`../Alloy-reference-records/knowledge/specs/viewport-navigation-compatibility-2026-10-14.md`.
The target-product evidence documents cube Home. Related public cube guidance
corroborates adjacent-face and roll controls, without establishing their exact
behavior in the target product. The exact button layout, turn sign and
enabled-state details remain unknown. Alloy
places seven accessible Qt buttons beside its existing GPU cube. Home invokes
the existing `view.home` isometric fit. Four adjacent and two roll buttons use
the shared `view.cubeAdjacent` and `view.cubeRoll` commands. The latter six are
enabled only when the current camera has a principal-axis face direction and
cardinal up direction, including 90° rolled face poses.

Adjacent controls rotate direction and up together by 90° about the current
screen right/up axis. Roll rotates up 90° about the current viewing axis. Both
preserve pan, zoom, ordinary view target and stored custom orbit pivot. They
leave authored geometry and history unchanged. An invalid command returns
`unsupported_view`. The accessory QWidget is a live child of ReferenceView;
`capture.screenshot` composes that live control after the OCCT framebuffer so
the saved image matches its visible window presentation.

The focused production-theme Qt fixture
[`navigation_cube_accessories_controls.cpp`](../../tests/navigation_cube_accessories_controls.cpp)
passes 3/3 in
`../Alloy-reference-records/deliverables/viewport-navigation-2026-10-14/nav012-qt-03.log`.
Shared API creates an asymmetric box, sets Top and establishes pan/zoom. Actual
Qt button clicks roll, turn to an adjacent face, and invoke Home. Assertions
check retained pan/zoom and default orbit pivot, face-only button state, unchanged
authored/undo data, a body hit outside the controls, and the actual Home button
as `QApplication::widgetAt` recipient. The GPU captures
`nav012-qt-03/cube-face-accessories.png` and
`nav012-qt-03/cube-rolled-adjacent.png` show the readable controls and cube.
The box-only layout fills the central area; the previously observed broad
right-side blank band remains fixture-dependent REL-013 work.

This is one current-host Qt/GPU workflow. Physical pointer, DPI variants and
broader accessibility/runtime qualification remain NAV-008/NAV-009 and REL-013.
Nondefault custom-pivot retention and already-selected-object preservation are
source-reviewed here; this fixture does not construct those states.
