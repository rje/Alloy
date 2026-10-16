# Curved sheet Trim trials

Accepted 2026-10-15, SRF-016/#360. Product commits `c6c10d4` and `7ee0cf1`
retain one finite Surface Extrude tool body and one existing named surface target
per Trim feature. Native and API commands share tool-first target binding,
explicit portion selection, red removal preview, Cancel and authoritative commit.
Connected multi-face tool outputs and multiple curved portions are supported.

OCCT's non-destructive Splitter supplies exact face and edge history. Region IDs
derive from source-face, source-boundary and tool-face ancestry; edge fragments
use their uniquely named adjacent regions. Duplicate or unavailable signatures
refuse rather than rebind by location, area or traversal order. Unchanged faces
keep their identities. A nonseparating slit is refused. Existing qualified planar
signed-side and paired-sketch Trim paths retain their respective selection rules.
Session partition reuse preserves revision/input invalidation; full commit still
validates the current history.

The actual accepted four-body iron fixture contains the exterior, the retained
handle bridge and two separate Extrude tool outputs. This bounded trial targets
the handle bridge (Inner). The upper tool splits three faces; one explicitly
selected rear portion is removed. The lower tool then splits the current result
into four faces; two explicitly selected rear portions are removed. Each operation
leaves a valid two-face, zero-solid bridge with its original body identity. Both
tools remain unchanged. Public target grouping and exact portion choices remain
qualified; this does not establish every possible target in the tutorial.

Focused core checks pass sequential operations, immutable preview/partition,
all face/edge IDs through a meaningful source-spline edit, unavailable and duplicate
ancestry refusal, exact history/reopening, independent tool Remove/undo and exact
pre-trial restoration. The existing planar and paired-sketch core checks pass.
Native Qt input passes 3/3 cases: actual compact-menu activation/closure, viewport
tool and target hits, produced focus, red preview and Cancel. The exterior is hidden
for that capture so the target is visible; source geometry is unchanged.

The real application API passes 62 calls covering both trials, editing, Cancel,
stable IDs, atomic refusal, Remove/undo, full trial reversion/redo and reopening.
Warm region picks take 144–295 ms with one reused partition. Cold commits still
cost 8.4–10.0 seconds; opening/editing/reopening remain separate latency limits.
Actual captures show readable controls and smooth Apple M4 hardware rendering,
OpenGL 4.1 Metal, 4×MSAA and native framebuffer capture. Physical OS input and
other-platform runtime remain unqualified.

Evidence is external under
`deliverables/surface-iron-continuation-2026-10-15/curved-trim-01/`:
`README.md`, `measurement-summary.json`, `sha256.json`, `core-run-03.log`,
`planar-sketch-regression.log`, `core/`, `native-run.log`, `native/` and
`api/result.json`, `api/transcript.json`, actual captures and edited model.
Earlier failed guard checks and an unadopted exploratory source variant remain
clearly identified. Astra independently accepted source, core, API/native and
captures. Private validation editors are closed; the isolated candidate is
integrated. No final bottom geometry, Boundary Fill, closed solid or complete
TUT-018 acceptance is claimed.
