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
shared exact-transform helper extraction. Final core 2/2 passed in 0.55 s.

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

Evidence is external at `validation/circular-body-pattern/`: `core-final.log`,
`native-attempt-01.log`, preserved `attempt-01/`/`attempt-02/` captures and attempt logs, `native-attempt-05.log`,
`native-eight-preview.png`, `native-eight-accepted.png`, preview/final state JSON,
`source-seed.alloy` and `native-eight.alloy`. The private editor exited normally.
App SHA256: `170384937d5687fdd6a0e2cc48f999c90e48e07d95d56133565bbd5182748d79`.
Native fixture SHA256: `400ed904536f7ed2b262c5a155cad0f80e9b2a9d36b22557a0a1e85a11bad8dc`.

Compatibility: matched documented Bodies/one seed/Full/total eight and actual
editable ownership/history behavior. Origin Z selection, initial count 3,
CCW phase-zero angles and first preview auto-fit are disclosed Alloy policies.
The captured simple spoke geometry is not the source ornament; exact source
axis-object choice, angular defaults and pixel parity remain unknown. Independent
Astra review is pending at this implementation receipt.
