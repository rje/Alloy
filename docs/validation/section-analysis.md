# Section Analysis candidate

DOC-021/#262, 2026-10-13. Origin-plane inspection is retained separately from
modeling features. This is a bounded candidate, pending independent review.

## Contract and reference

The external `tutorial.ornament-pattern` record and registered
`vis-ornament-section-analysis-20261013` image establish Inspect → Section Analysis,
a vertical origin plane, zero offset/angles, From Component color, Show Hatching,
OK and later hide. The source registry records creator, URLs and retrieval date
2026-10-13 outside the repository. Its platform/build/scale are unknown. The
source does not establish persistence, Cancel, undo or half-space defaults.

Alloy exposes XY/XZ/YZ origin planes through a Cutting Plane selector, with no
inactive transform fields. Fixed positive world half-spaces are retained: +Z for
XY, +Y for XZ, +X for YZ. Camera motion never reverses the cut. From Component
inherits the solid presentation material; independently generated diagonal hatch
textures replace legacy polygon stipple, which was ineffective on the verified
Mac core-profile driver. The solid cavity remains open. General transforms,
custom colors and simultaneous sections remain in DOC-014.

`analyses` is an optional native-document array of exact records:
`{id,type:"section",name,plane,visible,hatch}`. IDs share the document-wide authored
namespace. Open rejects malformed records, invalid planes, duplicate IDs and more
than one visible section. Old documents without the field remain valid.

A replacement preview transiently hides the previous section. Cancel restores
it and adds no undo entry. Accept creates/edits an analysis and deactivates the
previous visible section in one undo entry. This single-active rule is an Alloy
policy. Analysis-only transactions reuse the current evaluated document and BRep
handles through the existing undo authority; authored modeling JSON is untouched.
Ordinary modeling commits retain analysis records. Analysis creates no Timeline
feature. Browser Analysis checkboxes show/hide; its context menu edits the record.

## Shared controls

- `inspect.sectionDialog`: native Inspect controls; optional analysisId edits.
- `inspect.section`: direct create/edit, plane required; optional analysisId,
  name and hatch. Creation defaults Show Hatching on.
- `command.update`, `command.preview`, `command.commit`, `command.cancel`:
  disposable authoritative sessions with revision checks.
- `analysis.inspect`, `analysis.visibility`: retained records and atomic visibility.
- GPU diagnostics expose sectionAnalysis and sectionClippedActors.

UI and API use these same commands. Panel fields mirror session inputs, including
API updates. A blank creation panel requires plane selection before OK. Escape
within the panel cancels. ReferenceView owns the clipping plane and reapplies it
to recreated solid actors; sketches and UI decorations are not clipped.

## Evidence

Focused core `section_analysis_test` passes exact evaluated-document pointer,
BRep handle and authored-feature preservation, one-entry replacement undo/redo,
invalid-plane/global-ID/multiple-active atomic refusals, ordinary commit retention,
and exact save/reopen. Existing files without analyses are exercised by its initial
model setup. Build uses C++20, Qt 6.11.2 and OCCT 7.9.3 on macOS arm64.

Actual application entrypoint `tests/section_analysis_api.py` passes 79 calls in
external `validation/section-analysis/api-attempt03/`: hollow 50×60 mm cylinder
with 5 mm inward Shell, hatched/plain/unclipped distinct native framebuffer views,
all three origin planes, preview/Cancel, retained replacement and undo/redo,
hide/show, exact modeling-feature/evaluated geometry summaries, save/reopen and
two design tabs. Equality excludes documentRevision bookkeeping only. GPU evidence
identifies Apple M4 / OpenGL 4.1 Metal hardware. Captures were inspected: cavity
is open, both walls and base are hatched. The body is selected blue in this run;
unselected material and corrected panel synchronization receive a focused capture.
Attempts 01/02 found and corrected an accidental empty-session mutation and a
fixture comparison of revision bookkeeping; their logs remain external.

Focused final actual-app captures in external `final-captures/` show the corrected
XZ selector, checked Show Hatching and unselected gray inherited body material;
hatched/plain states were inspected. This is a focused UI synchronization receipt,
not a replay of the API lifecycle.

Native Qt acceptance is pending. Attempts 01–04 lacked a reliable initial Qt
activation; subsequent pointer-popup attempts reached a visible list and real
sectionPlane focus but coordinate item clicks did not change its current selection.
The product combo was retained. The bounded harness now uses an actual combo click,
verifies popup visibility and focus, and sends Down/Return for selection; it has not
run. One initial OS foreground request and QApplication activation after API setup
are separate from native demonstration. No in-sequence focus repair is performed. The earlier isolated GPU feasibility proof remains distinct from
actual command-entrypoint evidence. Windows/Linux runtime and the full ornament
lesson are not verified by this candidate. Source screenshot comparison supports
bounded roles/readability, not matched viewport or pixel parity.
