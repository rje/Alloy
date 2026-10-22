# Native OBJ import

XCH-024/#733, 2026-10-21. This bounded child exposes strict material-free OBJ
through File > Import Mesh and the existing responsive preview. XCH-017/#724
accepts cumulative bounded OBJ behavior after independent review of both child
receipts; XCH-022/#731 retains strict OS chooser and
uninterrupted native focus qualification. Independent source and final evidence
reviews accept this bounded child.

## Contract and source boundary

The permitted public contract is external at
`../Alloy-reference-records/knowledge/specs/xch017-obj-import-2026-10-21.md`
(OBJ-01–06, retrieved 2026-10-21). The adjacent native workflow contract is
`knowledge/specs/xch016-native-mesh-visual-2026-10-21.md` (REF-X016-01–06).
No proprietary application was operated or inspected; no reference assets ship.
The [strict core receipt](strict-obj-core.md) records parser/persistence policy.

File > Import Mesh and command discovery use `document.importMeshDialog`.
The native chooser offers combined STL/OBJ plus individual format filters.
Case-insensitive `.obj` and `.stl` suffixes select their strict readers. For other
suffixes, a bounded 4096-byte first noncomment record recognizes the supported
OBJ vocabulary; otherwise the existing STL adapter handles the file. This is
bounded routing, not a universal format detector. Complete-file strict parsing
still follows; an unknown prefix or long leading comment in an extensionless
OBJ may require renaming to `.obj`.

`mesh.importPreview {path}` opens the shared cancellable worker panel, default
mm. Poll `app.state.meshImport.status` for ready/failed. `mesh.importUnits` uses
mm/in interpretation, source XYZ/origin and identity placement. OBJ owns a
bounded original byte snapshot and invokes the same strict parser for every
unit choice: polygon tolerances apply after canonical-mm scaling, just as for
`document.importObj`. It can retry another unit even after default-mm refusal
without rereading the source. STL retains its original owned-coordinate route.

Ready preview shows source filename, triangle/vertex count, scaled mm min/max,
ignored record counts, total source label count and at most three labels of
40 UTF-16 units each (ellipsis and remaining-count disclosure). All source text
is plain text. Complete labels and ignored names remain in authored metadata
and API inspection. The panel states that materials, UVs and source normals
are not applied. It retains the existing Unit Type, OK, Cancel and GPU facets.
Placement remains identity; there are no material, texture or placement controls.

OK invokes `document.importObj` with the exact opaque prepared candidate through
the common document commit authority. It never rereads the source; one history
entry is labelled Import OBJ mesh, with selected visible mesh/Browser identity.
Undo/Redo and native save/reopen preserve the geometry without a source file.
Cancel and validation failure leave authored geometry, selection, dirty/history
and saved bytes unchanged; viewport framing may change. Failed preparation shows
filename/reason/remedy in the native panel and removes invalid preview facets.

## Verification

Evidence root:
`../Alloy-reference-records/validation/xch024-native-obj-2026-10-21/`.
The accepted cache is Release AppleClang arm64, Qt 6.11.2 private runtime
(`/Users/rje/src/cpp/Alloy-dependencies/qt-private-runtime/install/lib/cmake/Qt6`),
OCCT 7.9.3 at `/opt/homebrew/lib/cmake/opencascade`, build `-j4`.
No paid CI, tag, release or reference runtime was used.

Reproduction (private editor lease required; preserve user-owned editors):

```sh
cmake --build build-preview-release --target alloy native_obj_import_controls_test strict_obj_import_test retained_stl_import_test -j4
ctest --test-dir build-preview-release -R '^(strict_obj_import|retained_stl_import)$' --output-on-failure
python3 tests/native_obj_import_api.py --app build-preview-release/alloy.app/Contents/MacOS/alloy --output ../Alloy-reference-records/validation/xch024-native-obj-2026-10-21/api-final
```

Final build succeeded. `focused-tests-final.log` records **2/2 CTests passed**
(1.62 s). `api-final/summary.json` and `transcript.json` record **248 checked
API responses**. `ready-dpr2-final/` and `ready-dpr1-final/`, with their adjacent
`native-dpr*-final.log`, each pass **4/4 QtTest results** (4.075 s and 3.101 s).
These are two independent single-preview scenarios per scale: naturally focused
keyboard mm→in plus actual-hit Cancel, and initial ready state plus actual-hit
OK followed by API history checks. They do not waive the after-Cancel focus gate. Native input covers the
grouped fixture; quad and concave geometry are separately covered through the API.

Inspected screenshots show readable bounded labels, counts and units; the
committed grouped fixture appears under Mesh Bodies with its independent mesh
icon, visible checkbox and real GPU facets. `api-final/invalid.png` shows the
native failure panel and disabled OK. The native framebuffer captures report
Apple vendor, Apple M4 renderer, OpenGL `4.1 Metal - 90.5`, hardware acceleration.
At effective DPR1 the ready GPU frame is 714×608 and resized committed frame
1108×548; DPR2 gives 1428×1216 and 2216×1096 at matching logical window sizes.
`presentation_probe.py`/`presentation/` additionally verify the offset quad's
API pick/highlight, hide/show, fit and post-resize pick; its two selected captures
were inspected. This is API selection evidence, not native canvas input evidence.

Independent source review identified a unit-threshold divergence in the first
candidate (mm triangulation then scaling). The shared owned-byte parser corrects
that before final qualification. Re-review found no remaining source blocker;
the final independent evidence review accepted this bounded result after
checking source/binary hashes, API transcript, native logs and inspected captures.
All private validation editors
exited; only user-owned PID72618 remained.

The API fixture creates original grouped tetrahedra, offset quad and concave
pentagon, each in mm/in; compares every authored vertex and deterministic triangle;
checks original geometry through repeated unit updates, Cancel, source deletion
before OK, exact preview commit, selected identity, Undo/Redo and source-independent
save/reopen. Uppercase `.OBJ` and content-routed `.mesh` are exercised. A late bad
face preserves document, selection and prior saved bytes. Two independently
specified threshold cases check direct/native agreement: a tiny nonplanar quad
passes mm/refuses in; a 1e-13 source-unit triangle refuses mm/passes in. Unit
switches still work after source deletion, including retry from failed default mm.

The explicit native fixture is not registered in automatic CTest. Set
`ALLOY_MESH_FIXTURES` to the API fixture directory and `ALLOY_MESH_EVIDENCE` to an
isolated output directory; invoke `native_obj_import_controls_test readyPanel`
for mm-to-in keyboard/Cancel, or `commitPanel` for one native actual-hit OK.
Each case starts with its own initial PID-targeted activation. No focus or
selection repair is allowed between demonstrated input steps. Effective DPR1
uses `QT_SCALE_FACTOR=.5` on this host's DPR2 display; run separately.

## Compatibility and remaining limits

Documented broad workflow matched: local mesh insertion, explicit unit preview,
read-only facets and OK/Cancel. This does not establish exact defaults, styling,
error text or keyboard parity with a reference application; those remain unknown
where the public evidence is insufficient. Material/placement breadth is a known
deliberate mismatch retained by the broader import families. The retained STL
panel provides an internal comparison at the same theme, window and scale;
public screenshots are unmatched and cannot support a pixel-exact claim.

`after-cancel-focus/observed.txt` records the first DPR2 attempt: initial natural
units focus, Down-to-inch and actual-hit Cancel passed with unchanged authored/
selection/save bytes, but a subsequent API-opened preview did not retain the
native editor key window. The test stopped before another key/click, with no
repair or global input. XCH-022/#731 owns that concrete sequence. Strict native
OS source selection/popup closure, uninterrupted multi-import native input,
physical hardware input and Win/Linux runtime are unqualified here. Full-cap
OBJ latency and process memory remain separate from the parser resource bounds.

## Cumulative XCH-017 disposition

The independent cumulative review accepted XCH-017/#724 against its bounded
criteria without rerunning unchanged workflows. XCH-023's final real-app
transcript refuses a late invalid index at line 6/face 2, a bowtie and a
nonplanar face at line 5/face 1, each with a remedy; following document and
selection inspections equal their pre-error values, and the fixture checks
unchanged saved bytes. Original grouped and planar-quad imports in mm/in have
independently checked vertices and triangles, source-independent history/reopen,
and XCH-024 adds native preview/commit plus Browser/GPU presentation. This
closes the specified material-free OBJ slice. XCH-022 and XCH-004 retain strict
chooser/focus and combined STL/OBJ parity; later owners retain broader materials,
hierarchy, placement, arbitrary polygon support and physical/platform coverage.
