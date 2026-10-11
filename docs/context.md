# Continuation context

Updated 2026-10-11. This is the project handoff for a fresh session. Read the root
[AGENTS.md](../AGENTS.md) first; its reference restrictions apply to all work.

## Decisions from the conversation

- The initial candidates were standalone open-source Maya and Houdini equivalents.
  reference CAD application was added and became the selected target. The earlier alternatives are
  preserved in [target-comparison.md](target-comparison.md).
- Alloy must eventually be a functional feature-for-feature reference CAD application replacement,
  open source and modifiable. It is a standalone app, not a Blender extension.
- Users should find the same feel: familiar visuals, layout, styling, command
  behavior, keyboard controls, navigation, and workflows. A small initial release
  does not reduce that long-term goal.
- Features that currently require cloud access may be deferred. This exception
  is feature-specific, not a reason to discard entire feature families.
- Initial emphasis is shape creation, sketches, solids/surfaces, common imports,
  and exports suitable for 3D printing.
- This is a clean-room implementation. Public docs, screenshots, and instructions
  are the reference material. Never run, inspect, or capture reference CAD application or use
  non-public information. Public API descriptions are references only.
- Agentic control must be built in from the start for operation, self-testing,
  state inspection, and captures. UI and agents use the same application commands.
- The user selected C++ and dropped Rust from the planned stack after reviewing
  the CAD ecosystem rationale. The scaffold uses C++20/Qt 6, with OCCT as the
  proposed CAD kernel and Python for the initial agent client.
- The user selected MIT for Alloy's original source and documentation.
  Third-party libraries retain their own licenses.
- The project lives at `~/src/cpp/Alloy`. Alloy is the working name. Astra was
  requested for the initial public-interface research and project foundation,
  and completed that assignment.

## Actual implementation

A native Qt Widgets shell exists in `src/`, built by CMake. It has command search,
Browser/Timeline/navigation layout controls, and an independently drawn reference
grid with named orientations, pan, and zoom. A private local control socket and
`tools/alloyctl.py` expose command discovery, execution, state, and capture.

OCCT 7.9.3 was discovered on the development host but is not linked/integrated.
There is no CAD geometry, sketch solver, editable document/history, persistence,
import, or export yet. Modeling controls are explicitly unavailable. Visual and
physical-input parity have not been established.

The native Debug build and CTest real-app smoke passed on Apple silicon macOS
with Qt 6.11.2. The smoke exercised real commands, state changes, errors, search
open/close, screenshots, and clean exit. See [development.md](development.md) for
exact commands, dependencies, evidence paths, and limitations; see
[agent-control.md](agent-control.md) for the protocol and client.

Local evidence is in ignored `build/evidence/`. Three public reference images
are present locally and ignored by Git; their source URLs, dates, hashes, and
dimensions are retained in the [provenance manifest](research/reference-images/provenance.json).
They are research references, never app assets. A fresh checkout can rebuild
Alloy evidence and re-fetch permitted references from the recorded URLs.

The user authorized a public GitHub repository/project and an issue backlog with
cumulative validation gates. The repository is [rje/Alloy](https://github.com/rje/Alloy)
and the public board is [Alloy](https://github.com/users/rje/projects/1).
Publication of the foundation and researched backlog is being prepared.
GitHub operations must use `gh` and APIs; the user explicitly rejected browser
computer control for GitHub. The current CLI token supports repositories/issues
but lacks the GitHub Projects scope. The board was created before that preference
was given; its auto-add workflow targets open issues from rje/Alloy.

## Where to resume

The proposed next increment is [roadmap milestone 1](roadmap.md): integrate OCCT
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

The exact reference CAD application reference release/platform/theme/navigation baseline, first
release OS coverage, sketch solver, and native document format
remain unresolved. They are recorded gaps, not forgotten decisions. Current
public-interface research is initial and does not yet specify every command,
default, error, visual state, or offline feature family.
