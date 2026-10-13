# Retained Extrude Join checkpoint

SOL-018 #168 is a bounded one-target One Side Distance Join, researched from
public source `straw-bung`, University of Warwick WMG Outreach, retrieved
2026-10-12. Source URL, PDF and provenance remain in the external reference
archive at `knowledge/tutorials/warwick/sources.json`. Public PDF page 2 steps
4–8 prescribe a 3 mm New Body, top-face circle and 8 mm Join, followed by another
top-face circle extruded as a separate tool. The measured examples are Ø26,
Ø19 and Ø6.2 mm. This checkpoint does not implement the later body transform.

## Compact contract

Select an accepted profile, invoke Extrude (E or the shared dialog command),
choose Join and enter a signed Distance. Start remains Profile Plane, Direction
One Side and Extent Type Distance. Exactly one target body is retained; Auto
chooses only one visible eligible body whose union is a single connected solid
with added positive volume. Ambiguity requires explicit selection in Target Body.
The default remains New Body; automatic operation switching is not claimed.
The disposable blue preview and distance arrow use the existing extrusion
presentation. OK commits one history feature; Cancel/Escape preserve authored
state. Existing extrusion operation cannot change during editing; distance,
profiles and target references remain authored and recomputable.

Join owns no new body UUID. The target keeps its original body and producer
identities; `lastModifierFeatureId` identifies Join. A single positive prism cap
may be supported only when its exact complete face survives in the fused solid.
Its support provenance belongs to the Join feature. Older cap naming is cleared;
general Boolean face persistence, multiple targets, Intersect and other extents
remain outside this slice.

## Evidence (2026-10-12)

Private Qt candidate `build-join/alloy.app` builds locally. Focused CTest
`profile_join`, `profile_cut` and `face_support` all pass (3/3). Join checks actual
one-solid validity, closed topology, analytic stepped-cylinder volume, retained
identity, a Ø6.2 mm third supported circle/New Body, Join distance edit with
third-tool recompute, exact history, atomic rejection and native save/reopen.

`tests/workflows/check_profile_join.py` passed through the actual local app:
blank document, Ø26×3 flange, Ø19×8 Join with auto target, disposable
preview/cancel, commit, exact Undo/Redo, third supported Ø6.2 circle/tool, Join
8→10 mm edit and native save/reopen. Evidence is
`build-join/evidence/profile-join.json`, `join-preview.png`, `joined-bung.png`,
`edited-bung-tool.png` and `straw-bung-join.alloy`. Actual renderer is Apple M4.
Captured Join controls are readable, the stepped solid is smooth and the target
is retained. These are API-operated shared widgets; physical E, pointer profile
selection and operation dropdown are not verified by this script.

The public tutorial reference has older UI and unmatched viewport/theme/scale.
Its cylinder construction and Join intent are matched by geometry; exact visual
parity and uninterrupted native tutorial input are unknown. Independent review
is pending. Other-platform runtime and packaging remain unqualified.

Independent review found the inherited thin extrusion arrow hard to distinguish
on the blue preview. The existing cue now uses a filled blue arrow with a white
geometry border instead of relying on driver-dependent GL line width. Join
refusal handles also retain their actual operation instead of reporting Cut.
The rebuilt live API workflow passes in `build-join/evidence-arrow/`; the final
focused capture is `join-preview-final.png`. Independent native review follows.
