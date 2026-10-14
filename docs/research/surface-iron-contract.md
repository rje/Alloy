# Surface iron source contract

RES-018 qualifies the public `surface-iron` tutorial for implementation. Its
creator begins with **prepared sketches**; their dimensions and construction
are absent from the demonstrated sequence. The independently authored Alloy
fixture must disclose every chosen size, sketch plane and point position. The
external `knowledge/tutorials/surface-iron-2026-10-14/source-contract.md`
contains dated transcript links, image provenance, a step ledger and a
parameterized prepared-sketch recipe. The two original photos stay outside Git.

Use a declared local tip→heel / left→right / sole→crown frame for the fixture.
Author a sparse mirrored plan silhouette, side crown/sole curves, a back arc,
a mirrored lower two-entity section and one intersecting guide rail. The
creator states that one upper sketch uses four spline fit points, but does not
specify their coordinates. Author the XZ handle ellipse and paired splines,
then a separate XZ back/bottom sketch with two splines and a notch line. Keep
scale parameters free until a real geometric fixture is built; none is a
source-documented dimension. Project/Coincident contact at section and rail
intersections must be exact, not nearest-screen selection.

The demonstrated feature order is:

1. Exterior open surface Loft: back arc → mirrored lower chain, plus guide rail.
2. Handle ellipse and paired spline sketch; Trim both inner regions from the
   exterior sheet; bridge Loft across spline → ellipse → opposite spline.
3. Back/bottom sketch; Two Sides surface Extrude beyond adjacent sheets.
4. Trial surface-against-surface Trims and optional history-preserving Remove;
   revert those trial stages.
5. Edit the back/bottom sketch to add a bottom line and edit the *same*
   Extrude to include it. Boundary Fill all sheets, select the one bounded
   cell and choose Remove Tools, yielding one valid closed solid with handle
   opening.

The later discussion of more guide rails, analysis and fillets is advice, not
an additional required modeled stage. The public side/bottom photos are
perspective silhouettes without metric dimensions or a calibrated camera;
comparison may assess broad proportions but cannot establish pixel-exact CAD
parity. The full TUT-018 gate must label the prepared sketches as an Alloy
adaptation, show a creation movie for its actual authored recipe, prove both
reverted and final history branches, edit/reopen and check solid/mesh output.

Source-sized prerequisites are SRF-012 (ordered open-section rail and bridge
Lofts), SRF-013 (retained sheet Trim by sketch/sheet) and SRF-014 (Two Sides
surface Extrude from open chains). SRF-011 provides cell selection/Remove
Tools, DOC-024 the local reference canvas, and HIS-007 the trial Remove. The
broad SRF-001/002/005 issues retain Revolve, continuity and Extend/Untrim
scope outside this lesson; SRF-003 Patch and SRF-004 Offset are not required
for the demonstrated Trim. TUT-017 remains the prior queue milestone. This
research slice has no Alloy modeling or native-input acceptance.
