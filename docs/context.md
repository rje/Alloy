# Continuation context

Updated 2026-10-12. This is the project handoff for a fresh session. Read the root
[AGENTS.md](../AGENTS.md) first; its reference restrictions apply to all work.

## Current checkpoint: complete bracket chain

The registered `tutorial_bracket_api` case now passes from blank document through
all three illustrated activities, exact history and save/reopen. Astra accepted
source/geometry and representative visuals. A separate native walkthrough also
reached the final five-hole solid; see [its receipt](validation/illustrated-bracket.md)
for the extra A1 Finish/reopen, two API multi-selection substitutions, physical
modifier gap and remaining visual mismatch. Exact TUT-003 stays open.

Root owns `build-full-tutorial/alloy.app` (TutorialCandidate) and
`build-tutorial-api/alloy.app` (API runner candidate). The human CutCandidate in
`../Alloy-worktrees/profile-cut/build-cut/alloy.app` and older build-input app
remain untouched. Next: correct fully-constrained sketch colors with Sol and
Astra verification, then continue tutorial visual/interaction gaps. Preserve
accepted evidence; do not repeat unchanged geometry suites for a color-only fix.
No release/tag/CI was triggered. Status requests do not pause autonomous work.

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
- Continue working autonomously through the ready queue after reporting useful
  human checkpoints; a status update or request to show the app does not pause
  implementation. The user explicitly permits Sol implementation agents with
  Astra as independent verifier. Keep human test windows untouched while building
  and validating separate candidates.
- Luna maintains an external searchable workflow/visual knowledge base using
  public web research; Astra independently checks high-risk behavior and the
  integrated implementation. Detailed records, source URLs, images/video frames
  and derived databases belong in `../Alloy-reference-records/knowledge`, with a
  lightweight query helper and usage guide in the repo. Keep relevant defaults,
  selection, visual state, commit/cancel/history and material edge cases sourced.
- Face selection → face-supported sketch → referenced model-edge dimensions →
  Extrude Cut/Join → another face sketch is an early core workflow. DOC-006/012,
  SKT-013/010/016 and SOL-015 are P0; SKT-017 verifies the joined local workflow.
  The overall UI still falls short of the target: FND-011 tracks concrete icon,
  density, contextual-toolbar, palette, theme and stale-label corrections.
- The first manual sketch coordinate controls are functional helpers. User review
  accepts continuing modeling now and a later public-reference compatibility pass
  on numeric entry, drawing inputs, dimensions, selection and palette behavior.
- The user wants compatibility corrected through bounded independent milestone
  reviews, preferably Astra on work it did not implement. Compare public
  documentation, screenshots, original community articles and video tutorials
  against actual Alloy visual/input behavior and outputs. Record source IDs/dates
  and video timestamps, matched/mismatch/unknown findings and specific fix issues;
  use the API for deterministic fixtures and native input for physical behavior.
  Another reviewer checks Astra-authored code. Follow compatibility.md and the
  compact sketch-controls record; do not sweep every command after every commit
  or claim overall exact parity from one state.
- Public-reference research must lead each user-facing implementation, including
  its controls and presentation. Gather public examples independently and produce
  a bounded specification with verification steps; do not wait for the user to
  supply screenshots or identify missing behavior. In particular, bare dimension
  value labels do not satisfy sketch annotation presentation: research witness
  lines, arrows, placement/editing, glyphs, colors and profiles proactively.
- The user requires the researched visual quality throughout implementation, not
  a later cosmetic pass. Treat actual readable annotations/controls, selection,
  profile/solid presentation and native interaction as feature acceptance, with
  bounded independent review and honest limitations.
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
integration, arbitrary orbit, import and export remain pending.
Visual/interaction parity and the broader product gates are not accepted.

SKT-001/002 now implement origin-plane sketches, connected lines and two-point
rectangles through the shared application commands. XY/XZ/YZ planes have explicit
frames; stable sketch, point and line IDs live in authored JSON. A rectangle is
four authored edges with shared endpoints; new rectangles also author four automatic
horizontal/vertical constraints. Previously saved sketches preserve their intent. Sketches have separate evaluated OCCT
edge caches and never claim solid body IDs or volume. Create/edit sessions retain
a disposable draft; Finish creates one QUndoStack entry, while Cancel and palette
close preserve the prior document. The UI has an origin-plane chooser, contextual
tools and a docked Sketch Palette. Canvas clicks and numeric coordinates use the
same commands; L/R select drawing tools and canvas Escape stops a pending tool
without cancelling the sketch. The later extrusion slice is recorded below.

The [sketch receipt](validation/sketches.md) records the integrated build and
three focused CTest checks, the previous shell/document/session/primitive cases,
and the new sketch workflow on the same candidate. Native checks separately
covered choosing XY, two-click rectangle drawing, L/Escape, Finish, Timeline
re-entry, adding a line and palette-close cancellation. The independently authored
`examples/rectangle-sketch.alloy` was opened in the real application. Three-plane
world transforms, draft preservation, mixed primitive/sketch undo and native
save/reopen are covered by actual core and application checks.

FND-005 selects PlaneGCS at a pinned public revision through a replaceable shared
library. SKT-003/004 now integrate it in the actual application: coincident,
horizontal, vertical, parallel, perpendicular and fixed constraints; linear,
horizontal and vertical driving dimensions; driven measurements and solve/DOF
inspection. Point/line selection is shared by UI and automation, with orange
highlights, green fixed points and whole-sketch black/blue constraint status.
Failed solves preserve the usable draft and report conflict identities. Width
changes update real OCCT edges and driven measurements before one Finish undo
entry. Authored constraints/dimensions survive save/reopen and backward-compatible
unconstrained files remain valid. See [constraints receipt](validation/sketch-constraints.md).
The native adapter/document fixtures passed. Existing application workflows and
the new constraint workflow passed on the initial integrated binary; the focused
new workflow was repeated after the explicit line-ID fix and native palette
readability fix. Keep these candidate boundaries explicit in the receipt.
Application and library operation are qualified only on macOS arm64; portable
exports are implemented but Windows/Linux compilation and runtime/package closure
remain unqualified. The later [annotation/profile receipt](validation/sketch-annotations.md)
qualifies D/placement/inline numeric input, driving label double-click editing,
placement-only label dragging and a selectable real single-loop profile. Native
point dragging, inference and per-entity constraint badges remain linked follow-ups.
This is bounded acceptance, not full interaction parity.

HIS-001/SOL-001 now implement ordered sketch/profile recompute and editable
signed-distance extrusion into a new body. Finished profiles are selectable;
E opens the right dock with a disposable real OCCT solid preview. Commit uses one
shared transaction and hides the source sketch; Browser visibility is reversible.
Editing the source temporarily hides dependent bodies. Exact profile references
resolve against newly evaluated preceding sketches. Invalid dependencies or
failed geometry reject the complete candidate without replacing live paired
authored/evaluated state. Width 40→50 at height 25 and depth 5 rebuilds volume
5000→6250 mm3 with stable profile/feature/body IDs; undo/redo and reopen preserve
the chain. The bounded local SKT-015 checkpoint passed the primitive, constraint,
annotation and extrusion cases on one binary, five focused core tests, separate
native drawing/dimension/extrusion/upstream edit/cancel, and Astra's independent
source/capture/state review. See [the extrusion receipt](validation/sketch-extrusion.md).
MSAA is capability-selected (four samples on Apple M4); world lighting gives
distinct face tones and dark outlines. Preview is neutral/opaque with a blue
normal cue. Broader profile/operation/extent modes and interactive manipulators
remain absent; the opaque Home preview occludes its selected base. This accepts
the declared local workflow, not exact parity, tutorial or product/release gates.

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
Astra's deep dive and backlog are published: **150 issues** across ten phases,
comprising **95 implementation tasks, 15 research tasks, 22 epics, and 18
validation tasks**. All remote issue bodies, dependency links,
labels, milestones, and live readiness were verified through the API. The shell
baseline [`GATE-000` #12](https://github.com/rje/Alloy/issues/12) is closed;
FND-010 #10 is also closed with independently reviewed runner evidence;
FND-003, FND-002, DOC-002, FND-006, FND-004, FND-009 and DOC-001 are closed after
independently reviewed evidence. DOC-003/004/005/007/008 are closed with reviewed
primitive evidence. UX-001 is closed with independently reviewed input/editor evidence;
FND-005 and SKT-001/002 are closed with their bounded solver/sketch evidence;
SKT-003/004 are closed with independently reviewed integrated constraints/dimension
evidence; SKT-014 is closed with reviewed real-profile/annotation evidence.
HIS-001/SOL-001 and SKT-015 are closed with reviewed extrusion and local checkpoint
evidence; their board items are Done. There are 25 tasks closed, 125 open and
13 dependency-ready at this handoff. Live counts
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
computer control for GitHub. The CLI token now supports Projects; the current board items are tracked through the API.
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

The latest local application binary includes placed dimensions, real closed
profiles and editable dependent extrusions.
Use `python3 tools/alloyctl.py instances` for authoritative live identity; old
ready files may be stale. Preserve live authored work and any unfinished draft
before restarting. Recovery documents remain under the local Alloy Application
Support directory. The new sample `examples/dimensioned-rectangle.alloy` has a
50 × 25 mm anchored rectangle, driving width/height and a driven diagonal.
The last human test used `examples/sketch-extrusion.alloy`, an
independently authored 50 × 25 × 5 mm part with driving width/height and editable
Sketch → Extrude history. Click either Timeline item to edit, try undo/redo and
save/reopen. That historical launch record is `build/evidence/human-ready-extrusion.json`
(private instance registry; verify liveness before selecting it). The prior annotation
ready file is stale. Preserve any later human changes before replacement. The
native test part was saved separately in `build/evidence/extrusion-native-part.alloy`.
Detailed transcripts/build/test/native artifacts are retained under
`build/evidence/constraints-*`; the [constraints receipt](validation/sketch-constraints.json)
identifies which checks ran against each binary. Repeat checks only for material
changes, new failures or unresolved risks.

**Active correction:** SKT-005 #32 remains In Progress. Its D/placement/inline
value, label editing/dragging and proper line graphics now have focused API/native
evidence and independent Astra acceptance. Broader drawing inputs/inference and
center-rectangle entry are still open. SKT-008 #35 retains glyphs/per-entity DOF
and visibility controls. The annotation correction's earlier binary SHA256 is
`1610e1ed19378ff5c0ec2d1d4b133559df2d089028598e5c94caa175c9d4bb56`;
`build/evidence/annotations-final/report.json` identifies source/fixture/artifacts,
and `annotations-native-final-*` retains physical checks. Earlier visually failed
captures are not accepted. No unchanged full regression, tutorial gate or CI matrix
was repeated for this bounded correction.

**Latest human-test checkpoint:** HIS-001/SOL-001/SKT-015 have independently
accepted implementation and API/native evidence. The final executable SHA256 is
`5ddb024941d36c7497d8f89b3a40ef0f0004f4b0b4ed06f2a11a3c804d171594`;
`build/evidence/extrusion-final/report.json` records the four same-candidate cases
and checkpoint alias, with 37 matching source/fixture hashes. Native evidence is
`build/evidence/extrusion-native-checks.json`; its origin has a 0.12605042 mm offset,
so it establishes physical drawing/dimensioning without a snapping claim.
[The extrusion specification](research/commands/extrude-first-slice.md) records
public sources and boundaries. No unchanged full suite, tutorial, CI matrix or
release was run. Pull the live ready queue before selecting the next bounded
modeling increment: boolean extrusion and circles/arcs are important remaining
parts of the sketch-led path. SKT-005 #32 owns inline width/height, canvas dimension placement/editing
and drawing inference; SKT-008 #35 owns per-entity diagnostic colors/badges and
visibility controls. Keep functional modeling and these concrete fidelity
corrections moving in bounded increments.

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
decision 004; the extracted shared adapter is now integrated and portable exports are implemented,
while actual other-platform builds and distributable runtime closure remain open.

The exact dated reference release/platform/theme/navigation baseline,
minimum OS versions/architectures and portable solver runtime qualification
remain unresolved. They are recorded gaps, not forgotten decisions. Current
public-interface/build research now covers the major feature families with 157 dated
reference entries, including original community tutorials, contradictions, and open questions. It does not yet specify
every command, default, error, or visual state.

## Future tutorial exercises

Public beginner-project research is recorded in
[the tutorial recipes](research/tutorial-gates.md). TUT-001 #146 follows the full
toy-block lesson; TUT-002 #147 follows the library die/component/STL lesson.
Neither has been executed. Every source step must be available through shared
real-app commands and the human UI; skipped modeling steps mean incomplete,
and visual expectations need inspected public images before matched claims.
The recipes separate source dimensions from Alloy-chosen deterministic fixtures.
SKT-016 #144 and SOL-014 #145 explicitly cover missing construction/distance and
spherical-cut capabilities. The full die waits on component organization; early
adaptations must name that deviation. These additions bring the backlog to
150 tasks: 95 implementation, 15 research, 22 epics and 18 validation tasks.
The nearest sketch/extrude checkpoint SKT-015 and version/manual CI policy remain
unchanged. Pull readiness and issue state remain authoritative through gh/API.


## Active external knowledge and implementation work

The external corpus is at `../Alloy-reference-records/knowledge`: portable JSON
records and source/image provenance are authoritative, with a rebuildable SQLite
FTS index. The repository keeps only [the usage guide](knowledge/README.md),
`tools/knowledge.py`, and neutral planning/source pointers. Initial topics are
face-supported sketches, projected geometry, edge-relative dimensions, curves,
selection/constraint status, and solid operations. Visuals are research evidence,
never shipped assets; unknown version/theme/scale and unplayed video material
are explicitly qualified. FND-013 #148 accepts the initial six-record, nine-image library after Luna research
and Astra independent review. Ongoing expansion remains part of implementation.

The next implementation candidate is in progress: analytic circles/arcs and
Combine Join/Cut/Intersect cores have source review, but are not yet accepted as
working application features. UI integration and real-app verification remain.
Build this candidate separately in `build-next` while preserving the live human
extrusion window. SOL-015 #149 tracks direct Extrude operations; SKT-017 #150
tracks the subsequent face-sketch/projected-distance/Cut/Join local checkpoint.

The external visual contact sheet is `knowledge/visuals/index.html` beneath the
sibling archive. A second batch adds fillet/chamfer and an explicitly separated
beginner face-sketch/Cut/Join recipe, bringing the corpus to eight records and
12 images; Astra accepted the corrected expansion after independent source review. No research
image or bulk record is committed. The initial six-record acceptance remains
separate from later expansions.

Implementation handoff: Combine core CTest passed in `build-next`; its first
integrated app compile passed before a later multi-tool union correction.
Astra identified missing history-prefix display/picking in Combine edits; the
helper correction is written and the curves agent is integrating renderer stage
support. Circle/arc UI and renderer integration remain active. Neither new UI
feature has real-app/native acceptance yet. Focused cases are prepared in
`tests/boolean_api.py` and `tests/sketch_curves_api.py`, registered in the workflow
ledger. Relink and test this candidate only after source ownership is released.
Do not overwrite or operate the existing human extrusion window in `build/`.


## Latest candidate: curves and Combine

The new implementation in `build-next` compiles, passes focused core/API cases
and has independent Astra source/visual acceptance for its bounded behavior.
Read [curves/Combine evidence](validation/curves-combine.md) for exact candidate
boundaries. Final renderer candidate SHA256 is
`8f5a9f2e74f8c8c5b334410992036c6a2d33b2be03b037262d43082aee732a9b`.
Circle/arc controls and dimensions, circle-profile extrusion and Combine history
are implemented. The final visual fixes make diameter labels readable and curved
solids smooth; Combine edits display the correct history prefix and consumed tools.
Native verification remains incomplete because the desktop controller could not
bind the separate candidate window (`cgWindowNotFound`). Temporary test instances
were closed; instance discovery showed no remaining original human process at
that point. Old ready files are historical, not proof of a live app. No human
model was changed. SOL-002 and SKT-006 remain In Progress; full angular dimensions
and further sketch input compatibility are not complete. Next: bounded native
C/D/arc and Combine selection/Keep Tools checks, then face-selection/naming/sketch
support work already prioritized in the queue. Research library expansion is
accepted at eight records/twelve images, fully external.

## Human feedback and active input correction

The next human test found: C needed an extra viewport click after XY; circle
selection/D was unclear; extrusion flickered; Combine OK was difficult to find;
Browser cylinder selection during Combine crashed. The retained crash report is
`~/Library/Logs/DiagnosticReports/alloy-2026-10-12-100835.ips`; its top frame is
Browser itemClicked after a checkbox visibility rebuild invalidated the item.
Working-tree fixes defer tree/list checkbox commands, focus the viewport after
sketch entry, pin Combine OK/Cancel below scrollable fields, replace previews
without intermediate removal frames, and hide coplanar source profile fill during
extrusion preview. The user asked to relaunch before further verification.

`build-next` was rebuilt and launched for that retest: PID 95777, instance
`40ef488a-9608-45b1-94d8-f24b46d94a47`, ready file
`build-next/evidence/human-ready-input-fixes.json`. Verify liveness; preserve any
new user work. No tests or API mutations should use that human instance.

Astra's source review then identified checkbox selection/session-staleness as a
remaining issue. A further source-only fix suppresses checkbox-as-body-selection
and lets a currently valid Combine session follow a visibility-only revision.
That final change is NOT in the running human binary yet. New
`tests/widget_interactions.cpp` exercises actual synthesized Qt events against the
shared `alloy_application` core. It compiled before the final source correction,
but has not run; coordinate its visible test windows with the human retest.
Physical OS input and flicker acceptance remain pending. Do not describe these
as verified fixes yet. The constrained-value field is awaiting identification
from the user; public research is recorded externally in
`knowledge/specs/circle-dimension-edit-semantics.md`. Existing driving dimensions
are editable, driven measurements read-only; the exact control the user meant
has not been confirmed. Keep source edits, tests and build ownership centralized.


## Input/theme correction candidate (2026-10-12)

The human retest exposed a black Combine scroll container and continuing sketch
focus trouble. Application startup now applies a shared explicit light QPalette
(active/inactive/disabled roles), including unstyled scroll descendants. Palette
creation and existing-dimension editing are separately labeled; selecting a
circle/arc or a dimensioned line chooses a matching existing dimension. Driving
values require Apply; driven measurements and missing dimensions disable editing.
Line dimensions match endpoint pairs in either order.

The sketch-entry focus handoff now repeats after panel/toolbar layout and requests
native viewport activation. Actual native coordinate mouse input on macOS passed
XY -> C without a viewport click, circle creation, Escape, circle-edge selection
-> D, placement, typing 20 and Return. Authored state confirms radius 10 mm and
one driving diameter of 20 mm. An earlier apparent failure used an accessibility
button action followed immediately by C: logging showed the key arrived while
still in sketch.plane, before its animated button activation completed. Do not
confuse that test-order artifact with the user's original report. Root-cause
certainty for the user's original focus failure remains limited; retain human
retest coverage.

Final focused widget tests pass, including circle Apply/read-only measurement,
line endpoint dimension selection, Browser visibility during Combine and pinned
OK/Cancel. Real-app extrusion and Combine API/history/reopen regressions pass.
Evidence is local under build-input/evidence: widget-input-final.txt,
native-circle-d-complete.json, native-circle.png, combine-light.png,
extrude-final/sketch-extrude-api.json and combine-final/boolean-api.json.
Astra independently reviewed final source/captures and found no blocker within
this scope. Temporal extrusion flicker still requires human acceptance; static
captures cannot prove its absence.

Use build-input/alloy.app for the updated retest candidate (separate local bundle
identity Alloy Input Candidate). The earlier build-next human window may contain
unsaved sketch work: preserve it and verify live discovery before any operation.
The candidate includes Browser/Combine crash prevention and preview changes from
the preceding section. No milestone release or CI build was triggered.

## Exact illustrated tutorial requirement (2026-10-12)

The user requires tutorial steps to match exactly, including visible constraint
icons and presentation. The earlier suggestion to skip missing operations is not
an acceptance path. All three illustrated bracket activities were audited against
current source and published screenshots with Astra review. See
[the gate contract](research/illustrated-bracket-gate.md). Source-located detailed
instructions, uncertainties and 22 unique visual references are external under
`knowledge/tutorials/illustrated-bracket/`. The exact workflow is blocked, not run.
Five new backlog tasks #151–155 cover document tabs, constraint-first sessions,
sketch fillets, multi-profile Press Pull and TUT-003. Existing owners were tightened for
inference, two-line angles, Equal, construction, region extraction and model-edge
references. TUT-003 feeds GATE-020; geometry alone cannot close its visual gates.

## Tutorial-first continuation (2026-10-12)

User requested reprioritization around exact tutorial completion and automated
end-to-end validation of each increment. `docs/backlog/tutorial-sequence.json`
is the active ranked sequence; `tools/backlog.py ready` prioritizes its available
owners without waiving dependency closure. GitHub issues have P0 tutorial work
and the project has Delivery order ranks. Start with constraint/input/angle/badge
slices, then exact first activity, rounded profile/Press Pull, face references,
and complete five-hole Cut. Full TUT-003 remains blocked. Short-candidate research
is external `knowledge/tutorials/short-candidates.json`; the supplementary wall
outlet lesson adds mirror/concentric and is not an easier replacement gate.

Sol implemented Equal and persistent constraint-first tools; Astra reviewed the
core and caught a failed-attempt presentation refresh bug that root corrected.
Root built an isolated `build-tutorial` candidate. Solver and existing synthetic
widget regressions pass; `tests/constraint_tools_api.py` passed the real-entrypoint
five-circle diameter/Equal chain and line-relation workflow with glyph identities,
undo/redo, save/reopen/edit. See `docs/validation/constraint-tools.md` for exact
coverage and evidence paths. Native repeated picking and source-matched visual
acceptance remain pending; SKT-007/008/018 stay open. Next resolve these bounded
interaction/visual checks, then origin/inference and two-line angular dimension
for the first illustrated activity. Do not treat API-created coordinates as an
inference or native-input pass. The human `build-input` app was preserved; do not
replace its binary or discard its document. No CI or release was triggered.

## Parallel tutorial work and angular milestone (2026-10-12)

User explicitly requested subagents working through the board and notification at
major testing milestones. Use disjoint implementation ownership, root-owned
build/app/integration and independent Astra verification. Keep tasks/issues/board
current; stop verification after proportional evidence and proceed to the next
visible workflow. Research material remains external.

Sol core and UI implementers delivered real two-line angular dimensions for
SKT-006: stable entity IDs plus raySigns retain quadrant intent, native D supports
two consecutive line picks, placement chooses acute/obtuse wedge, labels follow
the arc tangent, and degree values survive length-unit changes. Root's real-app
`tests/angular_workflow_api.py` passes a constrained triangle -> 5-mm extrusion ->
45-to-60-degree upstream edit -> undo/redo -> reopen, including 45/135-degree
pointer preview checks. Core tests passed. Native 45-degree extension and final
145-degree obtuse entry passed with Astra-reviewed captures. See
`docs/validation/angular-sketch.md` for exact evidence and limitations.

Native testing caught a Qt macOS accessibilitySelectedChildren crash on a sketch
list rebuilt every pick. Selection-only refresh now reuses tree items. Retest
passed; draft-changing refresh still rebuilds so this is targeted mitigation.
The existing circle-widget preview test failed once, then passed in isolation;
FND-007 tracks the unresolved flake. Do not claim general input or full-tutorial
acceptance. The native Equal follow-up also passed; its receipt is updated.

All private native test windows were closed. Human build-input was untouched.
Updated candidate: build-tutorial/alloy.app (local identity Alloy Tutorial
Candidate). Useful saved part: build-tutorial/evidence/angular-workflow/angular-plate.alloy
(current angle60, displayinches, real extrusion). No release/tag/CI run.
Next primary #32 work: origin/endpoint snapping and durable coincidence before
claiming exact first activity. Luna prepared bounded public-source instructions
in ../Alloy-reference-records/knowledge/specs/origin-endpoint-inference.md.
Read it before implementing; documented snap symbols/retained relationships are
separate from proposed pixel threshold and unresolved override behavior.

## Autonomous continuation correction (2026-10-12)

The user clarified that milestone notifications are progress updates, not stopping
points. Continue pulling and implementing the tutorial-first queue with subagents
and independent verification after reporting a milestone. Do not yield merely
because one slice was committed. Stop only for a genuine blocker requiring user
input, explicit user direction, or completion of the requested queue. Preserve
proportional verification and update issues/board as work advances.

## Origin snapping checkpoint and next work (2026-10-12)

Origin/endpoint inference now creates durable coincidence constraints for native
line/rectangle pointer creation. Focused core and real-app profile/extrusion/edit/
history/storage checks pass. Native L and five clicks produced a closed four-edge
outline with a fixed origin and retained chain joints; native Finish and subsequent
API history/reopen passed. Astra reviewed source and captures without a material
blocker. See docs/validation/sketch-snaps.md for exact limits and evidence. The
private candidate was closed; human build-input remains untouched.

The backlog is157 tasks. SKT-020 (#156) tracks accepted-edit undo inside unfinished
sketches, the next implementation slice. FND-014 (#157) tracks shareable real-app
tutorial videos with cursor/click/key and step overlays. macOS capture capability
was inspected, but recording permissions, framing and output have not been tested;
no video exists yet. Videos and research remain outside git. Label API-driven
actions distinctly from native inputs in eventual demonstrations.

## Active sketch history milestone (2026-10-12)

SKT-020 (#156) now has a small accepted-draft journal. Undo cancels an inline editor,
then the active sketch tool, then traverses geometry. Redo requires Select. Public
product-team evidence resolving this precedence is recorded externally in
knowledge/specs/sketch-draft-history.md. Accepted geometry/constraint/dimension/name
edits participate; hover, rejected/no-op edits do not. Annotation drag is one entry;
Finish is still one document transaction and Cancel preserves committed work.

Build and real-app ledger workflow pass. Actual native Cmd+Z/Cmd+Shift+Z removes
and restores a snapped line with exact IDs; native label drag undoes in one step.
See docs/validation/sketch-history.md. Do not overlap same-bundle API runners with
native CUA checks: that confused process/window bindings during this checkpoint.
Private test app closed. No release or CI run.

Next bounded tutorial fix is automatic aligned dimension placement on the short
sloped edge for Activity1's100-mm dimension. Astra found current cursor bounding-box
logic chooses horizontal/vertical at the source placement. Core linear length and
tangent rendering exist; adjust selection policy then qualify the actual six-edge
outline with repeated Perpendicular/HV/Equal and constraint-stage captures. Document
+tab remains separate DOC-015; full tutorial remains blocked.

## First activity modeling checkpoint (2026-10-12)

The actual six-edge first-activity modeling sequence passes through the real app:
three Perpendicular pairs, two H/V relations, two Equal pairs, then500mm,45degrees
and aligned100mm; fully constrained closed profile, exact relations/glyph targets,
area, Finish, undo/redo and save/reopen. tests/bracket_activity1_api.py is registered
as bracket-activity1 with covers[]: this does not close TUT-003. Native isolated
D/pick/place/type100 proves the new automatic aligned policy on an API-seeded edge.
See docs/validation/bracket-activity1.md for receipts and limits.

The workflow exposed actual defects fixed in this slice: a1-mm projection could
quantize to zero and hide constraint badges at10% zoom; the default PlaneGCS
particular DogLeg step collapsed a free cap while adding45degrees. Badge scale now
samples100mm, and the solver uses its supported least-norm step without inventing
constraints. Existing independent residual/collapse checks remain. Five focused
sketch/solver/profile/extrusion/curve regressions pass. An adaptive fine grid now
covers the drawable across zoom/pan/resize; initial OCCT grid-center sign bug was
corrected and final captures independently reviewed by Astra. No CI/release ran.

Next DOC-015 (#151): genuine independent document tabs required by exact Activity1.
Plan: heap-owned CadDocument per stable tab because QUndoStack is noncopyable;
one shared viewport/palette rebound centrally, with all revision/draft/widget
caches invalidated even for equal-revision documents. Preserve accepted sketch
session+journal/camera/selection across switching. Guard raw inline input, partial
gestures and solid dialogs rather than silently discarding them. Deferred callbacks
must capture tabId/sessionId. Close/quit considers every dirty document and draft;
Save must require Finish/Cancel for uncommitted drafts. No reorder/detach scope.
Sol has staged the implementation and a coherent real-app workflow test. Root is
compiling and Astra is reviewing ownership/lifecycle before acceptance. After tabs, perform the
complete native first activity with constraint-stage visual comparison. Full
three-activity tutorial remains blocked by later modeling capabilities.

## Document tabs checkpoint (2026-10-12)

DOC-015 now has independent heap-owned documents and per-tab sketch draft/history,
selection/camera retention. Build passes; 85-command real-app workflow passes at
build-tutorial/evidence/document-tabs-ledger/report.json. Astra reviewed ownership,
cache resets, deferred callback guards and dirty lifecycle. Native plus, A/B/A,
Cmd+N, inactive dirty-close Cancel/Discard passed with authoritative state receipts.
The native pass caught canceled closure changing active tab; fixed and retested.
See docs/validation/document-tabs.md for scope and remaining Save-close qualification.
No private test app remains running. Next: complete native illustrated Activity1,
then construction and inferred circles for Activity2. Video capture remains #157,
planned but unqualified; no recording was made. No release or CI run.

## Native Activity1 interruption (2026-10-12)

After 6fabc42, root native input created plus tab, XY, six edges, three Perpendicular,
two H/V, two Equal, and500mm dimension. API navigation configured zoom/pan; all
modeling was native. Constraint AX toggles change checked appearance without the
clicked handler, so physical clicks were used. Repeated physical tools worked.
Native angle45 entry plus AX observation crashed PID10635. Matching report is
alloy-2026-10-12-125610.ips (older112454 is another PID); fault is Qt macOS
accessibilitySelectedChildren. Fullnative gate remains FAILED/incomplete.
FND-015 #158 tracks this; board In Progress. Sol staged stable sketch-tree items
for ordinary dimension changes; Astra reviewed source, native retest pending.
Construction core/UI work is also staged separately by Sol agents, unverified.
Current private app exited in crash. Evidence activity1-native-* records stages.

Native recovery retest passed: root reconstructed the previously observed native
500-mm authored draft as activity1-native-length500-recovered.alloy, opened and
edited through API, then used native D/picks/type45/Return/Escape and AX query,
followed by native100 aligned and AX query, Finish. Final DOF0 and500/45/100 with
12 visible glyph records; Astra inspected captures and accepted bounded recovery.
This is not an uninterrupted native Activity1 pass. Specific accessibility crash
path no longer reproduced; checkbox activation remains open under#158.
Construction core sketch_curves test passed; full application constructionflow
still pending. Private app quit. DOC-015#151 closed; snapshot27closed131open/158.

Construction real-app ledger now passes. Native physical cap-edge selection+X
converted exactly that line, retained its constraints, removed profile shading,
and selected dashed capture is underconstruction-native-selected. Accessible
constraint control activation now updates authoritative tool state; clicking off
returns Select. Right-click context menu did not appear onmac at edge or blank
viewport despite valid state; investigate native event routing before claiming
exact source contextmenu step. Private candidate quit; SolUI adding narrow input
diagnostics before next build. No full tutorial/recording pass claimed.

Context-menu probe resolved: native rightclick reached the viewport and opened the
menu; intervening CUA AX inspection dismissed it. Batched rightclick/Down/Return
converted the selected API-seeded line (`context-menu-sequence.json`). No behavior
fix needed. Lightweight viewport diagnostics now expose right/contextmenu counts,
last guard state and popup lifetime for future native automation. Final diagnostic build passed; no app remains active. NativeX + menu conversion + accessibleconstraint
activation accepted as bounded checks; exact two-edgeShift tutorial step remains.

## Persistent circle midpoint checkpoint (2026-10-12)

Core/UI now retain circle-center midpoint relations to stable normal/construction
line IDs. Triangle cue during firstcenter placement, atomic freshcenter/circle/
relation at circumference click; radiusfree. Build and targetedcoretest passed,
realapp sketch-midpoint ledger passed drivingparentchangescenter, conversion,
history/storage. Native C/center/circumference/Esc on API-openedA1leftend passed:
center(0,50), realrelation, DOF1. See docs/validation/sketch-midpoint.md. Astra source
and capture review accepted the bounded circle-center slice. No appactive. Next tangentcircle
inference and mixedregion profiles remain distinct CP03 blockers. ExternalLuna
research is indexed in circles-arcs-dimensions; knowledgevalidate/build passed.

## Uninterrupted native first-activity checkpoint (2026-10-12)

At370c735, native plus/XY/six-edge outline/3Perp/2HV/2Equal/500/45/100/Finish passed
in one app session with accessibility reads throughout. No recovery or crash.
View navigation and final save/inspection were API; modeling native. DOF0 and
exact dimensions, residual1.79e-11. Evidence activity1-uninterrupted-*; private
app quit. Astra review requested for FND-015 closure. Tangentcircle source is
staged and compiling, not yet accepted. See validation/bracket-activity1.md.

## Tangent circle and native accessibility checkpoint (2026-10-12)

Persistent circle/line tangent placement now passes build, targeted core test,
real-app two-end workflow and one native end-circle placement. Parent cap100→120
updates radii50→60 with exact retained IDs/history/storage. Square inference and
persistent glyph captures were accepted by Astra; see validation/sketch-tangent.md.
Private app quit. Mixed circle/line regions and sketch fillets still block CP03.
Astra also accepted uninterrupted native A1 on370c735 for bounded FND-015 closure.
Next: SKT-019 two corner fillets, shared radius100 and obsolete500 dimension removal,
then mixed-region extraction. Public corner pair identification is inferred from
source wording/topology; exact second small arc is not independently legible.

## Fillet staging and accessibility recurrence (2026-10-12)

SKT-019 source is staged: paired radius100 fillets with endpoint tangent relations,
explicit500 dimension removal, shared radius edits and one draft transaction.
Core and real-app sketch-fillet ledger pass. Native two corners/type100/Return
also passed on pre-sync binary. Astra found APIpreview100/editor10 mismatch;
field synchronization fixed and rebuilt. Postbuild bulkAPIsetup→Filletpreview→
firstAX query crashes; #158 reopened, #153 remains open. Matching PID51719 report
alloy-2026-10-12-134028.ips and LLDB PID52946 both accessibilitySelectedChildren+184;
not attributed to sync itself. No private app now alive. Fillet source/current evidence
were committed locally asdf0c363 while investigating the accessibility bridge.
Nextregion architecture experiment is external knowledge/specs/occt-planar-regions.md.


## Region integration and Qt patch experiment (2026-10-12)

Isolated region core commitsab15671/de02fef are integrated locally. OCCT splitting
and planar face extraction now return plural evaluated regions with authored
source/contact signatures, preserving legacy single-profile IDs. Astra reviewed
the bounded implementation; root build and actualA2 workflow checks are pending.
Sol is adding the real-app region test. Mixed-reference intersections without
retained contacts reject explicitly; this is not full SKT-009 closure.

Qt Cocoa plugin experiment is outside the checkout in ../Alloy-dependencies.
Exact6.11.2 source plus proposed upstream765434 ownership guards compiled and
loaded through QT_PLUGIN_PATH (not QT_QPA_PLATFORM_PLUGIN_PATH, which still picked
the system plugin). APIpreview100→AXeditor100→nativeReturn passed, with twoR100
arcs. NativeUndo/Redo plus AX then crashed PID56575 report135311; FND-015 and
SKT-019 remain open. No globalQt change, no shipped patch, no release/CI.
The private app CUA relaunched empty after this crash was identified and quit.
Human build-input stays untouched.


The second patched-runtime crash was QAccessibleTable::modelChange reached by
QComboBox::clear during Undo. Retained/incremental dimension rows remove that
reset. Final native preview100/editor100→Return→Undo→Redo with AX reads passes;
exact accepted/redo authored state confirmed, Astra independently reviewed.
See fillet-combo-* evidence and dependency recipe. FND-015 stays open for default
runtime/deployment qualification; no global dependency replacement. Region core,
legacy profiles and extrusion tests pass. Fillet core's only repeat-solve delta
was7.1e-15degrees; an actual save/reopen check found the same authored drift. Solver serialization
now preserves degree fields when both solved radian parameters are exactly
unchanged; no tolerance hides solver movement, and exact roundtrip checks remain. The first region API run passed geometry and all3 picks,
but stopped at a save/reopen expected-state comparison currently being corrected.


## Three-region workflow checkpoint (2026-10-12)

The final sketch-regions ledger passes actualA2 outline/endcircles/fillets, three
OCCTfaces with analyticareas, stableIDs radius100→90/cap100→120, eachprofilepick,
history and exactsave/reopen. See validation/sketch-regions.md. Solver now retains
authoreddegrees onlywhen both solvedradianparams exactlyequal theirseeds; this
removes observed7.1e-15degree reopen drift. Region/filletcore pass; olderprofile
and extrusion regressions passed. No fullSKT-009closure or generalintersectionclaim.
SKT-019#153closed bounded; snapshot28closed130open/158. FND-015 remains open for
defaultpatchedruntime deployment; its tested native crashpath passes.

NextSOL-016 underway: Solcore owns CadDocument/GeometryKernel and focusedextrusion
test; SolUI owns AlloyWindow/ReferenceView/ExtrudeControls. Exclusive legacyprofileId
or pluralprofileIds, sameearliersketch, contiguousselectedfaceunion→one50mmprism,
transactional rejectionofinvalid/missing/disjointinputs. UI Shiftselection and
PressPullQ shareauthoritative extrusion. Rootowns builds/app; Astra verifies.


## Multi-profile typed extrusion checkpoint (2026-10-12)

The connected three-region core, actual 123-command sketch-multiprofile ledger,
and native Q → three ordinary clicks → Distance 50 → OK pass. The actual solid
is valid and closed: one body/feature, depth 50 mm, volume 5,185,592.30050822 mm³.
Parent width edits, history and reopen retain IDs. Public staff research corrected
initial selection to collect ordinary clicks before depth entry.

See validation/multi-profile-extrusion.md. The saved rounded-bracket-50.alloy is
under build-tutorial/evidence. The private app quit; human build-input is untouched.
SOL-016 remains open for arrow dragging, five-profile Cut and native platform
toggling. Astra verified the bounded typed route but flagged gray preview, a tiny
arrow and missing canvas label. Those were corrected, rebuilt and independently
accepted in presspull-preview-final.json/.png; preview Cancel preserves exact saved
authored state. Arrow dragging remains unsupported. Sol core is
planning the planar face reference/frame foundation for CP04, read-only; no face
implementation is accepted. Root owns builds/app/GitHub. No release or CI ran.

## Active face-support worktree (2026-10-12)

The accepted typed extrusion milestone is committed/pushed as f97c9ba. Its
build-tutorial app and saved rounded-bracket-50.alloy are preserved. New CP04 work
is isolated in ../Alloy-worktrees/face-support on feat/face-supported-sketches.
Sol core owns geometry/document/frame helpers and core tests; Sol UI owns
AlloyWindow/ReferenceView. Root owns builds and application operation; Astra
independently reviews. See decisions/005-supported-sketch-frames.md for the bounded
positive extrusion end-cap contract. No face-support runtime evidence exists yet.


Private bundle launch now loads the patched Cocoa plugin from the candidate's
Contents/PlugIns/platforms directory with plugin-path environment variables unset.
Native fillet Return/Undo/Redo with accessibility reads passed and accepted/redo
sessions match exactly; see dependency recipe and qt-bundled evidence. This is a
local bundle qualification, not portable packaging. The private test app quit.

CP04 core and UI source is staged in the face-support worktree. The first build
passes after a test-local variable rename; focused face-support, legacy extrusion
and profile tests pass. Astra found and the UI agent fixed selection through an
occluding solid face. Real-app and native face workflow qualification remains
pending; no implementation acceptance or issue closure yet.


## Positive end-cap sketch checkpoint (2026-10-12)

CP04's bounded face foundation passed combined build, three focused core tests,
the 147-command sketch-face-support ledger and native face selection/Create Sketch/
rectangle/Finish. Astra independently accepted source, state and captures. Parent
50→60 depth changes transport the evaluated frame while local authored coordinates,
profile identity and support references remain stable. History and reopen pass.
See validation/face-supported-sketches.md. General faces, Boolean-modified supports
and projection remain open; DOC-006/012 and SKT-013 are not fully closed.

Next SKT-010: retained cap-edge projection so construction lines can use Parallel
and distance constraints against model geometry. Core proposal requires real OCCT
edge lineage through region union and prism, not indices or nearest-edge guesses.
Public evidence and design review precede implementation. No app remains running;
human build-input stays unchanged. No release or CI run.


SKT-010 implementation is now active in the same isolated worktree, after the
accepted face checkpoint. Current public documentation resolves automatic planar
face-edge availability; the tutorial must not gain an extra Project step. The
external spec linked-support-edge-projection.md records evidence and uncertainty.
Sol core owns retained OCCT boundary lineage, projection resolution/solver and
focused tests; Sol UI owns direct support-edge selection and reference display.
The bounded first slice supports locked straight-edge references and does not
claim complete projected-region/curve behavior. Root owns builds and integration;
Astra reviews independently. Board #46 is In Progress, full issue remains open.


## Linked straight cap-edge checkpoint (2026-10-12)

Implementation c8327ee is integrated. Build, focused core checks and 182-command
sketch-linked-projection ledger pass. Native Parallel control + two consecutive
construction-line/model-edge pairs + Escape/Finish pass (API-seeded guides).
Astra accepted actual reference colors/glyph pairs and authoritative constraints.
See validation/linked-cap-edges.md for evidence and strict retained-reference limits.
Projected endpoint dimensions are API-qualified; native endpoint/center inference
and full tutorial construction drawing remain next. Curved projection, repair and
general profile participation remain unsupported. SKT-010 stays open/In Progress.

Luna researched exact five-profile Cut into an indexed external record,
bracket-five-profile-cut.json (knowledge now9 records). Public screenshot uses
Distance -200mm, One Side, Cut and five red previews; that value is an observed
example, not a default, and Through All is not required by these source steps.
Further research is enumerating exact construction and hole-center inferences
before implementing the next tutorial slice. No private app remains active.


## Native projected endpoint checkpoint (2026-10-12)

Implementation f62ebc7 is integrated and Astra accepted. Native endpoint picking,
read-only refusal, circle-center Coincident snap and D-created 100mm horizontal
distance all pass. Source edit/history/storage are covered by focused core
regression; native Finish/save also passed. See validation/linked-cap-edges.md.
The private candidate is closed; human build-input remains untouched.

Luna completed the exact construction/five-center contract in external
knowledge/specs/bracket-construction-snap-path.md; knowledge now has 10 records.
Next active slice is lazy retained center points of actual circular cap edges,
using stable source curve lineage. Two anchored ends plus two independent
Parallel directions should fully constrain the connected guide chain. This is an
implementation/geometry inference, to be verified, not a claim about reference
internals. Sol core/UI implement in the isolated face-support worktree; root owns
builds/apps and Astra verifies. Then implement the tutorial's inline 50mm circle
entry, five attached centers and four Equal pairs, followed by five-profile Cut.
SKT-010 remains open/In Progress. No release/tag/CI is triggered.


## Anchored construction guide checkpoint (2026-10-12)

Implementation 62e3efd is integrated and Astra accepted. Actual circular cap-edge
centers support lazy retained snaps. Native two-line chain, X construction and
two guide/model-edge Parallel pairs reach zero DOF. Source-edit/history/storage
core regression passes. A Browser bulk-row-removal Qt accessibility crash on
Undo was fixed with persistent ID-keyed rows and hidden/disabled retired rows.
Post-fix native selected-row Undo/Redo plus AX inspection passes exact 3→2→3
feature restoration; save/reopen passes. Qt cached AX names remain inconsistent
after hide/reveal, tracked in #158; actual visible Browser rows are correct.
See validation/linked-cap-edges.md. Private candidate closed; human app unchanged.

Next active slice: inline circle diameter (C, center, type50, Enter), one accepted
transaction retaining circle + driving diameter. Sol core/UI implement in the
face-support worktree. Root then validates five centers endpoint/midpoint/bend/
midpoint/endpoint plus four Equal pairs. Cut follows: disconnected ephemeral
profile prisms, one explicit retained target body, signed Distance, native red
preview/arrow drag. Astra identified a required prefix-history fix for editing
the earlier supported sketch after Cut; recorded on #154. No new issue count,
release/tag or CI run.


## Five-circle and Cut work in progress (2026-10-12)

In the isolated face-support worktree, native C/center/type50/Return, four
endpoint/midpoint circle placements and four successive Equal pairs now reach
five radius25 circles at zero DOF. Finish, diameter50→60, history and exact
save/reopen pass; Astra independently reviewed the bounded source/state/captures.
Evidence is build-face/evidence/five-circle-native-constrained.json/.png and
native-five-equal-circles.alloy. This resumes an existing edited bracket fixture;
it is not an uninterrupted exact full-tutorial pass. Inline circle source is committed locally as9719c08 and cherry-picked into the
Cut candidate as7e1b69d; final runtime qualification remains pending.

A separate two-dimension insertion check exposed another Qt accessibility crash:
QAccessibleTable::modelChange dereferences an expired child interface during
QComboBox row insertion (report alloy-2026-10-12-151125.ips, PID97793). Earlier
Alloy row-retention workarounds do not qualify the general path. FND-015 #158
remains In Progress. A coherent private Qtbase build is underway outside the
checkout at ../Alloy-dependencies/qt-private-runtime, using pinned6.11.2, the
previous Cocoa ownership patch and a proposed narrow expired-ID cache fix.
Installed Qt and the human build-input app remain untouched. No private app is
currently running. Do not claim this new patch works until regression and actual
native editing checks pass.

In ../Alloy-worktrees/profile-cut, core81b9cc7 supports five disconnected
profiles as ephemeral cutters against one retained target, with one Cut feature
and one valid closed solid. Focused profile_cut checks pass diameter/depth edits,
prefix evaluation for earlier supported sketches, atomic refusal, history and
storage. This uses rectangular stock, not the full bracket UI. Sol is finishing
Cut controls/red previews/arrow dragging; Astra is reviewing independently.
Root owns integration/builds/apps and the next actual five-hole bracket test.
No release, tag or CI run has been triggered. Continue the queue after status
updates; they do not pause implementation.


Private Qt runtime now builds and passes the cache regression: unpatched
Table/Tree/List fail recreation; patched versions pass (5QtTest results,0failed).
All Qt frameworks/Cocoa/native style loaded exclusively from private install,
GPU AppleM4 remains active. Native firstØ60/freecenter and secondØ50/origin entry,
selected-row draftUndo/Redo, Finish and BrowserUndo/Redo pass; Astra accepted
bounded evidence. AX row-label inconsistencies remain under#158, so no general
accessibility closure. No globalQt changes.

Integrated profile-cut candidate7e1b69d builds against that runtime; focused
face_support/profile_cut checks pass. tests/workflows/check_profile_cut.py passes
actual bracket5toolpreview, one retained validclosedCut solid, earlier supported
sketchdiameter50→60, Cutdepthedit, exacthistory and reopen. NativeQ/fiveordinary
profileclicks pass. First arrowdrag found missing coordinate schema bounds;
Sol fixing that plus stale panel error (75c120b) before native retry. Private
app intentionally quit for rebuild; human app untouched. Full tutorial gate
remains open and is not replaced by this supplied-fixture checkpoint.


## Native five-hole Cut checkpoint (2026-10-12)

Integrated candidate749a667 includes inline circle entry, retained one-target
profile Cut, prefix-history support while editing earlier sketches, native
signed-distance arrow and corrected blue selected-profile/red cutter overlays.
Native Q, five ordinary region clicks, arrow drag to -200 mm, Cut and OK pass.
The real bracket is one valid closed solid with five holes. Final API workflow
passes earlier diameter50→60, Cut depth edit, exact undo/redo and save/reopen.
Hidden retained-target edit preview/cancel passes. The focused Qt widget modifier
test passes remove/re-add after distance entry; physical held Cmd-click is not
verified because the available CUA API does not expose held-modifier clicks.

Astra accepted native geometry/input, final edit-preview visuals and private Qt
runtime evidence. See validation/five-circle-cut.md and dependency recipe. Full
TUT-003 and#154 remain open for remaining exact/cumulative coverage, including
physical platform-modifier evidence. No issue was closed by this bounded slice.
The task count remains158, with28closed/130open at this checkpoint.

A separate Cut Candidate application is available for human testing in
../Alloy-worktrees/profile-cut/build-cut/alloy.app, bundleorg.alloycad.CutCandidate.
It uses the private coherent Qt runtime; preserve it while the user tests. The
saved native bracket is build-cut/evidence/native-five-hole-bracket.alloy. The
older build-input app remains untouched. Continue next with the exact illustrated
tutorial from a clean document, per docs/research/illustrated-bracket-gate.md;
use another private instance for automation. No release, tag or CI run.
