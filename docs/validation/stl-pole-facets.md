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

The unchanged curved ASCII path emits duplicate-vertex zero-area pole facets
at 0.05 mm / 8 degree settings in the full die, its four-feature prefix, and a
simple hemispherical box fixture. Reported bad facets have unit normals. This
confirmed remainder is tracked as PRT-008/#206. Earlier binary failures preceded
the final direct-equality fix; no remaining binary or separate non-unit-normal
defect is established by that evidence. The integrated
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

## ASCII correction and independent review (PRT-008)

2026-10-13: the ASCII path now parses the private OCCT staging file's facet
records, removes exact duplicate-vertex/zero-area facets at printed coordinate
precision, and preserves every retained facet's original text and normal.
It requires complete facet/solid records and finite coordinates; malformed,
empty or incomplete staged output fails before atomic destination replacement.
No CAD, tessellation settings, area tolerance or binary filtering changed.
Published triangle/byte/removal metadata reflects the corrected ASCII file.

The same four-feature prefix now passes the unchanged strict reader in both
ASCII mm and inches, with one closed oriented component, finite unit normals,
positive areas, scaled 20 mm bounds and volume approximation. The current
receipt has nine ASCII removals and 7,614 retained facets; regression assertions
use actual header/reader/published counts rather than pinning these totals.
Existing binary and placed-body/ASCII/atomic-refusal checks also pass.

One private real-application run reopened the saved complete die and exported
its actual component occurrence through shared `document.exportStl`, ASCII,
0.05 mm / 8 degrees, in both units. Each output has 53,414 facets, 29 serialized
removals and 13,513,758 bytes. Unchanged strict readback passes one closed oriented
component, finite unit normals and scaled 20 mm bounds. Millimeter mesh volume is
7801.733058817477 mm³. Authored data, undo state and revision stay exactly unchanged
across both exports. The harness closes the application with a successful clean
exit and releases the serialized desktop slot.

Evidence is in the isolated ASCII candidate's
`build-ascii/evidence/shared-app/`: `die-mm-ascii.stl`, `die-in-ascii.stl`,
`ascii-export-report.json`, `transcript.json` and `app.log`.
Astra independently inspected both actual-app artifacts and reviewed the staged
parser, retained records, refusal/atomic paths and metadata; bounded #206 review
accepts the candidate. This does not establish universal curved-mesh quality at
all settings. Root owns integration, issue closure and publication decisions.
