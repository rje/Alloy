# Independent straw bung candidate review

Reviewed 2026-10-12 against integrated candidate `11b5c61`, stable source
`SRC-WARWICK-STRAW-BUNG` and the external original-video ledger. This uses the
video variant: base diameter26/depth3, boss diameter19/depth8 Join, tool
diameter6.2/depth40, Y rotation10 degrees then local Z displacement-20 mm.
The distinct PDF tool60/X rotation-10 variant is not this evidence.

## Geometry and API outcome

The real-app driver and saved receipt in
`build-tutorial-polish/evidence/straw-bung/` were independently read. The
transformed cutter and final-hole captures were inspected: the tool visibly
leans through the stepped body, and the final smooth solid has the expected
angled opening. The final viewing angle alone does not establish a through
hole; the geometry and mesh provide that evidence.

The retained Join has volume3861.0173712618557 mm3. Rigid Move preserves the
tool volume1207.6282160399164 mm3. Its bounds extend below the flange and above
the boss. Final Cut leaves one valid solid with volume3523.7964686083506 mm3,
matching `1229*pi - pi*3.1^2*11/cos(10 degrees)`. Authored undo/redo, temporary
upstream Join edit and exact save/reopen are separate recorded API assertions.

The exported `straw-bung-mm.stl` was independently passed through the existing
mesh inspector: 1788 triangles, bounds(-13,-13,0) to(13,13,11) mm, closed
oppositely oriented shared edges, finite unit normals aligned with winding,
signed volume3522.6592656919624 mm3. The mesh volume difference is consistent
with its finite circular tessellation. This is export evidence, not a print.

## Scope and remaining interaction evidence

Move source composes accumulated local rigid gestures; separate Y10 and Z-20
produce the researched sequence. The pivot at the cutter foot and broader
default selection rules remain declared source-informed Alloy choices.
Attaching a later sketch to a moved face and generalized face naming are not
claimed by this bounded candidate.

The combined API/artifact result is accepted. Native Join acceptance is recorded
separately in `join-independent-review.md`. Native Circle and Move controls,
Move handle readability, and the complete uninterrupted native tutorial remain
separate from this API gate. Historical references use unmatched viewport/theme
and do not support an exact visual-equivalence claim.

## Command-first Circle review

Candidate `bf48371` was independently checked by source diff, focused widget/API
receipt and implementing agent's physical native evidence. `sketch.circle` and C
preserve the pending Circle tool through the disposable support chooser and
reuse the existing authoritative circle authoring. The three native JSON states
have tool Circle and radii13,9.5,3.1 mm; the supported frames have origins z3 and
z11 mm. The third-circle capture visibly shows diameter6.2 and a fully
constrained centered circle. Evidence lives in
`../Alloy-worktrees/circle-entry/build-circle/evidence/native-*-circle.json/.png`.
This bounded entry behavior is accepted for #175. The physical Create dropdown
route remains unknown after unsuccessful automation attempts; its QAction route
has focused Qt widget evidence. This review does not claim the dropdown was
physically verified or that the entire native tutorial was replayed.
