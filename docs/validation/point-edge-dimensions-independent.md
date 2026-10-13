# Point-to-edge dimensions independent review

2026-10-13, SKT-034/#205. Independently accepted within the bounded straight-edge
contract on candidate c340ce5. No feature implementation changes by this reviewer.
Source contract: TUT-001 step05, with public provenance and supporting-line/sign
uncertainties retained in the external knowledge records referenced by
[the implementation receipt](point-edge-dimensions.md).

The reviewer inspected retained point/line/side authoring, PlaneGCS P2LDistance,
residual measurement, transient perpendicular-foot annotation layout, and shared
selection/preview/place/accept routing. A positive supporting-line distance with
retained oriented side is an explicit Alloy choice. No frozen circle coordinates
or authored synthetic witness point replace the constraint.

The existing focused Qt case was independently executed against the candidate:
`point_edge_controls_test toyCenterEdgePlacementAndValue` passed all 3 results,
0 failures, in 2.223 seconds. Actual synthesized viewport D, circle-center click,
straight model-edge click, pointer movement, separate placement click, and inline
3.9/Enter run twice. The resulting center is (3.9,11.9), radius 2.5, with two
retained point-line dimensions on opposite sides. Escape preserves the draft;
label dragging preserves geometry and changes annotation placement. This is Qt
event delivery to real widgets/GPU, not physical OS input. The test editor exited.

The delivered-viewport-key fallback is limited to the active sketch session and
routes through existing authoritative commands. Ctrl/Meta/Alt combinations are
excluded for D; text editors are not intercepted. Auto-repeat still redispatches
D, as the existing shortcut default does, and could reset a pending placement
when D is held. This is a nonblocking keyboard follow-up, not a new claimed
hold-key workflow. No general shortcut compatibility claim is made.

The reviewer reused the passing real-app API/history/storage receipt rather than
reconstructing the same fixture. Independently inspected report data retains the
same point and dimension identities while width 40→50 moves the center (35,10)→
(45,10). Cut volumes are 4858.62833058846 and 6108.628330588459 mm³, agreeing with
5000−45π and 6250−45π. The runner verifies value edit/Undo, stable projected
references, upstream rebuild, exact history and saved-file reopen. Existing core
checks supply DOF, conflict refusal and driven-distance evidence separately.

Actual API and widget captures were independently inspected: dimensions, arrows,
witnesses and palette coordinates are readable. The selected light-blue 3.9 label
partly overlaps the circle after the deliberate label drag; placement remains
editable. Captures use an isometric view, not matched public Top-view scale.
Exact pixel parity, physical OS events, other-platform runtime, broad SKT-017
coverage and a complete tutorial replay are not asserted. This bounded acceptance
supplies the previously missing direct center-to-straight-edge dimension gesture.
