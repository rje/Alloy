# Enclosure rim and retained-tool fit stage

TUT-023, 2026-10-14. Independently accepted by Astra evidence review.

`tests/enclosure_rim_stage_api.py` records the continuation from the accepted
29-feature enclosure through the actual U-cap inset −1.25 mm, Join 2 mm,
two tangent-chain R0.5 fillet, and Top-minus-Bottom Cut with Keep Tools.
It finishes with separate Bottom and Top STL/STEP exports and an editable model.
The 267.4-second raw recording uses the real application API and native GPU
frames, with explicit API captions. Geometry waits remain in the movie.
This is a complete stage recording, not a new from-blank creation recording.

The movie and captures show the signed inset and chain option, both selected
rim seeds with R0.500 and Tangent Chain On, Cut target Top with Keep Tools,
the complementary U groove, and the separately retained Bottom. The Board and
Top are hidden during rim work. The visible Fillet count is two authored seeds,
not the expanded fourteen-edge count seen in public source frames. The Cut panel
shows one selected tool; its checked Bottom row lies below the visible list
scroll, while the API ledger identifies the exact Bottom body.

The cumulative stage produces two valid closed single solids. Each actual movie
export was independently read back: STL has one connected manifold component,
STEP has one valid solid matching evaluated volume within 0.0001 mm³.
The accepted [Boolean gate](boolean-cleanup-veto.md) separately establishes a
64→65 mm upstream edit, undo/redo, reopen, retained planar support, retained-tool
ownership and no positive-volume overlap. Those checks are reused here rather
than repeated inside the recording. [Paired rounding](paired-ridge-fillet.md)
and [retained inset](trimmed-seam-inset.md) retain their bounded contracts.

External evidence is under
`deliverables/enclosure-rim-2026-10-14/tut023-stage-02/`: movie/event ledger,
transcript, runner snapshot/hash, result, saved model, exports/readback and stage
captures. `tut023-stage-01` preserves the initial harness refusal before geometry:
Offset preview required activating the existing Offset tool first.

Public source sequence/dimensions are corroborated; exact topology is adapted to
the independently authored enclosure's single U boundary. The source's other
boundary value/pick sequence remains unknown. This is not exact UI parity or a
native pointer/keyboard tutorial gate. Physical printing clearance, whole-board
fit and the full-source TUT-016 workflow remain outside this acceptance.
The external research handoff preserves creator, public URLs and timestamps.
The accepted movie, saved model and exports are linked in the external gallery
`demos/2026-10-13/index.html#enclosure-rim-fit`, alongside earlier creation footage.
