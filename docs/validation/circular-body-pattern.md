# Circular Body Pattern validation

Candidate #263 implements the bounded [body pattern contract](../research/circular-body-pattern.md).
The exact-offset ornament Join and complete lesson remain separate #265/#264 gates.

Core `circular_body_pattern_test` verifies eight actual valid closed OCCT solids,
equal independently expected volumes and angular centroids, original seed body/face
identity, uniquely mapped generated supports/edges, count 8→10 retaining prefix
UUIDs, meaningful seed width 12→14 rebuilding all occurrences, exact undo/redo,
and save/reopen. An existing Extrude Join with all eight retained body participants
and a simple radius-6 hub produces one valid connected solid. Reducing the count
with downstream occurrence references refuses without altering authored state;
fractional count also refuses. This simple fixture is independent ingredient
proof, not an ornament approximation. Existing Mirror regression passes after the
shared exact-transform helper extraction. Initial core 2/2 passed in 0.55 s. Final rigid-transform core 2/2 passed in 1.61 s.
The added regression constructs the actual declared fit points (-4,-1), (6,19),
(-5,45), (3,89), exact -5 offset and explicit endpoint connectors in XZ, extrudes
the strip 60 mm as a New Body, then evaluates eight occurrences twice. All actual
solids remain valid/equal-volume with mapped supports/edges and repeat identities;
the original serialized BRep and authored document stay unchanged. Proper rotations
use exact OCCT locations; Mirror keeps reflection deep-copy behavior. This fixes
independently observed pcurve damage from deep-copying that valid exact prism,
without approximating its curves or claiming the cumulative Intersect/Join works.

Native `circular_body_pattern_controls_test` loads the core-authored real seed,
then clicks the toolbar command and actual body in the viewport, chooses Origin Z
through the native selector, and types Quantity 8. It checks evaluated eight-body
preview and unchanged authored data, clicks Cancel, repeats the selections and
clicks OK. History edit types 10, verifies the real ten-body preview, then Escape
retains the accepted eight. Shared undo/redo and save/reopen are exact. Only initial
harness activation sets focus; later gestures verify the focus produced by the
command. The axis selector is explicitly adapted selection, not direct axis picking.

Corrected native attempt 05 passed 3/3 in 3370 ms. Actual preview and accepted
captures were inspected: all eight bodies fit the viewport, the seed is blue,
derived copies neutral, and the selected axis/count and labels are readable.
Attempts 01/02 passed earlier checks but retained an open axis popup; the first
also clipped Objects text. Attempt 03 caught the popup with an explicit assertion,
and attempt 04 pointer targeting failed to activate the selector. These original
logs/captures remain preserved. The final fixture verifies actual popup focus
ancestry and delivers keys to that focused view, then asserts the popup is closed.
No fixture focus repair or speculative product timer remains. Objects text is concise. The viewport reports Apple
M4, Apple OpenGL 4.1 Metal and display ratio 2, with actual GPU framebuffer captures.
This is macOS evidence; Windows/Linux runtime qualification remains separate.

Evidence is external at `validation/circular-body-pattern/`: `core-final.log`, `core-rigid-final.log`,
`native-attempt-01.log`, preserved `attempt-01/`/`attempt-02/` captures and attempt logs, `native-attempt-05.log`,
`native-eight-preview.png`, `native-eight-accepted.png`, preview/final state JSON,
`source-seed.alloy` and `native-eight.alloy`. The private editor exited normally.
Final app builds on rebased main cf07ebc with the rigid-placement fix.
App SHA256: `61ab4cc0b99d1fbfef34ee1abbbddc0c1cdd176e01ad3ad41be7d04b5b90232e`.
Native fixture SHA256: `05fdbd821b321c2f54b55bcce6cd2db5c2012e44b1434231a8e24b9b1f558b07`.

Native attempt 05 predates the final rigid-placement helper change and dependency
rebase; its UI source is unchanged. Final core/app-build evidence covers the
helper and additive integration; no unchanged native replay was required.

Compatibility: matched documented Bodies/one seed/Full/total eight and actual
editable ownership/history behavior. Origin Z selection, initial count 3,
CCW phase-zero angles and first preview auto-fit are disclosed Alloy policies.
The captured simple spoke geometry is not the source ornament; exact source
axis-object choice, angular defaults and pixel parity remain unknown.

## Independent bounded acceptance

2026-10-13: final candidate `1d2a336` independently reviewed without implementing
production changes or opening another editor. Review covered generated identities,
owner/dependency checks, mapped transform lineage, simple eight-participant Join,
source/count edits, exact history/reopen and final rigid-placement regression.
The source and passing logs for both final core tests were inspected. Native
attempt05 fixture/log and actual settled preview/accepted images were inspected;
the earlier open-popup capture was explicitly rejected before the corrected
event-target fixture passed. Evidence reuse is bounded as described above.

| Contract | Result | Evidence and limits |
| --- | --- | --- |
| Bodies, one seed, Full, total eight | Matched | Eight actual equal-volume solids at independently checked angular centroids; original seed is counted once. |
| Retained ownership and transforms | Matched | Stable prefix UUIDs, exact source identities, mapped supports/edges and same-owner dependencies; final exact-offset prism rotations preserve source BRep bytes and repeated IDs. |
| Existing multi-participant Join | Matched for simple fixture | Eight selected spoke bodies and radius6 hub become one connected solid, with exact undo/reopen. This does not prove ornament Join. |
| Native selection, preview and cancellation | Matched for Qt fixture | Actual body pick, focused popup-key selection, typed eight, Cancel/OK and ten-preview/Escape; initial-only activation. Edit entry is shared API. |
| Readable settled GPU presentation | Matched within scope | Attempt05 shows all eight bodies and unobscured Bodies/Objects/Origin Z/Full/Quantity8 controls. |
| Source defaults, exact pixels and physical OS input | Unknown | Declared Alloy defaults and adapted axis selector; unmatched source geometry/viewport and injected Qt input. |

Accept #263 as the bounded Circular Body Pattern ingredient. A later Join retains
its explicit target IDs: pattern count edits do not imply automatic target-set
expansion. Removing a referenced occurrence refuses atomically; full-gate count
edits should be checked at the retained Pattern stage. Source edits after Join
remain a separate cumulative rebuild gate. #265/#264 and other-platform runtime
remain open.
