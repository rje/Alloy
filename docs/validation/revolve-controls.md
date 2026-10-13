# Native Revolve controls — SOL-030 candidate

2026-10-13. Prepared from427579f, then merged the accepted Centerline prerequisite
checkpoint783f6ce before validation. New Body Full/Partial uses the accepted
SOL029 authoring/evaluation authority. SKT038's real `centerline` flag participates
in the source fixture; no synthetic flag or construction substitute is used.

## Command/workflow contract

`solid.revolveDialog` accepts optional sketch/profile/axis IDs, extent, canonical
angle, feature ID and name. Only the unambiguous supported case—one eligible
closed-profile sketch and exactly one Centerline in it—auto-selects both. Current
selected sketch takes precedence; otherwise candidates must be visible and in
the active component. Multiple Centerlines leave explicit selection required.

The right dock reports Profile/Axis counts with full IDs in tooltips, Extent Type,
Partial Angle, Project Axis and New Body. Selector buttons and clear buttons use
`solid.revolveSelection({role:'profile'|'axis',clear?:boolean})`. The next actual
viewport pick uses `selection.pick` and the retained finished profile/line.
Finished authored-line picking uses its resolved frame independently of AIS
selection activation. Axes remain straight lines in that same sketch. Project
Axis is checked and disabled with a tooltip explaining same-plane identity;
external/off-plane projection is not implemented or implied.

Full sets360° atomically. Partial starts at90°, an explicit Alloy choice. The
orange angular ring is constructed perpendicular to the retained axis, with its
radial basis/radius derived from the real profile centroid. A pointer drag invokes
`solid.revolveDrag({x,y,phase:'begin'|'move'|'end'})`; only logical screen
coordinates are accepted, not a numeric angle pretending to be a gesture. Begin
must hit the actual ring/handle. The projected basis derives the positive angle
and updates a real disposable OCCT preview. Edge-on views refuse drag until the
user rotates the view. The mapping, ring styling and Partial default are Alloy
choices; exact undocumented reference drag mapping/style are not claimed.

`app.state.viewport.gpu.revolveCue` exposes world center/bases/axis/radius and
screen center/bases/end/ring, angle, extent, IDs and drag availability/activity.
`angularDrag` reports the derived angle and pointer phase. The retained basis is
held through one drag. `revolvePointer` records actual event/mapping diagnostics.

Cancel clears the session, cue/gesture and temporary source display while
preserving authored state/history. OK commits one feature and hides the consumed
sketch. Edit temporarily reveals that retained source. Callbacks capture an
immutable session ID and use read-only guards; late field signals cannot recreate
an empty session or transfer values into a later panel. Undo/redo and save/reopen
use the ordinary document authority. No broader operation/axis mode is added.

## Verification

App and Qt test build pass with Qt6.11.2/OCCT7.9.3 on macOS/AppleClang17. Focused
native `revolve_controls_test` passes3/3 in4.3s. It seeds an Alloy-owned radial
rectangle10..20×height15 and converts its actual axis with SKT038's command.
It then uses real Qt toolbar/role controls, profile/finished-line pointer picks,
Partial extent, native press/move/release and Full/OK. Delivered pointer coordinates
(496,144)→(543,183) yield70.0432°; the center and both projected bases remain
exactly unchanged throughout, and preview volume matches4500π×angle/360 mm³.
Integer pointer quantization explains the difference from nominal70°. No OS-level
mouse/keyboard delivery or exact source coordinates are claimed.

Native Cancel checks empty session plus exact authored/history, followed by a
fresh Full360 begin/Cancel. Full commit produces4500π mm³, one editable feature;
edit/Cancel, undo/redo and exact reopen pass. Shared API checks screen-coordinate
begin/move/end giving70°/875π, Cancel, Full commit/storage, two-Centerline ambiguity,
and direct creation leaving an empty session followed by edit/Cancel.

During development, missing pointer schema bounds caused explicit coordinate
rejection, and mutable JSON guard reads/late callbacks broke Cancel teardown.
Those concrete failures were fixed and both final workflows passed. The earlier
before/after identical-basis captures are preserved under evidence/diagnosis;
no tolerance was widened or focus workaround used.

## Evidence and limitations

Local `build-revolve-controls/evidence` contains widget.log, api.log, API
report/transcript and saved model, native JSON/PNG states and native-revolve.alloy.
Partial controls are readable and show actual shaded geometry. The initial Full
preview and committed native capture slightly crop the model top at the existing
camera fit; API Home capture supplies fitted geometry. No extra modeling replay
was performed for that capture limitation. The added command is also available
under Create > Revolve; the source controls/selection/Partial→Full sequence is
supported. Some general toolbar labels elide at the native1400-pixel window.
That layout and the thin orange ring are not an exact matched screenshot claim.
Public source index REF004 and external tutorial.next-revolve-feature-checkpoint
steps46–49 carry the source provenance; dimensions and fixtures are Alloy choices.
Astra source/log/model/capture review found no bounded blocker; independent receipt
and acceptance/integration remain root-owned. Broader #49 remains Todo; the full
from-blank tutorial/movie/print output belongs to TUT008/#217.

Reproduce native (serialized private editor):
`ALLOY_REVOLVE_EVIDENCE=<absolute-directory> build-revolve-controls/revolve_controls_test`.
API: `python3 tests/revolve_controls_api.py --app
build-revolve-controls/alloy.app/Contents/MacOS/alloy --output <evidence-directory>`.
The GUI slot was released after both editors exited0.
