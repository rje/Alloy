# Ornament apex fixture correction

2026-10-13, #264 cumulative visual gate. Independent public-image review found
the source ribs meeting a pointed closed upper tip, while cumulative-04 has an
open crown at z70.483 mm rather than the authored80 mm apex. Kernel validity,
Join, bore and printing checks cannot accept that visual mismatch.

The source permits freely placed fit points; exact coordinates/tangents are
unknown. Its strip crosses the rotation axis near the upper tip. Change only
the fixture's upper fit point from(3,89) to(17,89), retaining the other three
points, signed exact Offset−5, explicit snapped end connectors, Intersect60,
80/5/R50 profile, Shell5, quantity8 and hub/bore dimensions. This is a disclosed
placement choice, not a source-coordinate match or mathematically minimal shift.
No geometry policy, cap patch or model extension is used.

Exact OCCT curve intersections at z80 put the original source/offset−5 at
x−6.21327/−12.22448, excluding the axis. The new source is x+0.481495; offsets
−5/−4 are x−7.147474/−5.717572, keeping the axis strictly between them with
minimum0.4815 mm margin. Actual retained closed profiles classify the apex
strictly inside. Both60mm Intersect seeds are one BRep/Boolean-valid solid and
reach z80.0000001; volumes2424.939197906/1975.291874990 mm3.

The proof updates the constrained duplicate connector endpoint with the moved
fit endpoint, matching fresh runner authoring. Otherwise a free coincident
solver averages the changed endpoint with its old connector coordinate; that
first incomplete proof is not a geometry pass. All source distances and the
exact authored evaluator remain unchanged.

The original−5 seed also passes retained Pattern8 and actual hub Join: one
BRep/Boolean-valid body, zmax80.0000001, with axis material at z79.9 inside.
Headless Join takes40.406s; this is not a full application timing claim.

The runner now checks the80mm seed and final body height; the saved-model oracle
also requires axis material at z79.9 before and after the bore Cut. These checks
reject the prior open crown. Python syntax and verifier build pass. Final actual
Pattern/Join visual, original/edit/reopen and strict-STL gates remain with root's
next cumulative run; no GUI was launched for this correction.

Public reference images/provenance stay external in
`knowledge/tutorials/after-slots-splines/independent-contract.md` and `images/`.
Headless evidence: `validation/ornament-feasibility/apex-strip-roots*.json`,
`apex-retained-proof.cpp/.json`, `apex-seed-{5,4}.alloy`,
`apex-tip-join-proof.cpp/.json` and `apex-tip-joined.brep`.
