# Retained coincident surface chains

Accepted 2026-10-15, SRF-022/#464, commits `30b27d7` and `744217e`.

A line snapped to a spline endpoint keeps separate point IDs joined by a
Coincident constraint. Surface Extrude previously recognized only shared IDs and
omitted that line from Chaining. The evaluated sketch now exposes its retained
constraints; chain discovery, ordering and shared-edge naming consistently use
the transitive point-to-point coincidence classes. Authored points are unchanged.
Nearby unconstrained points are not welded, and unsupported constraint forms do
not become contacts. Branching, closed and disconnected inputs still refuse.

Focused core evidence uses the authoritative snapped-Line path with distinct
IDs, checks a source edit with retained boundary identity, and verifies the
non-welding and ambiguous-topology refusals. The actual from-blank iron run
passes 127 calls and 16 stages: component-owned sketches, exterior and handle
surfaces, one two-curve upper profile plus an independent lower profile, both
Trim trials, tool Remove and exact undo to the pre-trial state. Actual captures
show readable Two Sides controls and smooth sheet previews. Astra independently
accepted source, core and this runtime evidence.

External evidence is under `surface-iron-continuation-2026-10-15/`:
`coincident-chain-01/core.log` and `from-blank-03/` (result, transcript, captures
and saved `iron-surfaces.alloy`). Earlier failures remain in `from-blank-01/02`.
This is an API creation workflow; native input has its separate ingredient
receipts. The captured Solid tab is a runner presentation limitation corrected
for the forthcoming Surface-workspace movie. Solid closure and full TUT-018
acceptance remain pending.
