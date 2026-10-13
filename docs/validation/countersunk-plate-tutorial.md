# Countersunk plate full source gate — candidate (#249)

Status: **candidate for independent review**, 2026-10-13. One blank-document
shared-API creation run passed, with full movie, editable source, strict STL,
saved-BRep openings, parameter edits and exact history/reopen. Native ingredient
checks are separate; this movie does not demonstrate physical keyboard or
pointer parity. Public source ID `plate-countersink-pattern-20261013` is the
four-page exercise, pages 1–4, retrieved 2026-10-13. Creator, URL, archived
pages and source uncertainty remain outside Git in
`../Alloy-reference-records/knowledge/tutorials/next-after-outlet/`.

The first run used current main's Qt 6.11.2 / OCCT 7.9.3 app at
`build-tutorial-polish/alloy.app/Contents/MacOS/alloy`, SHA-256
`be843c95072719a68b8d2120f934ddf6f22a7109b4a2be76d5a4535339031b01`.
Runner worktree `../Alloy-worktrees/countersunk-plate-gate`, branch
`test/countersunk-plate-gate`, base main `63a51a7`, preparation commits
`0674f69` and `d1e5b69`. The app reported OCCT OpenGL on Apple M4. Its private
editor exited after the run. No broad unchanged suite was replayed.

`tests/countersunk_plate_tutorial_api.py` starts blank and records all source
creation steps in one 21.1-second, 211-frame movie with command captions. It
reports zero capture failures, 73 delayed ticks and 137 repeated frames; those
timing counts are disclosed rather than treated as distinct user inputs. It
opens the actual Change Parameters dialog before adding Length75, Width50 and
Thickness10 mm, captures the readable table, then closes it before modeling.
The rough XY Center Rectangle acquires two named driving dimensions; the
center relationship and zero DOF persist. Extrude references the retained
Thickness ID. Four vertical R5 Fillets yield the rounded stock. A standalone
Point on the actual top cap is dimensioned 10 mm from its retained left and
upper straight rims. Hole uses that point and target with From Sketch,
Countersink Ø4/Ø10, 90°, All; its shared panel previews the 3 mm sink before
commit. Pattern selects that Hole and two retained top rims, then previews and
commits Features, Spacing, 3×3, signed 27.5/15 mm, One Direction, Adjust.
Source Save follows the final overview in the same movie.

The embedded dimensioned drawing on page 1 stands in for the separately named
companion drawing. Public pages do not identify exact Pattern edge picks or
their order/sign; Alloy selected two stable upper straight rims and adapted
their retained oriented signs to +X/−Y. In this run both authored spacing
values are negative because both picked edge roles point opposite the desired
grid directions. All nine occurrences are present; the screenshot's enabled
Suppression control does not show an omitted instance. The source ends at
Save. STL export, edits, undo/redo and reopen below are Alloy integrity checks.

Evaluated and saved state retain one valid closed solid, 75×50×10 mm bounds,
four R5 corners, true point identity, nine centers at x
`−27.5,0,+27.5` and y `+15,0,−15`, and volume
`36500−353π = 35391.0177933 mm³`. An independent executable reopens the saved
document and classifies actual OCCT material at all nine centers: top Ø10
entries, the radius-3.5 mm frustum section at z8.5, bottom Ø4 exits, and
continuous bores at interior heights. It also checks one solid and the exact
envelope; it does not rely on raw face counts. The accepted #248 padded
underside test separately distinguishes per-occurrence Adjust from a copied
finite seed cutter; this uniform plate alone cannot establish that behavior.

The millimeter binary STL has 7,760 triangles, one connected component,
strict two-way manifold edges, consistent facet normals and positive winding.
Independent readback bounds are `[−37.5,−25,0]` to `[37.5,25,10]` mm; mesh
volume `35393.6036740 mm³` differs from analytic BRep volume by about 0.0073%.
The native model SHA-256 is
`2b41806b5bc565acb734be03e9ebd5a4cde1f932f62e31fec8ac468be4f898e9`;
the STL SHA-256 is
`c6e27bb157ce9e25db493a07ce33ff741f2e2ead086330006e59be0cc557a1e8`.

After source Save, Length75→85 moves the retained seed to
`(−32.5,15,10)`, shifts the full nine-center grid and yields
`40391.0177933 mm³`. Exact undo/redo restores both authored snapshots.
Thickness10→12 moves the entry to z12, keeps All through the plate, shifts
the full grid in z and yields `42621.9027549 mm³`; its undo/redo is also
exact. Reopen restores the original saved source dimensions, identities and
geometry. The export does not change authored history.

External artifact folder:
`../Alloy-reference-records/deliverables/countersunk-plate-2026-10-13/attempt-01/`.
It contains `source-creation.mp4` with recording metadata/event log,
`countersunk-plate.alloy`, `countersunk-plate-mm.stl`,
`countersunk-plate-tutorial-api.json`, the actual parameter/Hole/Pattern panel
captures, source-stage viewport captures and app log. The report is preserved
even if a later run stops. The movie and final viewport were inspected: the
table and controls are readable, the red Hole preview is visible, and the
final nine countersinks are smooth and distinct. Matching to the public page
views remains bounded by different viewport/layout and unknown source picks;
pixel-exact or general Hole/Pattern parity is not claimed.
