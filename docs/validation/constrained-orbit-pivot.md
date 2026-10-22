# Constrained orbit and selected center (NAV-002)

The source-linked navigation contract is in
`../Alloy-reference-records/knowledge/specs/viewport-navigation-compatibility-2026-10-14.md`.
It distinguishes Free from Constrained Orbit and documents an explicit Set/Reset
Orbit Center workflow. Exact mode default, drag sensitivity, selected-point
filter and constrained tilt behavior are not established by that evidence.

Alloy now retains a visible Free/Constrained choice for Shift-middle drag and
the checked left-drag Orbit control. Constrained mode uses world Z as vertical,
keeps a level horizon and clamps the camera five degrees short of either pole.
The blank-canvas context menu starts a one-shot nearest visible body-surface
pick. A green marker identifies the custom pivot; Reset restores the fitted
scene center. Body-stage previews refuse pivot selection so a hidden rest body
cannot be chosen. Free-to-Constrained leveling, subsequent orbit, tab restore,
named views and Home use separate persistent camera target and pivot values.
Home keeps an explicitly picked pivot; named fixed views recenter their target
on the fitted scene. These are explicit Alloy policies. Sketch/model input,
authored geometry, selection and undo history are unchanged by navigation.

The focused shared-command/GPU run at
`../Alloy-reference-records/deliverables/viewport-navigation-2026-10-14/nav002-api-05.log`
passes 3/3. It creates an off-center 40 × 20 × 12 mm solid through the shared
API, ray-picks its actual visible face, and checks pivot-screen stationarity
after pan through Free-to-Constrained leveling and a constrained orbit. It also
checks projected-Z up, per-tab camera restoration, named-view recenter, Reset, unchanged authored data and
undo journal, and saves an editable model. The production-theme capture
`nav002-api-05/api-constrained-pivot.png` shows a readable Constrained Orbit
control, solid GPU presentation and green pivot marker.
An affected-control capture from the saved model at
`nav002-indicator-01/single-mode-indicator.png` verifies one dropdown arrow
after removing the duplicate literal glyph; no geometry workflow was repeated.

The separate native Qt control fixture stopped at the initial macOS
`isActiveWindow()` check twice, before delivering any navigation gesture. A
bundled-app desktop-control attempt then failed to bind the owned window with
`cgWindowNotFound` before any navigation input. Its binding receipt is under
`../Alloy-reference-records/deliverables/viewport-navigation-2026-10-14/nav002-cua-01/`.
Neither failure establishes a product input defect. Actual menu, pointer-pick,
Escape and Reset input behavior remains the separate [NAV-009 native gate](https://github.com/rje/Alloy/issues/428). The API
test does not prove OS-device, trackpad, ViewCube, configurable up-axis or
cross-platform behavior.

The 2026-10-21 [strict native retry](native-orbit-pivot.md) now passes the
bounded Qt Free/Constrained menu and visible left-drag/Orbit-tool Escape prefix
with genuine AppKit agreement. It stops before Set at undelivered PID-directed
OS right-click and lost editor focus; #428 remains open.
