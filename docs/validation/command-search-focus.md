# Command Search focus and session — FND-029

Accepted 2026-10-24 at `c4b4322`: Search button, S and local `commands.search` share the
existing authoritative command session. Fresh sessions synchronize an empty
query into the reused dialog; reopening the same session synchronizes its
retained query. Native typing and `command.update` populate the same input and
results. The query consumes ordinary text shortcut overrides, including s.
Dialog, query and result controls have stable object/accessibility identities.

Escape closes Search and restores its originating visible, enabled focusable
editor control, with the viewport as fallback. A result closes the session once
before invoking its registry command, allowing that command to establish focus.
Return and Enter explicitly activate the focused result list across native item
view styles; the session guard prevents duplicate delivery after closure.
Unavailable and argument-requiring commands retain the existing omission policy.

## Public contract

The bounded source/workflow record is external:
`../Alloy-reference-records/knowledge/specs/fnd029-command-search-2026-10-21.md`.
It references the FND-001 baseline and source IDs `keys` and
`kb-command-shortcuts-20261012`, including creator, source URL and retrieval dates.

- **Matched:** documented S entry is present and passes the bounded Qt/native test.
- **Unknown:** exact reference Search reentry, Escape focus, result-key behavior,
  placement and styling. Session/focus behavior here is explicit Alloy policy.
- **Mismatch:** no new reference mismatch established by this bounded evidence;
  there is no matched public Search capture or exact visual-parity claim.

Wider shortcuts, Browser/navigation/layout reset and general text-focus
suppression remain FND-007; broader accessibility remains FND-015.

## Reproduction and evidence

Accepted `build-preview-release` cache: Release, private Qt 6.11.2, OCCT 7.9.3,
Apple arm64 / macOS 26.5.1. No manual CI dispatch or tag push.

```sh
cmake --build build-preview-release --target command_search_controls_test alloy -j4
ALLOY_SEARCH_EVIDENCE="$PWD/build-preview-release/evidence/FND-029-search" \
  ./build-preview-release/command_search_controls_test
```

The explicit GUI fixture is excluded from automatic CTest. Final result:
**3 passed, 0 failed, 3.362 seconds** (one workflow plus setup/cleanup).
Artifacts archived outside the generated build tree in
`../Alloy-reference-records/validation/fnd029-command-search-2026-10-21/`
(original run: `build-preview-release/evidence/FND-029-search/`):
`report.json`, `qt-test.txt`, `build-final.txt`, `search.png`, `restored.png`.
The retained `first-return-mismatch.json` documents the first run's focused
result Return failure; the targeted fix and final rerun pass. Neither run repairs
focus between demonstrated steps.

One initial harness process/window activation and viewport focus setup precede
actual-hit Search. The uninterrupted final sequence checks native application
activation/key window and resulting Qt focus at each popup transition, then:

1. Click Search; type an s-containing query with zero results; Escape.
2. Verify exact usable origin focus and unchanged authored document; S reopens
   empty; type s without changing the session ID.
3. Invoke Search within that session; verify preserved query, then native text
   selects `view.home`. Click the real hit widget for its row and press Return.
4. Verify session closure and authoritative shell revision delta exactly +2
   (`commands.close` plus one `view.home`), proving a single result execution.
5. S/Escape again; verify restoration to the exact control observed before S.
6. API open/update/reopen/close preserves the same query/session. Final authored
   document and selection equal the initial values.

All recorded native key-window and application-active checks are true. The
native DPR2 Search and restored-editor captures were inspected: readable query,
result labels and selection color, no clipping. These are actual Qt UI captures;
reference display scale is unknown. The test uses Qt-delivered pointer/keyboard
input with macOS native window observations, **not physical OS keyboard input**.
It operates real AlloyWindow/document/command implementations through the shared
application library; no mocked document or automation state is used.

No geometry changed, so geometry/export/reopen checks are not applicable to
this shell increment. Deleted-origin fallback is implemented but not separately
exercised by this bounded fixture. Windows/Linux runtime, DPR1 and wider modal
combinations remain unqualified. The private validation editor is closed;
user-owned PID72618 was preserved.

Independent bounded source review accepted the initial focus/session changes,
requested exact origin and execute-once assertions, then accepted the strengthened
fixture and Return/Enter fix. A final independent Sol review on 2026-10-24
accepted source, report, native checks and both captures without repeating
unchanged app runs. Physical OS delivery, DPR1, deleted-origin fallback and
keypad Enter remain unexercised limits. This closes FND-029/#734; the wider
FND-007 gate remains open.
