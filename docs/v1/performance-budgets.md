# Initial v1 performance targets

Decision 2026-10-25: these are explicit Alloy engineering targets, not measured
passes or timings of a reference product. They resolve the budget decision so
REL-004/016/049 can measure and improve implementations. Do not retune a budget
merely because current code misses it.

The reference machine is the available Apple M4 with16 GiB memory, Release
build, Qt6.11.2/OCCT7.9.3, normal tessellation quality and1280x900 logical window
at native display scale. Record actual hardware, OS, package/dependencies, model
size, load and completion boundaries. Windows/Linux and Intel Mac qualification
must report their actual machines and these user-visible ceilings; a slower
machine or genuine kernel limit needs an explicit evidenced release disposition,
not a silent normalization. No new hardware or minimum-OS exclusion is implied.

| Operation / fixed workload | Initial warm target | Timing boundary |
| --- | --- | --- |
| Cached selection, camera/control update, invalid scalar input, pending-operation/Cancel acknowledgement |100 ms| Input/dispatch to observable state or presented response |
| Ordinary GPU orbit/pan on bracket/duct |30 fps sustained;100 ms input response| Presented frame/interaction intervals, not API return |
| Ordinary ready-preview parameter update |250 ms| Change to visible current preview |
| Small sketch solve/source width edit, Box/Cylinder and ordinary Extrude |500 ms| Shared command through valid current geometry and display publication |
| Ordinary analytic Boolean Cut or constant Fillet |1 s| Same completion boundary; include the corrected-iron simple Cut |
| Bounded complex14-body enclosure, duct/iron surface edit, fresh circular Coil mesh |5 s| Fresh valid geometry/display completion, with responsive pending UI |
| Cached Undo/Redo on accepted retained fixtures |250 ms| History action through current state/display; record fresh recomputation separately |
| Ordinary native save / reopen |500 ms /1 s| Durable save / valid reopened geometry ready |
|200,000-facet retained STL import |5 s| Read/parse/validate through committed current GPU result |
| Large retained-mesh native save / reopen |3 s /3 s| Full source-independent durable file / valid current result |
| Unrelated Box after retained200,000-facet mesh, including recovery |1 s| Entire action through geometry, recovery and state/display publication |

Startup target is cold3 s / warm2 s from process launch to ready local control
and drawable main window; document loading is measured separately. Long pending
work must show its state promptly and permit safe Cancel before publication.
These targets do not justify reducing geometry accuracy, tessellation quality,
reference correctness or save durability.

Use REL-004's seven exact fixture groups, including the egg-cup Loft/Shell,
enclosure finishing and retained-mesh cases. Record one cold plus three warm
runs for material computation; use median and range, not invented p95 from three
samples. Cheap query/UI aliases can be attributed once in an existing workflow.
Repeat only if variance, a suspected regression or a budget decision remains
unclear. Cold behavior and recurring outliers stay visible; do not hide them
behind a passing median. Host contention is reported, not mislabeled a code win.

REL-049 still accounts for every supported v1 command, grouping identical paths
only with explicit attribution. An actionable finding is a budget miss or a
measured avoidable cost with material wait/scaling impact: duplicated expensive
evaluation/kernel work, remeshing, serialization/copying or synchronous waiting.
Profile the dominant phase, create a bounded fix and rerun that fixture. Stop
once its target passes and there is no remaining demonstrated material defect,
or a reviewed trace establishes an explicitly accepted kernel/hardware limit.
Do not pursue hypothetical micro-optimizations on passing paths. Asynchronous
execution alone does not qualify intrinsic cost.

The implementation tasks remain open until actual measurements and required
fixes pass; this decision closes no performance or platform gate.
