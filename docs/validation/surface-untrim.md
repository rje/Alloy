# One-face External Edges Surface Untrim — SRF-032 / #635

The accepted bounded core restores one retained planar crossing-sheet Trim
portion to its exact finite pretrim boundary. The user selects the one kept
face and explicitly uses External Edges. Untrim appends an editable in-place
history modifier; it does not delete the Trim or create another body. The
restored body has the original face and four boundary edge IDs. Recovering the
finite earlier boundary is an Alloy retained-history policy; public command
documentation supports External Edges but does not establish exact finite
restoration for this fixture.

## Evidence

- Core `9931b3b` and shared UI `7bbe3d6` built the focused
  `surface_untrim_test` and app in Release with `-j4`, Qt 6.11.2 and OCCT 7.9.3.
  The focused test passes exact 800→400→800 mm² restoration, original face and
  four-edge identities, live source edit to 900 mm², atomic missing/stale/
  ambiguous history refusal, Undo/Redo, reopen and a downstream consumer of
  the restored face.
- Independent `tests/surface_untrim_api.py` passed 81 shared real-app API calls.
  A programmatic ray hit selected the exact retained face; preview/Cancel,
  commit, Timeline edit, invalid/refusal/retry, source width 40→50 mm,
  Undo/Redo and save/reopen passed. The authored Trim remains before Untrim.
  A separate probe reopened the saved models and inspected actual OCCT B-reps:
  one valid planar four-straight-edge open face, zero solids, exact bounds and
  areas 800→400→800 and 1000→500→1000 mm². The restored B-rep hashes equal
  the respective live pretrim stages, all original face/edge IDs return, and
  the tool B-rep is unchanged.
- Apple M4 hardware OpenGL 4.1 / DPR2 app captures show readable External
  Edges controls and a selected face. The crossing tool partly occludes the
  restored target in the isometric view; geometry acceptance comes from the
  direct saved-B-rep probe. A separate neutral capture with the tool and
  source sketches hidden shows the entire smooth restored sheet, without
  changing its geometry.

The real-app fixture, independent probe, saved models, transcript, review and
captures live outside git under
`../Alloy-reference-records/validation/surface-untrim-2026-10-20/`. The dated
public workflow and explicit Alloy policy/unknowns live under
`../Alloy-reference-records/knowledge/specs/srf032-surface-untrim-2026-10-20.md`.
No proprietary runtime or CI was used.

## Remaining acceptance

The Mac console is locked. Physical actual-hit face selection, documented
modifier-key preview suppression, focus/Cancel/OK/Timeline edit and 1×/2×
visual comparison remain SRF-048. All/Internal/Manual loop modes, extension
distance, Keep Copy, multi-face/curved/imported history and broader recovery
remain SRF-005. Windows/Linux packaged runtime remains a release gate.
