# One-face planar Surface Offset — SRF-029 / #632

This accepted bounded core creates a new independent sheet at a signed
distance from one exact retained **planar surface face**. Positive distance
follows the face's oriented normal; negative reverses it; zero creates a
coincident new sheet. The source body remains unchanged. The feature stores
the full source `{bodyId,producerFeatureId,faceId}` tuple and derives exact
translated face/edge identities when the source recomputes.

Public Surface > Create > Offset describes signed distance and face selection;
this is distinct from solid Offset Face. The basic dock has a typed signed
field, one-face selector and a visible +N cue. Canvas face picking and a
draggable normal handle remain SRF-041. A planar face cannot fold; the
subsequent retained partial-cylinder collapse case is recorded under
[SRF-042](surface-offset-cylinder.md).

## Evidence

- Implementation `ac73359` plus exact stale-error diagnostic correction
  `06a44ac` built in Release at `-j4` with Qt 6.11.2 and OCCT 7.9.3.
  Focused `surface_offset_test` passes, including reversed actual face
  orientation, valid zero, invalid/nonfinite/unsupported source refusal,
  source edit, Undo/Redo and reopen.
- Independent 84-response real-app fixture starts with a 40×30 mm planar
  Patch at z=20, normal +Z. Offset +5 gives z=25, −2 gives z=18, zero gives
  z=20. Each sheet has area 1200 mm², exact boundary and one face/zero
  solids/null volume. The source sheet remains unchanged. Editing source
  width 40→50 produces area 1500 mm² in source, Offset and a downstream
  +3 sheet at z=21, with retained IDs and zero boundary deviation.
- Preview/Cancel, signed edit, missing/curved source refusal, valid retry,
  Undo/Redo and authored/evaluated save/new/reopen pass. Failed operations
  leave revision, Undo and geometry unchanged. A focused 8-response follow-up
  after `06a44ac` confirms stale draft update/preview/commit errors include
  the exact source face tuple and ID; the full geometry workflow was not
  unnecessarily repeated.
- Three actual Qt/OCCT framebuffer captures on Apple M4 hardware OpenGL 4.1
  at DPR 2 were inspected. The distinct source/preview sheets and +N cue
  are readable. This is current GPU prototype evidence, not physical native
  input or matched reference styling.

The fixture is `tests/surface_offset_api.py`. The independent report, model,
transcripts and captures are outside git under
`../Alloy-reference-records/validation/surface-offset-2026-10-20/`.
Public source provenance and uncertainties are external at
`knowledge/specs/srf029-one-face-surface-offset-2026-10-20.md`. No
proprietary runtime was operated.

## Remaining acceptance

The Mac console is locked. Actual-hit native face selection, normal drag,
physical input, 1×/2× interactive controls and matched public visual
comparison are **not accepted**; SRF-041 owns them. Curved-face inward
collapse is outside this planar fixture; a separate retained partial-cylinder
result is [accepted](surface-offset-cylinder.md). Multi-face/chained offsets
and broader curved families
remain SRF-004; Windows/Linux packaged runtime remains a release gate.
