# Successive Extrude Join contract

SOL-020 #176 follows public source `straw-support`. Evidence and URLs are external
in `knowledge/tutorials/warwick/successive-join-research.json`, retrieved 2026-10-12.
The video 247/250 s observes disconnected geometry under Join; its Body Browser is
collapsed. Public help defines a body as contiguous 3D geometry; published feature
API exposes modified/created bodies as a collection. A creator's public command
description includes visible bodies touched/intersected by Join. This supports
multiple connected outputs as an Alloy compatibility proposal; exact reference
body count, UUID allocation and surviving identity remain unknown.

## Bounded authored/result contract

Keep one valid closed connected OCCT solid per `EvaluatedBody`. A new Join
feature can yield multiple such bodies. Existing Combine, Cut and New Body
single-solid policies are unchanged. Do not weaken their shared Boolean helper.
Use an isolated Join-result helper and actual connected OCCT solid extraction.

Creation discovers all visible bodies touched/intersected by selected profile
prisms, or uses an explicit nonempty distinct `targetBodyIds` array. Persist this
participation at commit; historical edits evaluate the original history prefix
and retain participants. At least one extrusion component must add material to
an existing participant. Entirely detached/no-addition Join remains an atomic
error. Detached components in a genuinely participating Join may create outputs.
Explicit participants are an Alloy control contract, not claimed exact public API
or screenshot equivalence. Current public participant property documents only
Cut/Intersect, so it is not evidence for a Join target-picker layout.

New Join features carry normalized authored `bodyResults` ownership records:
`{bodyId, participantBodyIds, profileIds}` for each connected output. A component
with prior participants retains the first participant in persisted target order;
other merged participant IDs become consumed. A detached component receives a
feature-owned UUID reserved from its sorted source-profile lineage and persisted
at first acceptance. Never assign IDs by kernel enumeration, volume, bounds or
nearest geometry. The two source intermediate solids therefore remain separate
selectable Browser bodies, not one authored disconnected compound.

At first disposable evaluation, derive result groups and stable reserved IDs,
then store them in the candidate before commit. Candidate normalization is
limited to Join ownership records; existing single-target files
without this record retain legacy evaluation. Recompute verifies prior connected
participant groups and detached source-profile groups. A split/merge or detached
reassignment that cannot retain those identities fails `join_result_changed`
atomically. Explicit distance edits within unchanged connectivity retain IDs.
Do not implement a broad persistent-topology migration/repair UI in this slice.

Dependency validation registers created result IDs at their feature, consumes
only participants absent from its surviving result IDs, and rejects references
to consumed/future/missing results. Snapshot Undo/Redo and storage carry ownership
records with intent. Inspection exposes participants, created/modified/consumed
IDs, per-result volume and valid connected topology; preview shows every result.
A renderer-only compound may compose result displays, while document/API bodies
remain separate authoritative solids.

The primary result is the first persisted participant for existing API `bodyId`
compatibility. Extend sessions with result body IDs/summaries/count. The Join
participant field must represent Auto/all discovered bodies and retained counts;
canvas profile selection, signed-distance arrow, commit/cancel and E stay shared.
Preserve positive cap support for exact surviving OCCT faces or unique actual
Modified descendants with unchanged area and unique boundary lineage, including
the disk cap at 20 mm. Independently named new prism caps are exposed per output.
No general edge naming rewrite is included. Future fillet naming remains #41.

## Exact source fixture and acceptance

After separately qualified Trim: disk diameter 15 at center (7.5,-5), bridge to x23 and
three strips x23..26/26..29/29..32, y-10..0. Source heights 20/4/24/22 are from the
sketch plane, not additive offsets. Proposed body ledger:

1. Disk20 New Body: B0, one solid.
2. Bridge + middle strip4 Join: B0 modified, B1 created, two solids.
3. Outer strip24 Join: B1 modified, B0 unchanged, two solids.
4. Inner strip22 Join: B0/B1 merge, B0 survives, B1 consumed, one solid.

This is Alloy's explicit ownership proposal, not a reference-internal observation.
Check each intermediate geometry/volume/ownership, previews/cancel, depth edit,
exact Undo/Redo and save/reopen, hidden-body participation and atomic no-Join,
ambiguous topology or consumed-reference refusal. Run the actual Trim fixture
through the real application; separate API, native selection and screenshot
acceptance. Existing one-target Join, Cut and rounded-region regressions remain
required. The root reviewed this contract before implementation.

## Candidate verification

The private Qt 6.11.2 / OCCT 7.9.3 full app builds on macOS arm64. Focused
`successive_join` checks the exact post-Trim source geometry, intermediate counts
1/2/2/1, volume, normalized ownership, original disk cap, depth edit, snapshot
history, reopen and atomic connectivity/consumed-reference refusals. Existing
`profile_join`, `profile_cut`, `boolean_document`, `sketch_regions` and
`overlap_regions` pass. Edge history aggregation includes the next fillet child's
small provenance foundation; unavailable/split edges remain explicit tombstones.

`tests/successive_join_api.py` prepares the real-app Trim/region selection,
all-visible participant discovery, disposable preview stability/cancel, source
heights, identity, cap and history/reopen receipt. It has not yet run for this
candidate; native input and visual acceptance remain independent root/reviewer
gates. Public screenshot/body-count uncertainty is unchanged.
