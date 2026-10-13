# Exact circular self-closing Shell candidate

2026-10-13, #266. The old source80/5/R50 Shell passed BRep checks but failed
untouched-input Boolean self-interference checks. Its inward seam had about
0.7657 mm curve/surface deviation and its degenerate upper edge was a finite
ring. Seam reconstruction alone failed endpoint consistency; no tolerance was
inflated. The extended kernel offset basis also lost exact circular axisymmetry.

The bounded branch admits only an unmodified exact circular full360 Revolve
with two actual faces, one unique generated radial disk, a circular disk edge
centered on the real axis, and an exact generating circle perpendicular to it.
The positive radial inward circle has two axis roots strictly inside the original
solid and above the selected disk. The original solid classifier determines the
inward disk side; face orientation alone gave the opposite sign for this fixture.
Unsupported classification falls through to ordinary Shell. Exact cavity revolve
and non-destructive Cut must pass BRep and Boolean-input validity, remain one
closed solid, and preserve the actual selected disk through Cut history. That
support retains its UUID with role `shell-closed-cap`.

This is a disclosed Alloy closed-cap policy for a self-closing inward meridian.
Public REF-ORNAMENT-01 establishes selected bottom disk and Inside5; its section
suggests a continuous floor, but exact post-Shell source topology is unknown.
No annular-opening or exact source-parity claim is made. Ordinary open Shell
paths and their regression fixture remain unchanged.

Focused source80/R50 and edited85/R55 core checks explicitly assert
`exactCircularClosedCap`, Boolean validity, independent disk-integral volumes
37836.66233494 and39907.16395561 mm3, regular normal wall5, cap lineage, atomic
missing/ambiguous/partial refusal and exact undo/redo/save/reopen. Both focused
core tests pass; the real application build passes.

Actual shared API source prefix now completes the exact fitted strip60 Intersect
(volume2278.110946607 mm3), replacing the earlier empty-result refusal. Real app
source-to-control dimension edit, undo/redo and exact reopen pass. Focused GPU
section captures show the whole base/cavity after zooming out; sketch overlays
and origin axes are hidden for boundary readability. These are API operations,
not a new native pointer test. Source reference viewport/scale is unmatched.

Evidence stays external under `validation/ornament-feasibility/`:
`exact-cavity-{source,control}`, `shell-cap-api-01`, `shell-cap-integrity-01`,
`shell-cap-captures-01`. The disposable cavity Boolean tool changes only its root
Free ownership flag after Cut; preserved pre/post BRep differs only in that flag.
The authored original outer shape remains unchanged. No approximation or
parameter/tolerance relaxation is used. Full eight-rib Join/tutorial acceptance
remains #265/#264; this receipt accepts only its Shell prerequisite pending
independent review.

## Independent acceptance

Astra independently reviewed candidate `c715ed8`, including bounded admission,
actual revolution axis/frame, exact circle/axis roots, non-destructive Cut and
selected disk lineage. Unsupported radius/root classification was corrected to
fall through before this review completed. The original input BRep byte check,
source/control oracle volumes and Boolean validity are asserted by the focused
core fixture; the existing ordinary Shell fixture also passes (final 2/2 log).

Independent review of actual API evidence confirms the source prefix now reaches
one valid Intersect member, and source-to-85/R55 edit/history/reopen preserves
the cap UUID and expected volumes. The disposable cavity's preserved before/after
BRep diff changes only its root Free flag. Corrected neutral source/control GPU
captures show the entire flat bottom cap, inner lower cusp and continuous
hatched wall without origin or sketch obstruction. Hardware state identifies
Apple M4. These API/capture checks are distinct from historical native controls;
no new native input replay is claimed or needed for unchanged controls.

Accepted as the disclosed bounded exact circular closed-cap Shell policy and
prerequisite. Its section is qualitatively consistent with the archived source's
apparent lower floor; unmatched view/scale and obscured source boundary leave
exact topology compatibility unknown. This does not accept the eight-rib Join,
final bore/STL or full tutorial. No additional test replay was performed.
