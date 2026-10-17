# Enclosure connector clearance adaptation

TUT-035, 2026-10-17. The user approved an explicit printing adaptation: at least
0.5 mm model clearance around the nominal Ethernet connector, with the imported
reference unchanged. This is a measured CAD clearance, not a physical fit test
or a claim that the undocumented source dimensions match.

The real Alloy workflow edited the retained side-port Cut end offset from 0 to
−1.5 mm on the accepted 29-feature pre-rim enclosure. The changed split-cap
boundary has 16 rather than 18 edges, so the four dependent rim operations were
authored again with ordinary commands. The new inset is −1.25 mm, the Join is
2 mm, the two-seed fillet is R0.4, and the final Cut retains separate Top and
Bottom halves. The earlier R0.5 on this changed mouth refused; R0.4 is the
disclosed adaptation. [HIS-008](../backlog/README.md) tracks general retained
reference repair for upstream edits; this workflow does not implement it.

The saved 33-feature model reopened exactly. Each half remains a valid closed
single solid. A real-app visibility edit passed undo/redo and returned to the
original authored state. Independent BRep inspection found zero positive-volume
intersections across all 24 half/reference pairs. The nominal SIDE-ETH-001 has
0.5000000000000071 mm minimum separation from **each** half. Top and Bottom
also have zero common volume. Other reference distances vary and are not
claimed to have this minimum clearance.

Both halves were exported separately to STL and STEP. STL readback found one
connected, watertight, oriented component per half. Independent STEP readback
found a valid single solid per half, with volume agreeing with evaluated Alloy
geometry within 0.0001 mm³. The final recording is a stage continuation from
the saved 29-feature checkpoint; earlier creation footage is separate. It uses
the shared local control API and actual GPU frames, not a native-input parity
demonstration. Physical fit, snap force and a real print remain unverified.

External evidence:
`deliverables/enclosure-cumulative-2026-10-17/clearance-prefix-03/` (edit and
history), `clearance-rim-r0p4-06/` (movie, final model, exports, independent
readbacks, 24-pair clearance and half fit), and `final-reopen-07/` (real-app
reopen/history). The external video gallery links the accepted stage, model and
exports alongside earlier enclosure creation footage.
