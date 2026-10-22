# Shared body selection set

DOC-043/#721, 2026-10-21. Independent Sol review accepts the bounded
Body Priority/Browser/API selection set at `89b735b`. DOC-006 remains open.

The dated public documentation audit and workflow contract are external in
`../Alloy-reference-records/knowledge/specs/doc006-selection-completion-2026-10-21.md`.
It records source URLs, retrieval dates and the distinction between documented
modifier behavior and unknown visual/interaction details. No reference application
was operated or inspected.

`selection.set` accepts `operation: replace|add|toggle|clear` (default replace).
Non-clear operations require a `bodyId`; an exact `occurrenceId` is required when
a body definition has multiple visible instances. Each request resolves its
identity before changing selection. State and results expose ordered `members`
objects containing both identity fields, including an empty occurrence for root
bodies. Existing singleton fields identify the last member for commands requiring
one body. Add is idempotent; toggle removes an existing member. Selection lives
in window/tab state, never authored model/history; reopen starts with no body
selection. Document evaluation prunes unavailable members without resurrecting
them on Redo.

Body Priority canvas and Browser clicks share this authority: ordinary replace,
Shift add, Command/Ctrl toggle. A blank ordinary Body Priority pick clears;
modified misses preserve the set. Auto retains its established ordinary planar
face preference. Auto modifier body hits invoke body selection when no finished
profile set owns the pick. Ordinary Auto body fallback also uses the authority.
Consequently this slice does **not** establish ordinary Auto canvas body
selection; DOC-045/#726 retains that researched compatibility route. Face/edge sets, window selection, Components,
filters and model Select Other remain outside this slice. Command-specific body
pickers retain their existing meanings and reject explicit set operations while
active.

The GPU view applies the same exact occurrence members to AIS selection, and
Browser selection cues derive from that membership. Native tests deliver events
to `QApplication::widgetAt` receivers, observe Qt/AppKit activation/focus, and do
not repair focus or selection between demonstrated steps. Qt-delivered input
remains separate from physical OS input.

## Verification commands

Accepted cache: `build-preview-release`, Release, private Qt runtime
`../Alloy-dependencies/qt-private-runtime/install`, OCCT 7.9.3, arm64 system C++.
Build with `cmake --build build-preview-release -j4 --target alloy
body_selection_set_controls_test selection_priority_controls_test`.

- `python3 tests/body_selection_set_api.py --app build-preview-release/alloy.app/Contents/MacOS/alloy --output ../Alloy-reference-records/validation/doc043-2026-10-21/api`
- `ALLOY_BODY_SET_EVIDENCE=<absolute output> build-preview-release/body_selection_set_controls_test`
- Repeat the focused native fixture at effective DPR1 with `QT_SCALE_FACTOR=0.5`
  on the native DPR2 display; save each run separately.
- `python3 tests/selection_priority_api.py --app build-preview-release/alloy.app/Contents/MacOS/alloy --output <absolute output>` preserves singleton priority/Auto behavior.

Only one private validation editor may run at a time; preserve user PID72618.
No paid CI, tag, push or publication is part of these checks.

## Candidate evidence

The final application and focused native fixture build passed. The final
real-entrypoint API run passes **105 responses** in external
`validation/doc043-2026-10-21/api-final/`: ordered replace/add/idempotent/toggle,
separate repeated occurrences, atomic stale/ambiguous refusal, body-priority
picks and clear, unchanged authored/evaluated/document history state, unchanged
application revision for direct selection, byte-identical Save, tab retention,
reopen reset, Undo pruning and no Redo resurrection. A first Shift pick still
selects a finished sketch profile in Auto. The saved fixture SHA256 is
`d69c4ce0b7a73c91dccec73ef8df8b742b283d3d17a2d86fe8fef2fb555670d6`.
The earlier `api/` attempt compared dirty metadata before/after Save; it is a
fixture-baseline error, superseded by the after-Save baseline in `api-final/`.

Final actual-hit Qt runs pass **3/3 at DPR2 (5.902s)** and **3/3 at effective
DPR1 (3.722s)**. External `native-dpr2-final/` and `native-dpr1-final/` contain
saved models, full state/transcripts and hardware captures; their adjacent logs
record results. Initial fixture setup collapses Named Views and expands body
folders, before the demonstrated sequence. Canvas replaces A, adds B twice,
toggles A/B off, then replaces A. Browser replaces B, adds A twice, toggles B/A
off. Two further actual canvas hits replace with the original linked occurrence
and Shift-add its copy; both share a body UUID but retain exact occurrence UUIDs.
Every state has matching Browser membership. All **15 recorded states** in each
run have genuine AppKit application-active/editor-key-window agreement; input
receivers come from actual `widgetAt` hits. Canvas and Browser clicks naturally
produce the checked focus, with no focus/selection repair between steps.

Native fixture hashes: DPR2
`98b63b3755a42548822d6270e1c947d44e1630ca9efaea180024173ecd58f347`;
DPR1 `f31ac0d874fc0a9d4a8e6232ba0cf04ca975e0dcd2be3384882e30375fc945e7`.
GPU diagnostics identify Apple M4 / Apple / OpenGL4.1 Metal-90.5 / GLSL4.10,
hardware accelerated with4× MSAA. Both scales use1188×608 logical viewport,
then1068×558 after resize. DPR2 framebuffer is2376×1216 then2136×1116;
effective DPR1 is1188×608 then1068×558 (native backing594×304 then534×279).
Single, multiple, linked, clear and resize captures were inspected across the
runs. Selected bodies are visibly blue, Browser rows agree, clear removes old
cues and resize retains the two-body cue. The final linked captures confirm
these cues after actual occurrence hits. No matched public screenshot state was
available, so exact visual parity is unknown.

## Separate retained-edge mismatch

The existing `tests/selection_priority_api.py` run stops at line37: the selected
root edge disappears across repeated inspection/refresh before the post-miss
assertion. External `priority-probe/transcript.json` isolates the behavior:
the first state returns the edge, but its refresh leaves the next state empty,
before capture. Source comparison against pre-candidate HEAD
`beaa862824c3c832d629349aa2ef011a3fac046b`, lines8273/8275, shows the same
mutable `selectedEdge_["occurrenceId"]` lookup in `highlightSolidEdges`.
It inserts a null key; the next `contains("occurrenceId")` check compares an
empty root occurrence to null and invalidates the selection. A narrow follow-up
should use owned `.value("occurrenceId").toString()` reads and string-normalized
identity comparison. This preexisting bug is unchanged here; the broader
priority regression is **not passing**, and no face/edge preservation acceptance
is claimed from it. Root will track the separate bounded fix.

Independent Sol source/evidence review accepts the stable body/occurrence
identities, shared command authority, exact GPU membership and strict Qt
actual-hit evidence. It found no additional blocking defect in this bounded
slice. Ordinary Auto remains DOC-045/#726; physical OS modifier delivery and
Windows/Linux runtime are unqualified and remain broader gates. The retained
edge mismatch is DOC-044/#725. No paid CI or release ran. All private candidate
editors exited; user PID72618 remains preserved.
