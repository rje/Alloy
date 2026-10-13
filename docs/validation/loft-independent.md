# Circular Loft independent review

Reviewed 2026-10-13. The reviewer did not implement Loft. Scope: four coaxial
circles for tutorial #173, diameters 38/16/30/44 at heights 0/18/38/58, smooth New
Body Loft, Free ends, no rails, Chain Selection On, Closed Off, then top Shell 2.
Public evidence is indexed externally by the egg-cup ledger, particularly page 4.

The verifier-authored Qt case `loftOrderedViewportProfilesCancelCommit` passed:
actual toolbar input and four ordered viewport profile clicks, enabled preview,
unchanged authored state before commit, exact Cancel, then OK yielding one valid
closed body with top height 58. The controls and buttons are legible at
1180 × 760; four selected rows exist, with the fourth accessible by scrolling.
Input is Qt-dispatched, not physical OS input.

The actual-app `solid_loft_api.py` run passed initial pick/preview/cancel, commit,
2 mm Shell, edit-dialog cancellation, and invalid-order atomic refusal. Inspected
solid and shelled-cup images qualitatively match the public tutorial's smooth
waisted silhouette and open rim. Exact surface interpolation parity is unknown.

Two findings prevent final acceptance in this receipt:

1. Selected top-profile fill visibly competes with the solid cap in preview,
   producing striped/triangular blue-gray artifacts. Root subsequently committed
   a blue-outline preview fix; this receipt has not inspected its new pixels.
2. Editing the first offset plane from 18 to 19 with downstream Shell exceeded
   `alloyctl.request`'s five-second socket deadline. The harness terminated the
   private app. This proves a timeout on the existing control path, not a kernel
   failure or nontermination. Later parameter-edit/history/reopen assertions did
   not execute and must not be claimed as passed from this run.

Evidence: `build-tutorial-polish/evidence/loft-independent/qt-input.txt`,
`qt-panel.png`, `qt-preview.png`, and `api/` captures/log. The API script writes
its ledger only on success; no completed ledger exists for this timed-out run.
Desktop was released with all private editors closed.


## Follow-up on c6ee735 and the client deadline fix

The named Qt Loft case passed again alongside Mirror and circle Offset: five
including setup/cleanup, zero failures, 8.352 seconds; see `qt-input-final.txt`.
Selected section fills now use blue outlines, eliminating the prior fine
blue-gray stripe conflict. The visible coplanar amber construction plane still
produces broad tan/gray patches on the preview cap. This remaining presentation
difference was reported to root; the final hidden-plane artifact is clean.

The original five-second client deadline was the failure boundary, not a data-
loss finding. Root's complete diagnostic ledger in `evidence/loft-timing/` records
successful source-plane and diameter edits, history, and reopen. The full
normal-client egg gate in `evidence/egg-cup-movie/egg-cup-api.json` has 61 successful
commands and result `api-pass`. The request default is now 30 seconds; recompute
latency remains tracked separately under REL004/#83. Independent normal-client
reopening of the saved source-correct egg completed in 1.853 seconds. Accept the
bounded deadline fix #197; do not interpret it as a UI performance improvement.

Loft geometry, retained history, ordered input, and downstream Shell are accepted
for the bounded circular workflow. Final preview visual acceptance retains the
coplanar construction-plane overlay note until corrected. Complete #173 API and
artifact acceptance is documented in `egg-cup-independent.md`.

## Final preview qualification

Independent read-only review of `evidence/egg-cup-final/02-loft-preview.png`
passed after transient construction-plane suppression. The cap is uniform gray,
section outlines are crisp blue, and four profile rows are legible. Browser
visibility remains authored-on; only preview presentation is suppressed. The
final61-command workflow and36.7second complete movie pass, with the same valid
closed13,450triangle STL. The earlier overlay finding is resolved. Bounded Loft
#190 and complete egg-cup API/artifact gate #173 are accepted; exact interpolation
and physical OS input remain qualified as above.
