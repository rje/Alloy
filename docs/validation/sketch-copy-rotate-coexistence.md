# Copy beside retained Rotate

SKT-189/#712 candidate, 2026-10-20. This fixes an Alloy Copy/Create Copy
integration refusal in a sketch with an unrelated retained Rotate. The full
candidate still evaluates the complete sketch with strict Rotate source,
axis, angle and output-cache validation. Only the disposable one-profile
geometry check now starts from the qualified clipboard context and receives
the fresh copied records from the solved candidate. It excludes the unrelated
Rotate as one whole owner, without changing its authored data or dependencies.

This issue is an Alloy code interaction found during SKT-188 validation; it
does not establish reference-product behavior. Public sources in the external
SKT-180 and SKT-179 records document the separate Rotate and Copy workflows.
The source/geometry composition behavior and exact visual parity are not
documented there. See
`../Alloy-reference-records/knowledge/specs/skt189-copy-rotate-coexistence-2026-10-20.md`
and [the adjacent Rotate source-size receipt](sketch-rotate-source-size.md).

## Bounded workflow contract

In one XY sketch, retain a valid rotated four-Line profile and author a
disconnected ordinary rectangle. Select and Copy the rectangle, then place a
fresh independent copy without overlap or contact. The Move/Copy Create Copy
route uses the same independent geometry authority and must leave the user's
clipboard unchanged. The copied points, entities and supported internal
relations receive fresh deterministic IDs; neither copy depends on the Rotate
owner.

The full-sketch solve remains authoritative for all authored geometry. A
malformed Rotate owner refuses the operation atomically. Copying Rotate-owned
members, an incomplete selection, or an overlapping/touching placement also
refuses without draft/history changes. Editing either the Rotate angle or the
copied profile preserves the other profile's authored geometry. Undo/Redo and
save/reopen preserve stable IDs and owner/cache data.

## Candidate evidence

External captures and reports are in
`../Alloy-reference-records/validation/skt189-2026-10-20/`.

- Build: existing `build-preview-release`, Release, Apple Clang, Qt 6.11.2,
  OCCT 7.9.3; `-j4`. No new build tree, CI, or release.
- Focused core regression: `sketch_copy_test` passes the ordinary
  Copy/Paste-with-Rotate case, exact Rotate owner/axis preservation, fresh IDs,
  deterministic preview identity, internal relation remapping, edits in both
  directions, exact history/reopen, overlap/contact and malformed-owner atomic
  refusals. Selecting Rotate-owned members remains refused.
- Real-app API: `tests/sketch_copy_rotate_api.py` passes both Copy/Paste and
  Move/Copy Create Copy in one model. It checks preview/accept, independent
  IDs, exact retained Rotate and axis, clipboard preservation, independent
  edits, Undo/Redo and authored save/reopen. Four separate 8 mm extrusions
  retain stable profile/body IDs and total 7,712 mm³.
- Native actual-hit input: `sketch_copy_rotate_controls_test` selects the
  ordinary rectangle by four canvas hits with Shift selection, verifies the
  resulting view focus and selection, opens Move/Copy through the Modify menu,
  toggles Create Copy in its actual checkbox, enters placement in the native
  fields, and accepts. Source/cache/axis, copied records, and Undo/Redo pass.
  The native screenshot shows the live sketch with the retained rotated profile
  and two independent rectangles. Labels in the small rotated fixture are
  crowded; no matched public visual comparison is available.
- Independent binary STL readback: four watertight components, each with Euler
  characteristic 2 and 12 triangles; combined volume 7,711.9998798 mm³ (analytic
  7,712 mm³), bounds `[12.503863, 2.514548, 0]` to `[165, 22, 8]` mm.
- GPU capture: Apple M4 hardware-accelerated OCCT OpenGL, OpenGL 4.1 Metal -
  90.5, 2x framebuffer scale. This is prototype OpenGL evidence, not renderer
  migration or cross-platform runtime qualification.

Reproduce the focused checks from the repository root:

```sh
cmake --build build-preview-release --target alloy sketch_copy_test sketch_copy_rotate_controls_test -j4
./build-preview-release/sketch_copy_test
ALLOY_COPY_ROTATE_EVIDENCE=/absolute/evidence/path ./build-preview-release/sketch_copy_rotate_controls_test
python3 tests/sketch_copy_rotate_api.py --app build-preview-release/alloy.app/Contents/MacOS/alloy --output /absolute/evidence/path/api
```

Independent Sol read-only review accepted `753f2c8` for this bounded scope. It
checked the disposable clipboard-derived model against unchanged full-sketch
Rotate validation, both Copy routes, saved identities/history and independent
STL readback. The reviewer did not rerun the app. Crowded Rotate annotations
in the cumulative capture are disclosed above; matched visual parity remains
unknown. SKT-189/#712 is closed/Done.
