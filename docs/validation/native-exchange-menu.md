# File-menu export entry — DOC-035/#516

Accepted 2026-10-17 on the same macOS arm64 local candidate as the
[native exchange-sheet check](native-exchange-sheet.md). One private editor
loaded the accepted bracket STEP through Cmd+O and was closed afterward.

Alloy's custom `QMenu` popup did not appear in the main-window accessibility
tree or main-window screenshot. Native pointer input nevertheless selected
**File > Export…** and **File > Save As Mesh…**. Each opened the expected
actual app sheet with the imported body selected, with no local-control call
to open those sheets. The mesh sheet was canceled back to the design. The
File-menu STEP sheet then saved `file-menu-export.step` through the macOS
chooser. This is a menu-to-native-chooser-to-output path, not a simulated
command invocation.

The standalone OCCT reader found one valid millimeter STEP solid with bounds
`[-50, 0, 0]` to `[868.19805, 438.90873, 50]` mm and volume
`4,694,718.448386` mm³, matching the imported source. The full Alloy
document object, including authored state and undo stack, was structurally identical
before and after export; revision stayed 2. A native sheet Cancel and the
unchanged document were observed. The prior [DOC-016 receipt](native-exchange-sheet.md)
has a named canceled-output absence check and independent inch STEP/refined
STL results.

External evidence is in
`../Alloy-reference-records/validation/doc035-native-menu-2026-10-17/`:
`pre-export-state.json`, `post-export-state.json`,
`independent-step-readback.json`, `file-menu-export.step`, and the private
editor quit result. The interaction observations are recorded here; the
custom popup itself was not captured by the main-window screenshot API.

The body Browser path was subsequently accepted under
[DOC-036/#517](native-browser-mesh.md) after a bounded nonblocking menu change.
This File-menu result alone does not establish hardware mouse, other-platform,
or reference visual parity.
