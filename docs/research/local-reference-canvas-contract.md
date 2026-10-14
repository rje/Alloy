# Local reference canvas contract

DOC-024/#294 uses a user-local raster as a drawing reference, separately from
sketches and evaluated geometry. Public inputs/workflow are recorded under stable
references `canvas-insert-public-20261014`,
`canvas-placement-public-api-20261014` and `canvas-dialog-public-20261014` in the
external knowledge corpus. Retrieved2026-10-14; detailed provenance, public
screenshot and compact specification remain outside Git under
`knowledge/specs/local-reference-canvas-2026-10-14/` and
`records/local-reference-canvas.json`. Query with
`python3 tools/knowledge.py search "local raster canvas"`.

Insert → Canvas selects a local image and opens a native dock. Origin,
construction, sketch and retained planar-face sources share existing plane
resolution. The dock offers origin/construction planes and a previously selected
planar face, X/Y position, angle, uniform width, opacity and OK/Cancel. The
origin-plane default is centered, longest side17.5mm; image DPI is ignored.
Opacity50% is an explicit Alloy choice, supported by a public screenshot showing
50 but not established as a current default. Independent assets and branding only.

The Browser Canvases folder offers visibility, Edit Canvas and Calibrate.
Calibration selects two distinct image points then a positive known distance;
it scales uniformly around the existing authored image center. Center preservation
is a disclosed Alloy choice. Native points derive from viewport rays intersecting
the real plane; API points use top-left image pixels. Neither is screen-distance
measurement. Native distance/placement fields explicitly use millimeters;
opacity uses percent, while API opacity is0..1.

A stable canvas UUID identifies embedded user-owned PNG, pixel dimensions,
retained plane source and authored placement/calibration. Undo/save/reopen use
existing document authority. Temporary previews preserve authored state. Missing
support is reported unavailable; geometric exporters see bodies/sketches rather
than canvas texture. Input image limits are8192 per axis,32million pixels and
32MiB embedded PNG. Texture presentation uses an in-memory OCCT GPU texture on
an actual plane and keeps aspect ratio; no temporary copied application asset.

This bounded source contract does not implement Display Through, nonuniform
scales, flips, exact drag manipulators or Render-workspace flags. Those public
options remain broader compatibility work; the source iron lesson requires
local reference placement/calibration/visibility, not their complete command
family. Exact native layout and viewport parity remain unknown from the unmatched
public dialog screenshot. This reference feature does not itself create CAD.
