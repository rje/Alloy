# Three-Point Rectangle command record

Public input contract reviewed 2026-10-17. Dated provenance, creator links,
video timestamps and visual references are kept outside the repository in
`../Alloy-reference-records/knowledge/specs/skt-094-rectangles-2026-10-17.md`.
The documented operation is P1 corner, P2 first-edge endpoint/direction, then
P3 perpendicular second-side distance and side. Cursor positions can provide
rough distances; first and second distances may be typed and locked with Tab.
The third click completes a closed four-edge profile. The tool stays active for
another rectangle; Escape exits. Public demonstrations describe opposite-edge
Parallel relations and a Perpendicular corner. They do not establish exact
current snap priority, modern glyph positions, numeric angle entry, or a
dedicated post-creation rectangle editor.

| Aspect | Public contract | Alloy target for SKT-094 |
| --- | --- | --- |
| Entry | Active sketch > Create > Rectangle > 3-Point Rectangle | Sketch menu action and `sketch.tool` |
| Selection | No geometry preselection required | Three sequential canvas points |
| Preview | Rotated outline follows P2 and P3; inline distances | Disposable GPU edge outline and two-stage distance field |
| Commit/cancel | P3 completes; repeats; Escape exits | One authored four-edge profile per three clicks; pending preview can be canceled |
| Constraints | Opposite edges Parallel, corner Perpendicular | Solver relations with editable driving lengths when typed |
| History/edit | Ordinary sketch geometry/dimensions | Shared sketch authority, dimension edit, Undo/Redo, save/reopen |

The 30/15 mm lengths in the public demonstration are example values, not
defaults. Exact appearance comparison uses the external old-version frames
with unmatched viewport/theme/scale noted. Other rectangle variants and a
separate Parallelogram mode were examined under SKT-106; no parity claim is
made for a distinct Parallelogram command.

The [bounded implementation receipt](../../validation/sketch-three-point-rectangle.md)
records the native, solver, source-edit, GPU and independent export evidence.

## SKT-106 disposition (2026-10-18)

Current public command help and tool indexes enumerate only the 2-Point,
3-Point and Center Rectangle variants; the public line-constructor inventory
also lacks a separate Parallelogram method. Official instructions demonstrate
closed Line geometry and editable Parallel constraints. An independent public
tutorial corroborates the three-variant Rectangle family, and a community
example describes a parameter-driven parallelogram sketch without naming a
dedicated creation command. The dated source-by-source evidence is appended
to the external SKT-094 knowledge record.

The bounded compatibility disposition is **no separate documented target
Parallelogram creation mode**. A shape made with ordinary Lines and Parallel
constraints remains a valid user workflow; the exact complete recipe is an
inference from the documented primitives. A dedicated Alloy constructor could
be considered as an Alloy-specific usability extension, but is not a target
parity requirement or v1 blocker. Future public evidence can reopen this
assessment.
