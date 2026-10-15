# Orthographic orientation cube

NAV-020, accepted 2026-10-14 after independent Astra review.

The cube uses OCCT's combined triedron and orthographic transform persistence.
It remains anchored to the corner and follows camera orientation, while its
projection stays orthographic regardless of the model's projection policy.
The model camera is unchanged. This addresses the user-reported skew.

The existing projection fixture now records actual GPU camera direction/up
before and after an orthographic-to-perspective switch, confirms the same cube
face at the same viewport coordinate, and delivers a target-checked Qt click
that reorients the view. Paired actual captures show matching cube size, pose
and position while the model changes projection. Root and Astra inspected them.

Evidence: external `deliverables/nav020-cube-perspective-2026-10-14/qt-01/`,
including `cube-orthographic.png`, `cube-perspective.png` and `ctest.log`.
The validated `build-tutorial-polish` app and fixture were relinked; final CTest
passed 1/1 (three Qt cases) in 4.58 seconds. An accidental system-Qt build was
stopped before application launch; an outdated test binary's earlier failure
is excluded. Physical input and broader platform qualification remain separate.
