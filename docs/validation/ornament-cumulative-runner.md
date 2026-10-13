# Cumulative ornament API runner

`tests/ornament_cumulative_api.py` is prepared for the #265 cumulative kernel/API
gate after accepted #266 and the retained-feature prerequisites. The first real
run reaches Pattern8 and refuses hub Join; #265 is active and #264 remains Todo.
Neither the partial recording nor the ingredient passes satisfy the full gate.

The runner reuses `ornament_source_api.create_seed` from a blank model through
source profile, Full Revolve, Inside Shell 5, Section Analysis, exact -5 fitted
spline Offset and retained-target Intersect. It then authors Full Origin Z body
Pattern 8, checks all eight retained occurrences, edits count 10 with stable
prefix UUIDs and exact undo/redo, and restores eight. A diameter-10 XY circle
extrudes -5 with all eight explicit Join participants; exactly one valid closed
connected result must retain its original primary body and ownership record.

The transverse hole uses a disclosed fixture: XZ Offset Plane 10 at y=-10,
circle (u=0,v=-2.5), diameter 4. Actual frame origin, normal -Y and U/V are checked
before choosing distance -20, which reaches y=+10. The Cut must reduce volume
and preserve one valid closed result. These coordinates are Alloy authoring
choices; the public source does not prescribe numeric hole placement.

The original final model is saved. An upstream offset edit -5→-4 must rebuild
the complete chain while retaining final body, Pattern feature and explicit Join
participants/ownership. Exact undo/redo and reopening original/edited authored
models are checked. Binary millimeter STL uses the existing unchanged strict
reader for finite positive facets, normals/winding, closed manifold edges and
one connected positive-volume component, plus export metadata and scale/volume.
Readable stage captures are collected; their visual acceptance is still pending.

Structured transcript and report are always written, including failure stage,
pending command and exception trace. `--command-timeout` optionally allows known
slow operations (for example 120 seconds); the shared test harness retains its
30-second default for all existing callers. This changes no application protocol,
modeling algorithm or production timeout. A root-granted exclusive editor lease
and accepted prerequisite candidate are required before executing the runner.

Source-only API review confirms that `sketch.begin` returns the active session
directly with `resolvedFrame`. Plane creation returns `planeId`, while evaluated
construction-plane records use `featureId`. Join iterates authored `targetBodyIds`
in their supplied occurrence order and retains that order in participant ownership;
only profile IDs are sorted. The runner asserts the authored target array as well
as the ledger, so it does not confuse creation/evaluation identifiers or impose
a sorted participant order. The first run reaches the Pattern stage; later assertions remain pending.

The draft now requires a built `ornament_geometry_verify` via `--geometry-verifier`.
It independently opens both saved final models with CadDocument, requires one
BRep/Boolean-valid closed positive solid, checks the complete transverse bore axis
outside and eight neighboring hub wall points inside, and verifies three points
were material before the Cut. Both bore exits and remaining hub material must be
real. Before Join, all eight explicit member solids and the exact hub prism are
recovered from retained history. Each ancestor-minus-joined Cut must execute and
produce a valid result with adaptively integrated residual volume within
max(1e-6 mm³, input volume*1e-8). Failed/invalid operations or failed integration
refuse; an empty valid difference has zero material volume. Input/result BRep
hashes remain exact around these checks after baselining only OCCT's container
Free flag, as qualified in SOL-041. No production geometry is normalized.

Original reopen now also checks final body validity, identity and volume, alongside
authored equality. The verifier target builds and runner syntax checks pass;
the first cumulative run stops at Join, before saved-model verification. Its external
process timeout is separately configurable (default 600 seconds) for repeated
known-slow exact history evaluation; this changes no application protocol.

Optional `--record-video` uses the established application recording commands:
blank design through the original final Cut and Save, with API captions and
bounded pauses, then Stop before validation edits/exports. The updated shared
prefix supplies source-stage captions, readable section framing and disclosed
solid hiding while drawing the strip. This is shared-command creation footage,
not native physical-input parity or automatically accepted visual evidence.

A Pattern-8 checkpoint is saved during creation. After the movie stops, that
actual pre-Join checkpoint is reopened for count 8→10 undo/redo/restore 8; the
original final state is then reopened before the upstream offset validation.
Thus the required count test still runs before Join in its retained source state
without putting validation edits in the creation movie. Both the original -5
source result and edited -4 result export independently checked millimeter STLs.
Full recording/modeling/oracle acceptance remains pending the Join correction.

## First actual run — 2026-10-13

Main b94fbf2, external `deliverables/ornament-2026-10-13/cumulative-01`.
The corrected Shell, exact strip Intersect and eight retained Pattern bodies
pass. The diameter10, distance-5 hub Join refuses `join_lineage_ambiguous`: a
connected input does not map wholly to one result. The explicit target list,
full transcript, report, Pattern8 editable checkpoint and 44-second partial
recording are preserved. No bore, final STL, edit/reopen or geometry-oracle pass
is claimed. The private editor exited normally through harness cleanup.

Root inspected actual radial, section, strip and Pattern captures. The revised
strip capture now exposes both curves without solid occlusion; Pattern shows
the complete eight-rib arrangement with the original seed selected in blue.
The numerical coverage check versus true material preservation is the next
#265 investigation. Tolerances and authored geometry remain unchanged.

## Coverage diagnosis

The preserved retained Pattern8 fixture produces one BRep/Boolean-valid raw
Fuse solid (17818.82939296109 mm³). Each of the eight input members and the hub
has an empty exact input-minus-result Cut. Geometry loss is therefore not
observed in this diagnostic. `retained-join-coverage.json` is retained externally
under `validation/ornament-feasibility`.

The ownership check compares an adaptively integrated input (about2278.110944
mm³ per rib) with a fixed-rule Common volume (about2278.106 mm³). Even the
existing topology-dispatched adaptive Common value differs by about0.000049
mm³, exceeding the unchanged approximately0.000023 mm³ acceptance threshold.
Sol is evaluating consistent integration and error handling; no tolerance
increase or geometry substitution is authorized by this evidence. This raw
geometry diagnosis is distinct from successful retained application Join.
