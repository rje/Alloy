# Named Combine results and recovery — SOL-071 accepted

The target keeps its body UUID and explicit display name through supported Join,
Cut and Intersect. Keep Tools preserves tool identity/name/visibility. With Keep
Tools off, tool bodies leave the evaluated output while their name metadata stays
in authored history. Undo and editing Keep Tools on restore those same identities.
This fixes a real-app `renamed_body_unavailable` refusal on an otherwise valid
named-body Join. Unknown or actually unavailable upstream names still refuse.

Public source IDs `boolean-combine-howto` and `boolean-combine-reference` were
reread on 2026-10-18. The external contract is
`../Alloy-reference-records/knowledge/specs/sol071-combine-naming-recovery-2026-10-18.md`.
Target/tool roles, operations and Keep Tools match documented semantics. Exact
reference naming after consumption, error wording and contemporary visual styling
are unknown. Dormant metadata and explicit retry guidance are Alloy policy.

The Release build uses Qt 6.11.2, OCCT 7.9.3 and the existing accepted private Qt
runtime. `boolean_document` passes with named target/tool consumption, editable
Keep Tools, source edits, Undo/Redo and native-file reopen. The focused real-app
API fixture is:

```sh
python3 tests/combine_naming_recovery_api.py \
  --app build-preview-release/alloy.app/Contents/MacOS/alloy \
  --output ../Alloy-reference-records/validation/sol071-combine-2026-10-18/api-01
```

It passes independently named 20 × 10 × 5 and 10 × 10 × 5 mm overlapping boxes:
Join 1250, Cut 750, Intersect 250 mm³. Target width 22 mm retains Join 1250 and
Cut 750, and changes Intersect to 350 mm³. Each operation checks current body
counts, target/tool UUIDs/names/visibility, valid closed single-solid geometry,
consumption, Keep Tools edits, exact authored Undo/Redo and saved reopen. A failed
upstream move to a separated Intersect preserves the full accepted inspect state.
A separate separated-tool Intersect session refuses with `boolean_empty_result`,
keeps inputs editable, and commits a valid Cut retry. The error explicitly asks
the user to check overlap or change Operation; no candidate replaces the accepted
model on failure.

API captures identify Apple M4, Apple vendor, OpenGL 4.1 Metal 90.5, DPR2 and
hardware acceleration. The error panel is readable with pinned Cancel/disabled
OK; current named bodies remain in the Browser. This is bounded actual GPU
inspection, without a matched modern reference screenshot or new resize claim.

`combine_naming_recovery_controls_test` passes 3/3 in external `native-05/`.
It checks actual Browser rows and Combine target/tool labels, native Operation
popup selection, OK, consumed-tool absence/Undo restoration, native Keep Tools,
saved reopen, disabled OK/readable error on empty Intersect, and native Cut retry.
Pointer clicks assert the actual widget hit; the checkbox receives its indicator
click. Initial app activation is separate; no focus repair occurs between steps.
The test observes async Open completion and popup closure before later inputs.

The earlier native-04 immediate refusal capture caught queued layout before its
red label acquired the wrapped height. Native-05 waits for the error label's
height-for-width contract and its inspected full-window capture shows both lines
of guidance, the help and pinned actions. No product layout correction was needed.
Native-01–03 failures corrected harness assumptions about the complete tool list,
checkbox hit region and asynchronous Open; they are not product-fix claims.
Native-05's status strip still shows unrelated crowded text; this is outside the
specific Combine error/control acceptance and remains broader shell polish.
Independent Sol review accepted the bounded source, API, native input and
settled GPU evidence on 2026-10-18. Physical-device delivery, other-platform runtime/package qualification,
multiple/disconnected outputs, mesh fallbacks and universal Boolean robustness
are outside this child. No unrelated topology failure was discovered in this
fixture, so it creates no new broad topology work.
