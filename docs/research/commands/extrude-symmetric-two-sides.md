# Symmetric and Two Sides solid Extrude

SOL-080/#618, researched 2026-10-19. Dated public source IDs R1–R5,
creator/provenance, original URLs, and the actionable specification are external:
`../Alloy-reference-records/knowledge/specs/sol080-symmetric-two-sided-extrude-2026-10-19.md`.
R1 documents Direction, Measurement and commit; R2/R3 document editable retained
extents; R4/R5 support panel grouping. No reference application was operated.

A selected supported closed profile creates a retained New Body with Distance
and Profile Plane. One Side remains the Alloy default (reference default unknown).
Symmetric defaults to Half Length (an Alloy choice): `distance` reaches equally
to both sides; Whole Length splits it equally. Two Sides uses `distance` along
+normal and `negativeDistance` along −normal, explicitly labeled in the native
panel. Both are positive magnitudes in active mm/in units. Half Length and Two Sides reaches must be at least 0.001 mm; Whole Length
input must be at least 0.001 mm total (0.0005 mm per side). Total span is at
most 100000 mm. Signed One Side and
its existing independent Offset start remain unchanged. Numeric input is the
bounded interaction for new modes; the existing one-side canvas arrow is hidden.

The local commands share the same draft, preview and commit authority:
`solid.extrude`, `solid.extrudeDialog`, `feature.editDialog`, and
`command.update/preview/commit/cancel`. Added fields are `direction` (`oneSide`,
`symmetric`, `twoSides`), `measurement` (`halfLength`, `wholeLength`, symmetric
only), and `negativeDistance` (Two Sides only). Retained parameters store
`direction`, `measurement`, `distanceMm`, and `negativeDistanceMm` as applicable;
profile/feature/body identities and the existing positive `extrude.endCap`
identity remain authoritative. Existing One Side files need no migration.

Preview draws evaluated geometry. Cancel discards the draft. OK creates one
history step; feature editing preserves identity; Undo/Redo and save/reopen use
ordinary document history. Invalid or unavailable profile/kernel results refuse
atomically. These are Alloy acceptance policies, not observed reference behavior.
Native Enter/Escape and focus policies remain inherited Alloy behavior; public
sources do not establish exact keyboard parity for these modes.

Verification: centered XY 40×30 rectangle; Half10 and Whole20 both −10..10,
24000 mm³; Half12 and Whole24 both −12..12, 28800 mm³; Two Sides8/14 gives
−8..14, 26400 mm³. Edit independent bounds, attach a positive-cap circle,
check identity/history/reopen, refuse zero/negative input, and independently
read exported STL. Inspect actual native controls and GPU captures. The dated
public images have unmatched theme/build/viewport/scale; compare field structure
only, with pixel parity unknown. Taper, thin wall, other start/extent/operation
combinations and additional profile families remain under SOL-010.
