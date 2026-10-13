# Center Rectangle independent review

Reviewed 2026-10-13 against the integrated candidate containing 8026f86 and
11245c4. Reviewer did not implement Center Rectangle. Scope: the centered
12 × 12 mm square needed by tutorial gate #172; full gate acceptance is separate.

Matched: the named Qt case `centerRectangleGestureAndInlineValues` selected the
tool, clicked the center, typed width 12, Tab, height 12, Return, and checked six
entities (four boundary edges plus two construction diagonals), two driving
12 mm dimensions, and cleared pending input. The case passed in
`build-tutorial-polish/evidence/mirror-independent/qt-input-fixed.txt`; that
combined log also contains a separate export-test timing failure, so it is not
an all-pass suite receipt.

A separate real-application run previewed and accepted the same dimensions,
finished the sketch, and verified one evaluated profile. Inspected
`center-rectangle-input.png` and `center-rectangle-finished.png` in the same
evidence directory: both inputs and keyboard hints are readable, and the final
square and dashed construction diagonals are visible. No bounded blocker found.

Input evidence is Qt-dispatched widget events plus the shared application API,
not physical OS mouse input. The public source workflow is indexed by the
external `knowledge/tutorials/warwick/nested-cubes-ledger.json`. Exact layout
parity and full nested-body placement are not claimed by this receipt.
