# Rounded region-union prism provenance

GEO-006/#309 restores both existing bracket checkpoints that failed Open before
REL-012 native input. The cause is the circular curve parameter seam: one outer
semicircle survives region union as two exact edges, with retained contact →
empty and empty → retained contact. Prism generation then produces two
cylindrical faces for the same source-circle identity. The older edge symptom
uses the same split-boundary publication path.

`unionProfiles` now canonicalizes only a degree-two, same-source circular vertex
with no authored contact. OCCT same-domain edge unification protects every real
contact and every noncircular vertex. Its exact edge and vertex Modified history
supplies the canonical boundary and oriented endpoint contacts. Missing or
conflicting history refuses. There is no coordinate search, retry, tolerance
change, or relaxation of prism ambiguity refusal. Existing face/edge ID formulas
are unchanged; unaffected boundaries and the source-derived cylinder IDs remain
stable. Artificial seam fragments are replaced by the boundary between retained
contacts; arbitrary saved references to those fragments are not newly qualified.

## Evidence, 2026-10-18

Accepted native `build-preview-release` is Release/arm64, private Qt 6.11.2 and
OCCT 7.9.3. Both original saved documents fail on the baseline actual application
with `open_failed` / `evaluation_failed`, “Ambiguous cylindrical prism face
provenance.” Baseline errors and original-model hashes are retained externally.

Focused core checks pass 4/4: `prism_provenance`, `planar_faces`, `solid_fillet`,
`face_support`. The new capsule fixture proves exact volume, six selected-region
orders, unchanged source-derived cylindrical face IDs, source width edit and
continued refusal when distinct faces claim one circle identity. The existing
rounded-bracket face-support gate proves source radius/width/depth edits,
projection, undo and reopen. Two pre-existing temporary-object `QJsonValueRef`
lookups in that test now use owned `.value()` results; assertions are unchanged.

`tests/prism_provenance_api.py` passes through the real application with the
retained full-size and angle-bracket documents. Across two fresh private
processes it performs 20 original checkpoint Opens plus four edited-file reopens.
Both documents preserve exact original authored data; source radius 100 → 90 mm
changes volume from approximately 4,694,718.448 to 4,683,766.401 mm³, while all
8 current support-face IDs and 24 edge IDs remain stable. Source-edit Undo/Redo
and edited save/reopen preserve exact authored intent. Each result is one valid
solid. Native framebuffer captures show the intact rounded five-hole bracket;
Apple M4 reports hardware acceleration, Apple vendor, and OpenGL 4.1 Metal 90.5.
Private editors exited; user-owned PID33805 was preserved.

External evidence:
`../Alloy-reference-records/validation/geo006-prism-provenance-2026-10-18/`
contains `baseline/`, `candidate-probe/` and `api-final/receipt.json`, both process
transcripts, captures and saved radius-90 models. The original failed fixtures
remain in `validation/rel012-ordinary-input-2026-10-18/` outside the repository.

This qualifies the bounded provenance repair on macOS. Independent review is
pending. REL-012's native focus/selection/popup/export workflow, physical OS input,
other-platform runtime and broader topology repair remain separate gates.

Reproduce the actual-app check with explicit paths:

```sh
python3 tests/prism_provenance_api.py \
  --app build-preview-release/alloy.app/Contents/MacOS/alloy \
  --output ../Alloy-reference-records/validation/geo006-prism-provenance-2026-10-18/api-review \
  --fullsize ../Alloy-reference-records/validation/rel012-ordinary-input-2026-10-18/fullsize-failed-repro.alloy \
  --angle ../Alloy-reference-records/validation/rel012-ordinary-input-2026-10-18/angle-failed-repro.alloy
```
