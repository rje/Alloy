# Alloy working rules

## Starting or resuming work

Read [README.md](README.md) and [docs/context.md](docs/context.md) for the project
entry points, accepted decisions, current state, and remaining work. Maintain
that continuation context when implementation or decisions materially change.
The project documents must be sufficient to resume without this chat history.

## Product contract

Alloy is a standalone, open source, modifiable replacement for the reference CAD application
(formerly the reference CAD application). The long-term goal is functional feature-for-feature
coverage and an experience intimately familiar to existing reference CAD application users.
The intended visual and interaction target is the same feel as the referenced
product, including its styling, layouts, keyboard controls, and workflows.
Features that currently require cloud access may be deferred. All other feature
families remain on the long-term roadmap.

The initial release focuses on shape creation: constrained sketches, solids,
surfaces, editable feature history, common-format import, and export for 3D
printing. A useful first slice does not reduce the eventual replacement goal.

## Clean-room reference boundary

- Use publicly available documentation, screenshots, instructions, published
  API descriptions, and public tutorial material as reference evidence.
- Do not run, operate, inspect, or capture the actual reference CAD application application. This
  includes licensed installations, trials, remote sessions, and web clients.
- Do not inspect reference CAD application binaries, installed resources, private source, leaked
  material, private APIs, account-gated product data, or non-public information.
- Do not reverse engineer the product, intercept its traffic, or extract assets.
- Publicly documented API examples are reference material, not authorization to
  execute reference CAD application or to obtain runtime observations from it.
- Publicly licensed open source foundations such as Qt and OCCT may be studied
  and reused under their licenses. This permission does not extend to reference CAD application.
- Record the source URL and retrieval date for reference evidence. Distinguish
  documented behavior, screenshot observations, inference, and unknowns.
- Resolve ambiguity using additional permitted public evidence. If evidence is
  insufficient, record the uncertainty rather than inspecting the product or
  inventing an exact-match claim.

These rules supersede any earlier suggestion to verify behavior in reference CAD application.

## Familiarity and visual fidelity

- Treat command names, option defaults, keyboard controls, navigation, selection
  behavior, layouts, spacing, typography, colors, and styling as compatibility
  requirements to research and track.
- Match documented workflows and public screenshots with our own implementation
  and independently authored assets. Do not extract or ship Autodesk assets.
- Keep visual references with provenance separate from application assets.
- Use matched viewport size, scale, theme, and comparable state for visual
  comparisons when evidence permits. State limitations of unmatched references.
- The application remains Alloy-branded.
- Do not claim a screenshot, mocked control, or empty shell implements CAD.

## Agent control is a foundation

- Build agentic control into Alloy from the first runnable application.
- Provide a documented local control API and a small command-line client for
  command discovery, execution, state inspection, and screenshot capture.
- UI and automation must invoke the same authoritative application commands and
  document state. Do not maintain a separate mock automation model.
- Use stable identifiers, structured results/errors, and explicit completion
  semantics so an agent can reliably observe the result of an action.
- Keep the control interface local to the machine by default.
- Extend control and inspection alongside each new modeling feature, including
  undo, save/reopen, parameters, geometry summaries, and viewport state.
- Prefer Alloy's control API for operation, verification, and captures. Direct
  desktop UI control is reserved for Alloy behavior the API does not expose.
- Self-test through the real application entrypoint. Keep build, API, geometry,
  screenshot, and artist-workflow evidence distinct.

## Implementation and scope

- C++20 is the selected application language. Rust is outside the planned stack.
- Alloy's original source and documentation use the MIT license selected by the
  user. Third-party components retain their own licenses; evaluate solver and
  distribution choices against the project's licensing goals.
- Qt 6, OCCT, established sketch solvers, and Python automation are the current
  recommended foundation; dependency choices must be recorded and verified.
- Use mature libraries for geometry, constraints, and interchange where they
  fit. Keep custom infrastructure small and tied to a real workflow.
- The 3D viewport must use GPU acceleration. Verify the actual graphics vendor,
  renderer, version, native framebuffer capture, and resize/display-scale behavior.
  A CPU reference grid or software rasterizer does not satisfy viewport acceptance.
- Preserve authored sketches/features separately from evaluated geometry and
  tessellated display data. Plan for persistent geometric references early.
- Prefer coherent, runnable increments with proportional verification.
- Preserve user work. Do not publish, push, or create external services as part
  of local project setup unless requested.
- GitHub work uses `gh` and documented APIs. Do not use browser/computer control
  for GitHub operations. Track implementation and cumulative validation in the
  repository issues; read the backlog instructions before pulling work.
- Keep accepted requirements, proposed designs, unresolved decisions, and
  implemented/verified capabilities clearly distinguished in project docs.
