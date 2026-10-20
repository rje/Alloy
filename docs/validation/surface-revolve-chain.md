# Connected-Line Surface Revolve — SRF-037 / #660

The bounded implementation adds one ordered connected open chain of 2–32 ordinary
sketch Lines to the retained Partial / One Side / New Body surface command.
`entityIds` and `sectionVersion: 2` retain the ordered sources; evaluated
`orientedSources` exposes their orientation. The existing `entityId` encoding,
one-Line evaluator and stable IDs remain unchanged. The native dock offers an
explicit multi-Line Profile chain list, an ordered-source readout, and a separate
construction-Line Axis selector. This is an Alloy selection policy; public
multi-curve native gesture and default-Chaining parity remain unknown.

The evaluator creates one exact connected open shell with one generated face per
source span, without caps or solids. Face IDs derive from source IDs; shared
junction edges are retained once with stable IDs. Source, axis, angle and reverse
edits use the same command/document authority as preview and history. Disconnected,
branched, closed, self-intersecting, crossing/touching-axis and overlapping
backtracking selections are refused without changing the document.

## Candidate evidence, 2026-10-20

- Release build uses `build-preview-release`, Qt 6.11.2 private runtime and OCCT
  7.9.3; build concurrency was bounded to `-j4`. Focused `surface_revolve`,
  `solid_revolve` and `surface_extrude` CTests pass (3/3).
- `tests/surface_revolve_chain_api.py` passes through the real app (77 responses).
  The XZ chain (20,-10) → (30,0) → (20,10) mm about a retained construction
  axis gives 2 faces, 7 unique edges, 1 shell, zero solids and null volume.
  Its 90° area is exactly 250π√2 mm² and bounds are [0,0,-10]..[30,30,10] mm.
- Preview/Cancel, disconnected/three-branch/cross-axis/self-cross refusal,
  invalid-preview retry, dock inputs, 45° Reverse edit, source endpoint and axis
  edits, Undo/Redo and save/new/reopen pass analytic area/bounds and retained-ID
  assertions. The core fixture separately checks stable IDs after angle edits
  and the analytic area after moving the axis 5 mm.
- The unchanged `tests/surface_revolve_api.py` also passes through the candidate,
  including its legacy one-Line edit dialog, geometry, history and persistence.
- Three actual Qt/OCCT framebuffer captures were inspected: readable two-source
  dock selection with a distinct axis, coherent smooth conical patches and shared
  join, and a larger source-hidden reopened sheet with visible open boundaries.
  Hardware diagnostics identify Apple M4, Apple OpenGL 4.1 Metal layer, DPR 2 and
  4× MSAA. This qualifies the existing GPU prototype for this fixture only.

The model, transcript, result and captures are external under
`../Alloy-reference-records/validation/surface-revolve-chain-2026-10-20/`.
The public-evidence contract is external at
`knowledge/specs/srf037-connected-open-section-surface-revolve-2026-10-20.md`.
It records source IDs R1–R4, provenance, retrieval date and documented versus
inferred behavior. No proprietary application was operated.

## Independent review and remaining acceptance

An independent Astra compatibility review accepted commit `0d53862` for the
bounded joined-Line result, including source-derived face/edge identity, the
real-app geometry/history fixture and inspected GPU captures. The inactive
single-Profile combo can display “Select” while the explicit chain list contains
two Lines; this is a presentation follow-up, not a missing authored selection.
The Mac console remains locked;
this receipt does not establish actual-hit dock input, physical canvas selection,
angle dragging, OS input, DPR 1 or matched public visual parity. SRF-036 owns the
canvas-selection/manipulator gate. The available public one-curve/full-revolution
image differs in geometry and extent, so exact visual comparison is unknown.

This implementation accepts Lines only. The existing retained Sweep path helper
requires tangent-continuous joins, whereas the researched open-profile contract
permits non-tangent joins. Reusing it would silently exclude required paths.
Arc admission also needs whole-arc radial/axis and self-intersection validation;
the straight-Line half-plane checks cannot establish those properties. Line/Arc
chains remain Todo SRF-055/#715; splines, multiple profiles, alternate axes/extents
and complex topology remain in SRF-001. No curved-chain coverage is claimed. Windows/Linux packaged
runtime and native RHI remain separate release gates. The private editor was
closed after each fixture; no CI run or publication occurred.
