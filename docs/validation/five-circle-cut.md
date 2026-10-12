# Five-circle sketch and retained Cut checkpoint

Status: bounded local macOS workflow evidence on 2026-10-12. The complete
illustrated tutorial and all-platform release gates remain open.

## Authored sketch

Starting from the accepted face-supported construction guides, native C → center
→ type 50 → Return creates one circle and one driving diameter in one sketch
edit. Subsequent circles attach to guide endpoint/midpoint/bend/midpoint/endpoint
references. Four native Equal pairs leave five radius 25 circles at zero DOF.
Changing the single diameter to 60 changes all five radii to 30. Finish, exact
undo/redo and save/reopen pass. Astra reviewed the state and visible annotations.

The native center sequence is retained in the face-support worktree under
`build-face/evidence/five-circle-native-constrained.json/.png`; its reusable
document is `native-five-equal-circles.alloy`. This continues a previously edited
bracket fixture. It is not evidence of an uninterrupted exact full tutorial.

Typed preview is disposable; Return publishes the circle, fresh center,
Coincident/Midpoint relation and driving diameter together. Origin snapping uses
a lazily retained fixed origin plus a separately owned coincident center. Free
centers retain their two translation DOFs. A focused core check covers these
contracts and refusal without partial authored changes.

## Cut and history

An extrusion with `operation: cut` retains five same-sketch profile IDs and one
explicit target body ID. Each profile becomes an ephemeral OCCT prism. Their
Boolean difference modifies the retained target; no cutter bodies or separate
features enter authored history. New Body still requires connected regions.

Native Q, five ordinary region clicks, signed arrow drag to −200 mm, Cut and OK
produce one valid closed solid with five through-openings. Observed removed
volume is 490873.8521234058 mm³, equal to `5 × π × 25² × 50`. Remaining volume is
4695318.268052226 mm³. The −200 distance is our observed drag result and the public
example value, not a required tutorial default or an implicit Through All mode.

Earlier supported-sketch edits evaluate history before that sketch, preserving
its original support even after Cut changes the final body. The real-app API
check passes diameter 50→60, Cut distance −200→−100, exact undo/redo and save/reopen.
Creation discovers one visible intersecting target or refuses ambiguity; edits
retain captured target identity. Missing/no-intersection/invalid results reject
without modifying accepted history. General Boolean face naming remains ahead.

Evidence in the profile-cut worktree:

- `build-cut/evidence/cut-native-preview.json/.png`: five native selections,
  `lastArrowDrag.source: canvas`, five red tools and retained target.
- `build-cut/evidence/cut-native-committed.json/.png`: native OK and real geometry.
- `build-cut/evidence/native-five-hole-bracket.alloy`: saved native result.
- `build-cut/evidence/cut-api-final/profile-cut-api.json`: API geometry/edit/history/storage.

Run the bounded API check against an explicitly selected private app and supplied
five-circle fixture:

```sh
python3 tests/workflows/check_profile_cut.py --ready-file <ready.json> \
  --fixture <five-circle.alloy> --output <evidence-directory>
```

The runner neither launches nor quits the app. It explicitly replaces its
document with the supplied test fixture. Captures alone do not prove native input.

## Current acceptance limits

Build and focused `face_support`/`profile_cut` core checks pass. The coherent
[private Qt runtime](../dependencies/qt-cocoa-accessibility.md) passes the formerly
crashing native dimension insertion and history paths. All Qt libraries/plugins
come from one private build; Apple M4 GPU acceleration remains active. Some
accessibility row labels remain inconsistent after history changes under #158.

Native arrow and command flow pass. Astra accepted the corrected five blue
selected-profile disks with red rims and translucent cutters in an actual
reopened Cut edit (`cut-overlay-edit-preview.json/.png`). The final API workflow
passes on the same candidate. A focused Qt widget event test also passes
modifier removal/re-addition of the exact profile after distance entry, with
unchanged document state. Source camera,
theme and display scale differ; no pixel-equality claim is made. Native held
Cmd-click is not covered by the available computer-use API; the Qt input
integration check qualifies modifier dispatch without claiming physical input.
The full tutorial gate retains that distinction and remains open.

Public reference IDs are `tutorial-illustrated-bracket-a3` and
`solid.bracket-five-profile-cut`, retrieved 2026-10-12. Detailed sources and images
remain in the external knowledge store. No proprietary application was operated.
