# Separate Fit and Home navigation (NAV-013)

Public navigation evidence is indexed at
`../Alloy-reference-records/knowledge/specs/viewport-navigation-compatibility-2026-10-14.md`.
It distinguishes Fit, which frames visible model geometry without changing
the viewing direction, from the configured Home view. F6 is documented for Fit.
Exact treatment of hidden and transient geometry remains unspecified by the
public source; Alloy frames displayed canvases, visible bodies and sketches,
or the current valid Extrude/Combine preview. Home retains Alloy's existing
isometric home-pose policy.

The shared `view.fit` command and visible **Fit** control use the same
preview-aware body selection as `view.home`, but preserve the current camera
direction/up and stored orbit pivot. Both reset zoom, pan and view target to
frame the eligible visible geometry. A window-scoped F6 shortcut invokes
`view.fit`; its binding and tooltip were source/Qt-inspected.

The focused production-theme Qt fixture
[`navigation_fit_controls.cpp`](../../tests/navigation_fit_controls.cpp)
passes 3/3 in
`../Alloy-reference-records/deliverables/viewport-navigation-2026-10-14/nav013-qt-01.log`.
Shared API setup creates an asymmetric box and a distant sketch. An actual Qt
Fit button click retains a custom orientation while resetting pan/zoom; an
actual Home click restores isometric. During a valid 80 mm Extrude preview,
another Fit click frames both the original box and distant preview, leaves the
preview active and keeps authored data unchanged. The earlier solid Fit also
checks the undo state unchanged. The actual GPU
captures `nav013-qt-01/fit-custom-pose.png` and
`nav013-qt-01/fit-pending-preview.png` show those states; the saved fixture is
alongside them.

The fixture checks the F6 shortcut object/key/context, but the Qt window did
not establish natural keyboard focus for a native F6 input claim. Physical
input remains NAV-008/NAV-009. Visible Pan/Zoom/Zoom Window, selected-object
framing and planar Look At remain NAV-014/NAV-015, while broader Fit cases
remain under the NAV-005 parent.
