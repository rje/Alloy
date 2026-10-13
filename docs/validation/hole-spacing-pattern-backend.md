# Retained Hole Spacing/Adjust Pattern — backend and API candidate (#248)

Status: backend and real-entrypoint API evidence, 2026-10-13. The separate
native-controls receipt covers actual menu, edge, field, Flip, Cancel and edit
inputs. The public source is `plate-countersink-pattern-20261013`, page 4,
retrieved 2026-10-13; provenance and screenshots remain outside the repository
in `knowledge/tutorials/next-after-outlet/independent-countersink-contract.md`.
This is the Pattern ingredient, not the cumulative named-parameter tutorial.

The accepted additive Extrude Pattern keeps its authored Extent/Identical
schema and existing curve-parameter direction. The new Hole Pattern retains
the earlier Hole feature and target body, `distribution: "spacing"`,
`computeType: "adjust"`, and exactly two axes with retained straight edge IDs,
quantity, signed `spacingMm` and `direction: "one"`. The sign follows each
edge's retained oriented start/end provenance; it is not normalized to world
axes. Edits preserve the Pattern feature ID. Invalid or ambiguous axes,
overlapping entries, stale lineage and failed cuts refuse the entire candidate.

The seed Hole is already cut at this point in history. Each other occurrence
calls the shared `SolidHole::tool` against the current target, with translated
entry and target-derived All reach, then cuts that same owner body. This makes
the three middle bores reach through a joined underside pad rather than copying
the seed's 10 mm tool. A later feature that fills the original seed hole is
outside this bounded slice: the Pattern does not recut the seed.

Isolated worktree `../Alloy-worktrees/hole-spacing-pattern`, branch
`feat/hole-spacing-pattern`, base `4aebf9f`; backend `aecbc10`, API fixture
`e1f6faa`, native test CMake wiring `d966bed`. The combined Qt 6.11.2 / OCCT
7.9.3 app built in `build-pattern` with the concurrently authored native UI
source. API-run executable SHA-256 was
`f17da45058c1262b66506acdfe4a2d2a6c454deb074b30887a9d2bc2f8135285`.
The binary includes those native source changes, which were uncommitted at API
run time; final combined clean-build review follows native commit.

Focused `hole_spacing_pattern` core and unchanged `feature_pattern` additive
regression each passed 1/1 (1.30 s and 2.05 s). The Hole fixture checks the
source 75 × 50 × 10 mm R5 plate with retained point `(−27.5,+15,10)`, Ø4/Ø10,
90° All Hole: nine centers at x `−27.5,0,27.5`, y `15,0,−15`; volume
`36500−353π = 35391.0177933 mm³`. It also checks signed oriented axes,
width/thickness/point/spacing edits, atomic invalid-spacing refusal,
undo/redo and exact save/reopen. A joined 14 × 50 × 3 mm underside pad makes
the middle three bores 13 mm deep and the other six 10 mm; the expected volume
is `36500−353π+2100−36π = 37377.9204578 mm³`. Classifier samples below the
original plate distinguish Adjust from repeated fixed-depth tools.

`tests/hole_spacing_pattern_api.py` passed through the actual app entrypoint
and exited with its editor closed. Evidence is external at
`../Alloy-reference-records/deliverables/hole-spacing-pattern-2026-10-13/api/`:
`report.json`, `transcript.json`, both 2880 × 1800 PNGs, saved document and
app log. The shared API created the plate, dimensioned supported Point, Hole
and Pattern; it checked nine centers, both analytic volumes, exact authored
undo/redo and save/reopen. Both GPU captures were inspected: the top view
shows nine countersinks, and the isometric view shows the padded plate and
entries. The final body is selected blue in the latter capture. App state
reported OCCT OpenGL on Apple M4. This API fixture does not claim native
gesture input or full tutorial completion.
