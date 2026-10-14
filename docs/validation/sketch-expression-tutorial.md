# Short sketch expression tutorial

This gate follows the [audited short exercise](../research/sketch-expression-exercise-contract.md) from a blank design: rough circle and rectangle, three-click Center to Center Slot, explicit side Horizontal/Vertical, D45 and R5 with Dimension remaining active, then the connected polygon with Perpendicular and Equal. Pointer-created bottom/right Lines retain automatic H/V constraints. The vertical dimension is d3 = 100; direct `d3 + 50` gives 150. Editing the driver to 120 gives 170 while preserving alignment and the UUID dependency.

Native13 passes the complete workflow. The final document contains four real closed profiles: the rough circle, rectangle, slot and polygon. The polygon area is 17,400 mm² and the slot is 528.539816 mm². Actual Qt input creates all geometry and enters/edits dimensions; shared API calls configure the initial viewport, inspect state, check undo/redo and save/reopen. These API checks are captioned separately from the public source steps. Rough coordinates, metric entry and the120-mm edit are fixture choices, not source measurements.

The macOS test harness opens the existing command menu with nonblocking `QMenu::popup()`; menu-item selection, pointer movement, geometry picks and keys are delivered through actual Qt widgets. Synchronous toolbar opening stalls the test event loop. This is qualified Qt-input evidence, not OS-delivered pointer or toolbar-opening parity. The application and model use the shared authoritative commands; the fixture neither seeds geometry nor injects H/V constraints.

The original native05 exposed missing automatic alignment: formula values updated, but the polygon tilted. SKT-084 corrects that behavior; the test retains the original orientation/area requirements. The temporary origin-lock duplication found during implementation was removed; the focused regression now includes an actual origin snap.

Recording02 is independently accepted: **110.133 seconds**, 1652 encoded frames at 15 fps, 1125 repeated frames and zero capture failures. Raw timing is preserved; the encoded rate is not a claim of 15 distinct captures per second. One transient pre-Line pointer-preview refusal is logged; the full creation sequence and final checks pass. The fixed header provenance label leaves the **H preview cue visible at 58 s** and literal **`d3 + 50` editor text visible at 97 s**. Root and Astra inspected those actual movie frames. Recording01 remains archived; its cursor-relative label obscured those controls, corrected under FND-026.

The video, model, event sidecar, screenshots and compact receipt are in
`../Alloy-reference-records/deliverables/sketch-expression-exercise-2026-10-14/recording-02/`.
The external gallery entry is `demos/2026-10-13/index.html#sketch-expressions`;
all 134 gallery file links resolve. The source-step ledger sits beside the recording folders.

Native13's GPU capture shows readable 100/150 and 120/170 annotations, 45/R5 slot, shaded profiles and H/V, Perpendicular and Equal badges. The source screenshots have unknown viewport size, theme, OS and scale, so no pixel-equivalent comparison is claimed. Control/Command inference suppression is source-reviewed but not separately demonstrated through native input. The lesson stops before its later deletion instruction; no extrusion or fully constrained whole-sketch claim is made.
