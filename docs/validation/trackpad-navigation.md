# Qt-delivered trackpad navigation (NAV-003)

The public input contract and its source links are in
`../Alloy-reference-records/knowledge/specs/viewport-navigation-compatibility-2026-10-14.md`.
The documented gestures include two-finger pan, Shift plus two-finger orbit,
and pinch zoom on a Mac touchpad. Exact physical delivery depends on platform,
device and settings, so this child validates Alloy's Qt event routes rather
than claiming a hardware gesture.

Alloy now routes touchpad pixel-wheel deltas to shared Pan or Shift-Orbit,
ignoring the angle channel of that same event. Mouse-wheel angle deltas zoom
about the cursor. Native pinch and pan events use the same shared camera
commands; Shift plus native pan orbits. During a native Begin/End gesture,
interleaved touchpad wheel events are suppressed. Deactivation, focus loss or
hide clears that transient ownership. Unidentified pixel-only vertical wheel
events remain a zoom fallback. `view.zoom` accepts an optional logical-pixel
anchor; omitted coordinates retain its earlier camera-target and pan behavior.

The focused production-theme Qt event fixture
[`navigation_trackpad_controls.cpp`](../../tests/navigation_trackpad_controls.cpp)
passes 3/3 at
`../Alloy-reference-records/deliverables/viewport-navigation-2026-10-14/nav003-qt-05.log`.
It exercises each route, cursor stationarity after prior pan,
unchanged authored/revision/undo state, selection preservation and an actual
post-navigation viewport body click. The asymmetrical solid is rendered by the
GPU in `nav003-qt-05/navigation-after-gestures.png`; the fixture saves its
editable model in the same directory. Setup and zoom calls use the shared API;
wheel, gesture and final click inputs are Qt-delivered synthetic events.
The zoom-limit handling and final nonfinite native-anchor guard were reviewed
in source; this fixture does not reach a zoom bound.

Physical Mac touchpad, Windows Precision Touchpad, Linux gestures and native
gesture/wheel delivery combinations remain unverified. This receipt does not
establish source-exact gesture sensitivity, OS focus behavior or cross-platform
runtime support.
