# Enclosed iron handle recipe

Accepted 2026-10-15 for TUT-033/#468. Cumulative visual inspection rejected the
previous valid, watertight closing-cell candidate: its rear web was severed and
its connected mesh had Euler characteristic2 (genus0). Empty-space witnesses
alone had not established an enclosed handle.

The corrected independently authored recipe preserves every existing sketch
point/entity/feature identity. The upper profile starts at(100,20), passes
through(110,38) and ends at(135,60); its connector ends at(100,15). The lower
profile starts there, passes through the existing bottom contact(110,0) and
ends at(135,-8). A retained Coincident joins the connector and lower start.
The existing explicit upper/lower Extrude memberships remain separate, with
unchanged Two Sides60/65 and later bottomZ0. Actual finite cutter bounds confirm
X>=100 within tolerance, behind the handle ending atX90. Upper contact reaches
the exterior atX117.029 before heelX120; lower contact meets bottom atX110.
No kernel or tolerance changes and no parameter sweep were used.

The original curves remain in the initial reversible Trim/Remove trials. After
exact undo, the same sketch is edited to relocate its closing curves and add
the bottom line; the same Extrude gains that profile. Relocation is an explicit
additional engineering adaptation beyond the source's bottom-line addition.
The source begins with unprovided prepared sketches and gives no exact dimensions.

Actual application Boundary Fill yields one valid closed8-face solid of
131979.331mm³. Its closed, oriented, connected STL has6744vertices,20232edges and
13488triangles: Euler characteristic0/genus1, with0.00574% volume difference.
Rear-web material witnesses are IN and interior passage rays are clear. One
initial ray atZ10 lay on the original lens boundary; its reported intersections
were retained and diagnosed against the unchanged saved solid. Interior rays
were checked without another geometry attempt. Two collapsed-tip kernel
orientation warnings remain disclosed; the selected solid is valid.

A meaningful upper fit-point edit of+0.5mm changes volume while preserving face
identities. Exact undo/redo and save/reopen pass. Actual front and oblique GPU
captures show an enclosed lens opening, continuous rear web and smooth material.
Astra independently accepted the geometry, topology, application evidence and
runner adaptation. No additional test run was required after acceptance.

The public photographs and this model share a low tapered tip, raised crown/rear
and enclosed handle. Opening proportions, the concave rear and simplified shell
are different. Photographs are perspective references without calibrated
measurements; no exact visual/dimensional parity is claimed. Full cumulative
movie acceptance remains TUT-018.

External evidence:
`deliverables/surface-iron-continuation-2026-10-15/enclosed-handle-recipe-01/`
(contact/BRep/witnesses and provenance) and `enclosed-handle-api-01/`
(actual model, STL, captures, edit/history/reopen result and transcript).
