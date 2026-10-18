# Independent sketch presentation

SKT-126 / #551 implementation and bounded verification. Public evidence contract:
external `knowledge/specs/skt-023-sketch-freedom-presentation-split-2026-10-18.md`,
P2/P3/P4, retrieved 2026-10-18. Independent Sol review accepted this bounded scope
after inspecting the settled terminal capture.

The Sketch Palette and `sketch.displayOptions` share session state for Show
Profile, Show Points, Show Dimensions, Show Constraints, Show Construction
Geometries, Show Projected Geometries, and Hide Fully Constrained. Ordinary
curves remain explicit and visible unless their independently solved status is
fully constrained and the focus filter is enabled. Construction visibility also
controls centerlines. Profile visibility controls fill and profile pointer picks;
it does not hide the boundary curves. The focus filter hides profile fill while
showing the remaining movable curves. It includes fixed and linked geometry in
the hidden fully constrained category; unsupported/unknown freedom stays visible.

Public sources do not establish defaults, persistence or hidden-target picking.
Alloy policy is all six Show controls enabled, Hide Fully Constrained disabled,
with session-local state reset on the next sketch edit. Nothing is saved in the
model or preferences. Visibility changes clear geometry selection conservatively;
hiding annotations clears their selection and hover/editor state. Hidden geometry
is excluded from curve/point/profile and window pointer selection. Stable-ID API
selection of hidden geometry is refused with `geometry_hidden`; authored inspection
remains available. Shared endpoints remain visible if any owning curve is visible
and Show Points is enabled. Relation/dimension presentation remains independently
controlled; its selected-geometry overlays respect geometry visibility.

There is no preview, commit or modeling undo entry for presentation changes.
Finish/Cancel retain their existing draft semantics. No entity, relation,
dimension, profile or solver input is removed or suppressed by visibility.
Contextual point-relation reveal and glyph reconciliation remain SKT-127/128.
Exact visual parity, reference defaults/persistence, and Windows/Linux native
runtime remain unknown.

Evidence root:
`../Alloy-reference-records/validation/sketch-visibility-2026-10-18/`.
The actual-entrypoint API fixture is `tests/sketch_visibility_api.py`; the native
Qt event fixture is `tests/sketch_visibility_controls.cpp`, using the accepted
mixed-roles saved model and adding an ordinary curve directly over construction.
Set `ALLOY_ENTITY_ROLES_FIXTURE` and `ALLOY_VISIBILITY_EVIDENCE` for the native test.
Qt event synthesis is separate from physical OS input. Native events target the
actual viewport or checkbox; initial window activation is not repeated between
steps. API, native input, authored/history/reopen and GPU evidence are distinct.

Verification passes in `build-preview-release` (Release arm64, Qt 6.11.2, OCCT
7.9.3, macOS 26.5.1): actual-entrypoint API, and native Qt 3 passed / 0 failed /
0 skipped in 6.124 s. The native fixture checks actual point/curve/profile hits,
projection hiding, overlap fallback to ordinary geometry, independent annotation
arrays, fully constrained filtering, exact unchanged solver/draft/history across
toggles, one meaningful relation Undo/Redo and authored save/reopen. API checks
add hidden stable-ID refusal and session reset after reopen. Existing accepted
dimension/constraint toggle authority is retained.

The inspected native focus capture shows all seven readable controls and the
remaining free/construction geometry, with fixed/dimensioned/projected sketch
geometry absent; independent dimensions remain visible. All-visible capture
restores green fixed geometry, black circle with fill, and purple projections.
The unrelated source solid remains visible. Apple M4 / Apple / OpenGL 4.1 Metal
- 90.5 is hardware accelerated; native viewport framebuffer is 1864 × 1332 at
scale 2 in the 1500 × 1000 test window. Summary contains executable hash and
receipts. Private API and native editors were closed after verification.

Independent review rejected the original immediate post-reopen terminal capture:
its queued dock layout had not settled and left a large blank area above Name.
A bounded `settledReopenCapture` native follow-up opens only the saved fixture,
re-enters edit, observes Name within the top 60 logical pixels of the actual
scroll viewport without scrolling/focus repair, then captures the settled state.
It passes 3/0/0 in 2.085 s; Name is at y=6, height=35 in a 564-pixel viewport.
The inspected `native/all-visible-settled.png` is readable with no blank gap;
it replaces `native/all-visible.png`. The premature capture is retained explicitly
as `all-visible-premature-superseded.png`. No application change was needed and
the modeling/API matrix was not rerun. The main native fixture now also waits
for observed layout readiness before its terminal capture. Private editor closed.
