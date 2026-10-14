# Origin Section Distance and Flip candidate

DOC-027/#318, 2026-10-13. The external
`../Alloy-reference-records/knowledge/tutorials/advanced-audit-2026-10-13/enclosure-prefix-contract.md`
records the public creator, URL, retrieval date, observed Width-section move
near 32 mm and the adapted local model decision. The source does not establish
an exact world axis or clipping-side default. The adapted recipe uses XZ at
Y=32 mm for three side ports.

## Contract

Origin XY/XZ/YZ sections accept signed `offsetMm` and `flip` through the same
Inspect command and native Distance/Flip controls as selected-face sections.
The source normals remain +Z, +Y and +X respectively. The resolved plane
origin is offset along that normal; Flip reverses the clipping normal without
moving the plane. The renderer consumes this resolved plane for every section
mode, while origin hatching retains its existing axis mapping. Existing
six-field origin records remain valid and unchanged until edited. When an
origin Distance or Flip is authored, the record stores both fields; editing
the analysis restores them. Analyses still do not alter the evaluated solid.

## Evidence

The focused core test covers a six-field legacy record, typed transform
storage, malformed Flip refusal, undo/redo and exact save/reopen. The native
Qt section test enters Distance 32 on XZ, toggles Flip, inspects the resolved
Y=32 plane and Cancels without document change; its existing origin and
selected-face cases also pass. The 21-call real-app API run through
`tests/origin_section_offset_api.py` passes a box, direct legacy creation,
offset/edit, rendered clipping, history and reopen. Its Back-view capture was
inspected and shows the hatched cut at Y=32. The Mac GPU reports Apple M4,
OpenGL 4.1 Metal, hardware acceleration and one clipped solid actor. Captures,
transcript and GPU report are under `build/evidence/origin-section-offset`
outside tracked source. The native test uses injected Qt input with one initial
Mac activation; physical OS input and other-platform GPU runtime remain separate.

This slice supplies the later port-inspection control. It does not claim the
enclosure prefix geometry, ports, angle manipulation, or a full creator-workflow
match.
