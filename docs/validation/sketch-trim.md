# Bounded line Trim candidate

SKT-029 #177 is a bounded child of #47. Reference `SRC-WARWICK-STRAW-SUPPORT`
original public video170–191s adds Trim to the PDF's source sequence. Its external
ledger/spec reside in `../Alloy-reference-records/knowledge/tutorials/warwick`
and `knowledge/specs/sketch-trim-2026-10-12.md`; names/URLs remain outside Git.

The active sketch's T / Modify→Trim command previews the selected line interval
in red and removes it on click. Actual finite line/circle intersections bound
the interval. End trimming retains the line ID, middle trimming replaces it with
two line IDs, and an uncut line is removed whole. Retained circle geometry stays
whole. New endpoints retain point-on-curve relations through PlaneGCS. Unsafe
length/endpoint references are removed explicitly and reported, with a visible
constraint/dimension removal notice; independent safe data remain authored.
Enter/Escape exits Trim; each click is one accepted draft history step.

OCCT computes intersections. Ambiguous overlapping geometry refuses only when
it overlaps the selected interval; unrelated far-right source strips do not
block a circle-internal chord. Arc/spline targets, general curve trimming,
drag-across multiple intervals, Extend/Break/Offset remain in parent #47.
Tiny outside-left stubs are retained because their removal was not established.

Build and focused `sketch_trim`, `sketch_solver`, `sketch_document` checks pass.
The source fixture includes rect32×10, guide7.5, Ø15 circle and three overlapping
strip rectangles, then exact top/left/guide/bottom operations. It verifies
circle retention,32/Ø15 surviving while10/7.5 disappear, outside-left stubs,
atomic overlap refusal, history and exact storage. Source interval contacts are
7.5±sqrt31.25. No application/native/capture acceptance is claimed yet.

`tests/sketch_trim_api.py --app APP --output DIR` prepares one focused real-app
preview→click→draft Undo/Redo→Finish→document Undo/Redo→save/reopen receipt.
The API fixture omits far-right strips, which are covered in the core fixture;
the integrated region owner must qualify the complete source arrangement.
Native T, hover/click and Enter/Escape are pending independent combined review.

Local commands `sketch.trimPreview` / `sketch.trim` accept `entityId,uMm,vMm`.
`sketch.pointerMove` / `sketch.placePoint` resolve the same inputs for native
Trim. The accepted result diagnoses removed/created entities, removed
constraints/dimensions, and the trimmed interval. The existing sketch journal,
solver authority, and one-transaction Finish/Cancel remain authoritative.

## Integrated real-app checkpoint

The combined candidate in `build-support-integrated` builds and passes both
changed core tests. Its bounded real-app script passes with GPU captures under
`build-support-integrated/evidence/`. Root inspected Trim preview and actual
separated profile selection. Native input and independent acceptance remain
pending; this checkpoint does not close the complete support tutorial.
