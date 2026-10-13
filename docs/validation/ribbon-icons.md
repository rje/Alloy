# Original modeling ribbon icons and compact groups

2026-10-13, bounded FND-023/#218. Public source IDs `ui` and `extra-083`,
with external `knowledge/specs/interface-polish.md` and its dated screenshots,
support icon-only pinned actions above named groups, with full command names
in menus and tooltips. Public screenshot versions and dimensions are unmatched;
exact pixels and styling parity are not asserted.

`CommandIcons` draws independently authored geometry with Qt QPainter. No
reference-product assets, icon font, external raster files or new dependency
are used. The 16 existing modeling actions keep their stable command IDs and
shared execute callbacks. Buttons retain full text, accessible names and native
title/description tooltips. Create and Modify menus retain full names and now
show matching icons. Keyboard shortcut registration is untouched.

The ribbon is 94 logical pixels high, with 32-pixel artwork on 40-pixel targets.
Those metrics are Alloy choices. Hover, pressed, keyboard-focus and disabled
states remain distinct. Qt renders the vector engine at the requested device
scale. The unused local-design caption is removed, leaving space for all groups
at the application's minimum width. Contextual sketch toolbar duplication and
other interface work remain in parent #11.

`app.state.ui.ribbon` exposes actual widget bounds, names, tooltips, icon presence,
focus and visibility for real-app inspection. It does not create another command
model or change authored document state.

## Bounded verification

Actual app captures and JSON state are in `build-ribbon/evidence/ribbon/`:
blank, dimensioned-sketch, extrude-preview and solid at1040/1400 x900 logical
pixels, DPR2. All16 command buttons have nonoverlapping, fully visible bounds,
icons, complete accessible names and tooltips at both widths. The two group
menu buttons also fit. The ribbon occupies94 logical pixels, recovering13
vertical pixels compared with the prior107-pixel ribbon. Preview and sketch
docks retain readable controls. Original rounded Fillet and straight Chamfer
icons are distinct; Box/Shell and disabled Measure are distinguishable.

The existing `revolve_controls_test` passes3/3 in4400ms against this candidate,
including a real Qt click on the new Revolve icon, Profile/Axis input, angular
drag, Cancel/fresh session, Full commit and exact history/reopen. This reuses a
bounded input case; no tutorial replay or new geometry tests were needed.
`build-ribbon/evidence/widget.log` and widget PNG/JSON pairs retain the results.
The native event test and the separate shared-API capture process both exit0.
The screenshot runner initially had incorrect fixture arguments; its corrected
successful transcript is retained, with no application change required.

The private application is closed. Build uses macOS/arm64, Qt6.11.2 and OCCT7.9.3.
Keyboard focus border and menu callbacks/full labels are source-reviewed;
physical focus activation and popup rendering were not independently exercised.
Native Windows/Linux runtime remains unqualified. Astra source/public-reference
review reports no bounded blocker; final capture acceptance is pending.

## Independent acceptance

Astra accepts bounded #218 at `c8e13dd` (icon implementation `85b07ab`) on
2026-10-13. Independent review inspected the original QPainter icon source,
command wiring and widget inspection delta, external public overview and workflow
specification, all eight final captures, their real widget metadata and the
persisted 3/3 Qt result. No additional editor or modeling replay was launched.

At both 1040 and 1400 logical pixels, the four captured states retain readable
groups and a coherent icon row. Fillet/Chamfer have distinct curved/straight
corners; Shell has an opening distinct from Box, and Measure is visibly subdued.
All eight JSON states report DPR2, a 94-pixel ribbon and sixteen visible command
buttons with icons, complete names/tooltips and bounds inside the ribbon. The
actual new Revolve icon click is exercised by the reused event test, passing in
4400 ms. Labels remain available through accessibility, tooltips and named menus;
source routing still invokes the original authoritative commands.

The public overview supports icon-only actions above named groups. The artwork
is independently authored and the sizes/colors are Alloy choices; unmatched
references cannot establish exact pixel parity. Focus styling and popup labels
are source/metadata evidence, not newly exercised focus or popup-rendering proof.
The duplicate sketch toolbar, footer scaffolding and broader #11 requirements
remain separate. No physical OS input or other-platform runtime is claimed.
