# Ordinary Auto body-selection route

DOC-045/#726, 2026-10-21. Public-source review supports keeping ordinary Auto
face-first on a visible supported face. The documented whole-body routes are
Body Priority and a Browser body row; a deliberate hold-click Select Other
Parents route exists in public instructions but is still open in SKT-159/#585.
There is no evidence here for changing an ordinary face click into a body pick
or inventing a modifier/double-click shortcut. The dated sources, creator URLs,
retrieval dates, screenshot observations and unknowns are external at
`../Alloy-reference-records/knowledge/specs/doc045-auto-body-selection-2026-10-21.md`.

Alloy's accepted bounded route is Auto face selection, then explicit Body
Priority for whole-body canvas clicks or Browser body-row selection. The
[shared body-set receipt](body-selection-set.md) has strict actual-hit Qt
Body Priority and Browser replace/add/toggle sequences for two visible root
bodies and linked occurrences at effective DPR1/2, with GPU and state cues.
The [selection-priority regression](root-edge-refresh.md) passes the real-app
API Auto face and Body Priority routes. The body-set API fixture checks that a
first modified Auto pick still selects a finished sketch profile. The existing
native priority-menu fixture independently verifies the Select menu toggle back
to Auto; no single uninterrupted Auto-face → sketch-profile → Body → Auto native
recording is claimed from these separate fixtures.

Independent Sol review accepted this **explicit-route disposition** against
the issue's documented-or-implement choice. It is a bounded compatibility
decision, not exact screenshot parity or completion of DOC-006. Broader model
Parents selection, curved-face priority, physical OS input and Windows/Linux
runtime remain open. No paid CI or release ran.
