# Multiple planar-inset loop owners (SKT-158 / #584)

Accepted bounded qualification, 2026-10-18. Independent Sol review reran the
native test (3/3), matched app/native binary and saved-model hashes, and
inspected the GPU captures; it found no material blocker.
This validates the existing [multiple-owner contract](sketch-multiple-loop-offset-delete.md)
for two `planarInsetV1` owners in one retained support-face sketch. No product
controls, modeling code, defaults or ownership policy changed.

Repeat Offset of the same support at -0.75 and -1.25 mm. In Sketch Select,
click either generated interior and press Delete. Exactly that owner disappears;
the sibling, shared support and upstream sources remain exact. Undo and Redo
are each one draft-history step. Public evidence, explicit Alloy overlap-pick
policy and unknown reference cascade/styling are recorded in the external dated
contract `../Alloy-reference-records/knowledge/specs/skt153-multiple-loop-owners-2026-10-18.md`.
No proprietary reference application was operated.

## Evidence

Evidence directory:
`../Alloy-reference-records/validation/sketch-multiple-planar-inset-delete-2026-10-18/`.
Source fixture:
`../Alloy-reference-records/deliverables/enclosure-rim-2026-10-14/seam-probe/split-prefix.alloy`.

- Build: inspected `build-preview-release/CMakeCache.txt`, Release, native arm64,
  Qt 6.11.2 private runtime and OCCT 7.9.3. Reused the accepted unchanged app and
  native test binaries; no new build or core-implementation claim. Binary/source
  hashes are retained in `receipt.json` and `native-receipt.json`.
- Real entrypoint API: `tests/sketch_multiple_planar_inset_delete_api.py` creates
  both owners on the actual 18-edge split cap. Append-preview Cancel and exact
  second-owner Update/Undo preserve the first row. The retained 64-to-65 mm stock
  edit preserves both profile IDs and authored support/owner rows while areas
  change from 470.804510 to 475.253096 and 233.857696 to 236.289468 mm².
  Both deletion orders preserve the exact surviving profile summary, upstream
  authored features, evaluated bodies and support. Duplicate, multiple, stale
  and mixed targets refuse with unchanged session/authored/evaluation.
  One-step draft Undo/Redo and authored save/reopen pass for both orders.
  A downstream Extrude consuming the first owner refuses its Delete atomically
  with exact consumer/profile diagnostics; deleting the unconsumed sibling
  preserves the evaluated solid exactly. `safe-sibling-deleted.alloy` is retained.
- Native Qt: the existing `multipleOwnerProfileDelete` test opens the retained
  baseline separately for each owner. Each actual viewport hit selects the
  expected stable profile and establishes viewport focus; Delete goes to the
  current focus widget. Initial harness activation is separate, with no focus
  or selection repair between click and Delete. Exact row removal, one-step
  history, sibling/body geometry and save/reopen pass. All four API/native
  deletion files compare exactly as authored JSON. Native log: 3 passed, 0 failed,
  0 skipped (28.296 seconds).
- GPU/presentation: inspected actual API selection and native selected, deleted,
  undone and reopened captures. Smooth nested U-shaped boundaries, blue selected
  regions and gray support remain readable; Delete removes the chosen boundary,
  the sibling remains, Undo restores both. The top-view gray center includes
  the retained split half; the reopened isometric view shows its actual cavity.
  Apple / Apple M4 / OpenGL 4.1 Metal - 90.5, hardware acceleration true,
  framebuffer scale 2. Both runs use a 1500 × 950 logical window, unmatched to
  public references. Captures and exact graphics details are external.

The API and native private editors exited; user-owned PID 33805 was preserved.
Physical OS input, Windows/Linux runtime, resize/display-scale changes,
arbitrary multiwire support, outward offsets, coincident-owner disambiguation,
offset-of-offset dependencies and exact reference styling remain unqualified.
Stale support/lineage recovery guidance remains separate SKT-154/#580.

## Reproduce

```sh
python3 tests/sketch_multiple_planar_inset_delete_api.py --app build-preview-release/alloy.app/Contents/MacOS/alloy --source ../Alloy-reference-records/deliverables/enclosure-rim-2026-10-14/seam-probe/split-prefix.alloy --output ../Alloy-reference-records/validation/sketch-multiple-planar-inset-delete-2026-10-18
ALLOY_MULTI_LOOP_DELETE_EVIDENCE=/Users/rje/src/cpp/Alloy-reference-records/validation/sketch-multiple-planar-inset-delete-2026-10-18 build-preview-release/rounded_loop_offset_controls_test multipleOwnerProfileDelete
```

Use one private validation editor at a time and preserve user-owned instances.
