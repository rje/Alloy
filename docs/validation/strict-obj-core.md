# Strict material-free OBJ core/API

XCH-023/#732 implements the geometry/persistence child of XCH-017. XCH-024
owns native chooser, ready preview, input and GPU qualification. This receipt
records Alloy policy and bounded verification; it claims no reference-product
pixel or interaction parity.

## Contract selected before final verification

The public source/provenance and workflow contract is external at
`../Alloy-reference-records/knowledge/specs/xch017-obj-import-2026-10-21.md`
(OBJ-01–06, retrieved 2026-10-21). Original Wavefront syntax defines records and
indices; strict supported vocabulary and all numerical limits below are Alloy
policy. No proprietary application was operated or inspected.

The adapter constructs owned double-precision vertices and indexed triangles
directly. It never calls RWObj, resolves a sidecar, or passes untrusted OBJ to a
reader. Existing viewport code constructs OCCT triangulations from the retained
mesh. This supersedes decision 004's proposed RWObj decoder path for this slice;
its permissive raw-reader observations remain valid foundation evidence. There
is no new dependency or license. No TKDEOBJ/TKRWMesh linkage is needed.

Explicit `mm` or `in` multiplies received XYZ once by 1 or 25.4. Identity
placement, source v order (including unused positions), origin and triangle
winding are retained. There is no repair or inferred solid. `document.importObj`
uses the shared retained document commit/selection/history authority and returns
normal document state. It accepts only `path` and `units`.

- 64 MiB source bytes; 200000 predicted triangles; 600000 entries per v/vt/vn
  table; 3–256 corners per face; 65536 UTF-16 code units per physical record.
- Admission estimate: source bytes + predicted triangles × 2048 + total v/vt/vn
  entries × 256 must not exceed 512 MiB. This is a conservative adapter estimate,
  not a measured whole-process peak-memory guarantee or a latency qualification.
- Canonical coordinates must be finite and at most 1e9 mm in absolute value.
- Each face uses canonical-mm maximum axis extent E. Planarity tolerance is
  max(1e-9 mm, 1e-8 × E); projected linear tolerance max(1e-12 mm, 1e-12 × E);
  projected double-area tolerance max(1e-24 mm², 1e-12 × E²).
- The summed translated cross products establish the normal; projection drops
  its largest absolute axis. Repeated corners, collinear intermediate corners,
  touching/crossing nonadjacent edges and unsafe zero-area ears are refused.
  Ear clipping scans surviving corners in source order, emits the first valid
  ear and restarts at the lowest surviving original corner. Every accepted
  n-gon emits exactly n−2 triangles with received winding.
- Object/group labels retain the normalized name remainder, record kind and
  first source line, in first-occurrence order. Duplicate kind/name pairs need
  not repeat. Each label/name remainder is bounded to 256 UTF-8 bytes and each
  label collection to 1024 entries. A multi-name `g` remainder remains one label;
  this records provenance and does not create hierarchy or multiple bodies.

`v` requires three finite coordinates; homogeneous/color extensions fail.
`vt` takes 1–3 and `vn` three finite numbers; their tables are counted and
referenced optional indices validated, then discarded. Faces accept v, v/vt,
v//vn and v/vt/vn, including negative current-table relative indices. Zero,
forward, malformed, overflow and out-of-range indices fail on their source line.
Comments and blank lines are accepted; invalid UTF-8, NUL, continuations and
unknown directives fail. `s`, `mtllib`, `usemtl`, `l` and `p` are ignored with
counts; material/smoothing name remainders are bounded and retained. Line/point
indices are validated. There is no filesystem access to sidecars, including
stat/open/read. A missing material file does not prevent import. At least one
polygon face is required and the complete file must validate before commit.

Refusals carry format, filename, original line, current face ordinal, reason
and remedy. The face ordinal is zero before the first face; for non-face errors
it is the count of faces already encountered. UUID, authored geometry,
provenance, labels, ignored-data counts and mesh quality summary are embedded in
native saves; source paths are not retained or reread during open/history.

## Verification

Accepted build cache: Release AppleClang arm64, Qt 6.11.2 at the private runtime
`Qt6_DIR=/Users/rje/src/cpp/Alloy-dependencies/qt-private-runtime/install/lib/cmake/Qt6`,
`OpenCASCADE_DIR=/opt/homebrew/lib/cmake/opencascade` (7.9.3). The local build
succeeded. Focused CTests `strict_obj_import` and `retained_stl_import` pass 2/2
(1.71 seconds total). The strict test includes exact 200000/200001 predicted
triangle admission and 256/257 corner boundaries; this does not qualify
full-cap app latency or peak memory.

The real app fixture passed **114 API responses**, importing grouped tetrahedra
(8 triangles), offset quad (2) and concave pentagon (3), each in mm/in. It compared
every received source position and indexed triangle with independent fixture
expectations, then deleted the source, undid/redid, saved and reopened. Eleven
invalid cases preserved the entire inspected state, selection and prior save
bytes. Evidence is external at
`../Alloy-reference-records/validation/xch023-strict-obj-2026-10-21/api-final/`
(`summary.json`, `transcript.json`, six saved models and app log). Source and
save hashes are in the summary; the repository fixture is its source generator.
The private editor closed normally and only user-owned PID72618 remained.
Independent source review found two bounded issues: quality connectivity needed
exact-coordinate analysis identities for separately indexed duplicate positions,
and late invalid UTF-8/NUL needed original-line diagnostics. Both are corrected;
a focused six-authored-position/two-triangle fixture proves four boundary edges
and one connected component without merging authored geometry, while late
UTF-8/NUL/incomplete-UTF-8 fixtures prove line 6. Final focused tests and the
114-response app replay passed after those corrections. The root records final
review acceptance. `focused-tests-final.log`, `final-sha256.json`, fixture source
copies and `api-final/independent-bounds.json` retain bounded receipts. The
latter independently recomputes bounds on all 18 returned mesh pages.

Reproduction commands:

```sh
cmake --build build-preview-release --target strict_obj_import_test retained_stl_import_test alloy -j4
ctest --test-dir build-preview-release -R '^(strict_obj_import|retained_stl_import)$' --output-on-failure
python3 tests/strict_obj_import_api.py --app build-preview-release/alloy.app/Contents/MacOS/alloy --output ../Alloy-reference-records/validation/xch023-strict-obj-2026-10-21/api
```

Core tests separately check exact offset-quad and concave-ear index triples,
reverse winding/area, explicit unit scaling, mixed tokens/relative indices,
complete-file refusals, planarity/area threshold sides, 256/257 corner and byte
limits, one retained occurrence, history identity and source-independent reopen.
The real app fixture checks the original two-tetrahedra, quad and concave meshes
in both units, ignored diagnostics, source deletion/reopen, and unchanged
existing document/selection/dirty/history/saved bytes after eleven refusals.
Native chooser/preview, GPU captures, physical input, cross-platform runtime,
full-cap latency/memory measurement and broad OBJ compatibility remain unclaimed.
