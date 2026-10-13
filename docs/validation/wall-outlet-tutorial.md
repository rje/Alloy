# Fully defined outlet sketch — independent review

Accepted bounded TUT-011/#242 source workflow, source ID
`wall-outlet-sketch-20261013`, using the completed creation record and a separate
saved-model history tail. The source ends at Finish Sketch; no solid or printing
workflow is claimed.

Creation used runner `e131734` and application candidate `a117be8`. External
artifacts are under
`../Alloy-reference-records/deliverables/wall-outlet-sketch-2026-10-13/attempt-02/`;
the movie is `../Alloy-reference-records/demos/2026-10-13/wall-outlet-sketch/attempt-02.mp4`.
The original report intentionally remains `incomplete`: creation, Finish and save
completed, but its subsequent exact comparison used the pre-Finish draft rather
than the reevaluated edit-entry draft. Independent comparison found only six
reflected X-coordinate differences, at most 1.07e-14 mm. Both drafts satisfy the
independent geometric oracle.

The source-order runner creates a rough Center Rectangle by pointer commands,
retained midpoint construction axis, origin circle, rough upper rectangle and
two outward three-point arcs. It selects the actual window collection of six
curves and six related points, chooses the Mirror Line, previews and accepts,
then applies Concentric, center Vertical, Equal and six driving dimensions.
Source selection-count composition is not documented precisely; Alloy's
12-object interpretation is explicit, not a claim of twelve source curves.
Only the origin is fixed; there are no hidden final-coordinate pins.

Independent offline verification of the actual draft confirms 20 entities,
zero DOF, overall bounds (-35,-57)..(35,57) mm, circle radius 6 mm, upper/lower
arc centers (0,+25)/(0,-25), radius 17 mm, chord half-width sqrt(93), retained
Mirror mappings and construction flags, midpoint and center constraints, and
driving values 70, 114, 12, 11, 28 and 17 mm. Each cutout area is
829.2978657528639 mm2; the remaining outer region is 6208.30693296504 mm2.

The actual Mirror and fully defined captures show readable controls, truthful
12-object counts, smooth corresponding arcs, six readable dimension annotations,
constraint markers and Fully constrained status. The finished geometry is clear.
Some viewport footer view labels in the immediate post-Finish capture are clipped
after palette removal; this capture/layout timing limitation is retained and is
not evidence of complete layout fidelity. The 29.4-second movie has 294 frames,
zero capture failures, 124 delayed ticks and 169 repeated frames. Representative
blank, intermediate and final frames were inspected; it is a shared-command API
creation recording, not uninterrupted physical input or a performance benchmark.
Native ingredient evidence remains in the accepted #237–241 and #243 receipts;
no complete native tutorial run is claimed.

The focused tail uses helper `bb3859a`, the same application candidate and saved
model, with evidence under `history-tail-01/history-report.json` beside attempt02.
Independent transcript readback confirms all 14 responses succeeded, the
edit-entry draft passes the geometric oracle, 11→12 mm moves the retained mirrored
arc centers to ±26 mm at zero DOF, undo exactly restores the actual edit-entry
draft, redo exactly restores the edited draft, and reopen exactly matches both
the original authored document and the saved file. This is a second process,
not a rewritten successful creation report or an edited movie.

The independently accepted #243 correction `c6589ff` restores projected-origin
precedence outside this origin-plane fixture. Creation evidence predates that
correction; the focused regression covers its changed branch without an unchanged
full tutorial replay. Integration remains the parent's responsibility. Attempt01
and the incomplete attempt02 report are preserved; neither is relabeled as an
individually complete run. Together the inspected source-order creation, actual
geometry/visual evidence, accepted native ingredients and exact history tail
satisfy this bounded sketch gate.
