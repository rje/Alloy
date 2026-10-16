# Complete surface iron workflow

Accepted 2026-10-15 for TUT-018/#302, independently reviewed by Astra. The real
application completes224 successful API calls and25 stages from a blank document
through a retained solid and printable export. The243.8-second H.264 creation
movie contains1219frames at1262×902/5fps, with no capture failures. It is linked
in the external gallery with the editable model, STL and source-edit variant.

The sequence inserts both public reference canvases; authors the back arc,
mirrored outline and crown rail; creates the exterior Loft, paired handle Trim,
ellipse and inner Loft; creates separate Two Sides sheets; performs both curved
Trim trials and two Removes; exactly undoes those four temporary operations;
edits the same closing sketch and Extrude; selects five tools and one Boundary
Fill cell with explicit Remove Tools; and exports the finished solid. The source
sketch is explicitly shown before selecting its added profile, and the modeling
component is explicitly active for Boundary Fill.

Final geometry is one valid closed8-face,1-shell solid,131979.331mm³. The checked
binary millimeter STL has13488triangles, one connected closed oriented component,
Euler characteristic0/genus1 and0.00574% volume difference. Actual front captures
show the enclosed handle and rear material web. An upper fit-point edit of+0.5mm
changes volume while preserving face identities; exact undo/redo and reopening
pass. These edit checks occur after the creation recording. See the
[enclosed-handle recipe receipt](iron-enclosed-handle.md).

The actual GPU is Apple M4, OCCT OpenGL4.1 Metal,4×MSAA, framebuffer capture and
DPR2. Native feature controls were independently checked in their component
receipts, including [Boundary Fill](retained-boundary-fill.md). The cumulative
movie uses the shared application API and command overlays; it does not establish
all physical mouse/keyboard gestures, Windows/Linux runtime or native-RHI status.
Cold operations remain visible: the slowest recorded profile selection was20.0s,
opening the Extrude edit13.5s, and reopening9.7s. No responsiveness claim or edited
speedup is made. Two collapsed-tip kernel orientation warnings are disclosed;
the selected solid and exported mesh pass their separate validation.

## Reference qualifications

The public source begins with prepared sketches of unspecified dimensions.
Our sketch coordinates and photo display scale are independently chosen; the
photos are not metric drawings. The central ellipse is authored separately after
opening Trim. Exterior rail contacts use matching authored coordinates rather
than retained Project references. The declared trial target is Inner; the full
public target inventory is unknown. After exact trial undo, the retained closing
curves move behind the handle before adding the bottom. This extra engineering
edit is necessary for the independently chosen geometry. Explicit upper/lower
Extrude memberships are retained even after their endpoints become Coincident.

The broad low-tip/high-crown silhouette and enclosed grip agree qualitatively.
Opening proportions, the concave rear and simplified symmetric shell differ from
the public photographs. There is no matched bottom projection or calibrated
camera, and no exact source shape, dimensional or native gesture parity claim.
Source provenance and detailed references stay outside the repository.

## Evidence

External root: `deliverables/surface-iron-continuation-2026-10-15/complete-03/`.
Primary artifacts: `iron-creation.mp4`, `iron-finished.alloy`, `iron-mm.stl`,
`iron-upper-profile-edited.alloy`, `result.json`, `transcript.json` and25 captures.
Gallery: `demos/2026-10-13/index.html#surface-iron-complete`.

Earlier attempts remain labeled as failures: complete-01 exposed duplicate
Canvas preview payloads (REL-029 corrected); complete-02 omitted showing the
hidden source sketch before profile selection; closure-check-02 exposed the
rear-open slot (TUT-033 corrected). They are not published as completed tutorials.
The accepted final recording starts from blank and includes every creation stage.
