# Continuation context

Updated 2026-10-11. This is the project handoff for a fresh session. Read the root
[AGENTS.md](../AGENTS.md) first; its reference restrictions apply to all work.

## Decisions from the conversation

- The initial candidates were standalone open-source Maya and Houdini equivalents.
  Parametric CAD became the selected direction. The earlier alternatives are
  preserved in [target-comparison.md](target-comparison.md).
- Alloy must eventually be a functional feature-for-feature CAD replacement,
  open source and modifiable. It is a standalone app, not a Blender extension.
- Users should find the same feel: familiar visuals, layout, styling, command
  behavior, keyboard controls, navigation, and workflows. A small initial release
  does not reduce that long-term goal.
- Features that currently require cloud access may be deferred. This exception
  is feature-specific, not a reason to discard entire feature families.
- Initial emphasis is shape creation, sketches, solids/surfaces, common imports,
  and exports suitable for 3D printing.
- This is a clean-room implementation. Public docs, screenshots, and instructions
  are the reference material. Never run, inspect, or capture proprietary reference software or use
  non-public information. Public API descriptions are references only.
- Agentic control must be built in from the start for operation, self-testing,
  state inspection, and captures. UI and agents use the same application commands.
- Verification must be proportionate: useful unit tests and pragmatic integration
  tests for core behavior and material risks. Stop after reasonable assurance;
  do not repeatedly run unchanged workflows or chase hypothetical edge cases.
- The user selected C++ and dropped Rust from the planned stack after reviewing
  the CAD ecosystem rationale. The scaffold uses C++20/Qt 6, with OCCT as the
  proposed CAD kernel and Python for the initial agent client.
- The user selected MIT for Alloy's original source and documentation.
  Third-party libraries retain their own licenses.
- Before modeling operations accumulate, settle authored operation history,
  transaction authority, undo/redo and recompute boundaries. FND-003/FND-004
  lead this work; see architecture.md. The FND-003 comparison and DOC-002 authored
  integration are complete. See decisions/001-authored-document.md.
- The user requires a GPU-accelerated 3D viewport from the foundation, with
  actual renderer evidence. OCCT/OpenGL replaced the QPainter reference grid;
  software rendering does not satisfy viewport acceptance.
- macOS, Windows, and Linux are required product targets. Milestone releases
  must offer downloadable native builds with platform-specific runtime and
  cumulative workflow evidence. Currently only macOS runtime is verified.
- Native CI runs only for version tags or explicit manual dispatch. Cut coherent
  reviewed workflow versions; ordinary commits/PR updates do not run the matrix.
- The accepted renderer destination is Alloy CAD presentation on an established
  RHI with native Metal on macOS and Vulkan on Windows/Linux. OCCT remains the
  kernel; current visualization is still AIS/V3d/OpenGL. See decision 003.
- The next main modeling milestone is origin-plane sketch → rectangle →
  constraints/dimensions → extrusion → upstream edit, undo/redo and save/reopen.
  Primitives established geometry/history/storage; broad STEP/platform/face/region
  work stays tracked without blocking that first sketch workflow.
- The user authorized concurrent agents for independent feature work, with shared
  command/document contracts and one reviewed integrated app.
- The project lives at `~/src/cpp/Alloy`. Alloy is the working name. Astra was
  requested for the initial public-interface research and project foundation,
  and completed that assignment.

## Actual implementation

A native Qt Widgets shell exists in `src/`, built by CMake. It has command search,
Browser/Timeline/navigation layout controls, and an independently drawn reference
grid with named orientations, pan, and zoom. A private local control socket and
`tools/alloyctl.py` expose command discovery, execution, state, and capture.

OCCT 7.9.3 is linked for native AIS/V3d/OpenGL visualization; the live Apple M4
hardware driver and Retina framebuffer/startup/resize were verified. See the
[GPU foundation receipt](validation/gpu-foundation.md). `kernel.visualizationIntegrated`
is true. DOC-001 links the thin OCCT modeling boundary; `kernel.probe`
evaluates a real diagnostic solid without adding authored geometry. Its real-app
case and box/cylinder/open-face unit checks passed and Astra reviewed the slice.
See [kernel receipt](validation/kernel-boundary.json). The document now evaluates
authored boxes and cylinders through that same boundary and renders their real
AIS shapes; `kernel.integrated` is true.
The cumulative runner in `tests/workflows/run.py` now wraps the real shell case,
with fixture/candidate/artifact hashes, capability checks and live gate prerequisites.
Its real-app success and focused refusal checks passed, and Astra reviewed the
bounded implementation. See [runner receipt](validation/workflow-runner.md). Physical
and visual acceptance remain separate. DOC-002, FND-006, the shared `primitives`
case and DOC-003/004/005/007/008 aliases are registered. `primitive-regression`
runs shell, document, sessions and primitives once each on the same candidate.
The DOC-002 authored layer now has a root UUID, typed owned primitive intent,
canonical millimeters and mm/in display units. A bounded unit fixture and the
real-app DOC-002 case passed; see [authored receipt](validation/authored-document.md).
Box/cylinder creation, dimension editing, deletion, undo/redo and native `.alloy`
save/open are implemented. Candidate authored state and evaluated geometry are
validated before one journal entry is committed. Undo restores both snapshots;
redo preserves IDs, and divergent edits discard the abandoned redo branch.
Files retain editable intent; evaluated geometry is rebuilt on open. Reopening
starts a clean session journal. Invalid edits/files and failed saves preserve
the live document. Preview is disposable, with no authored edit on cancel.

The [primitive receipt](validation/primitives.md) records the passing cumulative
real-app regression, three focused CTest checks and Astra's independent review.
Separate native UI checks covered toolbar creation, dimension typing, keyboard
undo/redo, native Save and Timeline edit/cancel. An editable sample lives in
`examples/gauge-block.alloy`. Body picking and geometry inspection use the actual
OCCT scene and evaluated document. Face/edge selection, persistent-reference
integration, arbitrary orbit, application sketch constraints, import and export remain pending.
Visual/interaction parity and the broader product gates are not accepted.

SKT-001/002 now implement origin-plane sketches, connected lines and two-point
rectangles through the shared application commands. XY/XZ/YZ planes have explicit
frames; stable sketch, point and line IDs live in authored JSON. A rectangle is
four authored edges with shared endpoints. Sketches have separate evaluated OCCT
edge caches and never claim solid body IDs or volume. Create/edit sessions retain
a disposable draft; Finish creates one QUndoStack entry, while Cancel and palette
close preserve the prior document. The UI has an origin-plane chooser, contextual
tools and a docked Sketch Palette. Canvas clicks and numeric coordinates use the
same commands; L/R select drawing tools and canvas Escape stops a pending tool
without cancelling the sketch. There are no application constraints, driving
dimensions or extrusion yet.

The [sketch receipt](validation/sketches.md) records the integrated build and
three focused CTest checks, the previous shell/document/session/primitive cases,
and the new sketch workflow on the same candidate. Native checks separately
covered choosing XY, two-click rectangle drawing, L/Escape, Finish, Timeline
re-entry, adding a line and palette-close cancellation. The independently authored
`examples/rectangle-sketch.alloy` was opened in the real application. Three-plane
world transforms, draft preservation, mixed primitive/sketch undo and native
save/reopen are covered by actual core and application checks.

FND-005 selects PlaneGCS at a pinned public revision through a replaceable shared
library, with Eigen/Boost and a passing native-library fixture. See
[decision 004](decisions/004-sketch-solver.md) and its source-linked research.
Constraint/dimension records, application solving and diagnostics still await
SKT-003/004; the fixture is not an integrated application solver or a qualified
portable package.

A reported viewport mouse regression is fixed and independently reviewed.
Camera setters now defer redraw until a completed view is ready; Retina picking
and pan use native window coordinates rather than framebuffer DPR. Named views
retain direction, resize retains aspect, and fit keeps tall geometry visible.
Actual camera diagnostics, the fresh regression and separate native mouse checks
are recorded in [viewport input receipt](validation/viewport-input.md).
The user's three-body model was preserved in a local recovery file and reopened.

The subsequent [Mac editing receipt](validation/mac-editing.md) records fractional
JSON-to-native picking, distinct blue selection, Option/Alt-left pan and a real
Qt QDockWidget primitive panel. Field completion updates disposable previews;
editing temporarily hides the target presentation, with restoration on cancel,
failure or stale sessions. Dock/float state and bounds are inspectable through
the API. Final cumulative regression and Astra review passed; native typing→OK
and title-close checks passed on the docked candidate. Physical Option-drag
remains unverified because the exposed computer-use API cannot hold a modifier.

FND-006 adds verified live-instance discovery, shared command sessions, observable
units/search dialogs, draft preview, cancel, stale-commit refusal and transaction
IDs. Its real two-app workflow and focused selector regression passed; Astra's
independent review found no remaining blocker. See
[session receipt](validation/command-sessions.md). Primitive sessions extend this
same flow with real geometry previews; asynchronous jobs await a long-running
feature. Windows local-control adaptation remains FND-012.

The native Debug build and CTest real-app smoke passed on Apple silicon macOS
with Qt 6.11.2. The smoke exercised real commands, state changes, errors, search
open/close, screenshots, and clean exit. See [development.md](development.md) for
exact commands, dependencies, evidence paths, and limitations; see
[agent-control.md](agent-control.md) for the protocol and client.

Local evidence is in ignored `build/evidence/`. Dated reference URLs and three
public documentation images are stored separately in `../Alloy-reference-records/`,
with stable IDs in [the source registry](backlog/sources.json) and
[the image provenance manifest](research/reference-images/provenance.json).
They are research references, never app assets. A fresh checkout can build and
verify Alloy without the private reference records. Product-specific reference
names and URLs must not be reintroduced in public files or planning records.
The user authorized a complete reachable-history rewrite and public planning
cleanup. Older screenshots and source hashes remain historical observations;
they do not establish that a transformed historical commit was tested unchanged.

The user authorized a public GitHub repository/project and an issue backlog with
cumulative validation gates. The repository is [rje/Alloy](https://github.com/rje/Alloy)
and the public board is [Alloy](https://github.com/users/rje/projects/1).
The MIT-licensed foundation is published on `main`.
Astra's deep dive and backlog are published: **143 issues** across ten phases,
comprising **92 implementation tasks, 14 research tasks, 22 epics, and 15
validation tasks**. All remote issue bodies, dependency links,
labels, milestones, and live readiness were verified through the API. The shell
baseline [`GATE-000` #12](https://github.com/rje/Alloy/issues/12) is closed;
FND-010 #10 is also closed with independently reviewed runner evidence;
FND-003, FND-002, DOC-002, FND-006, FND-004, FND-009 and DOC-001 are closed after
independently reviewed evidence. DOC-003/004/005/007/008 are closed with reviewed
primitive evidence. UX-001 is closed with independently reviewed input/editor evidence;
FND-005 and SKT-001/002 are closed with their bounded solver/sketch evidence:
18 tasks closed, 125 open at this handoff. Live counts
are obtained with tools/backlog.py rather than this earlier publication snapshot. Later epics need
bounded child issues before implementation; this count does not exhaust parity.
Read [the backlog](backlog/README.md), [publication receipt](backlog/publication.json),
and [deep dive](research/cad-workflows.md) before pulling work. Run
`python3 tools/backlog.py ready` for the current queue and
`python3 tools/backlog.py ready --apply-labels` after changing issue states.
The later cross-platform requirement added five issues: FND-012, CI-001,
CI-002, GATE-005 and CI-003. Read [releases.md](releases.md) and the
[release research](research/cross-platform-releases.md). Native CI is seeded
for macOS arm64/Intel, Windows x64 and Ubuntu24.04 x64; its initial run compiled Windows and both macOS architectures. Linux failed
on a missing Fontconfig development header; that run and its artifacts were
removed with the old-history cleanup. The corrected
[matrix run](https://github.com/rje/Alloy/actions/runs/38192218283) compiled all
four targets successfully at source 12fe534. FND-009's independently reviewed
toolchain/artifact audit passed; subsequent candidates remain separate.
It retains unbundled compile artifacts, not yet independently runnable packages.
A later uniform Python pin failed Mac/Windows setup while Linux compiled;
platform-specific archive pins are corrected. The new matrix passed Python
setup on all four targets; Linux and macOS arm64 compiled, Windows Qt SDK archive
extraction failed before compilation, and
macOS Intel also compiled successfully. CI-001 remains open. The later automatic
run 38199898011 was cancelled under the accepted tag/manual build policy. See
dependencies/native-builds.md.
Windows named-pipe control, Linux X11/GLX runtime qualification, relocation,
dependency notices and tagged milestone publishing remain open.
GitHub operations must use `gh` and APIs; the user explicitly rejected browser
computer control for GitHub. The CLI token now supports Projects; all 143 board items were verified through the API.
The board auto-add workflow targets open issues from rje/Alloy. Update Status
through the CLI/API as tasks start and finish.

## Where to resume

FND-003 is complete with a permitted Qt/OCCT comparison and independently reviewed
[decision 001](decisions/001-authored-document.md). It selects authored JSON and one
QUndoStack, with no persisted session journal. DOC-002 integration and its focused evidence are complete;
FND-006 shared sessions and the first exposed modeling/undo/storage slice are
complete. FND-004 resolves stable geometric references independently.
Its bounded OCCT fixture now passed generator role rebinding, fillet history,
split/deletion/merge cases; [decision 002](decisions/002-geometric-references.md)
defines conservative unresolved behavior. DOC-012 still implements the real resolver.

The rebuilt sketch-capable application is open with the user's preserved three-body model.
Use `python3 tools/alloyctl.py instances` to obtain the current identity. Native
reopen resets the session journal while preserving authored feature history.
The launch record is `build/evidence/human-ready-sketch.json`; instance discovery
remains authoritative if that process is subsequently closed. Existing workflow
cases passed in `build/evidence/sketch-first-final/report.json`; that combined
report remains failed because its new sketch test had incorrect expectations.
The corrected new sketch workflow passed in `build/evidence/sketch-final/report.json`
on the same binary. Separate native interaction evidence is in
`build/evidence/sketch-native`. Repeat checks only when a material change warrants
them; do not rerun unchanged workflows to manufacture further assurance.

**Next execution priority:** SKT-003 integrates the selected PlaneGCS adapter and
real constraints/diagnostics; SKT-004 adds linear dimensions. Then implement
SKT-014 simple closed profiles, HIS-001 ordered sketch/profile
recompute and SOL-001 extrusion. SKT-015 is the next human-test checkpoint:
draw a 40 by 25 mm rectangle, extrude 5, edit width to 50, verify volume 6250 mm3,
undo/redo, cancel and save/reopen. HIS-001 is accepted on an actual sketch/profile
dependency first; SOL-001 then extends the same authority to solids.

SKT-001 no longer waits for GATE-010 or face-reference resolution. SKT-013 retains
face-attached sketches; SKT-009 retains circles/holes/complex regions; PAR-001 and
HIS-004 retain named expressions and broader recompute. Full bracket and product
release gates remain unchanged in meaning. STEP, face/edge selection, full camera
navigation and qualified platform packages remain separate required work.
RHI-001/RHI-002 and GATE-045 record the accepted native graphics migration.
The [RHI comparison](research/rhi-candidates.md) researches QRhiWidget and bgfx and
recommends a bounded pinned-Qt spike first. No native RHI spike or migration has
run yet, so the live application remains hardware-accelerated OCCT OpenGL.

The user authorized simultaneous feature agents. The completed tracks separated
authored sketch/history, UI/GPU interactions and Astra's solver research/spike;
they agreed the command/state contract before editing disjoint files. Root then
built and validated one integrated candidate. Reuse that ownership pattern for
independent next increments, with one integration authority and bounded verifier
review; do not launch conflicting app/build runs from several agents.

FND-002 admission supports shared Qt/OCCT and FTL FreeType. The pinned component
record, retained notices and local absolute-path link audit are in docs/dependencies.
It explicitly leaves package closure, sources and replacement instructions to
CI-002/CI-003. Solver selection and its passing native fixture are recorded in
decision 004; production extraction, platform exports and distributable runtime
closure remain part of the application integration work.

The exact dated reference release/platform/theme/navigation baseline,
minimum OS versions/architectures and portable solver runtime qualification
remain unresolved. They are recorded gaps, not forgotten decisions. Current
public-interface/build research now covers the major feature families with 119 dated
primary-source entries, contradictions, and open questions. It does not yet specify
every command, default, error, or visual state.
