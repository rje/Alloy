# Straw-support region contract

SKT-027 #174 uses public source `straw-support`, University of Warwick WMG
Outreach, retrieved2026-10-12. Public PDFs/video URLs, source hashes, rendered
pages and timestamps remain in the external
`knowledge/tutorials/warwick/straw-support-ledger.json`.

The illustrated sketch begins with a32×10 rectangle,7.5mm guide from its shorter
side midpoint,Ø15 circle at the guide end, then three adjacent3×10strips at the
opposite end. A bounded Alloy fixture places rectangle x0..32/y-10..0, circle
center(7.5,-5) and strips x23..26,26..29,29..32. Strip placement is inferred from
the screenshots and dimensioned adjacent widths.

The original public video170–192s explicitly uses Trim before Finish Sketch;
the PDF omits it. Internal top/bottom circle chords, the left boundary and guide
are removed. Only afterward does one selected full disk extrude20mm. The next
4mmJoin selects the bridge and the separated middle strip; outer and inner
prongs later extrude24/22mm. Trim is separately owned by SKT-029 #177; successive
result ownership/Join is SOL-020 #176. No artificial composite disk selection
will replace Trim on the raw crossing sketch.

The #174 implementation contract is actual atomic planar regions from OCCT.
Coincident authored straight segments coalesce as one geometric edge while
retaining all source UUIDs. Circle/line intersections use retained source sets
and ordered contact branches, with multiplicity encoded to avoid aliasing when
contacts collapse. Coordinates locate topology; numeric coordinates never define
region IDs. Coincident curved sources remain ambiguous and explicitly refused.
Legacy simple profiles and rounded bracket region IDs must remain unchanged.

A post-Trim geometric fixture has five regions: full disk, curved bridge and
three30mm²strips. Retain possible tiny dangling left fragments rather than infer
their deletion; they create no region. This fixture can verify the partitioner,
not the source Trim interaction. Upstream edits must preserve referenced IDs or
fail explicitly through the existing atomic document evaluation. Exact saved
intent, Undo/Redo and actual viewport highlight are separate checks.

Matched state means the five real bounded regions and illustrated selections.
Raw untrimmed geometry must expose its actual subregions, not the post-Trim disk.
Native Trim/tutorial acceptance waits for #177; broad sketch arrangements,
nested holes and current-product exact visual parity remain outside this slice.
