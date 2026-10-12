# Editable primitive sample

`gauge-block.alloy` is independently authored Alloy data: a 24 × 32 × 10 mm box
and a separate radius-5, height-10 mm cylinder. It contains editable feature
intent and stable IDs, rather than a stored display mesh.

1. Launch Alloy and choose File → Open → `examples/gauge-block.alloy`.
2. Click **Gauge block** in the Timeline. Change Width, then Preview, Cancel or OK.
3. Create another Box or Cylinder from the toolbar. X/Y/Z place its origin in
   world coordinates; this first slice creates a New Body.
4. Use Cmd+Z / Cmd+Shift+Z on macOS to undo/redo. Use Ctrl+Z / Ctrl+Shift+Z on
   Windows/Linux; those platforms still require runtime qualification.
5. Try Home, Top, Front and Right; middle-mouse pan and wheel zoom.
6. Use File → Save As to preserve a copy, then reopen it and edit again.

Clicking a Timeline button edits; its arrow menu deletes the feature. Body
selection uses the actual viewport geometry. Reopening preserves editable
features but begins a new empty session undo journal.

Sketches, face/edge selection, Boolean operations, import and printing export
are outside this first hands-on slice. Disabled toolbar controls identify work
still to come.
