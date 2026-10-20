# Move/Copy Create Copy

SKT-185/#708, 2026-10-20. This alternate active-sketch route uses the accepted
independent Copy geometry authority from SKT-179/#702. It creates ordinary
editable sketch records with separate IDs; it does not add a source-linked
owner or a sketch Timeline Move feature. The sketch-specific retained history/
Edit Feature question is preserved as research/design Todo #709. Root owns
continuation-context/backlog integration and independent acceptance.

Public provenance is external in
`../Alloy-reference-records/knowledge/specs/skt185-create-copy-2026-10-20.md`
(R1–R5, retrieved 2026-10-20). General documentation supports Move/Copy,
Sketch Objects, Create Copy, Translate and numeric distances/OK. Same-sketch
eligibility is qualified by the public support source. General Translate
Timeline statements do not establish Sketch Objects + Create Copy ownership;
that remains unknown. Astra's accepted architecture chooses independent authored
geometry here. Earlier retained-owner proposal paragraphs in the research are
not implementation acceptance. Defaults, preview/cancel timing, constraint
mapping and exact matched visual styling remain unknown. No proprietary
application was operated.

## Shared route and policy

While the source XY sketch is active, Solid > Modify > Move/Copy now routes to
the same Sketch Objects Translate dock as Sketch > Modify > Move/Copy and M.
Outside an active sketch the solid command keeps its existing body behavior.
Invoke then select four ordinary source Lines, or preselect them. Create Copy
is an explicit initially unchecked option; this is an Alloy state, not a
reference-default claim. The source remains blue/selected and in place, while
the copied preview is orange. X/Y use sketch units in the XY frame; Z is visibly
unsupported. The count, independent-copy policy and preview state are visible.

Create Copy and Retain Move are mutually exclusive. The other checkbox is
disabled while one is checked, and API requests combining either retained flag
or group ID with Create Copy refuse. Changing mode discards the prior candidate
and command-local copy capture before constructing the new mode's preview.
No stale copied/retained candidate can be accepted under another mode.

The source capture and one UUID token belong only to the active Move/Copy
command. Numeric previews reuse the token, including overlap refusal followed
by a valid value. Source membership or base-draft changes require a new capture;
mode changes and terminal cleanup discard it. The user's Copy/Paste clipboard
is neither read nor overwritten. A different seeded clipboard is compared
exactly through preview, mode changes, Cancel and acceptance.

`sketch.movePreview` adds optional `createCopy=true` and uses the same source
capture/`SketchGeometry::pasteProfile` authority as Copy/Paste. Existing
Start/Pick/Accept/Cancel/Exit commands own its native controller and cleanup.
API preview synchronizes mode checkboxes, source count and X/Y fields without
recursive input signals. Responses explicitly distinguish `createCopy` and
`retained`. The pending record separates source IDs from selected accepted copy
IDs. Copy identities are UUIDv5 from the transient copy token and source IDs;
only the resulting ordinary authored records persist.

The same isolated simple four-Line, internal-constraint/dimension remapping,
rigid-solve, exact-source preservation, 1e-6 mm overlap/contact and downstream
evaluation guards apply as [Copy/Paste](sketch-copy.md). Unsupported external
relations or retained-owned outputs refuse. Finite XY coordinates/displacements
are bounded by ±100000 mm. No unsupported constraint is silently dropped.

OK adds one sketch journal step; Finish Sketch uses document history. Cancel,
Escape, dock close and API/session cancellation share terminal cleanup and
preserve the accepted draft/history. Both finish authorities refuse a pending
Move/Copy or open interaction. After commit the new geometry is ordinary:
select its curves and use Move/Copy or edit its own dimensions. Source size edits
do not propagate into the independent copy. No separate Edit Feature/Timeline
route is claimed; that question is #709.

## Evidence

External root: `../Alloy-reference-records/validation/skt185-2026-10-20/`.
Build: accepted `build-preview-release`, Release/private Qt 6.11.2 and OCCT
7.9.3, Apple Clang, `-j4`. Geometry tests are reused; controller/native/API
checks cover this alternate entry and mode state.

- `native-final.log`, `native-final/native-report.json`: 3/3 Qt functions pass
  in 9.634 s. The actual general Solid Modify menu/action opens the sketch dock.
  Real hit widgets receive four source picks and checkbox indicator clicks;
  numeric previews preserve IDs; switching atomic/retained/copy replaces the
  candidate, enforces mutual exclusion and starts a new copy token. Cancel,
  Escape, OK, exact clipboard preservation, sketch/document Undo/Redo and
  save/reopen pass. Initial activation is separate; no interstep focus or
  selection repair is performed.
- `controller-api/report.json` and transcript: 88 real-entrypoint calls pass.
  Checks include stable preview mapping, command-local capture, unchanged user
  clipboard, malformed/absent selection, empty group ID plus Create Copy,
  mutually exclusive modes, overlap then valid reuse, both finish guards,
  Look At preservation, API Accept/Cancel→fresh controls, retained-output and
  external Coincident refusal, exact source/history preservation and session
  cancellation.
- `geometry-api/report.json` and its 91-call transcript reuse `tests/sketch_copy_api.py` against the exact
  `native-final/copied-profiles.alloy`. It verifies independent source/copy
  edits, all cloned references and IDs, profile/body lineage, refusal/history/
  reopen and independent binary STL readback through the real executable.
- The original source is (10,8)..(30,20) mm; the +35 X copy is
  (45,8)..(65,20), with a 15 mm gap and two 240 mm² profiles. Two 8 mm Extrudes
  export as two watertight, consistently wound components, 24 triangles,
  Euler 4, bounds (10,8,0)..(65,20,8) and signed volume exactly 3840 mm³.
- The later source width 20→24 leaves copied records unchanged; ordinary solver
  placement yields source bounds approximately (8,8)..(32,20). Moving the copy
  by (+7,-2) yields (52,6)..(72,18) while preserving source records. Edited STL
  components are exactly 2304 and 1920 mm³, total 4224, bounds
  (8,6,0)..(72,20,8), Euler 4. Saved models/STLs accompany the reports.

- `native-readable.log`: the final presentation build passes 3/3 in 10.059 s.
  Inspected `native-readable/final-create-copy-preview.png` shows readable
  source count, state, independent-copy policy, mode toggles and both sets of
  size annotations without label overlap. The separate native visual fixture
  keeps its own model IDs; the exact geometry/API chain above uses
  `native-final/`. Inspected its two-solid capture and the API's edited-solid
  capture separately. Captures include QWidget chrome plus real OCCT framebuffer
  composition and pixel hashes. Hardware diagnostics identify Apple M4 / Apple,
  OpenGL 4.1 Metal - 90.5, MSAA4, DPR2; the API framebuffer is 2376×1216 for a
  1188×608 logical viewport.
- Existing atomic Move native/lifecycle regression passes 4/4 (8.332 s), and
  retained Move actual-hit/edit regression passes 3/3 (7.274 s). The reused Copy,
  atomic Move and retained Move core tests pass 3/3 (`core-regression.log`).

Numeric controls, retained/internal history, exact geometry, mesh readback and
native/GPU captures are distinct evidence. The selection prompt was shortened
and label heights reserved after an inspected functional capture showed clipped
wrapped text; a separate final visual/native receipt records this presentation
fix without changing geometry authority.

Matched within Alloy: the alternate command vocabulary/entry and bounded source,
copy, numeric preview, OK/Cancel workflow. Unknown: exact reference defaults,
sketch Timeline owner/editing, source association and visual parity. Wider
planes/curves/groups, direct translation manipulators, direction-frame choices,
cross-sketch copying and physical/packaged-platform input remain outside this
bounded route. Retained sketch copy history is #709. No CI, push or release.
Private fixture editors closed after evidence; user PID72618 is preserved.

Independent Sol review accepted `ff93014` after checking the shared geometry
authority, mode/clipboard/terminal paths and actual-hit native evidence. It
independently re-read both saved binary STLs and inspected preview/solid
captures. The review was read-only and did not claim a fresh build or app run.
The bounded alternate command route is accepted; sketch-specific retained
history remains Todo #709.
