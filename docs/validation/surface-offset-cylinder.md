# Retained cylindrical Surface Offset — SRF-042 / #665

The accepted bounded core offsets one genuine retained partial-cylinder Surface
Revolve face into a separate open sheet. The signed distance follows the
selected face's evaluated oriented normal. It preserves exact source-face and
four source-edge lineage while the source radius and height change. An inward
distance that reaches or passes zero radius is refused before OCCT can create
a degenerate or inside-out result; the structured `surface_offset_collapse`
diagnostic includes the source face and attempted radius.

The shared command and compiled dock offer numeric distance entry. The curved
canvas normal handle is deliberately absent because the previous planar cue
would point from a centroid inside the cylinder. Native curved picking/drag
and actual-hit acceptance remain SRF-043; planar native work remains SRF-041.

## Evidence

- Implementation `941229b` and independent API fixture `320bf55` compile in
  Release with `-j4`, Qt 6.11.2 and OCCT 7.9.3. Focused `surface_offset_test`
  passes the new cylindrical and prior planar cases, including reversed actual
  face orientation, zero/near-collapse offsets, stable IDs, source edit,
  atomic refusal/retry, Undo/Redo and reopen.
- Independent 71-response real-app fixture uses a retained 120° R10/H30 mm
  Surface Revolve face. Signed 0/+2/−4/−9 mm produces radii 10/12/6/1 mm
  and areas 200π/240π/120π/20π mm². Each output has one valid open face,
  four edges and zero solids; source geometry remains unchanged. A source
  edit R10/H30→R12/H40 with a −9 mm offset recomputes to R3/H40 and 80π mm²,
  preserving source and output face/edge identities.
- Fresh and committed −10/−11 mm candidates, failed edit update/preview/
  commit and valid retry check authored state, revision, Undo and evaluated
  geometry. The collapse UI shows a readable source-specific error, disables
  OK and displays no invalid candidate or curved planar cue.
- A separate probe reopens all five app-authored models and inspects their
  actual B-reps with OCCT: analytic cylinder radius and 120° span, exact area,
  face/edge/solid counts, validity, oriented normal and serialized B-rep hash.
  The source hash is identical across the four distance variants. Three
  Apple M4 hardware OpenGL 4.1 / DPR2 application captures were inspected.

The fixture is `tests/surface_offset_cylinder_api.py`. Independent findings,
model files, transcript, B-rep probe and captures live outside git under
`../Alloy-reference-records/validation/surface-offset-cylinder-2026-10-20/`.
The dated public-source contract and unresolved reference behavior are in
`../Alloy-reference-records/knowledge/specs/srf042-cylindrical-offset-collapse-2026-10-20.md`.
No proprietary runtime or CI was used.

## Remaining acceptance

The Mac console is locked. Physical canvas face selection, signed normal
drag, keyboard flow and 1×/2× actual-hit checks remain SRF-043; the planar
native gate remains SRF-041. No cited public reference shows the cylindrical
collapse state, so exact visual/error parity is unknown. This is one retained
partial cylinder, not general curved/multiface offset, chained curved sheets,
arbitrary self-intersection repair or Windows/Linux package qualification;
those remain SRF-004 and release-gate work.
