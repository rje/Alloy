# Distance And Angle solid Chamfer — SOL-089 / #627

Bounded core: one retained straight extrusion `endRim` with two perpendicular
incident planar faces. The feature stores one selected edge, an exact incident
`referenceFace`, `distanceMm` and `angleDeg`. Distance is measured on the
reference face; the other face is the direction side. The face is an explicit
retained input, so a later edit does not infer direction from screen position.

The angle convention is an **Alloy policy**, not a verified reference-product
datum: on this perpendicular fixture, the opposite setback is
`distanceMm / tan(angleDeg)`. The OCCT `AddDA` angle is complemented internally
to meet this declared convention. A 45° angle gives equal side setbacks.
Public help documents Distance And Angle but gives no factory values or current
face-picking gesture; it documents Flip only for Two Distance. The dock exposes
an explicit Reference Face selector and does not claim exact interaction parity.

## Evidence

- Release `build-preview-release`, Qt 6.11.2 and OCCT 7.9.3 on Apple arm64;
  local build used `-j4`. Focused `solid_distance_angle_chamfer`,
  `solid_two_distance_chamfer` and `solid_chamfer` tests pass (3/3).
- Independent 133-call real-app fixture checks exact 30×20×10 mm stock,
  2 mm/30° bevel with top/front setbacks 2 and 3.464101615 mm, remaining
  volume 5896.07695155 mm³, reversed reference-face assignment and 45°
  datum. Cancel leaves authored and Undo state unchanged.
- A real unchanged-back-face 2 mm diameter, 1 mm blind Cut remains through
  independent distance and angle edits and an upstream 30→36 mm source edit.
  Zero/negative/90°/120° angles, zero/negative or impossible distance and a
  nonincident face refuse atomically; valid correction remains possible.
  Commit/edit/source Undo/Redo and authored/evaluated save/reopen pass.
- The final exact body is one valid closed solid, bounds
  [0,0,0]..[36,20,10] mm, volume 7072.15074920 mm³. An independent binary
  STL parser reports one closed component, 592 triangles, Euler characteristic
  2, matching bounds and 7072.15173953 mm³ mesh volume.
- The first real-app attempt exposed a missing numeric schema bound that
  rejected a valid angle before geometry evaluation. Commit `9855e33`
  corrected that discovery boundary; the full fixture then passed.

Implementation commits are `035e256` and `9855e33`. The fixture is
`tests/solid_distance_angle_chamfer_api.py`; model, STL, transcript, app log,
hashes and independent review are outside git at
`../Alloy-reference-records/validation/solid-distance-angle-chamfer-2026-10-20/`.
The dated clean-room source and uncertainty record is external at
`knowledge/specs/sol089-distance-angle-solid-chamfer-2026-10-20.md`.
No proprietary runtime was used.

## Remaining acceptance

The macOS console is locked, so actual-hit native edge/reference selection,
field focus, physical input and 1×/2× GPU presentation are **not accepted**.
SOL-096 tracks those first-v1 checks. Exact native angle datum, defaults,
reference-face gesture and matched styling remain unknown. Tangent chains,
generated-edge naming, multi-edge corners and broader topology remain SOL-005;
Windows/Linux packaged runtime belongs to release gates. API/capture completion
does not substitute for native interaction or visual evidence.
