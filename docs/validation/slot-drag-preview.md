# Live solved slot drag preview candidate

2026-10-13, #258, isolated `fix/slot-drag-preview` from 6ce0d5b. This is a bounded
presentation follow-up to independently accepted Center Point Slot #251. The
public command/source contract remains `sketch-slots-splines-20261013`; exact
historical reference drag presentation is unknown. No new source defaults,
width locks or generic interaction framework were added.

`sketch.slotDrag` supplies `begin`, `move`, `end` and `cancel`, with a retained
guide endpoint `pointId` at begin and canonical `uMm`, `vMm` during movement.
Coordinates use the normal ±100000 mm contract. Each move seeds the existing
`SketchGeometry::movePoint` from the gesture's unchanged base draft, then uses
the same exact kernel/profile evaluation as committed sketch edits. Session
inspection exposes `pendingSlotDrag.previewDraft`, `previewGeometry`, solver
diagnostics and `solveElapsedMs`; authored draft/history remain unchanged.
Actual GPU capsule, construction guide, profile fill and decorations replace
the previous displayed shape while the pointer token survives that refresh.
Release publishes one shared sketch history step. Small no-motion clicks only
select. Rough slot width remains free until dimensioned, so a solved drag may
also change its unconstrained radius; no presentation-only geometry is invented.

Escape, tool changes, history traversal and session teardown discard both
session preview and native token and restore authoritative actors. A different
published edit cancels the disposable gesture. Stale-base edits, solver/kernel
refusals, schema rejection and native out-of-viewport conversion refusal also
cancel and restore. Late release/end cannot publish discarded preview geometry.
Selection/focus are produced by normal application commands, not fixture repair.

The focused native Qt fixture passed 3/3 in 2931 ms. Initial setup uses the shared
rough-slot API and one harness activation. Actual Qt press/two consecutive
moves/release show different solved temporary capsules; intermediate authored
draft and history compare exactly to the original. The evaluated profile matches
independent capsule area `2*r*guideLength + pi*r*r`. The final intermediate
radius is 8.90956 mm, guide length 72.6602 mm and area 1544.12 mm². Two solve/evaluate
samples cost 1.695/1.906 ms; the fixture bounds each below 500 ms. These timings
cover the scoped small sketch, not a broad large-model responsiveness claim.

The same fixture verifies one history entry on release, retained slot group
identities, no-motion selection, actual Escape/late release, shared tool change,
Undo/late release and Redo, delivered out-of-viewport cursor refusal, shared API
out-of-range refusal/inactive late end, and session Cancel/late release. History,
tool and session actions use authoritative shared commands; these are not a
claim of physical OS input or a replay of the unchanged source creation chain.
No separate duplicative full API workflow was run.

The final actual capture was inspected: the entire smooth solved capsule and
warm closed-profile fill fit within the viewport, with distinct construction
guide, retained tangent/equality marks and readable palette. Palette point rows
continue to show the accepted authored draft (35,5), not transient preview
coordinates; evaluated preview data is exposed separately in session inspection.
GPU framebuffer
state reports Apple M4, OpenGL 4.1 Metal, GLSL 4.10 and resolution ratio 2. Native
Windows/Linux behavior and exact historical styling remain unverified.

External evidence under `validation/slot-drag-preview` in the reference records:
`native-attempt-04.log`, `solved-intermediate-slot.png`, `intermediate-state.json`.
Preserved earlier attempts: 01 assumed a fixed rough width; 02 assumed a
screen-rounded origin cursor necessarily collapses geometry; 03 exposed the real
out-of-viewport conversion exception, now cancelled through shared authority.
The final validation editor is closed.

App SHA256:
`a386b69def76e7b3632405c96a5b446ebb1d705f67201fda42886c96e956454f`.
Native fixture SHA256:
`9ab0ad64f4f2ba24810c844783f1d3bb58837d6027abf03f9d372d06d154d295`.
Independent source/native/capture acceptance remains pending.
