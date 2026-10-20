# Retained Plane At Angle — GEO-025 / #639

Alloy now constructs an editable datum plane by rotating an explicit origin or
retained construction plane around one earlier authored straight sketch Line.
The Line's sketch/entity and ordered endpoint IDs, source plane, numeric angle,
and Flip Side are authored. A source edit recomputes the plane and downstream
sketches; an unavailable, non-coplanar, or unsupported reference refuses the
operation without changing the document. The sign convention and Flip behavior
are declared Alloy policy because permitted public evidence does not establish
their exact reference defaults.

## Evidence

- Implementation `0e50b03` passed the focused C++ analytic, source-edit,
  dependency, history/reopen, and refusal checks in the accepted Release build
  with `-j4`. A compiled Qt numeric dock probe passed actual widget hits for
  source menus, typed `30` and Tab, Flip checkbox, preview, Cancel, OK, and
  Timeline edit restoration. This simulated Qt input is distinct from physical
  OS input.
- Independent fixture `tests/angled_plane_api.py` (`67d390f`) passed 72
  real-app responses. For a +Y pivot on XY, +30° yields normal
  `(0.5, 0, √3/2)` and Flip yields `(-0.5, 0, √3/2)`; both pivot endpoints have
  zero plane residual. It verified 45°, right-handed frames, a downstream
  rectangle on the evaluated datum, pivot-direction and source-plane edits,
  Undo/Redo, five atomic refusals, and exact authored save/reopen.
- Inspected Apple M4 hardware OCCT OpenGL 4.1 / DPR2 captures show a readable
  right dock and distinct oblique source and result plane outlines without
  panel overlap. Reopened framebuffer was 2056×1216 for a 1028×608 logical
  viewport. The private validation editor was closed.

Public-source behavior and stated unknowns are recorded outside the repository
at `../Alloy-reference-records/knowledge/specs/geo025-angled-plane-2026-10-20.md`.
The independent review, transcript, saved model, analytic results and captures
are under `../Alloy-reference-records/validation/angled-plane-2026-10-20/`;
the implementer's dock evidence is under
`../Alloy-reference-records/validation/geo025-angled-plane-2026-10-20/implementation/`.
No proprietary runtime or CI was used.

## Remaining acceptance

Physical canvas/browser source selection, an in-canvas rotation handle,
distinct pivot highlight and 1×/2× native visual qualification remain
first-v1 Todo GEO-028/#678 while the Mac console is locked. Straight model-edge
and planar B-rep face references are GEO-029/#679; documented Offset, Extent
Type and Resize controls are GEO-030/#680. Other datum construction forms stay
under GEO-001/#56. Exact reference default/sign/layout parity is unknown;
Windows/Linux packaged runtime remains a release gate.
