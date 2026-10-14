# Open rectangular Cut rim — accepted (#331)

Status: independently accepted and integrated, 2026-10-13. The external
enclosure port/card contract records the public source and disclosed fixture
adaptations. This ingredient uses a centered 16 × 12 card opening through the
case's short wall and rounds its three exposed U-lip edges to R2.

The existing linear Cut history already published these three exact current
edges on the retained target face. The Cut now promotes a boundary to the
solid-edge catalog only when it belongs to the current result and target face,
has one exact tool-rim contributor, and has a unique source-sketch/entity and
target-face semantic key. Duplicated physical or semantic candidates remain
unavailable. Stable IDs derive from the Cut feature, retained target face and
authored sketch entity; no world-coordinate match or nearest-edge choice is
used. The downstream Fillet consumes these predecessor edges by retained ID.

`cut_open_rectangle_rim_test` covers a bottom-open rectangular Cut, selection
of all three live edges, R2 Fillet, a 16 → 14 width edit with stable pre-Fillet
IDs and rebuilt Fillet, undo and exact save/reopen. `cut_hole_rim_test` protects
the existing circular Cut path. Both focused tests pass. The real-app corrected
card diagnostic in `build-tut021/evidence/tut021/card-corrected/` shows the
centered Cut and exact boundary provenance; `card-r2-view/card-r2-front.png`
shows the actual R2 result. An earlier off-center diagnostic is retained in
`slot-card-diagnostic/` as failed evidence and is excluded from claims.

This is Cut-boundary promotion for uniquely traced straight rectangular rims,
not general Boolean edge naming or a claim about the tutorial's exact lip-edge
selection. The complete port/card workflow is the separate #322 gate.

Astra independently reviewed the exact-history guards, reran both focused tests
and inspected the corrected centered U-lip capture. The full creation workflow
and its final export remain the separate cumulative gate.

Integrated main application build and both focused tests passed (2/2, 1.16 s).
