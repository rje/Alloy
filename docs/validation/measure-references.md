# Measure source integrity (DOC-042)

Status: bounded implementation and real-application qualification accepted after
independent Sol review. Native actual-hit controls remain
DOC-040; the broader Measure modes remain DOC-041/DOC-014.

The existing public-source contract is external at
`knowledge/specs/doc039-measure-minimum-2026-10-19.md`, retrieved 2026-10-19.
That contract documents selection/result flow; ambiguity wording, stable source
IDs and the following refusal policy are Alloy requirements, not observed
reference-product behavior. No proprietary application was operated.

## Bounded behavior

Measure's sketch-edge picker collects visible finite straight-edge hits within
seven logical pixels. It preserves the closest hit when distinguishable and
returns `measure_ambiguous` if multiple candidates fall within half a logical
pixel of the closest hit. This accounts for rounded GPU projection coordinates
and removes the former dependence on sketch edge hash iteration. An ambiguous
pick leaves transient Measure sources unchanged. Hide an overlapping sketch or
use exact source IDs to disambiguate; no new choice popup is claimed.

The shared evaluator resolves each source before checking duplicates. Sketch
identity uses sketch plus entity/point ID. Body identity uses body, resolved
occurrence and edge/face ID. Optional producer fields, explicit versus inferred
single occurrence, or additional point/pick metadata cannot disguise the same
source as two selections. Duplicate picks refuse without consuming a selection.
Invalid/missing/hidden sources refuse as unavailable; multiple current placed
instances require an explicit occurrence. Inspection always resolves current
geometry rather than supplied or cached coordinates. Original source IDs remain
unchanged in structured results and authored saves.

## Reproduction and evidence

Build with the existing Release cache, Qt private runtime and Homebrew OCCT:

```
cmake --build build-preview-release --target alloy -j4
python3 tests/measure_reference_api.py --app build-preview-release/alloy.app/Contents/MacOS/alloy --output ../Alloy-reference-records/validation/doc042-measure-references-2026-10-19/api-final
```

The final Release build and focused **147-call** real-entrypoint fixture pass
on macOS. The actual GPU reports Apple / Apple M4 / OpenGL 4.1 Metal - 90.5,
hardware acceleration and a 2× native resolution ratio. Inspected
`api-final/resolved-line.png` has a readable named selection and 10.0000 mm
result. This fixture covers overlapping visible sketch edges
(including overlapping entries in one sketch), API pointer ambiguity refusal,
unchanged transient selection on refusal, a distinguishable pick after hiding
the overlap, equivalent-source duplicate refusal and missing/hidden identities.
A measured sketch line changes from 10 to 20 mm under an endpoint edit, follows
Undo/Redo, refuses after deletion and recovers the original IDs under Undo.
A stable box top face changes from 200 to 300 mm² after its Width parameter edit
and follows Undo/Redo. Explicit placed face and edge identities survive
occurrence-copy Undo/Redo and save/reopen; a point-to-vertical-edge distance
changes from 6 mm on the original occurrence to `hypot(100, 6)` mm on the
translated occurrence, checking actual placed coordinates independently of the
translation-invariant face area. Bare shared body identity refuses as ambiguous.
Every inspection group compares the complete `document.inspect` response;
measurement-only saved files are byte-identical.

External evidence is under
`validation/doc042-measure-references-2026-10-19/`: build logs, API transcript,
retained editable fixtures and a native framebuffer capture. The capture is a
semantic Alloy layout check only; public matched-state/pixel comparison and
native input are outside this child. No Windows/Linux or packaged-platform
runtime, topology repair or broader geometry-family claim follows from this
fixture. One private validation application is used at a time and exits. Independent Sol review found no material blocker for this named scope.
