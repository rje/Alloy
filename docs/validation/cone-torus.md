# Cone and torus New Body primitives

SOL-062/#507, 2026-10-19. The dated clean-room command record is external at
`../Alloy-reference-records/knowledge/specs/sol-062-cone-torus-2026-10-19.md`.
It distinguishes the publicly documented Torus workflow from Alloy's explicit
major/minor-radius sizing and the proposed Cone convenience command. No source
documents an exact matching Cone primitive or exact Torus diameter mapping.

`solid.cone` and `solid.torus` now use the same retained primitive command,
preview, editor and document path as Box and Cylinder. The bounded New Body
subset creates a +Z circular cone/frustum with base/top diameters, height and
numeric center, or a full ring torus with major/minor radii and numeric center.
Zero top diameter makes a pointed cone; invalid/nonfinite sizes and a torus
whose major radius does not exceed its minor radius refuse before mutation.
Both have discoverable Create menu entries, explicit numeric panels and
independently authored icons. The Torus panel explains Alloy's radius meaning;
the inspected corrected capture shows complete fields, labels and action
buttons without clipping. A final reopened GPU capture shows the dedicated
Cone/Torus Timeline icons and the updated Browser guidance.

The focused kernel test passes analytic validity, bounds and volumes. A
60-call real-application API run passes preview/Cancel, stable IDs, editable
dimensions, a downstream Move after an upstream Cone height edit, two atomic
refusals, Undo/Redo, inch conversion and exact authored/evaluated save/reopen.
The Cone frustum and Torus were exported separately to STL. Independent mesh
readback found one closed connected component each, Euler characteristics 2
and 0 respectively, expected bounds and positive volume within 1.5% of the
analytic solids. The inspected real-app captures show smooth GPU geometry and
readable controls on an Apple M4 with an OCCT OpenGL framebuffer; Metal-RHI
qualification remains a separate release requirement. Evidence is external at
`../Alloy-reference-records/validation/cone-torus-2026-10-19/`, including
`report.json`, the editable `.alloy`, both STLs, panel/final captures and the
original native attempt log.

The focused Qt native-input test was added, but its two bounded attempts stopped
at initial private-window activation **before** any click or key. It now reports
an explicit skip in that environment rather than giving a false pass.
[SOL-077/#606](https://github.com/rje/Alloy/issues/606) retains native Create,
field, Preview/Commit/Cancel and Timeline-edit input qualification after the
[FND-028 active-window prerequisite](https://github.com/rje/Alloy/issues/589).
This acceptance covers the shared command and API/GPU result; it does not
establish physical input, exact source styling, plane picking, torus Position
or operation modes, arbitrary axes, named cone/torus face topology, or other
platform runtime behavior. Those remain on their owners and long-term roadmap.
