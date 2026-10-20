# Typed Angle and Cylinder Height parameters — PAR-009/#497

Status: bounded local candidate, 2026-10-20; independent review pending.

The external source/specification is
`../Alloy-reference-records/knowledge/specs/par009-angle-solid-2026-10-20.md`.
It records creator, URLs, retrieval dates and documented/inferred/unknown behavior.
Public evidence documents typed angle parameters, compatible named field inputs,
propagation and a Cylinder Height field. This specific direct Angle/Cylinder
binding pairing, stable UUID storage, numeric replacement and atomic refusal are
Alloy policies/inferences. No proprietary application was operated. Exact field
styling, suggestion controls and physical commit keys remain unknown.

The bounded implementation adds numeric `deg` user rows with stable UUID and
`valueDeg`; existing mm/in and length formulas retain their format. Driving
line-pair Angle dimensions bind `parameterId`; Cylinder Height binds
`heightParameterId`. Shared commands accept current names or stable UUIDs.
Native Change Parameters can create degree rows, the inline dimension editor
accepts an angle name, and the Cylinder Height control accepts a length name.
The grouped model table displays typed Expression/Value/Source and edits both
new fields. Untouched edit acceptance retains bindings; a numeric literal removes
them even at the current value. Renames follow the retained ID. Wrong parameter
types, bad values, missing references and failed downstream geometry refuse
atomically. Length-only expression and lookup paths reject or exclude degree
rows, preventing missing `valueMm` from becoming an accidental zero length.

The fixture is an independently authored radius10 mm cylinder with a closed
three-line cutter at the origin. Two radial lines retain length30 mm, one is
horizontal, and their driving Angle follows Opening30 deg. A 30 mm Extrude Cut
removes the sector through the stock. Opening30→60 and Height10→15 exercise both
upstream dependencies in one printable model. Remaining exact volume is
`(pi - radians(angle)/2) * 10^2 * height`: 2879.793265791, then
2617.993877991, then 3926.990816987 mm³. This is an Alloy fixture, not a claim to
reproduce a public tutorial file.

Evidence is external under
`../Alloy-reference-records/validation/typed-angle-cylinder-2026-10-20/`:

- Release build uses `build-preview-release`, private Qt6.11.2 and OCCT7.9.3,
  AppleClang17 arm64, `--parallel 4`. No CI/tag/publish.
- `typed_angle_cylinder_parameters` core checks exact solid volume, source
  edits/renames, wrong-type direct IDs and formulas, unchanged authored/revision
  on refusal, history, reopen, numeric unbinding and later source independence.
- Existing `named_length_parameters` and `model_parameter_table` core regressions
  pass. The latter's dangling temporary Qt JSON test locals were converted to
  owned values; production behavior was not changed for that test repair.
- `tests/typed_angle_cylinder_parameters_api.py` passed76 real-app calls, including
  bound edit/Return, source edits/renames, two-way mismatch refusal, history,
  save/reopen, direct UUID binding, numeric replacement and later independence.
  `api/geometry.json`, `api/transcript.json` and `api/report.json` retain evidence.
- Independent stdlib STL readback finds one closed consistently oriented
  component,488 triangles, Euler characteristic2, Z0..15 mm and signed volume
  3925.765347252 mm³ (0.032% below the exact curved solid). Export uses
  surfaceDeviation0.01 mm/normalDeviation5°. The saved bound model and STL are
  `api/typed-wedge.alloy` and `api/typed-wedge.stl`.
- Actual app `api/wedge-cylinder.png` and `api/typed-parameter-table.png` were
  inspected: smooth gray cylinder with deliberate wedge and readable typed rows.
  `api/state.json` records Apple M4, vendor Apple, OpenGL4.1 Metal90.5,
  GLSL4.10, DPR2 and native GPU framebuffer. Existing prototype renderer remains
  OCCT OpenGL; this is not RHI migration or other-platform evidence.

`typed_angle_cylinder_parameters_controls` passes two native workflows (4/4
QtTest init/workflow/cleanup cases): degree parameter creation through the owned
Change Parameters form, and Cylinder Height name binding/edit/reopen/numeric
replacement plus Angle inline name binding/unchanged Return/numeric replacement.
Only initial harness activation is explicit; each inline step verifies the focus
produced by the command, with no inter-step focus or selection repair. Native
parameter-dialog creation and modeling input use separate fresh private windows;
closure activation between those workflows is not claimed. The Unit combo is set
as test setup; real OS popup selection is not claimed. `ctest` focused core and
native result is4/4 targets in7.97 seconds. All private editors exit. Physical
OS input, exact visual parity, other angular/solid fields, degree arithmetic,
changing a parameter across dimensions and Windows/Linux packaged runtime remain
outside this candidate. A source angle at/above180° can exist as a user value,
but binding it to the supported line-pair Angle refuses its geometry range.


## Compact workflow record

Inputs are degree user `value`, length user `value`/entry unit, a selected driving
line-pair Angle or Cylinder Height, and a direct compatible name/UUID. Existing
primitive defaults remain radius5/height10 mm; Angle retains its measured numeric
value until explicit entry. No parameter suggestions or new canvas-selection
behavior is added. The native Cylinder uses its existing disposable preview,
Preview/OK/Cancel session; the sketch inline editor uses Enter accept/Escape
cancel. Binding acceptance solves the draft or complete model before publication.
Model table targets retain feature/dimension identity. Edit reopens the current
name; source rename follows the UUID. Numeric acceptance unbinds; history restores
the prior binding. Save/reopen retains authored IDs and regenerates geometry.
The retained API fixture proves the target behaviors; unchanged primitive preview
and Escape routing reuse existing controls and are not newly qualified here.

Compatibility result: **matched** for the documented typed parameter distinction,
reference propagation and separate Expression/Value presentation within this
bounded pairing. **Unknown** for exact reference field style, suggestions,
default degree unit, unbind gesture and mismatch text. Native Qt input, analytic
solid evidence and STL readback are separate receipts; no broad exact-match
claim follows from the captures. Independent source/evidence review remains with
root before issue closure.
