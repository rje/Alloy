# Early Extrude edit with later organized bodies

REL-032/#475 fixes a preview failure when editing an Extrude earlier than a later renamed or reparented body. The real enclosure Cut at feature index 9 initially refused preview with `Renamed local body is no longer active`: preview evaluated a temporary prefix ending at the Cut while retaining the final `Top` body name, although `Top` is created by a later Split. The full edited candidate was valid.

`previewExtrude()` now removes final browser `bodyNames` and `objectOwners` only from that temporary prefix, matching `CadDocument::evaluationBefore()`. The full candidate and committed authored data keep those fields.

The focused real-app regression `tests/extrude_edit_organization_api.py` passed early Extrude preview, Cancel, commit, exact later-body identity/name/owner preservation, undo/redo and save/reopen. Evidence is external under `deliverables/enclosure-cumulative-2026-10-17/extrude-prefix-regression-01/`. A separate real-app enclosure rerun under `clearance-prefix-03/` then passed the formerly failing Cut preview, commit, undo/redo, save/reopen and exact authored comparison. Its 17 successful calls and saved native model are recorded there. The final pre-rim STL request was malformed and is not counted as export evidence; final halves require separate rim, export and clearance checks.

This is a bounded preview-prefix metadata repair, not general recovery of downstream topology changes. The changed inset boundary remains an explicit repair/rebuild matter under HIS-008 and TUT-035.
