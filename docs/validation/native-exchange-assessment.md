# Native exchange gate assessment — #167

Assessment on2026-10-13 against main e5f102f and the existing
`build-tutorial-polish/alloy.app` binary. Existing single-solid STEP import,
mm/in export, STL refinement/encoding/manifold and independent-reader checks
remain accepted; see [STEP receipt](step-exchange.md) and
[bracket exchange](bracket-exchange.md). No translator replay was necessary.

The remaining gate requires actual File Open of a STEP source, changing units
in the STEP Export sheet, and nondefault Save As Mesh controls, native output
chooser, Cancel and independent readback. It cannot be replaced by API commands
or injected QtTest events.

The available macOS computer-use tool exposes native AX state, pointer/key/paste
input and native file sheets. A bounded plan was to bind one private current
editor, import the prior independent STEP fixture through File Open, export
inch STEP through the actual unit picker, and change mesh settings through
Save As Mesh. API inspection would compare exact authored state before/after;
independent OCCT STEPControl and mesh readers would inspect output. Existing
user apps and designs would remain untouched.

## Observed blocker

No Alloy instance was initially running in the desktop inventory. The tool
`getApp` using the full current app path launched a private editor but returned
`Computer Use server error -10005: cgWindowNotFound`; a repeated path binding
returned the same error. Binding `org.alloycad.Alloy` instead was refused as
ambiguous because multiple development bundles share that identifier.

An unchanged binary validation copy under
`../Alloy-worktrees/native-exchange/evidence/NativeExchange.app` received a unique
bundle identifier/display name only. Both its full-path and unique-identifier
bindings returned `cgWindowNotFound`. No native target was returned and no
native sheet or input action was performed. This is a computer-use binding
limitation, not evidence of an application dialog or translator failure.

The private instance's local API reported ready; its screenshot command captured
the actual QWidget chrome and OCCT OpenGL framebuffer at2880×1800 pixels,
viewport2376×1226, device pixel ratio2. Identity/state/capture are retained in
that worktree's `evidence/private-{identity,state,window}` files. API capture does
not establish native sheet accessibility. Both private launches were closed
through `app.quit(discard:true)`; no export file or authored edit was created.

Result: #167 remains unqualified for native STEP Open, unit switch and nondefault
mesh settings. Earlier native mm STEP/default STL and accepted API/independent
reader evidence remain unchanged. No app change is justified by this result.

Update 2026-10-17: the earlier window-binding limit was resolved. The
[bounded native-sheet receipt](native-exchange-sheet.md) now qualifies actual
STEP File Open, inch STEP and nondefault STL sheet controls, native Save/Cancel
and independent outputs. A further [File-menu receipt](native-exchange-menu.md)
qualifies File export entry. DOC-036/#517 retains the Browser body context-menu
path.

## Evidence relocation (2026-10-14)

The inactive checkout was retired after its commit was confirmed integrated.
Its untracked evidence, including the validation bundle, is preserved in the
external `maintenance/worktree-cleanup-2026-10-14/final-two/untracked-files.tar.gz`;
`manifest.json` maps original paths and verified SHA-256 hashes. The branch
`test/native-exchange` remains available. The paths above describe the original run.
