# Paired full-round ridge Fillet (GEO-020)

The exact final enclosure snap fixture now retains the inward 1.25 mm inset,
Join 2 mm and two tangent-chain R0.5 Fillet as editable authored features. The
selected paired rails bound a nominal 1 mm ridge. This completes the bounded
geometry blocker; the retained-tool Cut, exports and cumulative tutorial remain
TUT-023 work. The source-topology adaptation described in the
[original diagnostic](enclosure-rim-two-rail-blocker.md) remains explicit.

## Implementation contract

The ordinary OCCT Fillet builder remains first. Recovery is restricted to its
corner-stage failure with two successful, open, equal-radius stripe contours.
Both contours must be analytic coplanar extrusion end rims from the same profile
and generator. Their finite line/arc domains must overlap uniquely at a
separation of twice the radius; circular arcs must be shorter than a half circle.
They must bound one planar face whose remaining boundary forms exactly two
unbranched terminal paths joining opposite contours. There are no fixture UUID,
coordinate or seven-edge-count exceptions.

Two independent single-chain Fillets start from the same original solid. Both
must produce valid closed single solids and subtractive subsets of the original.
Their OCCT Common must also be a valid closed single solid. Otherwise the usual
Fillet refusal remains. The result reports `construction: pairedFullRoundCommon`.
This is a bounded full-width ridge construction, not a general interacting-fillet
replacement or a radius/tolerance retry.

Authored selected IDs and radius are unchanged. A small composed kernel-history
adapter follows original → either Fillet branch → Common, deduplicating actual
shapes. Existing transfer rules retain unique descendants and refuse ambiguous
downstream references; coordinates never choose replacement identities.

## Evidence

Evidence directories are external under
`../Alloy-reference-records/deliverables/enclosure-rim-2026-10-14/`:

- `geo020-common-01` preserves the first valid composition and the subsequent
  diagnostic empty-bounds error. That error occurred after saving the valid
  Common; it was not a kernel rejection of the composed solid.
- `geo020-common-inspect-01` reads those saved BReps without reconstructing them.
  Both branches and Common are subtractive subsets. The rear section contains
  two same-center R0.5 quarter circles forming the intended semicircle.
- `geo020-common-certify-01` samples existing joins: 33 expected tangent seams
  pass, with maximum tangent-plane angle about 2.11e-8 rad; five intended sharp
  terminal trims lie on original trimmed mouth faces within 1e-6 mm. Zero failed
  or unknown seams are recorded. These are bounded samples, not an exhaustive
  continuity proof; the diagnostic exit code alone does not assert its counters.
- `geo020-production-01` commits and saves the actual two-chain feature through
  the production document authority on the accepted 29-feature source.
- `geo020-production-api01` opens that model through the real app, edits source
  width 64 → 65 mm through sketch controls, and verifies unchanged authored
  Fillet/seeds, valid closed single-solid geometry, no ambiguous named edges,
  undo/redo and exact save/reopen. Source snapshots/hashes and transcript are
  retained. The three GPU captures show the reopened widened ridge with Top
  hidden; Apple M4, OpenGL 4.1 Metal, 4× MSAA and a 2296×1424 framebuffer are
  recorded. This is API workflow evidence, not native-pointer acceptance.

Existing `solid_fillet`, `grouped_fillet` and `fillet_rim_lineage` regressions pass
3/3. The console gate and real-app workflow also pass. No unchanged replay was
performed after these checks. Build/GUI validation instances were closed.

Independent review accepts the bounded admission, composed history, production
workflow and actual GPU mouth capture. The board remains visible in the capture.
Broader fillet families, exact
reference topology, physical enclosure fit and other-platform runtime remain
outside this receipt.
