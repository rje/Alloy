# Owned Qt popup capture candidate

FND-017 #179 follows the native straw-bung recording. Native right press/release
and a Qt context event arrived; `contextMenuVisible` became true, while the
composed capture and accessibility client did not expose the menu, and attempted
keyboard selection did not activate Move. The journal contains no native
Down/Return delivery during that interval. This distinguishes a real popup
capture defect from an unproved command-wiring or Qt accessibility defect.

Local publicly licensed Qt6.11.2 source establishes the coordinate contract:
`QMenuPrivate::popup` sets top-level geometry in global screen coordinates;
`QWidget::mapTo` walks QObject-widget parents adding their rectangles, unlike
`mapToGlobal` which accounts for the native window. Applying parent-chain mapping
to a top-level popup adds the viewport/browser offset incorrectly. Alloy now
composes owned top-level surfaces at popup global origin minus main-window global
origin. OCCT framebuffer replacement and ordinary child overlays are unchanged.
See Qt's [widget mapping API](https://doc.qt.io/qt-6/qwidget.html#mapTo).

Body menus now record actual origin, stable target, available actions, open/closed
phase and selected/cancelled result. `app.state.ui.ownedPopups` reports real visible
Qt surfaces, their global/window-local bounds, former parent-mapped bounds,
active/native-surface flags and actual QAction rectangles. This is inspection of
the UI objects, not a separate automation selection model. Both menus continue to
invoke the shared Move command with the selected stable body identity.

Candidate build passes. The focused QWidget regression
`popupCaptureUsesGlobalCoordinates` is compiled but not run while another native
workflow owns the desktop. It compares the distinct mappings and checks actual
captured pixels at a colored popup fixture. Actual native menu visibility and
Move activation are still pending; no general popup/AX compatibility claim or
upstream Qt change is made.
