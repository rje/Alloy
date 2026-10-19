# Cumulative enclosure tutorial gate

TUT-016, 2026-10-19. A fresh real-Alloy local-control run starts from a blank
document, imports the independently authored nominal board assembly, builds the
Case, ports and snap halves, and saves a 29-feature model. A separate run opens
that exact model and completes the alignment rim and Top-minus-Bottom fit Cut,
ending in a 33-feature editable model. The external
[stage ledger](../../../Alloy-reference-records/deliverables/enclosure-cumulative-2026-10-19/README.md)
links both untrimmed GPU recordings, chapter times, saved models, separate
Bottom/Top STL and STEP files, result records and limitations. Both accepted
recordings are in the external video gallery at `#enclosure-cumulative-2026-10-19`.

The port Cut uses the user-approved −1.5 mm end offset for at least 0.5 mm
model clearance to the nominal Ethernet connector. The modified rim uses
−1.25 mm Offset, 2 mm Join and an explicitly adapted R0.4 two-chain Fillet.
The final model reopens with exact authored data, two valid closed single-solid
halves and preserved reference occurrence. A visibility edit passes Undo/Redo.
Independent BRep inspection finds zero positive intersections in all 24
half/reference pairs and 0.5000000000000071 mm minimum connector distance to
each half. Independent STEP readback returns one valid solid per half with
volume within 0.0001 mm³ of the evaluated model; STL readback returns one
connected positive-volume component per half.

All declared dependencies are closed. The original face-linked Top section and
native Browser body rename have separate accepted gates in
[section-split-support.md](section-split-support.md) and
[body-rename.md](body-rename.md); older unavailable-section captions do not
prove those behaviors. This cumulative recording exercises the shared local
commands and actual GPU presentation. It does not establish physical pointer
or keyboard parity. Exact source dimensions where undocumented, creator-board
fit, snap force, wall strength and a real print remain unqualified. The nominal
board, placement and some snap coordinates are disclosed adaptations.

The from-blank runner completed its modeled work, save and exports. Its first
teardown returned a harness timeout while the large-document app exited just
afterward; the wait was increased to 20 seconds. The later rim and final reopen
runs exited normally. No product operation was inferred from teardown alone.
