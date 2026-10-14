# Retained Cut feature Mirror

SOL-050/#326 implements the first bounded feature Mirror ingredient. The authored
feature retains an ordered set of earlier Extrude Cut UUIDs, one target body UUID,
and the origin XZ plane. Evaluation captures each source Cut's actual straight-profile
tool at its history stage, reflects that tool once, and applies the existing Boolean
Cut authority to the current target. The source feature and target identities remain
authored; reflected cut boundaries receive retained edge IDs usable by downstream
edge selection. This is an Identical tool copy. It does not re-solve the source Cut's
extent at the opposite wall, or implement the later Join/Chamfer/Fillet group Mirror
tracked by SOL-051/#327. Unsupported source tools refuse atomically.

The focused core test verifies one closed target, two distinct pockets across XZ,
independently expected volume, generated edge availability, an invalid source edit
that leaves the document unchanged, stock resize, undo/redo and save/reopen.
`feature_mirror_cut` passed 1/1 in 0.25 seconds after the final commit guard change.

The real-app API run passed on an independently created sketch/Extrude/Cut seed:
volume 79820 to 79640 mm³, four generated mirrored edges, invalid seed refusal,
edit Cancel, undo/redo and reopen. Native Qt input clicked the actual Cut in the
Timeline, selected Origin XZ in the combo, inspected live preview, checked an early
commit refusal, clicked Cancel, repeated the selection and clicked OK. The accepted
bottom-view capture visibly shows both pockets and a readable command panel.
The final native run passed 3/3 in 2.4 seconds. The app/test process exited; only
Qt-delivered input is claimed.

Evidence is external at
`../Alloy-reference-records/deliverables/feature-mirror-cut-2026-10-13/`:
`api-02/report.json`, authored `.alloy` fixtures and API transcript; and
`native-04/test.txt`, `native-mirror-preview.png`, `native-mirror-panel.png`,
`native-mirror-accepted.png`. Native attempts 01–03 remain separately preserved;
attempt 03 was interrupted by a test-harness unsaved-window close prompt, corrected
in 04 without altering modeling behavior. The simple distance-Cut fixture qualifies
the first feature Mirror mechanism; the source-specific To Object undercut and the
later selected finish group remain cumulative acceptance work. Independent review
is pending.

Independent review accepts SOL-050 within this bounded scope. Main integration
builds the app and passes the focused Cut-mirror check. The actual enclosure
undercut and later edge-finishing group remain separately qualified by TUT-022
and SOL-051; this acceptance does not close the full feature-Mirror parent.
