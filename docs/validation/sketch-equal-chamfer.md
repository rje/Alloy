# SKT-096: Equal Distance sketch Chamfer

Accepted 2026-10-18 for one connected corner of two authored normal sketch
lines. The dated public workflow evidence and source URLs are held outside the
repository in
`../Alloy-reference-records/knowledge/specs/skt-096-sketch-chamfer-2026-10-18.md`;
the lightweight command contract is in
[`docs/research/commands/sketch-chamfer.md`](../research/commands/sketch-chamfer.md).

Sketch > Modify > Chamfer > Equal Distance supports a corner or two adjacent
line picks, a disposable preview, typed distance in the Sketch Palette,
Enter/OK, Escape/Cancel and editing the retained distance. The shared API
exposes `sketch.chamfer`, `sketch.chamferUpdate` and preview/accept/cancel.
The authored lines, constraints and dimensions remain attached to their
original source geometry. Evaluation derives trimmed display/profile edges and
a stable diagonal line from retained source IDs and distance. A source
width/height edit recomputes the setback and downstream solid.

Evidence from Release `build-preview-release`, binary SHA-256
`3cfedb8f623b8f8bcfd6c9105fbe07e3b2000eb8cccf6b2b5756a2908c64f821`:

- Focused core and Qt native-input tests passed 2/2 in 6.48 s. Native input
  exercised the menu, corner hover and two-line selection, preview, typed
  commit/edit, source edit, transactional refusal of a second corner, resize,
  Undo/Redo and reopen. Captures and framebuffer metadata are at
  `../Alloy-reference-records/validation/2026-10-18-sketch-chamfer/native/`.
- Independent real-app entrypoint runner
  [`tests/sketch_equal_chamfer_api.py`](../../tests/sketch_equal_chamfer_api.py)
  passed 32 commands. A 150×75 mm XZ rectangle with a 10 mm upper-right
  setback gave 11,200 mm² sketch area and 56,000 mm³ at 5 mm extrusion. The
  retained setback changed to 12 mm and the source rectangle to 160×80 mm;
  the result was 12,728 mm² and 63,640 mm³, with unchanged sketch, Chamfer
  group, diagonal and body IDs. Undo/Redo and native-file reopen preserved the
  result. The independent binary STL reader found one closed component,
  16 triangles, signed volume 63,640 mm³ and bounds
  `[-80,-5,-40]..[80,0,40]` mm. Model, STL, captures and structured transcript
  are at `../Alloy-reference-records/validation/skt096-sketch-chamfer-candidate1/`.
- Actual captures show the shortened boundary, diagonal, equal setback labels
  and readable palette in the tested window. The app reports hardware-accelerated
  Apple M4, OCCT OpenGL through Apple's OpenGL 4.1 Metal driver, with a 2×
  native framebuffer.

The accepted slice permits one corner per sketch and palette numeric editing.
Two Distance, Distance And Angle, multiple corners and canvas drag/direct
annotation edit are bounded Todo issues SKT-110/#534 through SKT-113/#537.
Exact reference-app defaults and styling remain unconfirmed by available public
evidence. Physical hand input and Windows/Linux runtime are not yet qualified.
