# Timeline visual polish

REL-025/#454, 2026-10-15. Independently accepted by Astra source, interaction-evidence and visual review.

The timeline uses original scalable CommandIcons artwork instead of font
symbols for feature types. The container border is scoped to the timeline
itself, removing inherited borders from its children. Tiles retain the existing
44 × 34 logical-pixel targets, 4-pixel spacing and 60-pixel strip; these are Alloy
choices, not measured public-reference dimensions. Hover and keyboard-focus
feedback remain separate from history position or selection.

Existing feature identities, full name/type tooltips and accessible names,
click-to-edit behavior, split-menu actions, drag routing, active-component
filtering and horizontal scrolling remain in scope for preservation. This pass
does not add playback, rollback or a simulated history marker. The broader
history interaction differences remain separate from visual polish.

Public reference ID `feature-timeline-readability`, rechecked 2026-10-15, supports
a compact icon strip and feature descriptions. Original public URLs, diagram
and uncertainty notes remain outside Git in
`knowledge/specs/timeline-visual-polish-2026-10-15.md`. Unmatched public images
cannot establish pixel parity or precise selection semantics.

Before/after application evidence is external under
`deliverables/timeline-polish-2026-10-15/`: the saved 24-feature die and accepted
shoehorn at 1400 × 900 and 1040 × 640, inspection state and transcripts.
The existing `timelineOverflowTargets` Qt check passes three results with zero
failures, covering stable widths, overflow, editing the intended beginning,
middle and end features, Cancel preservation and Show latest. This is delivered
Qt input rather than physical pointer evidence; no geometry suite was repeated.

The focused follow-up `widget-interactions-final02.log` passes 3/3 in 15.139 s.
It checks actual style menu rectangles idle and focused against the reserved
15-pixel drag exclusion, clicks the actual hit widget to open the split menu,
and delivers Escape to the activated popup without changing authored data.
The first popup harness attempt blocked in Qt's nested menu loop and was stopped;
the passing harness schedules Escape inside that loop with a failing watchdog.
This is a harness correction, not a product workaround.

The final row vertically centers its feature content. Its menu arrow is 10 × 7
logical pixels inside the unchanged menu hit area. Smaller trial glyphs
were too faint in captures; only the arrow visual size changed after the
interaction check. Centered normal/minimum captures are under `final/die` and
`final/shoehorn`; the final arrow-only adjustment is captured in
`final-arrow10/shoehorn-normal.png`.

The focus-state geometry check explicitly sets focus as diagnostic setup; it
does not qualify natural keyboard traversal. Popup activation and Escape are
observed without focus repair. Drag preservation is source-reviewed, not a new
pointer-drag acceptance. Root and Astra inspected the final normal capture;
Astra also reviewed minimum-size die and shoehorn captures. Icons, centered row,
menu arrows and border scoping pass the bounded visual review. Generic fallback
icons remain for feature types without dedicated artwork.
