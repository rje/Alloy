# reference CAD application replacement: public-reference deep dive

Research date: **2026-10-11**. Result: an initial dependency-ordered 130-issue
[backlog](../backlog/README.md), expanded to 135 issues after the required
macOS/Windows/Linux milestone-release work was added. See the
[cross-platform research](cross-platform-releases.md).
and twelve cumulative useful-product gates. This is researched specification
and implementation planning, not a claim of implemented reference CAD application compatibility.
The working application at the start of this research was a Qt shell with local
control and a QPainter reference grid, without a CAD document. Hardware-backed
OCCT/OpenGL viewport work is now explicitly required and being handled as a
bounded follow-up; consult [context](../context.md) for current implementation.

## Reference method and accepted constraints

Only public Autodesk documentation, screenshots, tutorials and published API
descriptions were used for target-product evidence. No reference CAD application application,
installation, trial, web client, account data, private API, traffic or extracted
asset was accessed. Open-source library documentation/source was studied for
implementation feasibility. The original 84 primary URLs and additional public
build/deployment sources, with retrieval dates,
are in [sources.json](../backlog/sources.json); broader foundation links and
specific licenses are preserved in [foundation notes](foundation-notes.md).

Labels used below:

- **Documented:** explicitly stated in the linked public source.
- **Screenshot observation:** visible in the previously downloaded public doc
  images; their provenance is retained separately from application assets.
- **Proposal/inference:** Alloy implementation, fixture, threshold or ordering.
- **Unresolved:** evidence does not establish the exact behavior/default.

C++20 and MIT for original Alloy code/docs are accepted decisions. Qt 6 and OCCT
are the proposed/installed native foundations; individual dependencies retain
separate licenses. Rust is outside the planned stack. We should preserve
existing library strengths without assuming a library automatically supplies a
complete familiar editing workflow.

## Initial Design workflow: what must be modeled

### Sketch intent, constraints and selection

**Documented:** sketches contain curves, constraints and dimensions on a selected
plane or planar face; closed regions drive solid features, while open profiles
also support surface/thin operations. Public sketch documentation assigns
semantic display roles: blue unconstrained, black constrained, green fixed,
purple projection, orange construction. These colors communicate model state,
not just decoration. Sketch overview (reference `sketch`).

Create Sketch opens a temporary contextual mode. Existing sketch nodes can be
edited from Browser/Timeline; ordinary sketch commands and constraints appear
in that context. A plane-picking shell with no retained entities is insufficient.
Edit a sketch (reference `sketchmode`).
The palette exposes tool-specific options and toggles for grid, snap, profiles,
points, dimensions, constraints and projected geometry. Exact initial values
vary or remain unestablished by these sources.
Sketch Palette (reference `palette`).

Driving dimensions determine geometry; driven dimensions report dependent
values and display differently. Overconstraint handling must be deliberate.
Dimension reference (reference `dimensions`).
The public constraint API identifies relations and valid entity types, but
its class names do not prescribe Alloy's solver implementation.
Constraint collection (reference `constraints`).

**Proposal:** `SKT-001`–`SKT-009` build a retained sketch, basic curves, mature solver,
driving/driven dimensions, physical placement, DOF/conflict diagnostics and
profile-region extraction. An anchored 40×25 mm rectangle with a radius-5 hole
has a known region area of `1000 − 25π mm²`. This independent numeric fixture
checks intent and topology before extrusion. Curve drawing alone does not
establish constraints or valid profile regions.

### Projection and editing dependencies

**Documented:** projected geometry can retain an associative link. Lost links
raise visible warnings and can be re-linked, broken or deleted. This directly
requires repairable reference states rather than closest-face guesses.
Projection and lost-link repair (reference `project`).
Trim/extend/offset and fit/control spline tools have distinct semantics and
edit structures. Sketch modifications (reference `sketchmodify`),
spline creation/editing (reference `splines`).

**Proposal:** name authored entities independently of solver array order or
OCCT traversal order. Preserve generated/modified/deleted topology provenance.
`FND-004`, `DOC-012`, `HIS-002` and `SKT-010` first qualify one extrusion/fillet and
projection chain, including intentional source deletion and split ambiguity.
The correct failure can be a repairable broken reference; silent attachment to
another edge is unacceptable. General topological naming remains a hard unknown,
not a problem solved merely by selecting OCAF.

### Solid commands are input contracts, not menu labels

**Documented:** Extrude includes profile/face selection; start, direction,
symmetric measurement and extent controls; taper/thin variants; and New Body,
Join, Cut, Intersect and component-related operations. It documents visibility
and target-body effects for cutting. These defaults must be specified explicitly
rather than inheriting whichever bodies happen to be displayed by an API test.
Extrude reference (reference `extrude`).

Fillet has several radius/selection/continuity variants, including constant,
variable, chord, asymmetric, rule and full-round forms. The initial feature is
constant selected-edge fillet, with other variants retained in later parity
children. Fillet reference (reference `fillet`).
Shell distinguishes body selection from face removal and inward/outward/both
directions. Shell reference (reference `shell`).
Hole distinguishes placement, extent, drill shape, counterbore/countersink and
thread-related options. Hole creation (reference `hole`).

Revolve, sweep and loft need their own ordered references and failure contracts.
A sweep's rail/orientation controls are not interchangeable with loft section
correspondence. Rails have documented geometric prerequisites. Revolve (reference `revolve`),
sweep (reference `sweep`),
loft (reference `loft`).

**Proposal:** every modeled command shares UI/API inputs, preview, validation,
commit/cancel, transaction, authored history and result inspection. Start with
one-side New Body extrusion and constant fillet, then target-aware Booleans and
remaining extents. `GATE-020` requires a usable mounting bracket, upstream width
edit, undo/redo, reopen and independent export readback. A successful kernel
call with no retained feature or usable UI does not pass it.

### History and parameters

**Documented:** model/user parameters drive dimensions/features through
expressions. Automatic compute is documented, and names/units/value/expression
are distinct fields. Parameters (reference `parameters`),
editing (reference `paramedit`).
The Timeline is an authored operation sequence; suppression affects dependents,
rollback changes the evaluated point, and reorder can affect results.
Timeline (reference `timeline`).
Direct mode and a direct-edit Base Feature inside parametric history are distinct;
converting direct geometry does not reconstruct its original feature sequence.
Modeling modes (reference `modes`).

**Proposal:** preserve authored intent separately from evaluated BRep and
presentation/export meshes. Use one transaction authority and versioned native
storage. `PAR-001` and `HIS-001`–`HIS-003` include unit errors, cycles, dirty
propagation, last-valid results and explicit failure states. Reopening a STEP
export cannot substitute for reopening editable native history.

## Surfaces, exchange and printing

**Documented:** Surface creation includes patch, extrusion, revolution, sweep,
loft, ruled and offset forms. Creation tools (reference `surface`).
Surface loft specifies boundary continuity and rail conditions;
loft reference (reference `srfloft`).
Stitch exposes tolerance and unstitched-edge information and creates a solid
only when the result closes watertightly.
Stitch (reference `stitch`).

**Proposal:** `SRF-001`–`SRF-008` culminate in a duct/scoop authored as surfaces,
trimmed, patched, stitched and thickened. Test analytic surface cases separately
from measured continuity and complex-offset failures. `GATE-030` retains all
earlier workflows plus focused enclosure/knob/slot fixtures for added solid and
sketch capabilities. The official shovel tutorial has inconsistent mass numbers;
use its workflow evidence, not its mass as a numerical oracle.
Public tutorial (reference `thicken`).

**Documented:** format support depends on format/version/subset and sometimes
subscription or translation workflow. A format appearing in a table is not
proof of local translation or restored parametric history.
Supported formats (reference `formats`),
open/import workflows (reference `open`).
DXF insertion exposes units and selected layers, with single/multiple-sketch
modes. DXF insertion (reference `dxf`).
Save As Mesh exposes STL/OBJ/3MF, units and one-file/per-body structure.
Save As Mesh (reference `print`).

**Proposal:** STEP/STL first; then IGES, richer STEP/XDE attributes, mesh import,
3MF Core, bounded DXF and SVG. Every importer has explicit entity-loss/healing
reports, units, source provenance and atomic cancellation. Read and write matrices
are separate. `GATE-040` requires independent input fixtures/readback, not only a
self-round-trip that could share the same mistake. Print qualification measures
closure/orientation/manifoldness/bounds; it does not promise strength, material
suitability or every printer process. Surface rendering is not mesh validity.

## C++ foundations and limits

[Foundation notes](foundation-notes.md) contain primary library links and exact
license caveats. Findings versus proposals:

| Foundation | Documented capability | Alloy adoption boundary |
| --- | --- | --- |
| Qt Widgets | Native desktop widgets/model-view/input | Keep existing C++ shell; add actual command/session state, physical input tests and independently authored visuals |
| OCCT AIS/V3d/OpenGL | CAD presentation, selection, camera and native graphics integration | User explicitly requires hardware acceleration; verify live GL vendor/renderer/version and actual framebuffer capture on installed 7.9.3, not an empty software widget |
| OCCT algorithms | Primitives, BRep Booleans, offsets, sweeps, fillets and exchange | Does not supply reference CAD application defaults, authored history, stable UI selection or universal robustness |
| OCAF/history | Transactions, attributes, persistence and naming/evolution facilities | Select one authored authority after bounded prototype; application must record operation semantics and repair ambiguity |
| PlaneGCS | Mature constraint solver with diagnostics and curve types | LGPL extraction/dependencies need pinned audit; not a dependency-free packaged SDK |
| SolveSpace library | C-facing solver API under GPLv3 | Comparison research only under current MIT decision unless later license path explicitly approved |
| lib3mf | C++ Core format handling/validation | Strong candidate; audit bundled dependencies and supported extensions |
| openNURBS | 3DM/NURBS interchange | Does not supply a replacement Boolean/modeling kernel or full SubD-to-BRep conversion |
| DXF libraries | Existing readers/writers with differing terms/subsets | Common candidates are GPL; MIT project requires deliberate compatible integration or alternative, not accidental adoption |
| CAM/mesh/freeform/analysis libraries | Useful algorithm families | Each still lacks substantial application workflow, validation, data and licensing decisions |

Live OCCT documentation can describe 8.0.1 while the installed baseline is 7.9.3;
compile against installed headers and record version-specific behavior. The
hardware viewport follow-up can qualify an empty 3D view before authored CAD
exists, but `DOC-005`/`GATE-010` remain open for actual document rendering/picking
and geometry evidence. A GPU label in static state is not a hardware receipt.

## Eventual feature-family inventory

[Breadth notes](breadth-notes.md) record 37 primary source entries, distinctions,
and concrete later object scenarios. All families below remain in scope:

| Family | Dependency and useful object | Backlog |
| --- | --- | --- |
| Assemblies, constraints and motion | Reusable component/occurrence identities; adjustable clamp and linkage; numeric DOF/limits | `ASM-001`–`003`, `GATE-060` |
| Engineering drawings | Associative base/projected/section/detail views, dimensions, BOM/balloons, exports; edited clamp sheets | `DRW-001`–`003` |
| Configurations | Stable variants with shared history and downstream references; two enclosure sizes | `CFG-001`–`002` |
| Sheet metal | Authored rules/flanges, distinct unfold/refold and derived flat pattern; chassis with cross-bend cut | `SHT-001` |
| Direct, mesh and Form | Imported-face edits, measurable repair, authored cage and valid conversion; grip/handle | `ADV-001`–`004` |
| Remaining solid/sketch/surface and plastic | Advanced options, modeled threads, ribs, boss/lip/rules and continuity; serviceable enclosure | `ADV-005`–`007` |
| Local rendering and animation | Licensed assets/materials, local render and exploded storyboard output | `VIS-001`–`002` |
| Manufacturing | Setup/tools/stock/WCS, strategies, collision/stock verification, posts, turning/multi-axis, additive/nesting/inspection | `MFG-001`–`008`, `GATE-080` |
| Analysis | Study/mesh/material/BC identity and traceable benchmark results; optional later local solver must earn numerical validation | `CAE-001`–`003`, `GATE-090` |
| Electronics | Authoritative schematic/board connectivity, ERC/DRC, package/MCAD synchronization and fabrication | `ECD-001`–`005`, `GATE-100` |
| Remaining formats, customization, generative lifecycle | Publicly implementable interoperability, offline scripting and study/outcome representation | `FULL-002`–`004` |

These epics are decomposition obligations, not enormous single PRs. Detailed
command/default inventories remain work as each family approaches implementation.
Large specialist domains must not be “implemented” with a menu, mesh preview,
a file launcher, or a stock screenshot.

## Cloud boundaries and contradictions

The cloud exception is **operation-specific**. Current public study help says
Simulation studies run in cloud; generative outcome generation also explicitly
uses cloud computation. Those operations can be deferred with dated evidence.
Local/in-canvas rendering remains required, and in-canvas startup is explicitly
documented offline. Study types (reference `B27`),
outcome generation (reference `B35`),
local/in-canvas rendering (reference `B26b`).

`FULL-001` must distinguish cloud compute, network-only library/project actions,
explicit local capability and unknowns. An extension/subscription label is not
cloud evidence. Local result import is not a solved analysis; mesh export is not
an additive manufacturing workspace; a local video destination does not prove
all animation operations work offline.

Public-reference inconsistencies remain explicit: older official simulation
teaching material mentions local solve, while current study help says cloud;
the joint-types page contains a conflicting Ball summary versus prose; surface
tutorial mass values disagree; UI screenshots depict differing toolbar
versions. Do not operate reference CAD application to arbitrate. Corroborate with more public
primary evidence or retain an unresolved record. See the breadth and interface
notes for exact source links and the screenshot provenance manifest.

## Validation and immediate execution order

The initial graph has 130 nodes, twelve cumulative gates, no cycles, and seven
structurally ready tasks. A local validator checks dependency existence and
publication order, source registration/dates, and that all tasks reach the final
coverage gate. It is evidence of a coherent plan, not functional implementation.

Pull the small decisions/prototypes that unlock a real object: dated baseline,
MIT-compatible dependency admission, document authority, persistent references,
reliable local control and the cumulative runner. The existing shell baseline
is independently reviewable. Then integrate actual hardware CAD rendering and
an editable/selectable primitive with undo/save/STEP. Follow with the constrained
bracket, surfaces and exchange gates before broad specialist work.

Every later gate reruns the earlier representative workflow suite on the same
build. Artifacts separate build, API, geometry, native reopen, exchange, physical
input and visual evidence. Normal operation/capture uses Alloy's API; computer
input only covers missing physical interactions. Public screenshots must match
viewport/scale/theme/state before a close visual-comparison claim, or mismatches
must be stated. Every newly discovered noncloud requirement adds concrete work
and a gate prerequisite; the final ticket cannot close on issue-count progress.
