# Compatibility and evidence

The goal is familiar functional behavior and presentation, implemented from
permitted public evidence. This document describes the tracking method. It is
not a claim of completed parity.

## Reference baseline

The exact reference CAD application release, platform, theme, display scale, and navigation preset
to target remain unresolved. Public help pages can change without a product
version marker, and screenshots can depict older interfaces. Preserve retrieval
dates and the reference's visible context; do not merge conflicting references
into an invented exact specification.

Initial research focuses on Design, Solid, Surface, and Sketch. See
[public interface research](research/interface-reference.md) for sourced findings
and uncertainty. We never operate reference CAD application to resolve gaps.

## Evidence vocabulary

| Label | Meaning |
| --- | --- |
| Documented | A public source explicitly states the behavior |
| Screenshot observation | A visible property of a public reference image |
| Inferred | An implementation hypothesis derived from incomplete evidence |
| Unknown | Permitted evidence has not established the behavior |
| Implemented | Alloy has code for the behavior, with limitations stated |
| Verified | A specified Alloy check ran and demonstrated the stated behavior |

Use source URLs and review dates for research. Use executable commands, fixtures,
results, and capture paths for Alloy verification. These labels can coexist:
something may be documented in reference CAD application and still absent from Alloy.

## Command records

As a command enters implementation, record:

- Public sources and their dates; product context, platform, and version if known.
- Command name, location, keyboard trigger, allowed selections, prerequisites,
  option defaults, units, numerical meaning, and output type.
- Preview, acceptance/cancellation, history, edit, undo, and documented errors.
- Agent command identifier and inspection fields.
- Representative workflow fixture, acceptance criteria, actual status, and gaps.

Start with a compact record alongside each implemented workflow. A large empty
catalog is less useful than complete records for the first real commands.

## Starter research inventory

All rows below are research/implementation targets, not completed features.

| Area | Public starting source | Needed acceptance evidence |
| --- | --- | --- |
| Workspace and context structure | Workspaces (reference `extra-078`) | Correct visible context and command availability |
| Sketch entry and finish | Edit sketches (reference `sketchmode`) | Plane/face input, contextual sketch UI, retained geometry |
| Timeline and editing | Timeline help (reference `timeline`) | Feature order, edit/recompute, supported reference persistence |
| Shortcuts and navigation | Official shortcuts (reference `extra-079`) | Documented keys/modifiers with declared platform/preset |
| Surface creation | Surface from sketch (reference `surface`) | Surface result, continuity/tolerance limits, later editing |
| Print export | Mesh export tools (reference `meshexport`) | Units, refinement, selected bodies, valid mesh output |
| Public command structure | API command inputs (reference `inputs`) | Reference for public concepts only; Alloy owns its control API |

Source review date: 2026-10-11. Extend this table when new workflow families are
actually researched. A URL alone is not a behavior specification.

## Visual and interaction review

Research command groups, Browser, Timeline, dialogs, selection/preselection,
numeric inputs, navigation, and contextual mode transitions. Typography, colors,
spacing, icons, and proportions require inspected screenshot evidence. Author
Alloy's assets independently. Do not bundle Autodesk branding or extracted assets.

Use comparable state, viewport dimensions, scale, theme, and framing for a
comparison where possible. State differences when matching is impossible.
Public screenshots usually cannot establish every hover, drag, focus, or error
state; document those gaps. A shell arranged into familiar regions does not
establish exact styling or functional equivalence.

## Functional verification

Use the real Alloy entrypoint and its control API. Assert meaningful outcomes:
dimension/constraint state, valid geometry, body count, bounding boxes and
volume/area with declared tolerances, correct undo/redo, saved authored history,
and exported data that a separate reader can validate.

Do not use a second automation-only document or a mocked kernel to report
workflow success. Direct UI checks remain useful for behavior the control API
cannot expose, but the control API should grow alongside that behavior.
