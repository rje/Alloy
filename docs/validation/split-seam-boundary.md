# Complete retained Split seam boundary

GEO-016 publishes missing analytic boundary edges on a single closed Split cap
when at least three unique retained edges anchor every intervening path. It
uses actual vertex adjacency and a canonical anchor-pair/curve-kind sequence
plus position within that path. Existing IDs remain unchanged. Generated
provenance explicitly identifies this as a topology convention, not Boolean
history. Unsupported or ambiguous shapes keep their previous partial catalog;
consumers must require complete membership and validate the whole wire identity.

The actual final enclosure seam is one U-shaped wire: ten lines and eight arcs.
Six existing anchors now identify the remaining twelve exact edges. The focused
external-fixture test verifies18 distinct physical edges/IDs, preservation
through subsequent finishes, stock-height30→31, undo/redo and save/reopen. Its
full-fixture run passed; a shorter prefix rerun also passed the added distinct
physical-edge assertion. Existing Split core regression passed.

Evidence: external `deliverables/enclosure-rim-2026-10-14/seam-probe/` contains
both logs, geometry inventories and saved actual Split prefix. Run
`split_seam_boundary_test <enclosure-model.alloy> [prefix-output.alloy]`.
This is geometry/reference evidence, not new UI or a completed Offset/ridge
workflow. SKT-076 and TUT-023 retain those acceptance gates.
