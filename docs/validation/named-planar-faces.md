# Named planar faces: candidate for SKT-030 #187

2026-10-12. Implementation candidate, not complete native tutorial acceptance.
Extrusion now offers retained base and straight-side planes alongside its prior
positive end cap. Names derive from authored generator/profile/boundary lineage.
Unique OCCT Modified descendants retain source frames through Combine, Cut and
Fillet; split or missing support cannot silently attach to a nearby face. Cut and
Combine also transport existing named solid edges, including new base rims.

Focused checks pass for six named cube faces, four outward side-sketch frames,
side extrusion, ambiguous split refusal, height edit and save/reopen. Existing
face-support test passes; five-hole Cut now verifies a retained cap rather than
its old intentionally unsupported policy. Independent Astra draft review found
and prompted duplicate-provenance rejection. Real-app native picking and the
Shell/Mirror chain remain required before issue closure.

New base/side faces currently expose no projected boundary links. General
Extrude Join still uses its stricter prior transfer policy, and negative-distance
end caps remain outside this increment. Broader repair UX remains #159.
