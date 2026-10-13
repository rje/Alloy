# Retained fit-spline Offset candidate — 2026-10-13

Issue #261 adds a signed, one-side Offset for an open authored fit spline. The retained offset stores its source entity ID, signed distance, stable derived endpoint IDs, and exact OCCT `Geom_OffsetCurve`; it does not shift fit poles or add solver unknowns. Two ordinary authored Lines can use the source and derived endpoint IDs to close one selectable profile. Edits to the offset distance or source fit points recompute the exact boundary, preserve the profile ID in the supported strip, and rebuild a downstream Intersect. Explicit offset UUID updates distinguish siblings; a source-only palette edit with more than one sibling refuses as ambiguous.

The focused core fixture uses four S-like fit points and a signed −5 mm offset. It asserts 129 independent normal-distance samples, a valid closed OCCT strip face, stable profile ID, a valid one-body Intersect, a supported source edit, undo/redo and exact save/reopen. A second offset and explicit-ID update leave the first unchanged. Oversized offset and a connector constraint that would bind a derived endpoint refuse without changing the draft. The signed-distance cusp guard samples finite curvature, while OCCT intersection and face validation provide further checks; this is a bounded open-spline policy, not a general proof for every curve.

Native Qt input selected the source in the viewport, entered signed distance, flipped side, previewed, canceled, reopened, and accepted. The final focused run asserts the renderer has one normal edge before preview, two during preview, one after Cancel, and two after accept. Its settled capture shows two distinct curves and readable palette labels. The native fixture does not draw the two closing Lines; the shared-API fixture exercises that full closure and Intersect. Native evidence: external `deliverables/fit-spline-offset-2026-10-13/native-attempt-04/` (`test.log`, `native-preview.png`, `native-accepted.png`, `native-state.json`). Earlier attempts 01–03 are preserved, including the discovered preview-display defect and its correction.

The real-app shared-API run made a stock body, fit spline, disposable Offset preview, accepted offset, two explicit snapped Lines, one closed strip profile, and Intersect. Editing −5 to −4 changed body volume from 4873.4976 to 3847.2344 mm³ while preserving the target body and profile IDs; undo/redo and save/reopen retained exact authored data. It passed 42 calls and exited cleanly. Evidence: external `deliverables/fit-spline-offset-2026-10-13/api-attempt-01/` (`report.json`, `transcript.json`, saved document, preview and solid captures); app SHA-256 `fda1094dc01d08de6aa1704166098dc5e94662bc135efaf0a3a77273a68a304a`. The later native-only preview display correction did not change geometry authority.

This ingredient does not establish the later eight-member ornament Join or the full tutorial; that cumulative Boolean risk is separately tracked in #265. Manual connector Lines are explicit because automatic closure is not established by the public source.

## Independent bounded acceptance

2026-10-13: candidate `20bf114` independently reviewed without implementing its
production changes or opening another editor. Review inspected the exact offset,
derived endpoint ownership, normal-distance oracle, sibling selection, profile
integration and preview refresh correction. Final native 3/3 in 3.601 seconds,
42-call real-entrypoint API evidence and actual preview/accepted/Intersect images
were inspected. The focused core binary was run once because its earlier pass
had no located durable log; it passed in approximately 0.73 seconds. Output is
external `deliverables/fit-spline-offset-2026-10-13/independent-core.log`.

| Contract | Result | Evidence and limit |
| --- | --- | --- |
| Signed 5 mm normal Offset | Matched | 129 independently calculated tangent/normal projections on the actual source and offset; OCCT retains the exact curve. |
| Explicit closed strip and downstream Intersect | Matched | Two snapped authored Lines, one valid face, retained target/profile identities, distance/source edits and exact history/reopen. |
| Disposable native preview, Flip, Cancel and accept | Matched within fixture | Real viewport selection and field/button Qt input; rendered edge count returns to one on Cancel and two on accept. Offset tool entry uses its QAction. |
| Visible preview and readable controls | Matched within scope | Final attempt04 shows both complete curves/endpoints; labels are separated. Earlier API preview predates the display correction and is not final visual evidence. |
| Independent sibling edits | Matched | Explicit UUID core update leaves the other distance unchanged; palette dispatch honors explicit selection and refuses ambiguous source-only selection. |
| Exact source pixels, physical OS workflow and arbitrary curves | Unknown | Source fit points/side are unspecified; native closure is not demonstrated, and cusp checks are finite samples with further kernel validation. |

Accept #261 as the bounded retained open-fit Offset/closed-strip ingredient.
The known cumulative ornament Boolean risk #265 remains open; this receipt does
not claim the complete tutorial or other-platform runtime acceptance.
