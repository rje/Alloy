# Browser body mesh export — DOC-036/#517

Accepted 2026-10-17 on local macOS arm64 `build-preview-release/alloy.app`.
This is the remaining bounded native entry path after the
[exchange-sheet](native-exchange-sheet.md) and
[File-menu](native-exchange-menu.md) gates. One private editor was closed after
the check.

The Browser body menu used blocking `QMenu::exec`, which prevented the
computer-control tool from selecting a second popup action before its first
right-click call returned. The body menu now uses a nonblocking `QMenu::popup`,
as the viewport body menu already did. Selection, action ID, and cancellation
remain observable through the shared viewport state; the chosen command is
dispatched after the popup closes.

In the rebuilt real app, native Cmd+O loaded the previously checked bracket
STEP as one solid. A native right-click on its actual Browser body row opened
the body-specific menu with enabled `context.exportMesh`. A subsequent native
pointer click selected that action. The actual **Save As Mesh** sheet showed
that same body, binary STL, millimeters, and the default refinement values.
Its macOS Save chooser wrote `browser-body-export.stl`; the app reported
`STL saved.` No local-control call opened the menu or export sheet.

An independent stdlib STL reader found 1,864 triangles in one closed,
consistently oriented component, bounds approximately `[-50, 0, 0]` to
`[868.19806, 438.90872, 50]` mm, and mesh volume `4,694,842.9792` mm³.
The source exact STEP volume is `4,694,718.448386` mm³; the roughly `0.00265%`
difference is tessellation error. The full Alloy document object, including
authored features and undo stack, was structurally identical before and after
export; document revision stayed 2. The focused Qt/GPU pointer test also
opened the same body menu, selected the body in the sheet, canceled with
the sheet's Cancel button, and confirmed unchanged document state.

External evidence is in
`../Alloy-reference-records/validation/doc036-browser-mesh-2026-10-17/`:
pre/post API state, independent STL readback, STL, GPU framebuffer capture, and
private editor quit result. The custom Qt popup is not included in the bound
main-window accessibility tree or screenshot, so its exact visual styling was
not independently compared. The native action result and opened sheet are
observable. Physical mouse/trackpad delivery, other operating systems, and
reference visual parity remain outside this bounded gate.
