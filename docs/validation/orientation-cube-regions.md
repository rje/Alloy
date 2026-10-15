# Alloy orientation cube regions (NAV-010)

The public navigation evidence and installed OCCT API handoff are indexed at
`../Alloy-reference-records/knowledge/specs/viewport-navigation-compatibility-2026-10-14.md`.
The public training material shows labeled cube faces/corners, drag and Home;
related public guidance describes edge regions. Exact edge, adjacent and roll
behavior in the primary workflow remains unknown. This is an Alloy-branded
navigation control built from OCCT's interactive cube, not a copied asset.

Alloy displays a transform-persistent GPU cube in the viewport's upper-right
corner. Its six faces, twelve edges and eight corners have stable region names,
hover highlighting and shared `view.cubeOrient` camera actions. OCCT's default
camera animation is disabled. `view.cubeHit` checks the fresh exact cube owner;
a matching click is consumed before sketch/model selection, as are its release
and double-click. The resulting Alloy camera state retains pan, zoom, ordinary
view target and an explicitly stored orbit pivot. Cube orientation uses the
ordinary view target, so later Shift-Orbit can still use the custom pivot.
This target/framing behavior and cube colors, labels and size are Alloy policies.

The focused production-theme Qt fixture
[`navigation_view_cube_controls.cpp`](../../tests/navigation_view_cube_controls.cpp)
passes 3/3 in
`../Alloy-reference-records/deliverables/viewport-navigation-2026-10-14/nav010-qt-06.log`.
It creates an asymmetric editable solid through the shared API, checks all 26
region commands produce distinct finite camera directions, and uses actual Qt
clicks on currently detected representative face, edge and corner regions.
It checks prior pan/zoom/custom pivot, selection, authored data, revision and
undo remain unchanged. A directly delivered Qt move demonstrates a pointing
cursor and blue face highlight. GPU captures
`nav010-qt-06/cube-face-hover.png` and `nav010-qt-06/cube-after-corner.png`
show readable cube labels, model and pivot; the saved model is alongside them.
The initial 48-pixel cube had crowded labels; the final 80-pixel presentation
was captured and reviewed. All 26 orientations were checked through the shared
API; the Qt input check covers three representative regions on one display
scale and one solid, not 26 physical clicks. The Qt fixture is not a physical
mouse or packaged application input claim. Sketch-mode, resize and broader
display-scale behavior remain unverified here.

Cube drag is NAV-011; distinct Home, adjacent-view and face-roll controls are
NAV-012. Full high-DPI/platform and native-input qualification stays in the
existing navigation/readiness gates. Neither this capture nor the coincident
region count proves exact reference styling or projection policy.
