# SKT-146: retained fit-spline Offset result Delete

Accepted 2026-10-18 after independent Sol review of source, focused tests,
transcripts and actual-app captures. The shared
`sketch.deleteSelection` / `SketchGeometry::deleteEntities` command now removes a
selected `fitSplineOffset` result entity and its two derived endpoint records.
The authored source spline, its points, sibling Offset owners and unrelated
relations remain. Source-only and mixed source/result requests refuse before
publication. A surviving Line or retained metadata using a result endpoint also
refuses; endpoints never become free points as a side effect. Remove dependent
geometry explicitly before retrying result Delete.

The dated permitted evidence and implementation specification are external:
`../Alloy-reference-records/knowledge/specs/skt146-fit-spline-offset-delete-triage-2026-10-18.md`.
S1 documents selected geometry Delete and context Delete; S2/S3 document Offset
association removal. Exact reference result-geometry cascade is unknown. This
whole-result policy is Alloy's explicit implementation contract, not a parity
claim. Association-only removal while retaining an independently editable curve
remains SKT-149/#575; this owner has no separate glyph relation.

In Select mode, select the result curve or its sketch-list row, then press Delete
(or Backspace through the same existing command), or choose canvas context Delete.
No new options, preview or confirmation are added. Existing annotation-only
`constraintIds` Delete remains separate and never expands into this owner.
One successful result Delete creates one draft history step. Refusal preserves
exact authored draft and history. Undo/Redo restores/removes the same entity and
endpoint IDs; saved reopen retains exact authored data.

## Verification

The accepted build cache is `build-preview-release`: Release, this repository's
source directory and Qt 6.11.2 from
`/Users/rje/src/cpp/Alloy-dependencies/qt-private-runtime/install/lib/cmake/Qt6`.
The app, focused core and native-widget targets built successfully.

`ctest --test-dir build-preview-release -R '^(fit_spline_offset|sketch_constructor_delete)$' --output-on-failure`
passes 2/2. The Offset test now checks exact remaining parameters after deleting
one of two siblings, specifically excluding only the selected result and its two
endpoint records. It checks both mixed-selection orders, source/stale refusal,
one- and two-Line dependency refusal, and success after explicit connector
removal. Existing exact strip/profile, downstream Intersect, source edits and
history/reopen checks still pass. The constructor test guards the shared owner
Delete authority's other supported families.

`tests/sketch_fit_spline_offset_delete_api.py` passes through the actual app
entrypoint with a fixed unrelated circle, an authored source spline, and two
explicitly distinct signed Offset owners. It checks exact source/sibling/point
and unrelated fixed-relation preservation, endpoint removal, one history step,
atomic refusals, independent annotation-only removal, exact Undo/Redo and authored
save/reopen. The explicit `sketch.splineOffset` creation command is used because
the source-only palette route edits an existing owner rather than creating a
sibling. A live closed-strip dependency is refused; after explicitly removing
its two Lines, result Delete succeeds. This fixture remains an open sketch plus
an unrelated valid circle region after deletion; no downstream feature-dependent
Delete acceptance is claimed.

`widget_interactions_test sketchFitSplineOffsetDelete` passes all three native
Qt routes: canvas click/Delete, sketch-list row click/Delete, and selected curve
context Delete. A real blank-canvas deselect is an explicit first user step;
empty selection is observed before the result click. The receiving widget owns
focus before keys, exact result IDs are observed, and popup activation/closure is
observed. There is no focus or selection repair between those demonstrated
steps. Initial macOS activation is harness setup. This is native Qt evidence,
not physical hand input. The existing popup-window warning remains nonfatal.

Direct selection while the source's tangent grips are visible is a concrete
remaining hit-priority gap: an exact result-curve hit at logical pixel (292,185)
is covered by a source tangent grip at (291,186), inside its 9 px hit radius.
The source remains selected in the earlier direct-click attempt even though
`pickSketch` returns the result at zero pixel distance. The retained final
`native.log` records grip positions and source selection;
`native-source-selected.png` shows the two distinct curves before deselect.
The passing workflow explicitly deselects first and does not qualify that
source-selected direct-click path. SKT-151/#577 owns this separate hit-priority
gap; result Delete's geometry authority is accepted here.

Evidence is external at
`../Alloy-reference-records/validation/skt146-fit-spline-offset-delete-2026-10-18/`:
`core.log`, `api.log`, `native.log`, `api/receipt.json`, `api/transcript.json`,
`api/selected.png`, `api/deleted.png`, `api/reopened.png` and
`api/fit-spline-offset-deleted.alloy`. Build logs are adjacent
`skt146-build.log`, `skt146-core-build.log`, `skt146-native-build.log`.
The API receipt records the exact final app hash and GPU state.

All three actual captures were inspected: selected result is orange between the
source and sibling; after Delete the remaining two smooth blue curves and fixed
circle remain. Reopen leaves Sketch mode and widens the viewport, so it proves
visible persistence rather than pixel-identical framing. GPU: Apple / Apple M4,
hardware accelerated OpenGL `4.1 Metal - 90.5`, native framebuffer 2376 × 1216 at
2× scale in the reopened capture. Capture combines Qt chrome and the native OCCT
framebuffer. Matched public styling, physical hand input, downstream
feature-dependent deletion and Windows/Linux runtime remain separate gates.

Private validation windows closed; user-owned PID 33805 was preserved.
