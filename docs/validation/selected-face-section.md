# Selected-face Section Analysis

DOC-023 independently accepted at `87340d0` and integrated, 2026-10-13. This bounded slice extends the accepted origin-plane
analysis without changing modeling geometry. Public workflow evidence, source
provenance, retrieval date and the explicit unavailable-support decision are in
the external `../Alloy-reference-records/knowledge/tutorials/advanced-audit-2026-10-13/selected-face-section-contract.md`.
The public source documents planar canvas selection, Distance, Flip, hatching,
browser visibility and editing; exact default side and stale-reference behavior
are not established. The signed offset convention below is an Alloy decision.

## Contract

An optional face-backed analysis retains its exact
`{producerFeatureId,bodyId,faceId}` support, signed `offsetMm` and `flip` in the
authored `analyses` array. Existing six-field XY/XZ/YZ origin records remain
valid and use their prior clipping normals. A face plane is resolved from the
current evaluated support and occurrence world frame. Its origin is the current
face origin plus offset times source normal; Flip reverses the clipping normal
without moving that origin. The same frame supplies hatch directions. The
shared Inspect command exposes planar face picking, native Name/Distance/Flip,
Clear Face, preview, Cancel and commit. `analysis.inspect` returns availability
and the resolved plane; the Browser marks missing supports unavailable.

When a source face disappears, the authored analysis remains, its clipping is
disabled and unrelated modeling edits remain valid. Undo that restores the
exact face identity restores clipping. A new analysis with missing support is
refused. No nearest-face substitution or stale frame is used. Analyses remain
outside modeling features and retain the existing single-visible policy.

## Evidence

The focused `section_analysis_test` passes the serialized face variant,
signed offset, undo and malformed-offset refusal alongside the origin record
regression. The real application entrypoint run
`python3 tests/selected_face_section_api.py --app build/alloy.app/Contents/MacOS/alloy --output build/evidence/selected-face-section`
passes 32 calls: ray-picked box top, disposable offset preview/Cancel, retained
analysis, Flip at unchanged plane origin, source height 20→28 mm with section
origin 16→24 mm, save/reopen, source deletion leaving the analysis unavailable,
new-authoring refusal and undo recovery. Its report, transcript and captures
are under the build evidence path above, outside tracked source. The flipped
capture was inspected: an unselected gray solid has a readable hatched cap at
the resolved plane. The Mac runtime reports Apple M4, OpenGL 4.1 Metal and
hardware acceleration; `sectionClippedActors` is one when available and zero
when the source is gone.

The native Qt control test passes canvas face click, typed negative Distance,
Name, Clear, preview/Cancel/OK, alongside the existing origin-plane lifecycle.
Its injected Qt input and two captures are separate from physical OS input,
which remains unqualified. Windows/Linux GPU runtime and exact screenshot
parity remain unqualified. Angle transforms, manipulator dragging and custom
section colors remain in broader inspection scope.

Independent-review corrections: the origin-to-Selected Face selector now requires an actual valid face before OK. The focused native test switches modes, clicks the Distance and Name controls and asserts focus without repairing it. One initial macOS harness activation precedes all demonstrated input. The corrected focused test passes; refreshed captures are external under `deliverables/selected-face-section-review-2026-10-13`.

Main app build and focused Section Analysis/local reference/Split checks pass (3/3 with the qualified external board fixture). Independent review accepted the corrected selector and input capture alongside original geometry/API evidence.
