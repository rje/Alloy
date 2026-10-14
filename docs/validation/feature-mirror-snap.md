# Retained snap-group feature Mirror

SOL-051/#327 mirrors the selected source group in authored history order:
one Extrude Join, three simple Chamfers, and one simple Fillet on one target
body across origin XZ. The authored Mirror stores only the five source UUIDs,
target UUID, and plane reference. Evaluated Join prism, cap, edges, and the
pre-finish edge shapes are disposable source-stage caches. Reflection reuses
the existing Boolean Join, Chamfer, and Fillet evaluators. Each finish selects
the unique current straight edge with the reflected source endpoints and full
length. It refuses missing or ambiguous descendants and intervening
modifications to the target; independent Bottom work may occur between the
source group and Mirror. This is a bounded identical copy of the one-prism
Join and its four finishes, not a general feature replay or Adjust mode.

The source-shaped headless proof uses a rounded, shelled, split case with a
stepped snap sketch. It verifies a valid closed Top after all five mirrored
operations, reflected cap and edge identities, cap frame on the opposite
side of XZ, independently expected added volume, source Join edit, undo/redo,
save/reopen, same-target gap refusal, and a valid unrelated Bottom Fillet
interlude. Prior Cut Mirror, Chamfer, and Fillet focused core checks pass.

The real-app API run passed 22 calls on that source fixture: live preview,
Cancel, commit, reopened edit/Cancel, undo/redo and save/reopen. The Top's
volume changed from 32358.984 to 32515.597 mm³. Native Qt input clicked the
five actual Timeline tiles, selected Origin XZ in the combo, observed an
enabled OK and valid preview, then clicked Cancel; the authored document was
unchanged. That focused run passed 3/3 in 9.2 seconds. The test waits for the
Timeline to lay out and asserts each tile is on screen before clicking; it
does not repair focus or selection between gestures. Input is Qt-delivered.

External evidence is under
`../Alloy-reference-records/deliverables/feature-mirror-snap-2026-10-13/`:
`api-01/report.json`, `api-01/transcript.json`, the accepted `.alloy` model,
and preview/accepted captures; `native-05/test.txt`, panel and preview captures;
and `presentation-03/snap-top.png`, which shows both opposite-side snaps at
160% after hiding the unrelated Bottom body, construction overlays and
analysis for presentation. Native attempts 01–04 remain preserved: they
exposed a test-harness timing issue before Qt laid out the loaded Timeline;
no modeling change was needed. The presentation run changed visibility only
in its disposable session, not the saved accepted model.

Astra independently accepted source/core, native/API and final captures;
the bounded slice is integrated.
The later source-sized and reordered cumulative gates now close the bounded
SOL-049 parent; see [source-sized group](source-sized-snap-group.md) and
[Timeline reorder](timeline-reorder.md). Broader SOL-009 remains open.

Main integration: app build, existing Cut Mirror core and actual five-operation
snap fixture all passed.
