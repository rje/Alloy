# Alloy working rules

## Starting or resuming work

Read [README.md](README.md) and [docs/context.md](docs/context.md) for the project
entry points, accepted decisions, current state, and remaining work. Maintain
that continuation context when implementation or decisions materially change.
The project documents must be sufficient to resume without this chat history.

## Product contract

Alloy is a standalone, open source, modifiable parametric CAD application.
The long-term goal is functional feature-for-feature coverage of the researched
professional CAD workflows and an intimately familiar experience for existing users.
The intended visual and interaction target is the same feel as the referenced
product, including its styling, layouts, keyboard controls, and workflows.
Features that currently require cloud access may be deferred. All other feature
families remain on the long-term roadmap.

The initial release focuses on shape creation: constrained sketches, solids,
surfaces, editable feature history, common-format import, and export for 3D
printing. A useful first slice does not reduce the eventual replacement goal.

## Clean-room reference boundary

- Use publicly available documentation, screenshots, instructions, published
  API descriptions, community articles and original public tutorials/videos as
  reference evidence. Record creator/source, URL, retrieval date and relevant
  video timestamps; distinguish observed demonstration from inferred behavior.
- Do not run, operate, inspect, or capture proprietary reference applications. This
  includes licensed installations, trials, remote sessions, and web clients.
- Do not inspect their binaries, installed resources, private source, leaked
  material, private APIs, account-gated product data, or non-public information.
- Do not reverse engineer the product, intercept its traffic, or extract assets.
- Publicly documented API examples are reference material, not authorization to
  execute proprietary software or to obtain runtime observations from it.
- Publicly licensed open source foundations such as Qt and OCCT may be studied
  and reused under their licenses. This permission does not extend to proprietary software.
- Record the source URL and retrieval date for reference evidence. Keep reference
  product names, URLs containing them, and reference images outside the repository
  in `../Alloy-reference-records/`. Public documents use stable reference IDs;
  library documentation URLs remain public. Do not reintroduce removed names in
  files, paths, commit messages, issues, comments, or project fields. Distinguish
  documented behavior, screenshot observations, inference, and unknowns.
- Resolve ambiguity using additional permitted public evidence. If evidence is
  insufficient, record the uncertainty rather than inspecting the product or
  inventing an exact-match claim.

These rules supersede any earlier suggestion to verify behavior in proprietary software.

## Familiarity and visual fidelity

- Research the affected user workflow before implementing its controls and
  presentation. Gather permitted public examples independently; do not rely on
  the user to supply screenshots or discover missing reference behavior. Produce
  a compact actionable specification and verification steps from the evidence.
  Resolve material interaction questions before coding; record nonblocking
  uncertainties explicitly and keep the researched scope bounded.
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
- At coherent workflow milestones, perform a bounded independent compatibility
  review using [docs/compatibility.md](docs/compatibility.md). Use Astra when
  available, reviewing a candidate it did not implement; another reviewer checks
  its own code. Reuse real-app fixtures and evidence rather than sweeping every
  command after each commit.
- Keep one compact command/workflow record covering public inputs/defaults,
  selections, keys, previews, commit/cancel, history and edit behavior. Compare
  visual state when a matched public reference is available, and inspect actual
  geometry, authored save data and undo behavior separately.
- Report matched, mismatch or unknown for the specific evidence. Link concrete
  differences to bounded fix issues; do not turn incomplete references or one
  matching capture into a broad exact-match claim.

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
- macOS, Windows, and Linux are required product targets. Maintain native build,
  packaging, and runtime evidence for each. Plan downloadable GitHub releases at
  validated milestones; CI compilation alone cannot establish runtime or GPU support.
- Alloy's original source and documentation use the MIT license selected by the
  user. Third-party components retain their own licenses; evaluate solver and
  distribution choices against the project's licensing goals.
- Qt 6, OCCT, established sketch solvers, and Python automation are the current
  recommended foundation; dependency choices must be recorded and verified.
- Use mature libraries for geometry, constraints, and interchange where they
  fit. Keep custom infrastructure small and tied to a real workflow.
- Native build CI runs at version tags or explicit manual dispatch, not every
  commit/PR update. Publish qualified builds at coherent workflow milestones.
- The renderer destination is Alloy CAD presentation on an established RHI:
  native Metal on macOS, Vulkan on Windows/Linux. OCCT remains the geometry kernel;
  AIS/V3d/OpenGL is the current prototype. See decision 003 before migration.
- The 3D viewport must use GPU acceleration. Verify the actual graphics vendor,
  renderer, version, native framebuffer capture, and resize/display-scale behavior.
  A CPU reference grid or software rasterizer does not satisfy viewport acceptance.
- Preserve authored sketches/features separately from evaluated geometry and
  tessellated display data. Plan for persistent geometric references early.
- Prefer coherent, runnable increments with proportional verification.
- Use unit tests where they make sense and integration tests where pragmatic.
  Test the actual change's core contracts and material risks for reasonable
  assurance; do not pursue exhaustive hypothetical edge cases. Once appropriate
  checks pass, move on unless a meaningful change, failure or unresolved concern
  justifies further testing. Avoid repeated full-app runs for unchanged behavior.
- Preserve user work. Do not publish, push, or create external services as part
  of local project setup unless requested.
- GitHub work uses `gh` and documented APIs. Do not use browser/computer control
  for GitHub operations. Track implementation and cumulative validation in the
  repository issues; read the backlog instructions before pulling work.
- Keep accepted requirements, proposed designs, unresolved decisions, and
  implemented/verified capabilities clearly distinguished in project docs.
