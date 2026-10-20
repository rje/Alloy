# Retained Rotate source-size composition

SKT-188/#711, 2026-10-20. Numeric edits to existing, unbound internal driving
point-to-point Linear dimensions now resize the retained XY four-Line Rotate
source, then derive its same-ID output once. Root owns independent review,
continuation-context/backlog integration and the adjacent Copy/Paste Todo.

The architecture contract is external in
`../Alloy-reference-records/knowledge/specs/skt188-rotate-source-size-2026-10-20.md`.
Public provenance remains the R1/R2/R4 normal-Line/Rotate/constraint evidence in
`skt180-sketch-rotate-2026-10-20.md` (external, retrieved 2026-10-20). Source-size
composition, persistent source orientation, sketch Timeline ownership and exact
visual behavior remain unknown in that public evidence. This is an explicit
Alloy editing policy, preserving the accepted normal +Z axis route. No
proprietary application was operated or captured.

## Authority and supported edit

Use the existing dimension label double-click/inline editor or
`sketch.dimensionUpdate {dimensionId,value,units}` in the active sketch. Typing
an existing dimension value does not preview changed geometry; Enter publishes
the solved value, and Escape cancels exactly. Impossible geometry leaves the
editor open with an error, preserving source/cache/snapshots/history. This reuses
the existing native route rather than adding another editable point model.

An unframed owner gains a version-1 `sourceFrame` only in a supported size-edit
transaction. Opening, evaluating or saving an unchanged unframed document does
not migrate it. The frame records the first retained topology edge's start as
anchor, its adjacent end as heading point, the fixed source anchor coordinates
and the directed heading angle in radians. It is captured from authoritative
pre-Rotate `sourcePoints`, never the rotated cache. It uses existing IDs and
stays unchanged through later size, angle and helper-pivot edits.

A small optional typed `SketchSolver::SourceOrientation` adds PlaneGCS's existing
`addConstraintP2PAngle` rule transiently, with only the anchor point locked.
The heading point's distance remains free to change. The gauge adds no authored
constraint, dimension, point or Line. Existing solver callers keep their old
default behavior. The size builder restores source space, changes only the
requested numeric driver, solves with this pose, checks unselected records,
anchor/heading, finite bounds, prior signed-area orientation and continuous
source-triangle branches, then updates `sourcePoints` and its dimension snapshot together. Angle/helper IDs and
all relation definitions remain unchanged. It refuses an ambiguous, reflected,
flipped, collapsed or self-crossing branch instead of finding an unrelated one.

Materialization applies `pivot + R(angle)*(updatedSource-pivot)` once. Ordinary
framed evaluation uses the same source gauge and rejects material source drift
instead of discarding the solve. It never updates source coordinates itself.
The existing output-space residual check runs without the source gauge/anchor
lock. Source/rigid checks use 1e-6 mm and heading checks use 1e-8 radians. Full
downstream evaluation precedes publication of a source-size edit.

Snapshot validation stays strict. Only this builder changes the accepted size
snapshot and source together. New/deleted dimensions, parameter bindings,
expressions, raw point/owner edits and raw model-table dimension changes remain
unsupported/refused; driven measurements cannot drive size. Source-size edits
also refuse while Rotate, Move/Copy or Paste input is active. The accepted
owner, helper, curve, point, profile and downstream body identities persist.
Existing XY/normal-axis eligibility, numeric bounds and external/fixed/placement
ownership refusals remain in force. Broader 3D modes and Timeline parity are not
claimed.

## Evidence

External root: `../Alloy-reference-records/validation/skt188-2026-10-20/`.
Build: existing `build-preview-release`, Release/private Qt 6.11.2 and OCCT
7.9.3, Apple Clang, `-j4`. No new build tree, CI or push.

- `sketch_rotate_source_size_test` passes analytic source/output vertices,
  anchor/heading, stable IDs, a legacy owner gaining its frame only on edit,
  exact unselected point preservation, repeated exact JSON idempotence, angle
  and helper edits after resize, impossible lengths, unsupported bindings/
  expressions, snapshot/source/frame/cache tampering, document history/reopen.
- `native-final.log` / `native-final/native-report.json`: 3/3 Qt functions pass
  in 9.368 s. Actual dimension-label double-click goes to the real hit widget;
  focus produced by that command receives input. Typing 22 then Escape leaves
  the unframed owner/history exact. Entering impossible 1 mm refuses; correcting
  to 22 succeeds in the same editor. Native 3D Sketch/Create/helper controls
  change the pivot to (6,5), preserving the entire Rotate group. Sketch/document
  Undo/Redo and authored reopen compare exactly. Initial activation is separate;
  there is no interstep focus/selection repair. Captures wait 30 ms for paint
  settlement and were inspected at native size, including clean successful
  controls after the refusal. `policy-capture/rotate-source-size-policy.png`
  also confirms the updated existing Edit Rotate guidance is readable. The fixture uses existing +30 Rotate authority;
  its selection workflow is independently accepted under #703.
- `api-final/report.json` and its 98-call transcript verify legacy exact
  round-trip, independent analytic 22 mm source/output and mesh, a later 23 mm
  resize, bound/expression/model-table/raw-snapshot refusal, active Rotate/Move/
  Paste source-size guards, unchanged source frame through −15 angle and pivot
  edits, same body/profile lineage, history/reopen and a second checked STL.
- The first side changes from sqrt(416) to 22 mm, retaining the other three
  side lengths and P0–P2 diagonal. Independent circle intersections give
  P0=(12,6), P1=(33.5727748652,10.3145549730),
  P2=(27.3080799650,21.7054986481), P3=(9.2337506509,18.0560302147) mm.
  Area is 250.3818169699 mm² and the same 8 mm Extrude is
  2003.0545357589 mm³. The frame holds P0 and heading atan2(4,20), while
  +30 about pivot (4,3) has bounds
  (1.0045459134,9.5980762114)..(25.9534968071,30.8534770022) mm.
  Pivot (6,5) preserves that source and moves bounds to
  (2.2724951058,8.8660254038)..(27.2214459996,30.1214261946) mm.
- Independent binary STL readback for native 22/+30/pivot (6,5) is one
  watertight, consistently wound component, 12 triangles, Euler 2 and
  2003.05443166 mm³. The later 23/−15/pivot (4,3) source has analytic area
  253.3096820038 mm² and volume 2026.47745603 mm³; its STL is likewise one
  watertight component/12 triangles/Euler 2, volume 2026.47748556 mm³ and bounds
  approximately (12.503863,2.346961,0)..(35.456181,16.179890,8) mm.
  Checks use 3e-6 mm float32 bound tolerance and 0.001 mm³ volume tolerance.
- Apple / Apple M4, OpenGL 4.1 Metal - 90.5, hardware accelerated, 4x MSAA,
  2x ratio, native 1188×608 / framebuffer 2376×1216. Source editor, resized
  profile/pivot and separate finished solids were inspected. This is OCCT
  prototype OpenGL evidence, not renderer migration, physical-input or
  Windows/Linux runtime qualification.
- Focused existing Rotate, normal-axis, Copy, atomic Move and retained Move
  core regressions pass, as does accepted Rotate native input. The existing
  expression-editor native regression requires its evidence environment
  variable; an initial invocation stopped before opening an editor because that
  variable was absent. With the required environment it passes all three
  Qt checks (4.566 s); the corrected log is retained separately.

An adjacent test found that Create Copy of an ordinary distant rectangle in a
sketch containing an unrelated retained Rotate refuses `Invalid Rotate source
identity`, atomically. Copy's isolated-profile evaluation retains owner metadata
whose points were filtered away. Successful unrelated-owner Copy/Paste
coexistence is not claimed or fixed here; root will preserve a small Todo for
safe metadata isolation and Copy/Paste preview/history/source preservation.
The final API test records that refusal and uses an exact Undo to clean up its
clipboard fixture. Earlier harness setup used a clipboard from another sketch
and then assumed Delete removed an empty optional constraints key; these were
fixture assumptions, not source-size solver failures. Failure logs are retained.

Reproduce via the two new CMake test targets and:

```sh
ALLOY_ROTATE_SOURCE_SIZE_EVIDENCE=/absolute/evidence/native ./build-preview-release/sketch_rotate_source_size_controls_test
python3 tests/sketch_rotate_source_size_api.py --app build-preview-release/alloy.app/Contents/MacOS/alloy --native-evidence /absolute/evidence/native --output /absolute/evidence/api
```

Private fixture editors were closed; the user-owned PID 72618 was preserved.
