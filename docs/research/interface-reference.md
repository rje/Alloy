# reference CAD application interface research

Retrieved **2026-10-11** from public Autodesk documentation. This is an initial,
bounded reference survey for an independently implemented, Alloy-branded product.
No reference CAD application application, installation, account, trial, remote session, private API,
or installed resource was accessed. Public API examples were read, not executed.

## Documented structure

The desktop interface reference (reference `ui`)
describes application-level controls and design tabs across the top, a workspace
selector, grouped toolbar tabs, a Browser listing design objects, the model
canvas, a ViewCube, a right-click marking menu, a navigation bar, and a timeline.
The Browser controls object visibility. The timeline represents operations and,
in parametric mode, allows editing and changing their evaluation order. Data
Panel and assistant panels are additional regions. These are workflow requirements,
not proof that a matching shell implements their behavior.

The published UI API guide (reference `extra-083`)
distinguishes workspace, tab, panel, and individual control. Panel drop-downs
contain commands; selected commands can be pinned into the visible panel.
Commands share definitions across their visible controls. Its examples place
model modifications in Modify and view controls in bottom navigation. This
supports Alloy's shared UI/API command dispatch without requiring a compatible
implementation of Autodesk's API.

## Sketch mode and command inputs

The sketch entry/edit reference (reference `sketchmode`)
documents Create Sketch from Design/Solid, then selection of an origin plane,
construction plane, or planar face. It documents a contextual Sketch toolbar,
visible constraints, blue temporary-mode emphasis, and edit access through
Browser or Timeline. Exact plane-picking behavior remains to be specified.

Sketches in reference CAD application (reference `sketch`)
documents the contextual toolbar and palette. Sketch geometry is blue when
unconstrained, black when constrained, green when fixed, and purple when
projected; unconstrained construction geometry uses orange dashes. Closed profiles
receive blue shading. Its toolbar image visibly groups Create, Modify,
Constraints, Inspect, Insert, Select, and Finish Sketch. The cyan Sketch tab and
pale cyan Finish Sketch panel distinguish the active context. These are semantic
color references; exact color values have not been established.

The Sketch Palette reference (reference `palette`)
lists contextual tool/selection options, linetype, Look At, grid, snap, slice,
profile/point/dimension/constraint visibility, projected/construction geometry,
and 3D Sketch. It does not establish every option's initial value for every
context. Alloy has no Sketch mode or palette yet.

The command-input API guide (reference `inputs`)
provides public descriptions of selection fields, unit-aware values, drop-downs,
checkboxes, groups, tables, and in-canvas manipulators. For example, selection
inputs can restrict geometry types/counts; an invalid value can disable command
acceptance. Future modeling commands need both editable input state and explicit
validation/commit/cancel behavior. The shell's local command protocol does not
claim this CAD command lifecycle yet.

## Keyboard and navigation contract

The official shortcut reference (reference `keys`)
is the source for these selected requirements:

| Context | Documented binding | Alloy shell |
| --- | --- | --- |
| Command toolbox | S | Opens command search |
| Pan / zoom | Middle-button drag / wheel | Changes reference-grid view |
| Orbit | Shift + middle-button drag | Not implemented; drag currently pans |
| Sketch | L line, R rectangle, C circle, D dimension | Not implemented |
| Solid | E extrude, F fillet, Q press/pull | Not implemented |
| Browser toggle | Ctrl+Alt+B / Cmd+Option+B | Implemented via native View menu |
| Navigation toggle | Ctrl+Alt+N / Cmd+Option+N | Implemented via native View menu |
| Layout reset | Ctrl+Alt+R / Cmd+Option+R | Implemented |
| Undo / redo | Ctrl+Z / Ctrl+Y; Cmd+Z / Cmd+Shift+Z | Not implemented |

The shortcut reference says text-entry focus suppresses shortcuts. Alloy guards
S for a focused line edit. Full shortcut/focus compatibility and physical input
behavior have not been tested. macOS menu bindings use Qt's native Ctrl-to-Command
mapping. Current smoke coverage invokes commands through the local API.

## Screenshot observations and styling choices

Three public doc images were downloaded and visually inspected. Source URLs,
checksums, dimensions, and retrieval dates are in
[reference-images/provenance.json](reference-images/provenance.json). The binary
images are local research aids, excluded from Git and app assets; their
redistribution license has not been established. Re-fetch only from those public
URLs if needed.

- The annotated desktop overview shows neutral light-gray chrome, white canvas,
  a compact top command strip, Browser at canvas left, navigation near bottom
  center, and Timeline across the bottom. Cyan accents appear in tools; magenta
  boxes/numbers are documentation annotations, not UI styling.
- The API structure image has an empty canvas and a much smaller Browser. It uses
  a different toolbar generation from the newer overview. Their differing tabs
  are evidence of version/context variation, not one exact universal layout.
- The Sketch image is only a toolbar crop. It does not establish palette position,
  canvas spacing, or the complete sketch editing layout.

Alloy's initial app bar (43 logical px), toolbar (107 px), Browser (252 px), and
Timeline (54 px) are **implementation choices**, not measured reference CAD application constants.
The shell uses system text, simple geometric glyphs, muted gray surfaces, a blue
tab accent, and an independently painted reference grid. It does not copy icons,
logos, font resources, or screenshots into its application bundle.

## Evidence limits and next comparison

No matched visual comparison has been completed. The reference image sizes,
operating system, scale, content, and product versions differ; the initial Alloy
capture is 1440×900 logical pixels with macOS device scale 2. The shell has larger
text-based controls, a fixed full-height Browser, a simple orientation label,
and no modeled object. It is a regional-layout foundation, not visual parity.

Next evidence should cover a publicly documented sketch-rectangle-to-extrude
workflow: plane choice, constraint/dimension display, Sketch Palette, feature
parameter dialog, Browser identity, and editable Timeline. First record defaults
and selection/cancel semantics from permitted documentation. Then compare Alloy
at the reference viewport size and matching state where possible. Exact fonts,
spacing, color tokens, context-menu placement, snapping tolerances, camera
sensitivity, and platform differences remain unverified.
