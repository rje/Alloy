# Responsive STL export on the saved corrected iron

REL-047/#493 is independently accepted for the committed single-selected-solid,
millimeter binary-STL path. The REL-046 candidate15 full tutorial replay exposed
a 2,034.890 ms synchronous `document.exportStl` call and a 2,088 ms actual-frame
gap, above its frozen 2-second gate. Candidate15 remains a failed cumulative
recording. The complete failure trace is outside this repository at
`../Alloy-reference-records/deliverables/iron-reassessment-2026-10-17/candidate-15-responsive-from-blank-recording/`.

The existing mesh validation, tessellation, facet filtering and atomic STL
writer now run on the serial worker using a private deep copy of the selected
solid. A value-only GUI authorization immediately before `QSaveFile::commit`
rejects stale or canceled results. Cancel after authorization reports that
publication is already in progress and waits for the real writer result. The
native export dialog consumes terminal success or error rather than treating
pending as a completed save. Other export paths remain outside this bounded
change.

On the saved canonical final iron, focused controls/kernel tests passed 2/2.
The real-app command submitted in 97.443 ms and completed in 702.272 ms.
Pending status, pan and GUI heartbeat remained under 1 second. Eight actual
captures had a maximum gap of 389 ms; the 5 fps video encoded 12 frames with
four repeats disclosed. The GUI snapshot was below the timer's 1 ms resolution,
worker preparation took 400 ms, and publication after approval took 1 ms.
The entire before/after document inspection including history is identical.
The exported STL is byte-identical to the accepted candidate14/candidate15 STL:
26,070 oriented triangles, one closed genus-1 component. Its tessellated volume
differs from the analytic BRep by the same accepted 0.08393% approximation.
The private editor quit normally. Astra independently reviewed the source and
saved-model evidence.

Full timings, source/build identity, actual frame events, captures, model and
readback are in external
`../Alloy-reference-records/validation/rel047-stl-export-2026-10-17/`.
REL-046 still requires the complete 44-stage replay, final STEP, postmovie
source edit/history/reopen and independent recording review. This saved-model
gate alone does not close cumulative responsiveness or other STL routes.
