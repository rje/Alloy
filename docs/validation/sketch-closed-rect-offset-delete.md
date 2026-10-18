# Projection-backed rectangle Offset Delete — SKT-144

Accepted 2026-10-18 after independent Astra review of source, retained
authored/evaluated deltas, tests and actual-app captures.
Public evidence and the bounded workflow contract are external at
`../Alloy-reference-records/knowledge/specs/skt144-closed-rect-offset-delete-2026-10-18.md`
(S1–S3, retrieved 2026-10-18). Those documents support geometry selection/Delete
and distinguish Offset association removal. The generated-result deletion cascade
is unknown. Whole-owner deletion below is an explicit Alloy policy, without an
exact-reference parity claim.

## Command and retained behavior

In an active sketch, select one generated rectangle Offset line in the canvas
or the Curves list and press Delete. Canvas context-menu Delete invokes the same
`sketch.deleteSelection` authority. There is no preview or confirmation: one
solved draft edit removes the `closedRectOffsets` owner, its four result edges
and four derived corner points. Undo and Redo each restore the exact draft in one
step. Finish Sketch commits it; Save/Open preserves authored IDs and flags.

The four linked projection rows and their source body/edge/optional occurrence
references remain. Each newly created owner now records `sourceProfiles`, four
booleans aligned with `sourceEntityIds`, so Delete restores each consumed
projection's actual prior profile flag, including false flags. Support geometry
and adjacent owners remain unchanged. Five-field legacy owners still load,
evaluate and edit; their result Delete refuses because prior flags were never
recorded. Recovery requires recreating the legacy operation with retained prior
state; guessing true is deliberately unsupported.

Source selections, source/result mixed requests, stale targets, malformed owner
boundaries, missing linked sources, surviving corner users, retained metadata,
constraints/dimensions and downstream explicit profile consumers refuse before
publishing. The existing historical downstream preflight remains authoritative.
No source geometry is detached or cascade-deleted, and derived shared corners
cannot become frozen free points.

## Evidence

Accepted build directory: `build-preview-release`, Release, Qt 6.11.2 private
runtime SDK. Application, kernel and focused tests built successfully on macOS
26.5.1 / arm64. Focused CTest `closed_rect_offset` and
`sketch_constructor_delete` pass 2/2. The rectangle test separately checks
alternating prior true/false profiles, exact independent-owner preservation,
source/mixed/stale refusals, legacy editability/Delete refusal, malformed/missing
references, surviving shared corners and a retained measurement.

The real-entrypoint runner is `tests/sketch_closed_rect_offset_delete_api.py`.
Its independently authored two coplanar Box-top face fixture creates eight real
linked projections and two disjoint rectangle Offset owners. Result Delete
removes one owner exactly, restores its source profile and leaves the second
owner byte-equivalent. Evaluated closed profile areas are 200 and 140 mm²; both
support bodies remain identical. Exact one-step draft Undo/Redo and authored
save/reopen pass. A retained measurement and a later Extrude consuming the
second generated profile independently refuse Delete without changing draft,
history or committed document. The latter reports `downstream_profile_in_use`
with stable feature/profile/removed-curve IDs.

`closed_rect_offset_controls_test nativeRectangleOwnerDelete` passes with the
same saved fixture. One initial harness activation is followed by canvas
selection/Delete, context-menu Delete and list selection/Delete with real focus
and selection assertions. Popup activation and closure are observed; list input
hits the tree viewport only after confirming no embedded child receives that
point. No focus or selection repair occurs between demonstrated steps. Exact
Undo/Redo and native saved reopen pass. These are Qt-delivered inputs, not
physical-device evidence.

External evidence directory:
`../Alloy-reference-records/validation/sketch-closed-rect-offset-delete-2026-10-18/`.
`api/receipt.json` records owner/source IDs, exact profile areas, binary SHA-256,
GPU and downstream refusal; `api/transcript.json` records shared command results.
`api/native-before.alloy` is the reusable native fixture. `native/native-receipt.json`
retains before/after drafts and GPU; the saved models and PNG captures are next to
these receipts. Hardware reports Apple / Apple M4 / OpenGL 4.1 Metal - 90.5,
4× MSAA and scale ratio 2. Actual captures were inspected for closed shaded
profiles, purple retained sources, result-edge selection and readable palette
controls. They are native framebuffer captures, with no matched public-state
visual comparison available.

No physical OS input, Windows/Linux runtime, broad styling parity or other
Offset owner family is established here. Other-platform and release gates remain
separate. Private validation editors were closed; the user-owned editor is
preserved.
