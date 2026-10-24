# Box after retained mesh cost

XCH-020 / #729, 2026-10-24. This bounded change removes repeated strict
validation of unchanged, already validated retained mesh records. It does not
make the full command latency negligible; recovery snapshot cost remains
REL-057 / #738 under REL-049.

## Ownership and validation contract

A private proof owns an immutable `QJsonArray` of fully validated mesh records.
Qt's supported exact value equality determines reuse, including its implicit
sharing fast path. No UUID, source SHA, serialized flag, private Qt layout or
global cache grants trust. The arrays share authored storage; current document,
working evaluation and Undo/Redo snapshots own their lifetimes.

Candidate validation always checks document structure, global UUID/namespace
collisions, 64 mesh body and 200000 aggregate facet limits. Only strict mesh
record geometry/quality analysis is skipped when the whole record array equals
the private proof. Changed geometry or metadata misses that proof. Static
validation and native open/recovery validate incoming records before creating a
new proof. Sketch solving fails closed if it unexpectedly changes mesh records.
Reset and history clearing release their snapshots. JSON lookup values retained
across temporaries are owned values.

## Matched measurements

Same macOS Apple M4 host, Release `build-preview-release`, Qt 6.11.2 and OCCT
7.9.3; three real-API samples per condition. The original external capacity
fixture contains 200000 independent facets and 600000 vertices; each Box is
4 × 5 × 6 mm, volume 120 mm³. Undo restores the imported authored document between
Box samples. `ALLOY_TRACE_MESH_COST=1` enables stage logging; profiling is opt-in.

| Wall time, ms | Before samples | After samples | Median before → after |
| --- | --- | --- | --- |
| Normal Box | 140.1, 99.4, 97.9 | 145.9, 105.4, 96.9 | 99.4 → 105.4 |
| Box after capacity import | 874.5, 869.2, 920.8 | 561.5, 574.8, 547.6 | 874.5 → 561.5 |

The capacity case improves 35.8%. Normal ranges overlap; these small samples do
not establish a change in its latency distribution. Import itself measured
1551.7 ms before and 1694.5 ms after and is not the optimized operation.

Before, each Box repeated strict validation three times, at 143–159 ms each
(quality analysis 126–138 ms). After, all three points reuse the owned proof.
Warm candidate evaluation is 24–26 ms and command handler 36–37 ms; history push,
state construction and response JSON serialization each round below 1 ms.
Cached mesh presentation is 9–10 ms, with no new mesh actor; total refresh is
136–138 ms. Recovery writes the full approximately 89 MB native snapshot and
costs 363–391 ms, now the dominant measured stage. Before recovery was
269–289 ms; that unchanged stage varied with this run and is not attributed to
the validation change. REL-057 addresses it separately.

RSS was sampled using `ps` every approximately 25 ms plus operation endpoints.
Benchmark peak was 750.0 MB before and 747.5 MB after (decimal MB), with baseline
RSS 194.4 and 189.9 MB respectively. No material memory reduction is claimed;
OS reclamation and sampling affect endpoints. The after run's separate native
save/reopen/lifecycle peak was 786.9 MB and is excluded from the matched benchmark
peak. Sampled RSS is not a hard allocation peak.

## Verification and evidence

- Focused CTest: `retained_stl_import` and `mesh_validation_proof`, 2/2 pass.
  The proof test covers Box/history, changed coordinates retaining UUID/SHA,
  refusal atomicity, global namespace collision, untrusted native reopen,
  source-independent reload and reset.
- `tests/mesh_box_cost_api.py` through the actual application passes at capacity:
  exact retained metadata, final vertex `[498,1599,0]`, final triangle
  `[599997,599998,599999]`, valid Box B-Rep, whole-mesh selection, visibility
  Undo/Redo, save, deletion of the owned STL source, native reopen, later Box and
  over-capacity import refusal preserving authored state and selection.
- Native open performs strict record validation before subsequent owned reuse.
  Native save measured 356.7 ms, reopen 864.5 ms; these lifecycle observations are
  outside the matched Box sample set.
- Actual framebuffer capture and diagnostics identify Apple M4, Apple OpenGL
  4.1 Metal, hardware acceleration and a 2376 × 1216 framebuffer. The capacity
  soup is visible; the small Box is not visually distinguishable at this broad
  fitted scale, so its shape is established separately by B-Rep evidence. No
  native keyboard/focus or reference visual parity claim is made.
- Independent Sol review accepts the bounded source, trust/invalidation,
  performance and lifecycle evidence. No further runtime was requested.

External evidence (outside the repository):
`../Alloy-reference-records/validation/mesh-box-cost-2026-10-24/before/` and
`after/`, containing `report.json`, `app.log`, and the after model/capture.
Original fixture provenance remains in
`../Alloy-reference-records/validation/xch015-retained-stl-2026-10-21/`.
The private API editor exited cleanly; user editor PID 72618 was preserved.
