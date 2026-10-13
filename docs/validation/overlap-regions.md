# Overlapping sketch regions candidate

SKT-027 #174 is bounded to the straw-support region fixture. Read
[the researched contract](../research/straw-support-regions.md). The original
public video shows Trim omitted by the PDF; #177 owns that operation. Equivalent
post-Trim fixture authoring does not pass Trim or the native tutorial gate.

The existing OCCT splitter and planar face builder now coalesce coincident
straight edges while retaining every authored source UUID in boundary provenance.
Derived intersections carry incident source sets and ordered branches; contact
multiplicity is included so collapsing/changing intersections do not alias older
references. Numeric positions locate contacts, not profile identity. Internal
open guide wires are excluded from closed profile boundaries/prisms. Coincident
curved sources remain explicitly ambiguous. No body ownership/Join semantics
change is made here; #176 owns the source's later multi-height operation chain.

Adjacent exact API rectangles already share endpoint UUIDs. Their automatic H/V
inference now avoids inserting the same relation twice on the same point pair,
which otherwise made the actual three-strip fixture solver-redundant. Authored
line sources remain distinct; their evaluated coincident edge is represented once.

The new focused `overlap_regions` check verifies five post-Trim OCCT regions
(disk, bridge and three30mm²strips), independently computed analytic total area,
coincident source-set provenance, dangling fragment exclusion, nine genuine raw
arrangement cells, retained IDs under authored entity reorder and JSON storage,
Ø15→16 disk extrusion recompute, explicit `profile_unavailable` when a bridge's
incident contacts change, atomic refusal, Undo/Redo and save/reopen. Existing
rounded-bracket region identities, sketch solver and retained Join tests are
included in the targeted regression command. Local private-Qt build passes;
`sketch_solver`, `profile_join`, `sketch_regions` and `overlap_regions` all pass
(4/4). This includes the existing rounded-bracket retained-ID checks.

The actual-app script `tests/workflows/check_overlap_regions.py` separately
qualifies explicit post-Trim fixture authoring, actual viewport picking/highlight,
source two-region selection, disk editing/reference failure and storage through
the shared API. It is prepared but not yet run in this candidate. Native Trim,
full source tutorial, matched source-scale visual comparison and independent
compatibility review remain pending. Other-platform runtime remains unqualified.

The first combined app run reached the intended changed-bridge refusal. Its
public `sketch.finish` boundary exposes `evaluation_failed` with the missing
profile repair message, while the focused kernel check proves the internal
`profile_unavailable` diagnostic. The script checks that current public boundary
and verifies that Cancel restores exact accepted intent; it does not require an
unadvertised nested diagnostics object. Integrated rerun remains pending.

## Integrated real-app checkpoint

The combined candidate in `build-support-integrated` builds and passes both
changed core tests. Its bounded real-app script passes with GPU captures under
`build-support-integrated/evidence/`. Root inspected Trim preview and actual
separated profile selection. Native input and independent acceptance remain
pending; this checkpoint does not close the complete support tutorial.
