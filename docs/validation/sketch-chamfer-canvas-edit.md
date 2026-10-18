# SKT-113: sketch Chamfer canvas handle and dimension edit

Accepted 2026-10-18 for Equal Distance Chamfer. The dated public workflow and
figure provenance remains outside the repository at
`../Alloy-reference-records/knowledge/specs/skt-096-sketch-chamfer-2026-10-18.md`.
Its public tutorial shows a canvas manipulator and the help page describes
distance entry. Exact drag scale, release semantics and style are not
established by that evidence; the behavior below is Alloy's policy.

A cyan handle at the diagonal midpoint follows the source corner. Dragging
along its guide previews a new equal setback without changing authored
geometry; release publishes one draft history entry. Escape discards the
preview. The first on-canvas distance label opens the shared inline dimension
editor and drives the retained distance. The second label is parenthesized and
read-only because the equal setback derives it. Palette entry continues to
use the same `sketch.chamfer` state and geometry.

Evidence from Release `build-preview-release`, binary SHA-256
`5ca39137bd65a13597e10ee144479fd9c9fdb07a611ad44c7988a607a514e264`:

- Focused core/native Qt tests passed 2/2 in 12.90 s. The native test routed
  pointer press/move/release through the actual viewport hit target for
  creation and existing-corner drags. It checked live palette/label values,
  one authored history entry, exact Undo/Redo, Escape rollback, invalid
  out-of-span refusal, inline first-label focus/edit, second-label read-only
  behavior, source dimension edits and native save/reopen.
- The edited 160×80 mm rectangle retained its Chamfer identity and 12 mm
  setback, yielding one closed region of 12,728 mm². Existing
  [extrusion/STL validation](sketch-equal-chamfer.md) covers this profile's
  printable route. The native fixture and state/capture evidence are at
  `../Alloy-reference-records/validation/2026-10-18-sketch-chamfer-handle/native/`.
- Actual captures were inspected for the cyan handle/dashed guide, live
  numeric preview, readable inline editor and visibly driven second value.
  The app reported Apple M4 hardware GPU, OCCT OpenGL 4.1 Metal and a 2×
  framebuffer (1864×1232 native for a 932×616 logical viewport).

The native test is an automated Qt pointer path. Physical hand input,
Windows/Linux runtime and exact matched reference styling remain unqualified.
Two Distance and Distance And Angle modes remain SKT-110/#534 and
SKT-111/#535.
