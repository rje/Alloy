# Two-point Circle candidate (#230)

The public lesson indexed as `warwick-initials-keyring-20261013` says to choose
2-Point Circle on a broad face, click the first **diameter endpoint**, aim the
second endpoint, type diameter 4, press Enter and finish the sketch. The written
Ø4 is the gate value. A later illustration appears to show Ø5; the source does
not dimension the first endpoint's exact position or second direction. The
dated page-by-page evidence, source URL and uncertainty record remain external
at `../Alloy-reference-records/knowledge/tutorials/warwick/initials-keyring-independent-contract.md`.

Alloy's bounded mode uses the first click as a retained sketch point and the
pointer toward the second endpoint as a retained unit direction. Typed full
diameter creates one analytic circle with a driving diameter dimension. Its
center is constrained to `first endpoint + direction × radius` by PlaneGCS;
the first endpoint is numerically anchored as an authored placement parameter
when diameter changes. The direction is an authored constant, not a free
solver variable. Moving that endpoint
translates the circle. Moving the familiar center handle translates its first
endpoint. The circle entity stores `diameterStartPointId` and the two unit
direction components, so neither size nor position depends on a later viewport
coordinate. It does not imply automatic point snapping or free endpoint degrees
of freedom. This is an explicit Alloy representation, not a claim about the
reference application's storage or solver.

The command is available in Sketch > Create > Circle and through
`sketch.twoPointCircle`, including command-first plane or selected-face entry.
The existing inline diameter editor provides disposable GPU preview, Enter
accept, invalid-value refusal and Escape cancel; `sketch.addTwoPointCircle`
offers direct shared-API authoring. Center Diameter Circle remains separate.

The focused analytic test passed: Ø4 radius/midpoint, zero-direction refusal,
driving diameter and both point-handle edits, a real circle disk, and JSON solve
stability. The injected-Qt fixture passed through the real window in one
uninterrupted sketch sequence: select the 30 × 20 × 2 mm floor end face, start
2-Point Circle, click first endpoint near (8, 8), aim toward (12, 8), type `4`
and Enter; then edit Ø6 and undo, move endpoint and undo, refuse `0`, Escape,
finish, save and reopen. It used the existing Cut operation to remove Ø4
through 2 mm, yielding 1174.8672587712815 mm³ (1200 − 8π), followed by Cut
undo/redo. Saved authored data retained the endpoint, direction and Ø4.

The [settled native preview capture](../../../Alloy-reference-records/deliverables/two-point-circle-2026-10-13/two-point-circle-preview-settled.png)
was inspected at its original 2600 × 1800 pixels: the circle, `4` inline editor,
and Sketch Palette status are readable on the floor face. It is an actual OCCT OpenGL capture on
Apple M4, vendor Apple, GL `4.1 Metal - 90.5`, 1464 × 1142 framebuffer at 2×
display scale, four-sample MSAA. The test's end-state JSON and saved `.alloy`
models are in the same external deliverables directory. An earlier immediate
capture had overlapping palette lines before Qt processed a pending layout;
the settled capture supersedes it for visual review. No matched public screenshot
exists for the exact Alloy fixture. The source's exact hole
placement and broader circle modes remain outside this bounded candidate.

## Independent acceptance

Geometry/input review of candidate `c61855fb` completed on 2026-10-13.
Independent review covered the final solver/schema/input source,
focused test source, saved models/end-state JSON and actual preview capture.
The signed post-solve checks use the newly solved radius and enforce the center
on the retained direction side; diameter/endpoint/center edits preserve the
specified identities. Anchored endpoint and constant direction are explicit
Alloy choices, not inferred snapping or free-point constraint semantics.

The final test replaces temporary-object QJsonValueRef captures with owning
QJsonValue values for comparisons. That resolves the reported test-only
comparison crash without attributing a geometry failure to Cut. The saved Cut
model exactly matches final authored state; its evaluated volume is
1174.8672587712815 mm³, matching 1200 − 8π. The native passing run is author
reported; source and persisted final artifacts were reviewed without rerunning
the editor. Native first-point click, direction motion, typing/Return and Escape
are distinct from shared-API face selection, command launch, edits, refusal
request, history, reopen and Cut. The final source has initial harness focus
setup only, with natural focus assertions before subsequent typed actions.

The palette overlap was a capture-timing artifact, measured in the named
`settledPalettePreview` Qt fixture. Immediately after typing, the info label
was at y=47 with height 60 while the profile label remained at y=68. After
queued events and 80 ms, the profile label moved to y=113, below the info
label's bottom at y=106. The labels' `heightForWidth`, `sizeHint` and minimum
heights agreed (60 and 30 px at 274 px width). The settled capture shows clear
spacing. The branch already included #227's dynamic label-height correction;
the earlier pre-#227 attribution was incorrect. No production sizing change
was needed. The named probe passed 3/3 in 1.523 s; its persisted log and label
measurements are at `../Alloy-reference-records/deliverables/two-point-circle-2026-10-13/settled-palette-test.log`
and `two-point-circle-palette-layout.json`. The original geometry/input run was
not repeated. No physical OS input, exact unspecified source hole position, general
snapping, or cumulative tutorial claim is made.

Independent visual follow-up accepted `ec3bb11` on 2026-10-13. Review of the
actual settled image, before/after metrics, persisted 1523 ms passing log and
test delta confirms that queued layout processing resolves the overlap without
changing label sizes or application code. The summary, profile status and inline
4 mm input are readable in the settled capture. The earlier minimum-height
hypothesis is not supported by these measurements: preferred height and actual
height-for-width already agree. This closes the pending #230 visual gap and
completes bounded acceptance together with the geometry/input review above.
The added event settling is capture synchronization, not a focus/selection
repair or replacement for the earlier modeling evidence. No full replay or
additional independent editor was required.
