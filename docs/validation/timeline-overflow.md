# Fixed Timeline targets and horizontal overflow (HIS-005)

2026-10-13 bounded candidate for #207. Public reference ID
`feature-timeline-readability` and the external
`knowledge/specs/feature-timeline-readability-slice.md`, retrieved 2026-10-13,
establish an icon strip, full operation tooltips and horizontal overflow. Exact
metrics, scrollbar policy and automatic viewport position remain unknown.
Original source URLs/images remain outside the repository.

Alloy uses independently authored existing feature glyphs in fixed 44 × 34
logical-pixel tiles, a 60-pixel strip and a horizontal scrollbar as needed.
These dimensions are Alloy choices. Complete feature name/type appear in native
tooltips and accessible names. Existing single-click edit, Pattern seed selection,
active-component filtering, stable feature IDs and split-arrow Delete binding
remain. The public double-click/selection gesture is not silently substituted
for Alloy's already-qualified single-click editing behavior.

The permanently reachable **Show latest** button scrolls only the view.
`timeline.showLatest` and `timeline.showFeature {featureId}` invoke the same
navigation; the latter requires a feature in the active component's Timeline.
`app.state.ui.timeline` reports actual tile IDs/names/types, full tooltips and
accessible names, bounds, widths, fully-visible flags and scrollbar position/range.
Authored data/history are untouched. On a new document/component view, the latest
feature is revealed; new or changed authored feature records are revealed after
layout. Ordinary presentation refreshes preserve manually scrolled position.
No model-history Move to End, evaluation marker, rollback, suppression, reorder
or grouping is introduced. Those broader requirements remain HIS-003.

## Bounded evidence

The existing saved 24-feature die is reused; no tutorial replay is needed.
`WidgetInteractions::timelineOverflowTargets` passes 3 QtTest results, zero
failures, in 12.394 seconds. At widths 1040/1400/1800 every tile remains 44 pixels
wide with the intended full name available. After scrolling, synthesized native
Qt clicks on features 0, 12 and 23 open the exact intended sketch/Sphere IDs.
Cancel preserves exact authored data. Menu inspection retains the existing Delete
action on each intended tile; its unchanged ID-bound callback was source-reviewed,
without triggering a destructive model action. The actual Show latest button
returns from the beginning to the latest feature without altering the design.
This is Qt widget input, not a claim of physical OS pointer operation.

One separate real-application runner passes shared navigation/edit/Cancel/state
checks and captures at 1040/1400/1800 × 900. All 24 IDs remain inspectable.
At 1040 the viewport is 847 pixels and horizontal overflow is 301 pixels;
at 1400 the viewport is 1207 pixels and all fixed tiles fit; at 1800 it is
1607 pixels. Each latest target is fully visible. The application exits cleanly
and releases the serialized private-editor slot.

Actual captures and inspection records are in the isolated candidate's
`build-timeline/evidence/timeline/`: `timeline-1040-latest.png`,
`timeline-1400-latest.png`, `timeline-1800-latest.png`, `timeline-1400-final.png`,
`timeline-report.json`, `transcript.json` and `app.log`. Own inspection compared
the 1400 capture with the earlier main-checkout
`build-tutorial-polish/evidence/die-final/04-complete-die.png`: repeated truncated
names are replaced by stable glyph targets, with full names retained outside
the strip. Both use the same die, isometric Home view and requested window size;
the new strip changes viewport height by six logical pixels, and save/Browser
presentation differs. This is an Alloy before/after comparison. Unmatched public
screenshots do not establish exact visual parity.

Run the focused widget case in the serialized desktop slot:

```sh
ALLOY_TIMELINE_FIXTURE=/absolute/path/game-pieces.alloy \
  build-timeline/widget_interactions_test timelineOverflowTargets
python3 tests/timeline_overflow_api.py \
  --app build-timeline/alloy.app/Contents/MacOS/alloy \
  --fixture /absolute/path/game-pieces.alloy \
  --output build-timeline/evidence/timeline
```

Astra's bounded source/capture review reports no blocker; root owns final
independent acceptance, integration and issue closure. No broad history or
cross-platform native runtime acceptance follows from this presentation slice.
