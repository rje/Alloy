# Conic candidate evidence (#255)

2026-10-13. Candidate implementation is complete for the bounded three-point
creation, Rho arrow/field, context OK and editable retained curve sequence.
Independent acceptance is still pending. Public evidence and mathematical
qualifications are in [the contract](../research/conic.md).

## Evidence

The focused core passed 1/1 in 0.42 seconds: asymmetric analytic endpoint,
tangent and shoulder checks, strict invalid-domain refusal, retained shared
point identity, real OCCT edge, Rho edit and exact undo/redo/reopen.

The separate real-entrypoint API attempt-04 passed 39 calls with binary
`442fe94af1f14d73d299465c99c66ae3de959beafa361a980c31a45161b15636`.
It covers three projected-pointer placements, unchanged authored draft during
preview, invalid Rho refusal, valid acceptance, meaningful Rho edit and exact
history/reopen. It predates the arrow and native editing additions. Evidence is
external at `deliverables/conic-2026-10-13/api-attempt-04`.

The final native candidate test passed 3/3 in 2897 ms. App SHA-256:
`b4a1deb009b6534438e0922891a50f72d44ab864f207590a05cdd377bfc93272`;
native test SHA-256:
`b9e607fd90aca2a3bd9466d48ede763fc5ab0edf22ee7368271ef5f749de20dc`.
The test dispatches actual Qt clicks for three placements, two consecutive
held-button moves and release, context-menu OK, invalid field typing/Return,
Escape cancellation, and Escape while dragging followed by release. It verifies
focus without repair between those demonstrated actions. Sketch creation and
existing-curve edit setup use shared commands; tool launch triggers its QAction.
Initial harness activation is explicit. This is scoped Qt input evidence, not
uninterrupted physical desktop input or whole-tutorial acceptance.

External `validation/conic/native/attempt-01` contains the actual capture and
preview state. Inspection shows the fully framed smooth curve, green control
legs and shoulder arrow, readable Rho/OK/Cancel field, and explicit seven-DOF
preview. The numeric field sits at the upper left rather than obscuring the
curve. The test exits cleanly and its private editor is closed.

Independent source review caught preview refresh clearing the drag token after
one movement; that cross-cancellation was removed. The final two-movement test
passes, while explicit tool/session/cancel lifecycle clears the token. An owned
QString also replaces a temporary-array JSON reference in context selection.
Earlier API failures preserve the corrected pending JSON initialization and
missing schema range mistakes. They are not counted as passed evidence.

## Limits

Rho is an evaluator parameter and remains one reported degree of freedom; broader
conic relations are not claimed. The rational shoulder mapping is an explicit
Alloy policy, not a proven reference kernel match. The source-order combined
three-sketch movie and gate #257 remain ahead. No STL is implied by this sketch
lesson. Cross-platform runtime and exact styling remain unqualified.
