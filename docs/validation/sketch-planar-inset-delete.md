# Planar inset loop Offset Delete (SKT-152 / #578)

Accepted bounded qualification, 2026-10-18. An independent Sol review checked
the current binary/test hashes, transcript, saved models, native input route and
GPU captures and found no blocker; the existing focused execution was sufficient.
This extends the [single-owner Delete contract](sketch-loop-offset-delete.md)
to a retained `planarInsetV1` sketch on the actual 18-edge enclosure split cap.
It introduces no product controls or modeling changes.

In Sketch Select, click the generated interior profile and press Delete.
The shared command removes its one retained local Offset owner without
confirmation, preserving the face sketch and exact support reference. Undo and
Redo each use one draft-history step. Generated boundary edges are evaluator
results; they are not authored entity IDs. No new defaults/options are added.

Dated public evidence and the explicit Alloy ownership policy are external:
`../Alloy-reference-records/knowledge/specs/skt145-loop-offset-delete-2026-10-18.md`.
Select/Delete is documented; reference cascade, region-hit behavior and exact
styling remain unknown. No matched public reference screenshot is available.
No proprietary reference application was operated.

## Evidence

Evidence directory:
`../Alloy-reference-records/validation/sketch-planar-inset-delete-2026-10-18/`.
Source fixture:
`../Alloy-reference-records/deliverables/enclosure-rim-2026-10-14/seam-probe/split-prefix.alloy`.

- Build: `build-preview-release`, Release, Qt 6.11.2, OCCT 7.9.3, Darwin arm64;
  inspected the cache before building.
- API: `tests/sketch_planar_inset_delete_api.py` uses the real application
  entrypoint and shared authoritative commands. It creates two independent
  inset sketches on the same retained support, widens stock 64 to 65 mm,
  checks stable profile/support identity with changed evaluated area, then
  deletes only the selected owner. The upstream authored features, evaluated
  bodies and independent sibling remain exact. Duplicate, wrong-profile, stale
  and mixed targets refuse atomically. Draft Undo/Redo and authored save/reopen
  pass exact comparisons. A committed downstream Extrude consuming the inset
  refuses before draft/history publication, with stable consumer/profile IDs
  and unchanged session, authored graph and evaluation.
- Native Qt: `rounded_loop_offset_controls_test generatedProfileDelete` reuses
  `native-before.alloy` from that API run. A real viewport hit selects the
  generated profile, verifies its stable ID and viewport focus, then delivers
  Delete to the actual focus widget. Initial harness focus is separate;
  no focus/selection repair occurs between click and Delete. Exact one-step
  history, support retention and reopened authored state are checked.
- GPU and visual inspection: before, selected, deleted, undone and reopened
  API/native framebuffer PNGs are retained separately. The selected thin U
  inset is blue, support remains gray, and Delete removes the generated boundary
  and profile highlight; Undo restores them. The gray center in top view includes
  the other split half and is not an empty-cavity depiction. Labels remain
  readable. Apple / Apple M4 / OpenGL 4.1 Metal - 90.5, hardware acceleration true,
  framebuffer scale 2; details are in `receipt.json`. The measured inset area
  changes from 233.8576962999306 to 236.2894677168922 mm².

The first API attempt reached Delete/history/reopen, then correctly refused a
cross-component Extrude because reopen resets the active component. The runner
now explicitly activates the sketch owner before authoring its consumer. This
was fixture setup, with no product fix.

Stale support/lineage errors remain atomic with terse evaluator wording;
recovery guidance for those errors is unqualified and tracked separately.

Physical OS input, Windows/Linux runtime, resize/display-scale changes,
multiple owners within one sketch, arbitrary multiwire faces, outward offsets
and exact visual parity remain unqualified. The source edit is the retained
64-to-65 mm stock-width edit, not a claim about arbitrary topology changes.
Stale support/lineage recovery wording remains SKT-154/#580.

## Reproduce

```sh
cmake --build build-preview-release --target alloy rounded_loop_offset_controls_test -j 6
python3 tests/sketch_planar_inset_delete_api.py --app build-preview-release/alloy.app/Contents/MacOS/alloy --source ../Alloy-reference-records/deliverables/enclosure-rim-2026-10-14/seam-probe/split-prefix.alloy --output ../Alloy-reference-records/validation/sketch-planar-inset-delete-2026-10-18
ALLOY_LOOP_DELETE_EVIDENCE=/Users/rje/src/cpp/Alloy-reference-records/validation/sketch-planar-inset-delete-2026-10-18 build-preview-release/rounded_loop_offset_controls_test generatedProfileDelete
```

Run one private editor at a time; preserve user-owned instances. The API
harness closes its private process on success and failure. Native tests close
their window before exiting.
