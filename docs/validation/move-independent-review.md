# Independent bounded Move review

Reviewed 2026-10-12: Move candidate `fdeb2ad`/`cbc7d48` plus solid-triad
follow-up. Source, focused core/API receipts, saved authored steps, combined
tutorial result and native interaction were checked independently of its author.

The rigid transform composes successive local gestures correctly for the
researched Y10-degree rotation followed by local Z-20 mm translation around
the cutter foot. Within a mixed delta a point rotates X, then Y, then Z, then
translates; mixed-delta wording was corrected in the implementation receipt.
The retained target identity, one history transaction, undo/redo, save/reopen
and downstream Cut have core/API evidence. Copy, align, general moved-face
attachment and arbitrary pivot picking remain outside this bounded child of #53.

The initial wire triad was visibly faint under native macOS line-width limits.
The replacement solid GPU cylinders/cones/rings are readable over the gray
preview and accepted. This is a bounded readability correction, not evidence
of exact reference styling. Previewed selection stays neutral gray while the
historical public source shows blue; exact selection styling remains unmatched.

Native verification used the exact private Move application and API-seeded
plate/cylinder fixture. Typed Y10 then Tab stored a rotation and reset live
fields; typed Z-20 then Tab stored the following local translation and reset.
Dragging the visible local Z arrow accumulated8.120546 mm; dragging the blue
rotation ring accumulated-55.391410 degrees, each on mouse release. Native
Cancel preserved the source authored document exactly. A separate verified
Y10/Z-20 sequence plus native OK authored exactly those two steps in one Move.
Native toolbar entry worked; a Browser accessibility right-click did not open
a menu, so physical context-menu entry remains unknown here.

Evidence is in
`../Alloy-worktrees/bung-move/build-move/evidence/move/`: `native-numeric.json/.png`,
`native-arrow.json`, `native-ring.json`, `native-cancel.json`,
`native-accepted.json/.alloy/.png`. One batched automation attempt missed the
rotation; API Undo restored the fixture and separately inspected inputs then
succeeded. It is recovery evidence, not an uninterrupted tutorial claim.

Escape after a viewport drag did not dismiss the panel. The Cancel button
worked; the native Escape defect was sent to the implementation owner for a
focused correction. Final acceptance awaits that correction or explicit scope
tracking. No additional modeling suite is required for the presentation change.
