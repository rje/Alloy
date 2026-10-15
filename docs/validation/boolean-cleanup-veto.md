# Boolean cleanup validity veto

GEO-021, 2026-10-14. Independently accepted by Astra source/evidence review.

The enclosure's Top-minus-Bottom Cut produced a valid raw solid, but OCCT
same-domain cleanup invalidated it. The Boolean result and tool-group paths now
test the same cleanup configuration on a deep copy first. An invalid copied
cleanup leaves the original raw shape and its history untouched. Otherwise the
existing cleanup and final validation run normally. No copied topology is shipped,
no tolerances change, and no reference replacement rules are relaxed.

The explicit app build and `boolean_document` regression pass. The actual API
gate passes the exact paired-ridge source, retained-tool Cut, width 64→65 mm edit,
undo/redo, exact save/reopen, and sketch begin/cancel on a genuinely retained Top
support. At width 64 the valid closed Top changes from 30749.721987 to
30307.107753 mm³; Bottom remains valid and unchanged at 29482.618937 mm³.
Top's unavailable split-edge references increase from eight to twelve; all remain
unavailable rather than selecting arbitrary descendants. This is not a claim that
every old edge survives the Cut.

Separate width-65 Top and Bottom STL readbacks each have one connected component.
Separate STEP readbacks each contain one valid solid and match evaluated volume
within 0.0001 mm³. Their Boolean intersection has no solids and zero volume.
This establishes no positive-volume interference, not manufacturing clearance.
The Apple M4 native GPU capture shows the complementary U groove from below;
Board reference geometry remains visible, so this is a bounded geometry capture.

Evidence: external `deliverables/enclosure-rim-2026-10-14/geo021-api04/` contains
the transcript, source hashes, saved model, exports/readbacks, fit report and GPU
capture. `tut023-cut-diagnostic-01`, `geo021-stage-status01`, `geo021-api02` and
`geo021-api03` retain the initial cleanup failure, aliased-raw mutation,
always-copy provenance failure, and overbroad test assertion respectively.

The veto adds isolated cleanup work. It is not proof that copied and original
cleanup always agree; the original final validity refusal remains. Exceptions and
valid-but-wrong geometry are outside this recovery. TUT-023 still requires its
full stage recording and rim/tool-selection captures; no full-source tutorial or
physical fit acceptance is claimed.
