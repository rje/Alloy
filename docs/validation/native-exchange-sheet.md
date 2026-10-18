# Native exchange sheets — DOC-016/#167

Accepted 2026-10-17 on the local macOS arm64 `build-preview-release/alloy.app`
candidate. This is a bounded real-application check of native file sheets and
their output. The prior [assessment](native-exchange-assessment.md) stopped at
a computer-control window-binding failure; this run bound the app and completed
the File Open path. The private editor was closed after validation.

Using native computer input, Cmd+O opened **Open From Computer**. The macOS
Go to Folder sheet selected the previously independently checked bracket STEP
fixture. Open produced a new dirty tab with one visible solid, a Browser body
and a STEP Import timeline feature. The GPU capture is `imported-body.png` in
the external evidence directory below. This validates a native STEP opening;
the imported design is an embedded STEP feature, not proof of its original
parametric construction history.

The STEP Export and Save As Mesh sheets were opened through Alloy's shared
local control API because computer-control clicks did not activate the custom
File/Browser menus. **Inside those actual app sheets**, native input selected
`Inches (in)` for STEP and `0.05 mm` Surface Deviation / `5°` Normal Deviation
for binary STL (the defaults were `0.1 mm` / `10°`). The native Save choosers
wrote `bracket-inch-native.step` and `bracket-refined-native.stl`. A separate
mesh chooser named `cancelled-mesh-native.stl` was canceled, then its parent
sheet canceled. The target did not exist afterward. An earlier STEP Save
chooser was also canceled without creating output.

Independent `step_fixture read` reported one valid STEP solid with `INCH` file
units, bounds `[-50, 0, 0]` to `[868.19805, 438.90873, 50]` mm and volume
`4,694,718.448386` mm³. Its mm source has the same bounds and volume to the
reader's tolerance. The independent STL reader reported 3,636 triangles, one
closed oriented component with aligned finite normals, the same bounds to
float precision, and mesh volume `4,694,743.175` mm³. Mesh and exact STEP
volumes differ by about `0.00053%`; this is tessellation error, not an exact
volume claim for STL.

The full document object, including authored state and undo stack, matched
exactly before export, after both exports and after the named Cancel. Revision
stayed 2. Only transient focus/UI/mesh-export and viewport state changed.
`cancel-check.json` records target absence and state equality.

External evidence:
`../Alloy-reference-records/validation/doc016-native-exchange-2026-10-17/`
contains source/output hashes, API states, independent readbacks, the Cancel
check, and the imported-body capture. The STEP output SHA-256 begins
`7296fbea`; STL begins `94a6fa03`. Prior translator and default-setting checks
remain in [STEP exchange](step-exchange.md) and [bracket exchange](bracket-exchange.md).

**Limit:** a complete physical File/Browser-menu-to-export chain was not
observed. [DOC-035/#516](https://github.com/rje/Alloy/issues/516) retains this
Todo gate. The menu behavior may be a computer-control limitation or an app
defect; the present evidence does not distinguish them. No other-platform
native runtime, original bracket modeling history or visual reference parity
is claimed here. Astra independently checked the readbacks and state boundary.
