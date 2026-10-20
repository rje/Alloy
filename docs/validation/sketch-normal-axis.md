# Normal construction Line in an XY sketch

SKT-187/#710, 2026-10-20. This is the bounded reference-axis prerequisite for
sketch Rotate (#703), not a Rotate command or general 3D Sketch implementation.
Root owns continuation-context/backlog integration and independent acceptance.

Public provenance is external in
`../Alloy-reference-records/knowledge/specs/skt180-sketch-rotate-2026-10-20.md`
(R2, retrieved 2026-10-20). The public same-sketch instructions enable 3D Sketch
and draw a 3D Line for the rotation axis. They distinguish it from an in-plane
point. This supports the workflow prerequisite, not exact visual/default parity
for Alloy's restricted controls. Marker art, colors, numeric height default,
minimum height, coordinate bounds and dedicated edit/Delete policy below are
Alloy choices. No proprietary application was operated.

## Model and controls

An active XY sketch exposes a visible 3D Sketch checkbox and Create > Normal
Construction Line (3D). The canvas picks the start/pivot with 0.1 mm snapping;
Pivot X/Y and +Z Height fields provide exact values in sketch units. OK authors
one Line; Cancel, Escape, dock close and session cancellation discard the fields.
The input form itself does not author a geometry preview. Both Finish Sketch
and command.commit refuse while that form is open. Undo first closes open input
without changing history. Reopen the same Create command to edit the existing
Line or use its Delete Line button. The mode is session-local and initially off.
It explicitly supports only one normal construction Line.

`parameters.constructionLines3d` is a separate authored collection. Each Line
has a stable canonical UUID and separately identified start/end points with
local `uMm/vMm/wMm`. Start is `(u,v,0)` and end is `(u,v,h)`, toward +Z;
height is at least 0.001 mm and finite coordinates are bounded by ±100000 mm.
Dedicated edits preserve all three IDs. Invalid zero/negative height,
off-normal endpoints, malformed/duplicate/foreign identities, extra fields,
second Lines and out-of-bounds values refuse atomically. Existing global
sketch-ID validation also rejects collisions with planar records.

The collection is removed from the PlaneGCS input, then restored unchanged
after solving the ordinary planar source. It adds no solver variables, planar
points/entities, profile boundaries or Extrude regions. Rendering adds a real
3D segment and a seven logical pixel ring/cross marker at its start, with a
3D +Z label. A plain click in 3D Sketch mode routes the actual marker hit to the
shared axis-pick command and selects its Line ID, separately from planar entity
selection. Turning mode off clears the reference selection; ordinary 2D picking
does not acquire it. Rendering is currently active-sketch decoration; general
finished-sketch reference presentation remains outside this prerequisite.

Ordinary 2D Delete and point editing cannot mutate these IDs. Generic
feature.edit refuses replacing an existing collection; use the dedicated
commands. Dedicated edit/Delete refuse an axis whose Line or endpoint IDs are
referenced elsewhere in its sketch parameters. The future-owner refusal is
covered synthetically; no Rotate owner is implemented by this issue.

Local commands (discoverable through the normal command API):

- `sketch.3dMode {enabled}` enables the limited XY mode.
- `sketch.axis3d {start:{u,v,w},end:{u,v,w},units?,axisId?}` creates or edits;
  pass the existing Line ID to edit. Endpoint IDs are generated/read-only.
- `sketch.axis3dInspect {sketchId?}` returns coordinates, all three IDs,
  construction role, direction `[0,0,1]`, length, `referenceOnly=true`,
  `solverVariables=0` and `profileBoundary=false`.
- `sketch.axis3dStart {axisId?}`, `sketch.axis3dPivot {x,y}`,
  `sketch.axis3dPick {x,y}`, `sketch.axis3dCancel` share native control authority.
- `sketch.axis3dDelete {axisId}` removes the unreferenced Line atomically.

## Evidence

External root: `../Alloy-reference-records/validation/skt187-2026-10-20/`.
Accepted build: `build-preview-release`, Release, private Qt 6.11.2/OCCT 7.9.3,
Apple Clang, `-j4`. No CI or push.

- `sketch_axis3d_test` passes: exact planar source/DOF/profile preservation,
  stable three-ID edit, idempotent solve, normal/minimum/range/duplicate/foreign
  validation, 2D Delete/point-edit refusal, synthetic dependent-owner Delete
  refusal, exact document Undo/Redo/save/reopen.
- `native-final.log`, `native-final/native-report.json`: 3/3 Qt functions pass
  in 6.509 s. Actual checkbox indicator, Create menu/action, canvas pivot,
  numeric fields, OK/Cancel/Escape, mode-off ordinary picking, mode-on marker
  picking, existing-axis edit and Delete all use the real hit widget. Initial
  activation is separate; no interstep focus/selection repair. All three IDs,
  planar arrays, sketch/document history, profile/body IDs and reopen compare
  exactly. `normal-line-authoring.png`, `end-on-axis-picked.png` and
  `oblique-real-axis-segment.png` were inspected at native size. Initial capture
  exposed incorrect marker world scaling; the final capture uses camera scale
  and a readable ring/cross with separated label.
- `api-final/report.json` and 99-call transcript run through the real app:
  inspect, dedicated same-ID coordinate edit, source preservation, invalid
  input cases including empty/foreign axis ID, generic feature-edit ownership refusal, structured atomic refusal, both
  finish guards, API Cancel/accept cleanup, session cancel, Delete, history and
  exact authored reopen. An early harness-only run assumed absent constraints
  were an array; the final test compares optional records without that assumption.
- The asymmetric planar loop `(12,6),(32,10),(27,22),(9,18)` has one 244 mm²
  profile. The native axis is `(4,3,0)..(4,3,10)` mm; the later API edit is
  `(5,-2,0)..(5,-2,15)` with unchanged IDs. Both leave the same 8 mm Extrude
  body and profile references, and volume 1952 mm³. Independent binary STL
  readback before/after returns identical metrics: one watertight consistently
  wound component, 12 triangles, Euler 2, bounds `(9,6,0)..(32,22,8)`, signed
  volume exactly 1952 mm³. The reference Line contributes no exported facets.
- API hardware receipt: Apple / Apple M4, OpenGL 4.1 Metal - 90.5, hardware
  accelerated, 4x MSAA, 2x resolution ratio, native 1188×608 and framebuffer
  2376×1216. The final native marker, real oblique segment and separate finished
  solid captures were visually inspected. This is prototype OCCT OpenGL
  evidence, not renderer migration or Windows/Linux qualification.
- Focused existing Copy, atomic Move and retained Move core regressions pass.
  No broad repeated command sweep was performed.

Reproduce with the two CMake targets above and:

```sh
ALLOY_AXIS3D_EVIDENCE=/absolute/evidence/native ./build-preview-release/sketch_axis3d_controls_test
python3 tests/sketch_axis3d_api.py --app build-preview-release/alloy.app/Contents/MacOS/alloy --native-evidence /absolute/evidence/native --output /absolute/evidence/api
```

This matches the documented limited axis prerequisite. Exact public control
styling/defaults and general 3D Sketch remain unknown/unimplemented; tilted
axes, multiple 3D Lines and Rotate are not claimed. Private validation editors
were closed; the user-owned PID 72618 was preserved.

Independent Sol review accepted `e6a8e6c` after checking the separate schema,
solver/profile exclusion, native actual-hit controls and final captures. It
independently re-read both binary STLs and confirmed unchanged geometry. The
review was read-only and did not claim a new build or app run. Physical input,
finished-sketch axis display, general 3D Sketch and Rotate remain open.
