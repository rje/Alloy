# Enclosure mounting prefix candidate

`tests/enclosure_prefix_workflow_api.py` is a from-blank local-control runner for
TUT-020. It uses the independently authored, nominal v2 board assembly with
STEP SHA-256 `f695639a970642c3154f10758acccdafc9d71a8efdace512b1ce057e5805fe27`.
This is an adapted software-validation input, not the tutorial creator's file or
a certified physical enclosure fit. The XY assignment and placement are a
disclosed orientation/centering adaptation; the published source establishes
the stock dimensions and feature sequence, not these world coordinates.

The runner separately captures Z+90 insertion and the later
`[28, -42.5, 6]` placement after creating the sibling Case. It then authors a
centered 64 × 93 XY rectangle, Extrude30, a single grouped Fillet R5/R2.5,
closed Shell3.5, and named Length/Width/Top sections. The Top section follows
the retained outer top face with inward 5.5 offset and Flip. The inner floor
support drives a sketch with four linked circles projected from actual imported
PCB hole edges; each is offset inward 0.3 and the resulting four profiles
Join5 to the Case. The script checks the exact cavity and peg volume, source
IDs, hole centers transformed through the support frame, PCB/interior bounds,
an edited board placement with undo, authored save/reopen, and strict Case-only
STL/STEP export. Optional recording uses paced `API |` overlays from the blank
document.

The still-unopened case walls overlap some nominal connector/card envelopes;
the prefix does not assert whole-assembly collision freedom or production fit.
The source record and coordinate recipe are kept outside the repository in
`../Alloy-reference-records/knowledge/tutorials/advanced-audit-2026-10-13/`.

The focused real-app diagnostic passed after GEO-009 made the exact rounded
closed Shell inner floor selectable as sketch support. The final real-app pass
completed 110 successful local-control calls and 25 paced stages from a blank
document. The 145.2-second, 1120 × 902 movie has `API |` stage captions and
actual Alloy command labels; the recorder reported 726 frames and zero capture
failures. It is API-driven creation footage, not physical mouse/keyboard input
evidence. Section and sketch captures show the named analyses and four purple
linked projected circles with their offset profiles. The assembled view hides
most pegs under the board, so a focused capture from the reopened design hides
the reference, clears selection, and uses Top/170% to show all four peg tops.

Independent binary STL readback found two watertight, consistently oriented
surface components: a positive outer surface and negative cavity surface.
Their net mesh volume is about 64,467.487 mm³; the exact STEP export reports
64,474.016 mm³ and reads back as one valid solid. Both have the Case's
64 × 93 × 30 mm bounds. The runner proves a 0.25 mm shifted board edit leaves
0.25 mm minimum straight-side clearance, then undo restores authored intent
before save/reopen. The uncut connector/card envelopes remain outside the fit
claim.

Evidence is outside the repository under
`../Alloy-reference-records/deliverables/enclosure-prefix-2026-10-13/final/`:
`enclosure-prefix-receipt.json`, `enclosure-prefix-creation.mp4`, stage PNGs,
`four-pegs-top-case-only.png`, `enclosure-prefix.alloy`, and Case-only STL/STEP.
The initial failing run and subsequent diagnostic are retained alongside it.
Independent compatibility review remains pending.
