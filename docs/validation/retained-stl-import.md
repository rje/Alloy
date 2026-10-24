# Retained STL mesh import

The later [owned validation reuse receipt](mesh-box-cost.md) accepts XCH-020/#729
for unrelated-edit cost. Its matched capacity Box median is 874.5→561.5 ms; the
earlier measurements below remain historical. Recovery cost is separate REL-057.

XCH-015/#722, 2026-10-21. Bounded core/API implementation of
[decision 004](../decisions/004-mesh-import-foundation.md). REF-MESH-01–09 and the
public workflow contract remain external under
`../Alloy-reference-records/knowledge/specs/xch004-mesh-import-2026-10-21.md`.
Native chooser, Browser, selection/visibility and GPU mesh presentation remain
XCH-016; OBJ remains XCH-017. A retained mesh is not an evaluated B-rep, a solid,
an editable feature recipe, or evidence of printable watertightness.

## Command and storage contract

`document.importStl {path, units}` requires an absolute local path and explicit
`mm` or `in`. Content determines ASCII/binary encoding; extension is not used as
a validator. Import retains source XYZ without recentering or reorientation,
scales exactly once into canonical mm, and commits one document transaction.
Active commands, rollback and staged assembly/reference positioning block import.
Failure preserves the prior document, history, dirty state and selection.
Successful import is one Undo; Redo restores the same mesh UUID.

`document.inspect.meshes` exposes identity, source interpretation, triangle and
vertex counts, bounds and quality. To respect the existing 1 MiB local-client
response bound, mesh records in the inspected `authored` view omit the two large
geometry arrays. `meshGeometryPageCommand` identifies `mesh.inspect`; this does
not change native document serialization or authoritative internal state.
`mesh.inspect {meshId, vertexOffset?, triangleOffset?, limit?}` reads the exact
retained arrays, with zero-based indices/offsets, default page length 1000, and
maximum 1000 entries in each array. It reports next offsets and total counts in
the mesh summary. Offsets at the corresponding count return an empty page.

Native `alloy-document/1` files optionally contain top-level `meshes` records.
Each `alloy-mesh/1` record owns canonical double vertices, indexed triangles,
identity placement, UUID, source filename/format/encoding/SHA-256, exact unit
scale, decoder/loss diagnostics and geometry-derived summary. No source path,
OCCT handle or display cache is serialized. Reopen validates retained geometry
and summaries without reading the original STL. Existing documents without
meshes remain valid. The document permits at most 64 mesh bodies and 200000 retained mesh facets
in total; meshes are root records in this bounded implementation.

## Strict adapter and quality policy

Preflight validates the complete input before calling pinned OCCT 7.9.3
`RWStl::ReadFile` into `Poly_Triangulation`. Binary length must equal
`84 + 50 * declaredFacets`; ASCII requires one complete `solid` block, exact
facet/normal/outer-loop/three-vertex/endloop/endfacet grammar, `endsolid`, and no
trailing records. Multi-domain ASCII blocks are not accepted in this slice.
Empty, truncated, nonfinite, out-of-range, malformed and over-limit inputs fail
with filename, reason, original line/facet where available, and a retry remedy.
Coordinates must remain within ±1e9 canonical mm. Finite normals are ignored.

The decoder reads a temporary file containing the exact preflighted bytes,
avoiding a second read of a mutable source. Every decoded triangle is checked
against received oriented coordinates; unexpected coordinate/winding changes or
non-repeated-vertex facet loss fail. Exact-coordinate merging follows the pinned
reader's documented representation. The retained arrays restore received facet
order and preserve repeated-vertex facets omitted by OCCT, including their
original zero area. A file with no usable decoder triangles fails.

Quality reports boundary edges, non-manifold edges, inconsistent directed
shared edges, connected components and degenerate facets; no repair occurs.
Topology counts use nonzero-area facets, so a degenerate facet cannot create a
spurious non-manifold edge. Components use shared geometric edges, not vertex
contact alone. Inverted/open/non-manifold readable meshes remain mesh bodies.
These diagnostics do not establish a solid, orient an entirely reversed shell,
or prove freedom from self intersections. Nonzero binary attribute fields are
counted as ignored attributes (possibly color); their color encoding is not
claimed. Source normals are explicitly reported ignored.

## Resource admission and evidence

Admission limits are 64 MiB source, 64 mesh bodies, 200000 total document facets and 512 MiB
estimated adapter working set, with estimate `sourceBytes + facets * 2048`.
The estimate bounds admission before facet allocation and decoder invocation;
it is not an operating-system process-memory quota. The existing application,
JSON serialization, undo and transport have additional memory costs. Error text
reports observed/requested bytes, facets and working estimate plus limits and
asks the caller to reduce the source or use a separate document.

Original fixtures, generators, measurement scripts, app logs, API transcripts,
source-independent native saves and hashes are external under
`../Alloy-reference-records/validation/xch015-retained-stl-2026-10-21/`.
The small STL fixtures reuse the independently authored XCH-014 generator.
The capacity generator authors a 200000-facet soup with 600000 distinct vertices
and no third-party model data. Focused adapter measurement: 1.60 seconds,
380747776 bytes peak RSS on macOS arm64, AppleClang 17, Qt 6.11.2, OCCT 7.9.3,
Release. The final Release application build and focused adapter test pass (1/1,
1.45 s). Six original STL fixtures pass 189 real-app API calls: ASCII/binary
cubes, open square, inverted cube, three-faces-on-one-edge and the mixed
degenerate file. Independent Python source parsing matches every retained
facet coordinate, order and winding after the explicit scale. Expected quality
is closed/one component for both cubes; four boundary edges for the square;
three inconsistent shared edges for the inverted cube; one non-manifold edge
for the three-face fixture; and two retained facets/one diagnosed degenerate
versus one decoded triangle for the mixed file. Undo/Redo preserve authored IDs;
each native save reopens after its copied source is deleted, and its geometry
page matches the original. Truncated/empty/malformed imports preserve document,
dirty/history state, saved bytes and a real populated-design body selection.
Invalid geometry-page limits (oversized, huge, fractional, string) refuse.

The full-capacity actual-app session passes these observed gates:

| Operation | Wall time |
|---|---:|
| Import 200000 facets / 600000 distinct vertices | 3448.7 ms |
| Read final 1000-entry geometry page | 125.8 ms |
| Subsequent 4 × 5 × 6 mm Box commit | 1638.1 ms |
| Native save | 622.4 ms |
| Native reopen | 1683.0 ms |

Sampled whole-editor peak RSS is **660242432 bytes**, with **201080832 bytes**
baseline (approximately 438 MiB increase). The embedded native file is
89343407 bytes (approximately 85.2 MiB); source binary is 10000084 bytes.
The saved UUID survives reopen and the final geometry page matches the generator;
a 200001-facet source refuses without changing document revision. Sampling uses
`ps` every 25 ms and is not an OS-recorded hard peak. These are one bounded local
workload's observations under host build pressure, not cross-platform performance
promises. The source-byte/facet estimate is admission policy, not a process-RSS
claim. Full-capacity receipts are `capacity-app-results.json`,
`capacity-core-time.txt`, generator and scripts in the external evidence folder.
Only the final native capacity save is retained; disposable smaller probes were
removed.

Reproduction from the repository root, without CI:

```sh
cmake --build build-preview-release --target alloy retained_stl_import_test -j4
ctest --test-dir build-preview-release -R '^retained_stl_import$' --output-on-failure
python3 tests/retained_stl_import_api.py \
  --app build-preview-release/alloy.app/Contents/MacOS/alloy \
  --output ../Alloy-reference-records/validation/xch015-retained-stl-2026-10-21/fixtures-api \
  --fixtures ../Alloy-reference-records/validation/xch014-mesh-reader-2026-10-21/fixtures
```

## Responsiveness and follow-up

`document.importStl` currently reads, validates, decodes and commits synchronously
on the application UI thread. There is no mid-import Cancel/progress session.
The full-cap workload is therefore a blocking operation; core/API completion is
not a native responsiveness claim. XCH-016/XCH-008 retain async/progress/cancel
integration before native mesh-workflow acceptance.

Current general document validation also rechecks retained mesh geometry and
recomputes its quality on subsequent modeling commits. The capacity receipt
measures a subsequent Box; caching validated immutable mesh records and their
quality is a bounded performance follow-up, not part of the present evidence.

## Evidence boundaries

No native mesh controls, mesh display, mesh GPU, artist workflow or matched
reference screenshot acceptance is claimed by this core/API child. macOS local
build/runtime qualification does not establish Windows/Linux runtime support.
No paid CI, release, push or proprietary application was used. User-owned
PID 72618 was preserved; private validation editors are closed between runs.
Public-contract comparison: mesh identity, explicit unit interpretation and
received geometry are matched for this bounded API implementation. The local
transaction, strict parsing, diagnostics and persistence semantics are verified
Alloy proposals; exact reference defaults/errors/undo semantics remain unknown.
Native/visual workflow comparison remains XCH-016. Independent source review
reported no material blocker; the root records the final compatibility decision
and issue state separately.
