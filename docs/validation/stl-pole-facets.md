# Serialized spherical-pole STL facets (PRT-007)

2026-10-13 isolated correction. The saved complete die is a valid closed solid,
but its original binary millimeter export had 53,417 facets including three
zero-area facets with identical serialized vertices: indices 13,786, 22,671 and
51,844. Removing just those three in a diagnostic copy passed the unchanged
independent normals/manifold/winding checker, with one connected component and
20 mm bounds. This establishes the defective serialization, without claiming
that writer vertex merging was its cause.

The exporter now inspects actual binary facet records in its private staging
file, drops exact duplicate-vertex or exact zero-area records, preserves retained
records, updates the little-endian header and only then atomically publishes the
file. Direct duplicate-coordinate comparison avoids a cross-product-only test's
floating-point cancellation residue. There is no new area tolerance or CAD
healing. Existing detached pre-writer filtering and ASCII serialization remain.
Returned triangles and bytes reflect the published file; collapsed removals
include both stages, with `serializedCollapsedTrianglesRemoved` reported separately.

Non-GUI evaluation and export of the original saved complete die at its original
0.05 mm / 8 degree settings produces 53,414 facets and 2,670,784 bytes, with
three serialization removals plus 26 prior mesh removals. Strict millimeter and
inch readbacks pass finite unit normals, positive facet area, consistent winding,
one closed component and scaled 20 mm bounds. Mesh volume is
7801.733044824928 mm³ versus unchanged BRep volume 7801.394540616514 mm³.
The corrected millimeter file is byte-for-byte equal to the original with only
those three records removed and header count adjusted.

`stl_pole_facets` uses a four-feature authored prefix fixture: centered sketch,
20 mm extrusion, all-edge radius1 fillet and one diameter3 spherical pip. It
checks actual serialization removals, strict mm/in readback, header/returned
counts, byte metadata, bounds, volume approximation and unchanged authored bytes.
It also checks unchanged ASCII count/byte/removal metadata. The existing
`multibody_mesh` checks strict binary/ASCII placed boxes and atomic invalid-body
refusal. Both pass. Exact tessellation totals are receipt evidence, not regression
assertions. The strict independent checker was not changed.

The simple hemispherical box and curved ASCII paths can still have preexisting
non-unit or degenerate facets at other mesh settings; this bounded correction does
not qualify every spherical tessellation or curved ASCII export. The integrated
real-application die export and final tutorial recording remain the root's next
verification, separate from these kernel/artifact checks.

## Independent acceptance

Independent review on 2026-10-13 accepts bounded #204 at integrated c942a9c.
The reviewer inspected private staging, disabled direct-write fallback, exception
paths before `QSaveFile::commit`, finite serialized coordinates, duplicate/zero-area
filtering, header rewrite, verbatim retained 50-byte records, published-byte/count
metadata and unchanged ASCII copy path. No tolerance-based deletion or authored
shape mutation was introduced. The initially misplaced Python discovery now
precedes test registration, and the final regression avoids platform-specific
exact tessellation totals while checking actual removed serialized facets.

The reviewer independently reran the unchanged strict reader on the existing full
die millimeter and inch artifacts: both have 53,414 facets, one closed oriented
component, header/metadata/reader agreement and exact `84 + 50 * count` byte size.
The final companion test additionally checks ASCII zero-removal/count/byte
metadata. Reported candidate focused tests were reused rather than rerunning a
broad suite. Full application recording/export remains the cumulative #147 gate;
this acceptance does not qualify the preexisting curved ASCII limitations above.
