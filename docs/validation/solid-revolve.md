# SOL-029 bounded candidate evidence

2026-10-13, base916daff, isolated revolve-foundation. Build passes with private
Qt6.11.2, OCCT7.9.3 and AppleClang17. Focused `solid_revolve` core passes Full,
Partial70 and Half analytic volumes, retained profile/axis edits, partial cap IDs,
missing/crossing axis atomic refusal, ordinary boundary-line axis, invalid partial
angles, undo/redo and exact storage. Radial10..20×height15 gives4500π mm³ Full,
875π at70°,2250π at180°. These values are independently authored Alloy fixtures.

Real-entrypoint `tests/solid_revolve_api.py` passes active component ownership,
stable preview reservations, real Full/Partial preview, Cancel with unchanged
history, commit, feature edit, undo/redo, missing-axis preview/commit refusal,
axis upstream edit and exact reopen. A stepped profile with a normal boundary
axis gives5000π mm³. No native Revolve controls were invoked or accepted.

Artifacts: `build-revolve/evidence/api` report/transcript, retained/stepped saved
models and real GPU captures. The first stepped capture contains both fixture
bodies overlapping because they occupied the same coordinates; geometric checks
inspect each body's volume separately. The prepared runner now starts a fresh
document for the stepped fixture; that artifact-layout correction has not been
rerun by the implementer. Existing source sketch visibility also overlays the
solid. These captures are geometry evidence, not matched tutorial visual evidence.
Private editors exited; no user instance was changed.

Reproduce core: `ctest --test-dir build-revolve -R '^solid_revolve$'
--output-on-failure`. API: `python3 tests/solid_revolve_api.py --app
build-revolve/alloy.app/Contents/MacOS/alloy --output build-revolve/evidence/api`.
Serialize API launches with the private editor slot. Independent Astra review is
pending; root owns acceptance/integration. See the command contract in
[solid-revolve](../research/solid-revolve.md). #49, #215 and #216 remain scoped
separately; this foundation is not tutorial acceptance.

## Independent bounded review

Astra reviewed candidate `037635d` on 2026-10-13 without changing its
implementation or launching another editor. Accepted for #214 only. Review
covered the kernel, retained schema, shared command-session routing, focused
core test source, recorded real-app report/transcript and GPU captures. The
implementer's passing build/core/API runs are reused; this was not a new test run.

The axis resolves retained line endpoints in the evaluated sketch frame. Exact
transverse profile bounds refuse a straddling axis; boundary contact remains
valid. OCCT output is checked for one valid closed positive-volume solid. Full,
70-degree and half-turn fixtures have the stated analytic volumes; an ordinary
profile boundary line also works as an axis. Stable preview reservations, active
component ownership, upstream axis/profile changes, refusal before accepted
state mutation, history and exact reopen are represented in the recorded
evidence. Partial cap identity uses the sweep's exact first/last faces. Those
caps deliberately have no projected boundary-edge references.

The partial preview is readable geometry evidence. The recorded stepped image
really does contain overlapping prior fixture geometry; the fresh-document
runner correction has not produced replacement evidence and is not credited
here. No native control, automatic profile/axis selection, angular manipulator,
matched tutorial image or complete tutorial gate is accepted by this review.
The detached construction axis is a fixture adaptation, not Centerline evidence.
#215 owns distinct Centerline/profile semantics; #216 owns native interaction
and must canonicalize the angle to 360 when switching Partial to Full. The
positive right-hand direction and independently chosen fixture dimensions
remain explicit Alloy choices.
