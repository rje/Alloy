# One-edge Natural Surface Extend — SRF-031 / #634

The accepted bounded core extends one straight outer edge of a rectangular
planar Patch sheet by a positive Distance, using Natural and Free Edges with
chaining off. The current sheet grows in place while its earlier state remains
available before the Extend feature in history. The body, face, producer and
four boundary edge IDs are retained. The result remains one open face without
thickness or volume. A typed numeric dock, preview/Cancel/OK, Timeline edit and
shared canvas-pick API route are present; a physical actual-hit workflow and
canvas distance handle remain SRF-047.

## Evidence

- Implementation `0b2d677` built the focused `surface_extend_test` and app in
  Release with `-j4`, Qt 6.11.2 and OCCT 7.9.3. The focused test passes all
  four rectangle-side mappings, immutable prior-stage B-rep, source width and
  distance edits, atomic invalid/stale/shared/curved refusal, Undo/Redo and
  save/reopen.
- An independent 87-response real-app API fixture exercised edge selection,
  preview/cancel/commit, 40×30 mm source to 40×50 mm result, 20→10 mm distance
  edit, 40→45 mm source width edit, invalid/retry, Undo/Redo and reopen. An
  independent OCCT probe reopened the saved models: exact areas 1200→2000,
  1600 at 10 mm, and 2250 mm² at 45 mm width, with one planar face, four
  straight boundaries, zero solids, expected bounds/endpoints and retained
  identities. The prior-stage 40×30 mm B-rep hash remained identical.
- Inspected Apple M4 hardware OpenGL / DPR2 whole-app captures show a readable
  selected edge, numeric panel and continuous preview. With the source sketch
  hidden, the committed sheet renders smoothly. A visible coplanar source
  sketch/profile creates purple overdraw and faint edge striping on the
  committed sheet; this presentation issue is retained under SRF-047's native
  visual gate. It does not affect the inspected saved B-rep.

The fixture is `tests/surface_extend_api.py`. Independent transcript, saved
models, B-rep probe, review and captures live outside git under
`../Alloy-reference-records/validation/surface-extend-2026-10-20/`. Dated
public command, API and tutorial evidence with explicit unknowns is outside
git under `../Alloy-reference-records/knowledge/specs/srf031-surface-extend-2026-10-20.md`.
No proprietary runtime or CI was used.

## Remaining acceptance

The Mac console is locked. Physical actual-hit edge selection, field focus,
Cancel/Escape, 1×/2× native input, canvas distance drag and matched visual
comparison remain SRF-047. A proposed retained-target end condition is SRF-045;
the researched public Extend contract documents Distance and an Extend-then-Trim
workflow, with no target-object input established. Curved self-intersection/fold
refusal is SRF-046; this planar rectangle cannot exercise it. Perpendicular,
Tangent, chains, multiple edges and generalized surfaces remain SRF-005.
Windows/Linux packaged runtime remains a release gate.
