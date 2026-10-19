# Ordinary bracket modal gate: native readiness prerequisite

2026-10-18 diagnosis; 2026-10-19 FND-028/#589 native prerequisite accepted
locally pending independent review. REL-052/#587, macOS arm64 Release `build-preview-release`, Qt
6.11.2. This is a bounded harness diagnosis, with no application source change
and no accepted modal or OS chooser workflow.

The prior full-route driver forced `QApplication::setActiveWindow` during its
initial setup. Native AppKit observation now shows that this could leave
`NSApplication.isActive == false`, `keyWindow == nil` and neither the parent
nor export sheet a key window. The visible export window was a real attached
native sheet, could become key and had its own native first responder; Qt's
`activeModalWidget` alone therefore did not establish native input readiness.
The observed Browser focus is real, but these runs do not isolate a product
popup-to-sheet defect in an active native application.

The opt-in `ALLOY_ORDINARY_INPUT_REQUIRE_MODAL=1` route now requires an active
application, the actual top-level `QWindow` as the native focus window, and an
active top-level widget before opening the fixture or delivering user steps.
It does not set Qt's active window directly. The existing accepted prefix mode
retains its explicitly permitted initial Qt harness activation. Receipts now
record platform application activity and both native and Qt active windows.

The final strict probe stops at initial readiness with application inactive,
no native focus window, no Qt active window and no focused widget. It never
opens the fixture, displays the export sheet or writes a mesh. A LaunchServices
`.app` wrapper and a genuine `QApplication::exec` test-loop experiment also
failed initial native activation. CUA identified the private bundle as running
but could not find a native capture window (`cgWindowNotFound`). The later
unchanged prefix replay timed out in its existing initial AppleScript helper;
that setup failure is separate from the prior accepted prefix evidence.

The test target builds. The strict probe's expected failure is verified;
there is no passing full-route result. Temporary native tracing, bundle and
main-loop experiments remain external. One private editor ran at a time and
all private processes exited; user PID33805 was preserved. No focus-only
application workaround was retained.

External evidence is under
`../Alloy-reference-records/validation/rel012-ordinary-input-2026-10-18/`:
`modal-lifecycle01` (Qt/Cocoa lifecycle and inspected visible sheet capture),
`modal-native02` (exact AppKit observations and diagnostic driver),
`modal-bundle01..03`, `modal-exec01` (initial activation experiments),
`native-readiness02` (final strict driver, structured failure and capture), and
`readiness-prefix01` (later setup failure). The lifecycle capture remains
readable and its receipt reports Apple M4, Apple vendor, OpenGL 4.1 Metal and
Retina ratio 2; this does not prove native keyboard readiness or mesh output.

Keep REL-052 Todo, dependent on a finite native-session/harness prerequisite:
establish one reliably active private Alloy/test window through ordinary native
launch/initial activation, observe AppKit key/main window and Qt focus agreement,
and replay the unchanged strict route. Only then diagnose any remaining modal
handoff and complete natural field focus, Tab/Escape/Return, reopen, actual OS
Save Cancel/Save, return/next input and independent mesh readback on the exact
saved bracket. Do not repair focus between user steps. The accepted
[REL-051 prefix](ordinary-bracket-input-prefix.md) and its shared-API mesh
readback remain separate evidence.

FND-028/#589 identified the immediate session prerequisite: on 2026-10-18 the
logged-in Aqua console was locked (`IOConsoleLocked` and the current user's
`CGSSessionScreenIsLocked` were both true). The read-only
`python3 tools/native_input_preflight.py` exits 2 with that condition before an
editor is launched. Its external `native-session-preflight01` receipt contains
the exact observation and strict-gate restart command. Once the session owner
unlocks the Mac, rerun the preflight; an exit 0 only admits the subsequent
AppKit/Qt activation check and does not by itself accept modal input.

## Unlocked-session first-input qualification, 2026-10-19

The current console now reports `IOConsoleLocked=false`, Aqua, logged-in and
on-console. macOS omits the session-specific lock keys in this unlocked state.
The preflight now accepts that combination; an explicit lock at either level
still blocks, and a missing root Boolean or malformed session lock remains
unknown. Four focused Python checks cover those decisions.

The unchanged strict binary passed initial readiness and the native bracket
prefix in this session. It reached the later export modal and failed at
Tab-to-Format; that remains REL-052 and is not accepted by this prerequisite.
This changed environment supports the earlier locked-session diagnosis without
proving every historical launch experiment had the same cause. No application,
bundle metadata, permission or event-loop fix was needed.

The bounded strict first-input mode then passed 3/3 Qt checks in 2.766 seconds.
It observes AppKit directly rather than setting Qt's active-window bookkeeping:
PID 18380/window 78834 had `NSApplication.isActive=true`, the editor was the
actual key window with a first responder, and Qt reported ApplicationActive
with the same focus window. The actual-hit timeline pointer opened the sketch,
naturally focused the viewport, and C selected Circle. No focus or selection
repair occurs between these steps. The native WindowServer capture and separate
GPU framebuffer capture show the resulting Circle state and were inspected.
Qt delivers the tested pointer/key events; physical OS event injection and the
later modal/chooser workflow are separate qualification.

Finite restart from the repository root, with the sole private GUI lease:

```sh
python3 tools/native_input_preflight.py && \
ALLOY_ORDINARY_INPUT_REQUIRE_MODAL=1 \
ALLOY_ORDINARY_INPUT_INITIAL_ONLY=1 \
ALLOY_ORDINARY_INPUT_MODEL="$PWD/../Alloy-reference-records/validation/rel012-ordinary-input-2026-10-18/fullsize-failed-repro.alloy" \
ALLOY_ORDINARY_INPUT_EVIDENCE="$PWD/../Alloy-reference-records/validation/fnd028-native-readiness-2026-10-19/next-first-input" \
./build-preview-release/ordinary_bracket_input_controls_test bracketGate
```

Preflight exit 1/2 must stop the launch. Without INITIAL_ONLY, the strict route
continues into REL-052. The incremental target build passed at `-j4`; no CI ran.
The private editor closed after evidence capture; no user-owned editor was
running or modified. Exact observations, build/run logs, both captures and the
unchanged strict modal failure are external under
`../Alloy-reference-records/validation/fnd028-native-readiness-2026-10-19/`.
