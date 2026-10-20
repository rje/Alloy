# One-edge Normal Ruled flange — SRF-030 / #633

The accepted bounded core makes one editable open sheet from one retained
straight boundary edge of a planar face. The authored source records its exact
body, producer feature, face and edge IDs. `Normal` at 0° follows the source
face's oriented normal; Alloy defines positive angle toward the source
normal × oriented edge tangent. Distance is the positive ruling length. This
signed side and the 20 mm / 0° defaults are explicit Alloy policies; public
sources do not establish their exact reference values.

The Surface menu opens a dock with a canvas edge-pick route and numeric
Distance/Angle fields. Its actual Qt pointer delivery and the documented
canvas distance/angle handles remain SRF-044. The result retains exact four
boundary descendants, one valid planar face, no caps or solids, and stable
identities through source edits and history.

## Evidence

- Implementation `9fac810` built the focused `surface_ruled_test` and app in
  Release with `-j4`, Qt 6.11.2 and OCCT 7.9.3. The focused test passes exact
  prism lineage, signed angles including 120° and 210°, invalid/stale/shared
  input refusal, upstream source edit, Undo/Redo and reopen.
- Independent fixture `0518fd3` ran 96 shared real-app API responses. A fresh
  empty dialog refused preview; `selection.pick` at the projected source edge
  filled the exact four-part reference and previewed the default. A 40×30 mm
  planar Patch with a 40 mm retained bottom edge produced one valid 800 mm²
  open sheet at distance 20 mm for 0°, ±30°, 90°, 120° and 210°, with exact
  analytic bounds and endpoints. A source width edit 40→45 mm recomputed to
  900 mm² without losing face/edge identities. Cancel, invalid inputs,
  stale-reference recovery, Undo/Redo and save/reopen passed.
- A separate probe reopened all seven app-authored models and inspected actual
  OCCT B-reps: planar face type, validity, one face/four straight boundaries,
  zero solids, area, bounds and each endpoint pair. Serialized source B-rep
  hashes remain identical across all six angle variants. Apple M4 hardware
  OpenGL 4.1 / DPR2 preview and reopened captures were inspected: the blue
  selected edge, distinct sheet and numeric fields are readable.

The fixture is `tests/surface_ruled_api.py`. Independent findings, transcript,
saved models, B-rep probe and captures live outside git under
`../Alloy-reference-records/validation/surface-ruled-2026-10-20/`. The dated
public command/tutorial evidence and visual references are outside git under
`../Alloy-reference-records/knowledge/specs/srf030-ruled-surface-2026-10-20.md`
and `knowledge/visuals/srf030/`. No proprietary runtime or CI was used.

## Remaining acceptance

The Mac console is locked. Physical actual-hit edge pick, field typing,
focus/Cancel/Escape, 1×/2× GPU interaction, canvas distance/angle handles and
matched visual comparison remain SRF-044. Public evidence supports a Normal
120° example and the numeric dialog families; it does not settle the positive
angle side, native defaults or numeric limits. Tangent/Direction rules, chains,
multiple edges, shared/curved sources, mitering and broader recovery remain
SRF-004. Windows/Linux packaged runtime remains a release gate.
