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

The initial focused Qt native-input attempts stopped at private-window activation
**before** any click or key and reported an explicit skip. After the
[FND-028 active-window prerequisite](https://github.com/rje/Alloy/issues/589),
[SOL-077/#606](https://github.com/rje/Alloy/issues/606) completed the native
Create, field, Preview/Commit/Cancel and Timeline-edit qualification below.
This acceptance covers the shared command and API/GPU result; it does not
establish physical input, exact source styling, plane picking, torus Position
or operation modes, arbitrary axes, named cone/torus face topology, or other
platform runtime behavior. Those remain on their owners and long-term roadmap.

## SOL-077 accepted native input qualification, 2026-10-19

After the FND-028 unlocked-session prerequisite, the expanded
`responsive_controls_test coneTorusPrimitivePanels` passes at a 1360 × 850
logical window on both DPR 2 and DPR 1 (the latter uses `QT_SCALE_FACTOR=0.5`
on this Retina display). Both runs report 3/3 Qt checks with no skip. The
DPR 2 run also passes `primitiveDockActionsStayReachable` for Box/Cylinder at
normal and minimum window sizes, for 4/4 checks total. The Release target was
built in `build-preview-release` with `-j4`; no CI ran.

Initial native AppKit application/key-window/first-responder observations agree
with Qt application/active-window/focus-window state. After that one harness
activation, actual-hit Create menu rows open `solid.cone` and `solid.torus`.
Both routes exercise creation Cancel, editable numeric fields and Return,
explicit Preview and OK, then actual Timeline buttons, edit Cancel and edit OK.
Popup activation/closure, numeric focus ownership and fully visible numeric
field bounds are asserted; no focus or selection is repaired between steps.
Return naturally leaves focus on the spinbox or its editor. Cancel preserves
the complete prior inspection; preview preserves committed data. Both edits
retain feature/body IDs. Shared API Undo/Redo and exact authored/evaluated
save/reopen run on the native-authored document. Final valid closed-solid
volumes match the independently calculated frustum and ring formulas.

Capture inspection found that full-range numeric size hints could clip field
borders in the fixed-width primitive form. Name and numeric controls now accept
the form's available width. The final DPR 1 and DPR 2 panel captures show
complete field borders, readable units and action buttons; the Torus sizing
explanation remains explicit. The native Apple M4 OCCT OpenGL framebuffer
(`Apple`, `4.1 Metal - 90.5`, four-sample MSAA) shows smooth solids, blue preview
and the dedicated Timeline icons. The normal/high-DPI final runs, captures,
`.alloy` saves and structured focus/state/GPU receipts are external under
`../Alloy-reference-records/validation/cone-torus-native-2026-10-19/`, in
`final-normal-dpr1/` and `final-retina/`. The Box/Cylinder regression captures
are in `box-cylinder-regression/`.

This is Qt-delivered native widget input, not physical OS event injection.
Undo/Redo, persistence and camera framing use the shared API. DPR 1 is a Qt
scale-factor check on the same display, not another monitor. Public evidence
supports the Torus Create route, ring silhouette and general Timeline editing;
exact panel styling, Cone command parity and Torus radius-label mapping remain
unknown. The earlier plane-pick/manipulator/Position/operation-mode and
other-platform limits remain. No user-owned editor was running or modified;
all private test editors closed. Independent Sol review inspected the source diff, structured DPR 1/2 receipts, ten captures and saved models and found no blocker in this bounded native Qt slice.

The unmodified formerly skipped baseline reached its final unsaved-close
confirmation in this unlocked session; its bounded private process was stopped
and its sample retained. The expanded test saves before closure. An intermediate
run rejected an overly strict child-editor focus expectation; corrected final
runs accept the observed owning spinbox focus. These attempts remain separate
from the final acceptance artifacts.
