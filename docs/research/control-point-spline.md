# Control point spline and curvature comb

Source record: `sketch-slots-splines-20261013`; detailed permitted public sources,
images, mathematical design and provenance remain in the external reference
records. The public lesson authors three control poles, completes the curve,
moves its cage and enables Curvature Comb on one selected spline. It does not
specify coordinates, initial maximum degree, density or scale values.

Control point splines retain ordered pole UUIDs and a requested maximum degree
of 3 or 5. The documented maximum policy gives actual degree
`min(maximumDegree, poleCount - 1)`: three poles create an exact quadratic.
Alloy's default maximum is 5. The actual OCCT curve uses clamped uniform knots;
that knot policy is an explicit bounded Alloy choice. Cage legs are derived
presentation between authored poles, with no additional saved line geometry.
Open splines accept 2..128 poles; this increment does not add closed splines,
weights, knot editing or control-point insertion/removal workflows.

`sketch.controlSplineAdd` accepts `points` and optional `maximumDegree`.
`sketch.splineDegree` changes the pending native tool maximum. Existing
`sketch.movePoint` and atomic point dragging edit retained poles. Native Create →
Spline → Control Point Spline collects picks; Check or Enter completes, Escape
cancels pending picks or exits the tool. Selection and dragging use the same
application commands and history as the API. The edit cage has dashed brown
legs, distinct from fit spline tangent grips.

`sketch.curvatureComb` accepts a spline `entityId`, `enabled`, and optional
`density` and `scale`. An optional authored display record retains settings
when disabled and across save/history/reopen. No record is added to older
sketches without an explicit display edit. The contextual Sketch Palette
provides Curvature Comb, Density and Scale for one selected fit or control
spline. Comb changes preserve point geometry and solver DOF.

Curvature comes from exact OCCT first/second derivatives, with signed planar
curvature `(dx*ddy - dy*ddx)/speed^3`. Alloy samples the normalized curve
parameter uniformly, with density 32 and scale 1 defaults (public numeric
defaults unknown). Its gain is `curveLength^2 * scale / 4`; outward teeth use
the negative signed left normal. Gain and outward orientation are explicit
presentation policies, not an exact source formula claim. Display tooth length
is limited to half the curve length and reports clipping; reducing Scale is
available. Zero-speed samples are undefined and break the envelope; straight
curves have no invented nonzero teeth. Neither comb nor cage enters profile
geometry, constraints or bounds. Exact OCCT curve geometry supplies regions.

Inspection includes actual `degree`, retained `controlPoints`, evaluated
`curveSamples`, and `curvatureComb` settings, samples, raw signed curvature,
gain, clipping and undefined counts. Native inspection separately exposes cage
poles/legs, pending cage preview and displayed comb records. Fit spline solver
scope and free tangent diagnostics remain separate from control-pole DOF.
