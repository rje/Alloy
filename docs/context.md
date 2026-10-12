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
is true. DOC-001 now links the thin OCCT modeling boundary; `kernel.probe`
evaluates a real diagnostic solid without adding authored geometry. Its real-app
case and box/cylinder/open-face unit checks passed and Astra reviewed the slice.
See [kernel receipt](validation/kernel-boundary.json). Document operations and
rendered primitives follow separately.
The cumulative runner in `tests/workflows/run.py` now wraps the real shell case,
with fixture/candidate/artifact hashes, capability checks and live gate prerequisites.
Its real-app success and focused refusal checks passed, and Astra reviewed the
bounded implementation. See [runner receipt](validation/workflow-runner.md). Physical
and visual acceptance remain separate. DOC-002 and FND-006 are also registered;
later cases remain unavailable.
The DOC-002 authored layer now has a root UUID, typed owned primitive intent,
canonical millimeters and mm/in display units. A bounded unit fixture and the
real-app DOC-002 case passed. Modeling operations are not exposed yet; see
[authored receipt](validation/authored-document.md). There is no CAD geometry, sketch solver, native persistence,
import, or export yet. Modeling controls are explicitly unavailable. Visual and
physical-input parity have not been established.

FND-006 adds verified live-instance discovery, shared command sessions, observable
units/search dialogs, draft preview, cancel, stale-commit refusal and transaction
IDs. Its real two-app workflow and focused selector regression passed; Astra's
independent review found no remaining blocker. See
[session receipt](validation/command-sessions.md). Modeling previews and async
jobs await real features; Windows local-control adaptation remains FND-012.

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
Astra's deep dive and backlog are published: **135 issues** across ten phases,
comprising **87 implementation tasks, 13 research tasks, 22 epics, and 13
cumulative validation gates**. All remote issue bodies, dependency links,
labels, milestones, and live readiness were verified through the API. The shell
baseline [`GATE-000` #12](https://github.com/rje/Alloy/issues/12) is closed;
FND-010 #10 is also closed with independently reviewed runner evidence;
FND-003, FND-002 and DOC-002 are closed after independently reviewed evidence. Live counts
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
Windows named-pipe control, Linux X11/GLX runtime qualification, relocation,
dependency notices and tagged milestone publishing remain open.
GitHub operations must use `gh` and APIs; the user explicitly rejected browser
computer control for GitHub. The CLI token now supports Projects; all 135 items were verified through the API.
The board auto-add workflow targets open issues from rje/Alloy. Update Status
through the CLI/API as tasks start and finish.

## Where to resume

FND-003 is complete with a permitted Qt/OCCT comparison and independently reviewed
[decision 001](decisions/001-authored-document.md). It selects authored JSON and one
QUndoStack, with no persisted session journal. DOC-002 integration and its focused evidence are complete;
FND-006 shared sessions are complete; DOC-003/DOC-004 supply exposed undo and
native storage next. FND-004 resolves stable geometric references independently.
Its bounded OCCT fixture now passed generator role rebinding, fillet history,
split/deletion/merge cases; [decision 002](decisions/002-geometric-references.md)
defines conservative unresolved behavior. DOC-012 still implements the real resolver.

The proposed next product increment is [roadmap milestone 1](roadmap.md): integrate OCCT
and a real CAD viewport/document; create/select/inspect a primitive; add undo,
save/reopen, and small STEP import/export fixtures through the shared command path.
Research the public interaction details needed for those commands as they enter
implementation. Do not claim geometry success from the existing reference grid.

The first parametric workflow then adds a constrained/dimensioned sketch,
extrusion, fillet, upstream dimension change and recompute, save/reopen, and mesh
export. Test persistent face/edge references early. The proposed coverage is in
[product-scope.md](product-scope.md), with boundaries in
[architecture.md](architecture.md) and evidence rules in
[compatibility.md](compatibility.md).

FND-002 admission supports shared Qt/OCCT and FTL FreeType. The pinned component
record, retained notices and local absolute-path link audit are in docs/dependencies.
It explicitly leaves package closure, sources and replacement instructions to
CI-002/CI-003. Solver candidates remain unadmitted.

The exact dated reference release/platform/theme/navigation baseline,
minimum OS versions/architectures and sketch solver
remain unresolved. They are recorded gaps, not forgotten decisions. Current
public-interface/build research now covers the major feature families with 113 dated
primary-source entries, contradictions, and open questions. It does not yet specify
every command, default, error, or visual state.
