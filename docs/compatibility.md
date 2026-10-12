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

Use stable source IDs and review dates in public research records; retain original
URLs, images and reference names in `../Alloy-reference-records/`, relative to the
repository root. Official documentation is useful, but original community
articles, public video tutorials and creator demonstrations are also permitted.
For videos retain creator, URL, retrieval date and relevant timestamps. A creator's
public demonstration supports the behavior actually shown; unshown defaults,
keyboard/focus behavior and platform differences remain inference or unknown.
Use executable commands, fixtures,
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
The current [sketch controls record](research/commands/sketch-controls.md) is the
first bounded example, including known helper controls and remaining questions.

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

### Sketch controls awaiting a compatibility pass

User review on 2026-10-11 accepted continued functional implementation while
the manual sketch controls are refined over time. The initial Start X/Y and
End X/Y palette fields are verified Alloy coordinate-entry helpers, not accepted
reference interaction parity. Track numeric entry focus/acceptance, live drawing
inputs, dimension placement/editing, shortcuts, selection and palette organization
against permitted public instructions/screenshots as those commands develop.
FND-001 owns the dated baseline; sketch feature issues and the physical/visual
product gates retain their compatibility obligations. This follow-up does not
block the next constraints/dimensions increment or reduce the eventual target.

## Functional verification

Use the real Alloy entrypoint and its control API. Assert meaningful outcomes:
dimension/constraint state, valid geometry, body count, bounding boxes and
volume/area with declared tolerances, correct undo/redo, saved authored history,
and exported data that a separate reader can validate.

Do not use a second automation-only document or a mocked kernel to report
workflow success. Direct UI checks remain useful for behavior the control API
cannot expose, but the control API should grow alongside that behavior.

## Milestone review and correction

Run one bounded compatibility review when a coherent artist workflow becomes
usable or materially changes. This is not a full-repository sweep after each
commit. Prefer Astra as the independent verifier when it did not implement the
candidate; another reviewer checks Astra-authored code. Reuse the candidate's
existing representative fixture, captures and executed checks. A new capture or
physical check is justified by an unresolved visible/input question.

1. Choose the affected command/workflow and a permitted public reference state.
   Record the source IDs, dates, relevant tutorial timestamps and known platform,
   release, theme and scale. Preserve disagreements and missing information.
2. Review inputs/defaults, selection, keyboard/focus, preview, commit/cancel,
   history and subsequent editing. Use the deterministic Alloy API fixture for
   reproducible state; use direct desktop input for the physical interaction the
   API cannot demonstrate.
3. Compare a capture with matching state, viewport size, scale, theme and framing
   when feasible. Inspect geometry, analytic dimensions/area/volume, stable
   authored save data and undo independently. Public numeric examples can define
   output expectations; analytic checks do not establish reference-runtime parity.
4. Add a short result to the command record: **matched**, **mismatch** or
   **unknown**, precise supporting evidence, and a linked bounded fix issue for
   each actionable difference. Recheck the affected behavior after its fix, then
   continue implementation.

The compact record needs only reference context, intended workflow, one candidate
fixture/evidence link, a small results table and follow-up issue links. Keep
functional output and visual/input findings separate. Unknown evidence is not a
license to invent defaults or inspect the actual proprietary application, and a
single matching state cannot certify all commands or overall product parity.
