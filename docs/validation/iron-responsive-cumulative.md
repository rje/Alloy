# Responsive corrected-iron creation replay

REL-046/#492 is independently accepted for one complete corrected-iron
from-blank workflow on the integrated macOS build. The unchanged accepted
44-stage recipe produced a playable 417.4-second, 5 fps creation recording,
native model, STL and STEP. The prior candidate15 run remains failure evidence:
synchronous STL export then caused a 2,088 ms actual-frame gap above the frozen
2-second gate. REL-047 moved that bounded export path off the UI thread before
the successful candidate16 replay.

The new recording contains 2,087 decoded frames: 1,554 actual captures and 533
automatic repeats, with 512 delayed ticks and zero capture failures. The
largest gap between actual captures is 686 ms. Across pending operations,
background status requests stayed at or below 144.550 ms, navigation at or
below 104.852 ms, and GUI heartbeat gaps at or below 178 ms. The largest
terminal request was 559.296 ms and is reported separately. The long Trim,
Loft, sketch, Extrude and Boundary Fill evaluations still take wall time, but
their measured UI responses stayed below the frozen limits. STL export took
579.771 ms end to end. All 9,559 raw command responses succeeded; the private
editor quit normally.

Astra independently matched all 44 captions and reviewed decoded movie frames.
A consistent 156-identifier mapping proves the full twelve-feature authored
chain, ordered section/rail/tool inputs, four pre-Fill source bodies and final
body geometry/reference ancestry equivalent to the accepted candidate14.
The final BRep volume is 856172.8343589634 mm³. The STL is byte-identical to
the accepted 26,070-triangle closed genus-1 export. A standalone OCCT STEP
reader reports one valid solid with matching analytic volume, area, bounds and
through-aperture/rear-web witnesses. Outside the creation clip, a 0.5 mm fit
point source edit changes the solid, followed by exact Undo/Redo and native
reopen with one history operation and stable face IDs.

Full source/binary hashes, operation timings, actual-frame events, movie,
decoded review frames, native models, exports and independent receipt are in
external
`../Alloy-reference-records/deliverables/iron-reassessment-2026-10-17/candidate-16-responsive-from-blank-recording/`.
The [external demo gallery](../../../Alloy-reference-records/demos/2026-10-13/index.html)
links the accepted movie and artifacts. This gate covers one Apple M4/macOS
API-driven tutorial run; physical-device controls, other machines/platforms,
exact public dimensions and separate-tool gesture parity remain open under
their own tasks. Postmovie edit and STEP checks are transcript/artifact evidence,
not footage inside the creation clip.
