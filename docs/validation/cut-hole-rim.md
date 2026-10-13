# Circular Cut rim lineage — candidate (#236)

Status: bounded candidate, 2026-10-13; independent compatibility review pending.
Public basis: `warwick-initials-keyring-20261013`, page 5, step 18. Source
details and images remain in the external reference records.

An analytic single-circle Extrude Cut now carries its tool prism's entry-rim
descriptor into the retained body. The existing Boolean history resolves its
surviving OCCT edge, preserving an ID derived from the Cut feature and source
profile rather than a transient topology index. Only this circular Cut case is
added; arbitrary Boolean intersection edge naming remains outside this slice.

The focused route is a 60 × 30 × 2 mm floor, a supported two-point Ø4 circle,
Cut -10 mm, selection of its upper `baseRim`, and a tangent-chain R1 Fillet.
The source step shows a rounded hole rim; exact pixel parity with its public
image is unmeasured. Alloy's default Fillet dialog starts at radius zero,
previews the selected rim after radius entry, and commits one authored feature.
The broad original floor face remains the same support ID for a subsequent
Sketch Text preview. A Ø4→4.5 source edit retains the rim ID and rebuilds the
Fillet; undo and save/reopen retain the original Cut/Fillet chain. After Fillet,
its selected predecessor rim descriptor correctly reports unavailable while
its authored ID remains bound to the Fillet input.

Evidence in `build/evidence/cut-hole-rim/` from the isolated candidate:

- `ctest -R '^cut_hole_rim$'` passed the focused kernel history/edit/reopen case.
- `tests/cut_hole_rim_api.py` passed through the actual macOS application and
  wrote `report.json`, `transcript.json`, `hole-rim-r1.alloy`, and four GPU captures.
- `selected-upper-rim-preview.png` visibly shows the blue selected circular
  edge, the R1 dialog and a rounded preview. `filleted-hole.png` shows the
  committed opening. `floor-text-support-preview.png` shows live text on the
  preserved planar floor; that draft was canceled without altering history.
- The real viewport reported hardware acceleration, Apple M4 renderer, Apple
  vendor, OpenGL 4.1 Metal, and a native framebuffer capture.

The finite fixture verifies one circle and one Cut target. Other tool shapes,
multiple selected profiles, later Boolean splits, native mouse/keyboard input,
and full keyring completion are not established here; the cumulative tutorial
gate is #233.

## Independent acceptance

Accepted candidate `eb739051` on 2026-10-13. Independent review covered the
nine-line kernel delta, existing prism-edge identity/history transfer, focused
core test and persisted 0.33 s passing log, all 62 successful real-app API
responses, report and actual selected-rim/committed/text-support captures.
No additional editor or unchanged Boolean test sweep was needed.

Only a single analytic-circle Cut contributes its prism base-rim descriptor.
Its UUID derives from Cut feature, source profile/entity and role, not an edge
index. Existing transfer admits only exact surviving edges or unique Modified
edges actually present in the result; absent tool edges are unavailable and
split descendants remain ambiguous. Thus adding a descriptor does not make a
non-result cutter edge selectable. The focused fixture demonstrates the entry
rim's usable identity before Fillet and retained authored input identity after
that predecessor edge is consumed. Recompute after diameter change, original
broad-floor support, undo and exact reopen are exercised.

The actual preview has a readable R1 control and blue selected circular rim;
the committed opening and subsequent text preview on the retained floor are
visible. Head-on shading alone is not the geometric proof: the kernel chain and
reported volume reduction from 3574.8672587713 to 3571.8693070026 mm³ establish
the accepted fillet result. Selection here is shared viewport API input, not a
new native-pointer claim. General Cut-edge naming, multiple profiles and later
split recovery remain outside this narrowly supported entry-rim contract; the
complete rounded keyring chain remains the separate #233 gate.
