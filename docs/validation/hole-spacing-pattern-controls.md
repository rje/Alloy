# Native Hole Spacing Pattern controls (#248)

Status: candidate, 2026-10-13; independent acceptance and integration separate.
Public basis: `plate-countersink-pattern-20261013`, page 5, retrieved 2026-10-13.
The panel shows Features, one object, two directions, Spacing, quantities 3/3,
adjacent distances 27.5/15 mm, One Direction and Adjust. Public evidence and
unknown selection order/signs/defaults remain outside the repository in
`knowledge/tutorials/next-after-outlet/independent-countersink-contract.md`.
This is a bounded #248 increment, not the complete tutorial gate.

Create → Rectangular Pattern selects a real retained Hole from the Timeline and
two current straight target edges through native viewport picks. Hole selection
chooses the supported Spacing/Adjust policy; disabled mode rows display actual
session state. Other mode families remain unsupported here. This automatic
seed-specific policy and the initial 1 mm spacing are Alloy choices, not observed
source transitions/defaults. Additive Extrude Join retains the existing implicit
Extent/Identical schema and evaluation. Hole rows author signed `spacingMm` and
`direction:"one"`; additive rows continue to author `extentMm`. Quantity and
numeric fields, Flip, Preview, Cancel, OK and Timeline edit all use the existing
authoritative Pattern session and `CadDocument::patternCandidate`.

Hole spacing follows retained provenance `startMm → endMm`; negative spacing
reverses this direction. Viewport arrows use that same evaluated direction and
signed total span `spacingMm × (quantity−1)`. The native fixture picks current
named rim identities and explicitly uses −27.5/+15 mm to produce +X/−Y. Those
signs/edges are an Alloy adaptation because exact source direction selections
are unknown. Preview displays the actual evaluated final stock with smooth
cavities and disposable direction cues, without coincident removed-solid overlay.
Incomplete or failed input disables OK and preserves the authored document.
Callbacks guard the immutable session ID without inserting canceled-session keys.

## Evidence

Shared worktree: `Alloy-worktrees/hole-spacing-pattern`.
Native artifacts: `build-pattern/evidence/pattern-controls/`.

- Combined application and native test compile. Qt's initial harness activation
  uses the established deprecated `setActiveWindow` helper once; no build failure.
- `hole_spacing_pattern_controls` passes 1/1 in 6.32 s. API fixture setup creates
  centered 75×50×10/R5 stock, an actual face-supported standalone Point with two
  retained 10 mm point-to-edge dimensions, then the retained 4/10/90 All Hole.
  Native events click the actual Create menu and Hole Timeline button, pick both
  named rim edges in the viewport, type quantities/spacings, Flip Axis 1 twice,
  inspect the nine-instance preview, Cancel with exact authored equality, repeat
  and OK, then reopen the actual Pattern Timeline button and edit spacing 27→27.5.
  The Pattern UUID persists; source-grid final volume is `36500−353π` mm³.
- Initial harness activation occurs once after API setup, before native actions,
  and asserts viewport focus. Actual numeric QLineEdit focus is asserted before
  typing. No focus or selection repair occurs between demonstrated steps. These
  are native Qt injected events, not physical OS input.
- Inspected `native-pattern-preview.png` and `native-pattern-final.png`: all nine
  enlarged smooth countersink entries are countable; readable Features/one Hole,
  Spacing, quantities 3/3, signed distances, One Direction, Adjust, Cancel/OK and
  axis cues. Final stock has smooth cones and bores without preview depth fighting.
  The unmatched camera/theme/size do not establish broad exact visual equivalence.
- Backend core/API records separately cover true per-current-target Adjust,
  padded-target All termination, exact centers, retained identities, history and
  save/reopen; the native receipt does not substitute for those checks.

Missing/ambiguous references refuse atomically. Additional Pattern families and
source-order full workflow remain parent scope; no exhaustive replay claimed.
