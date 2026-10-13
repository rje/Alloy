# Die tutorial independent gate review

Reviewed 2026-10-13. Accept #147's complete adapted API/artifact gate using the
final `build-tutorial-polish/evidence/die-final/` run. The reviewer did not
implement its modeling features. Earlier failed export evidence is diagnostic,
not a passing deliverable. This acceptance reuses the independently accepted
component #200, Chamfer #201, Sphere #145, named-view #203 and binary-export #204
receipts rather than repeating their focused suites.

The external TUT-002 public ledger (`knowledge/tutorials/warwick/library-die-ledger.md`,
retrieved 2026-10-13) prescribes save, named component, center square, cube extrusion,
Chamfer/Fillet trials with Cancel, chosen edge treatment, face-positioned spherical
cuts and Repeat across all faces, then component-scoped STL. Cube size, rounding,
diameter, pip counts and positions are learner choices. The local fixture uses
20 mm sides, previews/cancels both 1 mm edge treatments, accepts Fillet and makes
21 diameter-3 hemisphere cuts in conventional opposite counts summing to seven.

The completed actual-app ledger reports `api-pass`: 230 commands, zero failed
responses, nine geometry/capture stages and normal application exit. It verifies
all six retained planar supports after every cut, rounding edit 1→0.8, undo/redo,
restoration and exact authored reopen. The saved document independently inspected
contains one named die definition, a distinct identity-placement occurrence and
24 features, all owned by that definition: sketch, extrusion, fillet and 21 spheres.
The original cube is 8000 mm³; the rounded baseline is 7949.834793498635 mm³;
final BRep volume is 7801.394540616514 mm³. The removed volume is
148.440252882121 mm³, matching 47.25π to numerical tolerance.

Independent unchanged strict STL readback of the final exported artifact found
one closed consistently oriented component, 53,414 triangles, finite unit normals
aligned with winding, bounds [-10,-10,0] to [10,10,20] mm, and mesh volume
7801.733044824928 mm³. Component export selects the die's retained body. The
separate #200 stock-exclusion check and #204 millimeter/inch serialization checks
remain applicable; no broad curved-ASCII export qualification is implied.

All six actual face captures were inspected and show counts 1,2,3,4,5,6. The
oblique final capture shows readable recessed rims and the rounded cube. Bright
head-on highlights can make concavity perceptually ambiguous; geometry is checked
separately. The corrected valid Sphere preview is red/pink over neutral stock
with blue face outline; origin arrow cones no longer obscure its center.

The external `demos/2026-10-13/die/die-complete.mp4` is 323.3 seconds, 1244 × 902,
10 fps. Representative frames at the beginning, rounding trials, sequential pip
creation, last face and export were inspected, together with samples spanning
the intervening work. It starts blank and ends with the complete die/export,
with API command and step captions. Recording metadata reports zero capture
failures, 3233 encoded frames and 3046 repeated frames; this preserves elapsed
operation time and is not evidence of continuous interactive responsiveness.
The final reopen took approximately 9.235 seconds, with upstream edit about
9.107 seconds; general responsiveness remains the separate performance backlog.

Compatibility qualifications remain explicit: Select All Edges replaces the
source selection window; named views replace its view cube; an ordinary context
menu replaces radial Repeat. Center rays are followed by canonical local-coordinate
updates to remove pixel quantization in the deterministic fixture. Repeat diameter
carry-over and activation defaults are documented Alloy choices. Focused Qt events
verify human controls separately; Enter-to-commit was not activated in the inactive
macOS test window, so actual OK is the qualified commit route. No uninterrupted
physical-OS tutorial or exact visual parity is claimed. The source has no final
matched die image. All 24 timeline entries are retained, but the final row is
crowded and labels truncate; broader timeline usability remains #43.

Independent media contact sheets and strict readback are in
`build-tutorial-polish/evidence/die-independent/`. Final editable design,
`die-mm.stl`, six face captures, preview captures, final oblique image and
`die-tutorial-api.json` are in `evidence/die-final/`. These artifacts satisfy the
bounded cumulative gate with the above adaptations and limitations.
