# Planar Patch — SRF-027 / #630

The accepted bounded core makes a retained one-face surface New Body from one
simple closed ordinary-Line sketch profile. It reuses the evaluated exact
planar profile face and stores stable sketch/profile and constituent boundary
identities. It does not infer a solid, fill a nonplanar loop or accept holes.

The public Surface > Create > Patch command accepts a closed sketch profile.
The current native dock has Sketch and Boundary Profile selectors; actual
canvas profile picking remains SRF-039. Public continuity choices for model
edges do not apply to this sketch-curve-only bounded input. Factory defaults
and exact native styling remain unknown from the permitted sources.

## Evidence

- Implementation `deeb35d` built locally in Release at `-j4` with Qt 6.11.2
  and OCCT 7.9.3. Focused Patch and unchanged Surface Revolve/Sweep C++ tests
  pass (3/3).
- Independent 93-response real-app fixture checks a 40×30 mm XY loop at
  z=20: one planar face, one closed wire, four boundary edges, zero solids,
  +Z normal, area 1200 mm², perimeter 140 mm and zero measured deviation
  from the retained source edges. Source width 40→50 mm recomputes to
  1500 mm² and 160 mm while profile, body, face and edge IDs remain stable.
- Preview/Cancel, same-session invalid profile correction, unsupported open,
  disconnected, self-crossing, missing and plural inputs, Undo/Redo, source
  edit and authored/evaluated save/new/reopen pass. Refusal leaves history,
  revision, Undo and evaluated state unchanged.
- Three actual Qt/OCCT framebuffer captures on Apple M4 hardware OpenGL 4.1
  at DPR 2 were inspected: the planar sheet, boundary and dock are readable.
  This is Alloy GPU evidence, not a matched reference screenshot comparison.
- Independent source review found `surface.patchDialog` initially routed to
  Sweep; it was corrected before the final build and real-app fixture.

The fixture is `tests/surface_patch_api.py`. The independent report, model,
transcript and captures are outside git under
`../Alloy-reference-records/validation/surface-patch-2026-10-20/`.
The clean-room public source and uncertainty record is external at
`knowledge/specs/srf027-planar-cap-2026-10-20.md`. No proprietary runtime
was operated.

## Remaining acceptance

The Mac console is locked. Actual-hit native canvas profile selection,
physical input, 1×/2× interactive controls and matched public visual
comparison are **not accepted**. SRF-039 owns those first-v1 checks.
Nonplanar fill, model-edge continuity, curved/compound loops, holes and
boundary networks remain SRF-028/SRF-003; Windows/Linux packaged runtime
remains a release gate. This accepted core is not complete Patch parity.
