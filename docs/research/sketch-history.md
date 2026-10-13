# Sketch history and action undo

Public-source review dated 2026-10-12. Detailed evidence, URLs and provenance
are external: `knowledge/specs/sketch-history-and-timeline-2026-10-12.md`.
Source IDs SKH-001/002 identify current published timeline/sketch-edit help;
SKH-003 identifies the original engineering explanation of sketch history.

The evidence supports a sketch as one feature in the main timeline, containing
geometry, dimensions and constraints solved together. Creation order is not a
linear sketch-feature evaluation history. Editable relationships such as
patterns can retain their own parameters without needing a nested timeline.

Keep Alloy's existing separation: persistent sketch intent in the main feature
history, temporary action undo/redo while editing its draft, and evaluated
geometry as derived data. Do not turn drawing order into solver dependencies.
No nested timeline implementation is proposed for the tutorial milestone.

Current Alloy Finish commits one sketch transaction and clears the draft
journal; Cancel discards the draft. These are explicit Alloy behaviors. The
historical public explanation of abandoning edits uses Undo rather than an
ordinary Cancel command (SKH-004/005). Exact current post-Finish undo grouping,
session-journal lifetime and ordinary Cancel availability remain unresolved.
Research a public uninterrupted Finish/Undo demonstration before changing
these boundaries or claiming exact interaction parity. The broader history
work remains tracked by #43; this research does not close that issue.
