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
