# STL/OBJ mesh-reader foundation

Decision 2026-10-21, XCH-014/#720; bounded dependency admission for XCH-004/#72.
This records library and isolated reader evidence, not implemented mesh import,
application geometry, GPU presentation, history or native-input acceptance.
The dated public workflow contract is external at
`../Alloy-reference-records/knowledge/specs/xch004-mesh-import-2026-10-21.md`
(REF-MESH-01–09). Public behavior and Alloy proposals remain distinguished there.

## Decision and installed configuration

Reuse OCCT **7.9.3**, the existing geometry/interchange dependency. Read STL via
`RWStl::ReadFile` into `Poly_Triangulation`; read OBJ via
`RWObj_TriangulationReader` with `SetCreateShapes(false)` and
`SetSinglePrecision(false)`. Do not use `StlAPI_Reader`, OBJ CAF shape import,
per-triangle B-rep conversion, inferred solids or automatic repair. OCCT is a
**decoder foundation requiring an Alloy validation adapter**, not a complete
implementation of the mesh-import contract.

The accepted `build-preview-release/CMakeCache.txt` specifies Release,
`/usr/bin/c++`, `/opt/homebrew/lib/cmake/opencascade` and the private Qt runtime.
The installed OCCT CMake config identifies 7.9.3; its DataExchange component
contains `TKDESTL`, `TKDEOBJ` and `TKRWMesh`. The app's generated Ninja link line
already links `TKDESTL`; it **does not yet link `TKDEOBJ`/`TKRWMesh`**. Add the latter
targets when implementing OBJ import. An isolated C++20 executable successfully
compiled and dynamically linked all three against Homebrew `7.9.3_1`, AppleClang
17.0.0.17000604, on macOS arm64; `otool -L` reports current version 7.9.3.
This is macOS reader evidence only, not Windows/Linux runtime qualification.

The exact upstream source recorded in existing notices is
[`a016080bf6738d6aeae020badee4e888ad1540a5`](https://github.com/Open-Cascade-SAS/OCCT/tree/a016080bf6738d6aeae020badee4e888ad1540a5).
Installed headers, package receipt, config and reader-library SHA-256 values are
saved with the external evidence. The bottle was not rebuilt from that commit
in this task; version/header/link checks and pinned source inspection are the
bounded provenance evidence. Existing [third-party notices](../../THIRD_PARTY_NOTICES.md)
retain copyright Open CASCADE SAS and contributors, LGPL-2.1 plus the
[OCCT exception](../../LICENSES/OCCT-exception-1.0.txt), with the
[license text](../../LICENSES/LGPL-2.1-only.txt). These readers introduce no new
third-party dependency or license. Preserve notices and existing shared-library
redistribution obligations in packages; no new package was produced here.

## Measured reader behavior

Original fixtures and generator, probe/build source, complete output vertices
and indexed triangles, logs, assertions, dependency snapshots and SHA-256
manifest are external in
`../Alloy-reference-records/validation/xch014-mesh-reader-2026-10-21/`.
`results.json` and per-fixture `.log` files distinguish the raw reader return
from usable geometry; `run.py` checks bounded observations independently.

| Input | Observed with pinned reader |
|---|---|
| ASCII 10-unit cube | 12 triangles, 8 exact-coordinate merged nodes, bounds 0..10 XYZ; every source facet's vertex order retained |
| Binary 1-unit cube | 12 triangles, bounds 0..1; explicit caller scale 25.4 produces 0..25.4 mm |
| Open square | 2 triangles, 4 geometric boundary edges; accepted as mesh |
| One reversed cube triangle | 12 triangles, 3 inconsistently directed shared edges; reader did not repair winding |
| Three faces on one edge | 3 triangles, 1 non-manifold geometric edge; accepted as mesh |
| OBJ two tetrahedra, `o`/`g`, mixed `v/vt/vn`, negative indices | 8 triangles, bounds (0,0,0)..(15,5,5); 16 output nodes because attribute index tuples split coordinate-identical vertices |
| Offset OBJ planar quad | 2 triangles, bounds (1,2,3)..(5,6,3); identity XYZ, no implicit Y/Z flip or recentering |
| Truncated binary / malformed ASCII / empty STL | Null triangulation; truncated input emits premature-end error |
| Invalid OBJ index at line 5 | Emits line-specific failure but **returns true**, with no triangulation |
| Comment-only / vertices-only OBJ | **Returns true**, with no triangulation |
| Bowtie / nonplanar OBJ quad | **Accepted**, split along 1–3 diagonal into two triangles; unsuitable for the strict contract |
| Repeated-vertex degenerate STL plus one valid facet | **Drops the degenerate facet**, returns one triangle |

Quality values above are probe-side exact-geometric-edge analysis, not OCCT
quality diagnostics. OBJ node identity is not connectivity: analyze matching
coordinates separately without mutating stored vertices. Group names were
present in input; retaining their metadata requires the adapter (not proved by
`GetTriangulation()` output).

## Required adapter policy and remaining implementation decisions

XCH-004 must satisfy these before app acceptance; no direct-reader shortcut is
admitted. The isolated probe deliberately records failures rather than masking them.

- **Units/coordinates:** require explicit `mm`/`in`, proposed default `mm`; interpret
  one coordinate unit as 1 or 25.4 mm and multiply once into canonical double
  precision mm. Store source unit and exact scale. Apply identity XYZ initially;
  no axis guess, recenter or grounding. The probe performs this scaling outside
  OCCT for both formats; it does not qualify the optional OBJ converter API.
- **Strict preflight and errors:** validate finite source coordinates, valid
  indices/tokens, nonempty faces, lengths/counts and complete parse before commit.
  A non-null result or true return alone is insufficient. Reject invalid OBJ
  faces with original line, filename, reason and remedy; preserve diagnostics
  even when OCCT reports success. Detect nonplanar/self-intersecting/degenerate
  polygons before passing them to OCCT. Preserve polygon winding. Triangles and
  planar convex quads have bounded evidence; concave/n-gon triangulation needs
  a dedicated fixture before acceptance. Pinned source has a polygon fan fallback;
  do not silently accept that fallback as valid strict polygon handling.
- **STL fidelity:** exact-coordinate merging is built into `RWStl_Reader`
  (zero linear tolerance, default merge angle pi/2); keep that documented
  representation and received winding. Because repeated-vertex facets can vanish,
  preflight must retain their original counts/diagnostics and preserve the
  contract's raw-facet data where required. Decide the bounded raw-facet adapter
  under XCH-004; changing the public acceptance to silently discard them is not
  authorized. Open/inverted/non-manifold quality remains an accepted diagnostic,
  not rejection or repair. Test multi-domain STL separately if claimed.
- **Ignored data:** count/report OBJ texture/normal/material/smoothing/line/point
  records and `o`/`g` names; one imported body per file. Discard imported shading
  normals/UVs in favor of Alloy-owned presentation. Strip `mtllib`/`usemtl` from
  a validated temporary stream before the decoder: pinned `RWObj_Reader` reads
  referenced MTL files automatically, which conflicts with ignoring sidecars.
  Preserve original source line mapping for diagnostics. Detect STL color fields
  where possible and report ignored color. Materials/texture/group hierarchy
  fidelity, repair, mesh editing and mesh-to-solid remain deferred.
- **Limits:** the OBJ reader defaults to unlimited estimated memory and exposes
  `SetMemoryLimit(bytes)`; STL has no equivalent file-reader memory cap. The
  probe explicitly uses 64 MiB for OBJ. Both adapters must preflight file size,
  node/face counts, expansion and overflow before allocation; count OBJ polygon
  output triangles (`n-2`) and attribute-split vertices. A shipping numerical
  file/triangle/working-set cap remains an **XCH-004 implementation decision**:
  these tiny fixtures cannot justify a production capacity or latency claim.
  Measure peak memory on a bounded larger original fixture, choose conservative
  caps with headroom for source + OCCT + retained/display data, record thresholds,
  and reject with observed/requested limit and remedy. OCCT's estimate is not a
  process-memory guarantee; cancel/failure must discard temporary data.

## Smallest implementation handoff

Add a versioned authored mesh record beside sketches/features: stable UUID,
source display name/hash (provenance only), format, source unit/scale, identity
placement, owned canonical vertices/triangles, source quality/loss diagnostics
and group labels. Keep GPU tessellation/presentation caches separate; do not
serialize an OCCT pointer, inferred B-rep or source-path dependency. Validate
and decode into temporary owned state, then commit one shared authoritative
command used by UI/API. Cancel/failure preserves document and selection; one
Undo removes the occurrence and Redo restores its UUID. Save/reopen embeds mesh
data and the exact authored interpretation without rereading the source.

Implement one Browser mesh row, properties and whole-body visibility/selection,
then GPU presentation/fit and the bounded import controls from the public
contract. Verify original fixtures through the real application entrypoint,
including invalid-input atomicity, geometry, UUID/history/source-independent
reopen, and separate GPU/native-input inspection. Independent compatibility
review is still required at that milestone. No app was built or operated by
this decision probe; no CI, release, push or proprietary runtime was used.

## Pinned public library evidence

Retrieved 2026-10-21, creator Open CASCADE SAS/contributors; open-source source
inspection, not reference-product observations:

- [`RWObj_Reader.cxx`](https://github.com/Open-Cascade-SAS/OCCT/blob/a016080bf6738d6aeae020badee4e888ad1540a5/src/RWObj/RWObj_Reader.cxx): parser returns, indices, polygon handling, sidecar loading and memory estimate.
- [`RWObj_TriangulationReader.cxx`](https://github.com/Open-Cascade-SAS/OCCT/blob/a016080bf6738d6aeae020badee4e888ad1540a5/src/RWObj/RWObj_TriangulationReader.cxx): direct mesh collection and quad diagonal.
- [`RWStl_Reader.cxx`](https://github.com/Open-Cascade-SAS/OCCT/blob/a016080bf6738d6aeae020badee4e888ad1540a5/src/RWStl/RWStl_Reader.cxx): exact-node merge, dropped repeated-node facets and stream errors.

Reproduce from the external evidence directory:

```sh
python3 generate.py
cmake -S . -B build -DOpenCASCADE_DIR=/opt/homebrew/lib/cmake/opencascade -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4
python3 run.py
```
