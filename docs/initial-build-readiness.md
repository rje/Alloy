# Initial-build readiness

User-prioritized 2026-10-14. Deliver a usable, honest downloadable development
preview while continuing the enclosure, shoehorn and surface-iron tutorial
chains. This is a release-quality burndown, not a reduction of the long-term
feature contract. The canonical issues and their dependencies remain in
[the backlog](backlog/github.md); only active work belongs in In Progress.

## What the first candidate must demonstrate

A new user can launch an extracted package, navigate comfortably, create/edit a
constrained sketch and solid, use timeline/undo, save/reopen/recover committed
work, and export a correctly sized printable part. Existing advanced examples
must remain usable and responsive. Controls must be readable, selection obvious,
and refusals actionable. Platform/input claims require evidence for that route.

The current hardware-backed OpenGL renderer may serve an explicitly labelled
development preview; this does not close the Metal/Vulkan destination or the
stable-release RHI gates. Native compilation is already demonstrated, but those
unbundled binaries are not installable packages. No tag, matrix run or release
promotion follows automatically from an ordinary commit.

## Navigation checklist

The source audit found fixed views, pan, center-based wheel zoom and sketch Look
At; arbitrary orbit, ViewCube and projection controls were missing at the audit.
NAV-001 is now accepted at `2d1a699`: free orbit works with Shift-middle drag or
the visible Free Orbit tool and left drag/Escape. The focused Qt/GPU evidence
is recorded in [its receipt](validation/free-orbit-navigation.md); other navigation
children remain open except NAV-002, whose source/API/GPU camera evidence is
accepted with its native route retained in NAV-009. NAV-003 now has accepted
Qt/GPU event-routing evidence; physical trackpad delivery stays in NAV-008. Research
must distinguish current public documented behavior from older screenshots and
platform-specific gesture limitations. Detailed reference records stay external.

| User capability | Bounded issue | Acceptance focus |
| --- | --- | --- |
| Free orbit with a mouse or an explicit no-middle-button tool | [NAV-001](https://github.com/rje/Alloy/issues/407) | Shift-middle drag or visible Free Orbit + left drag; camera survives refresh/tab switch; no modeling mutation |
| Constrained orbit and pivot choice/reset | [NAV-002](https://github.com/rje/Alloy/issues/408) | Level horizon, deliberate rotation center, supported selection and reset |
| Smooth cursor-centered zoom and trackpad navigation | [NAV-003](https://github.com/rje/Alloy/issues/409) | Pixel/wheel/pinch routing, pan/modifier orbit, no duplicate gestures, documented direction settings |
| Familiar orientation cube | [NAV-004](https://github.com/rje/Alloy/issues/410) | Faces, edges, corners, cube drag, Home, roll/adjacent controls, hover and current orientation; original assets |
| Discoverable Pan/Zoom/Zoom Window/Fit/Look At controls | [NAV-005](https://github.com/rje/Alloy/issues/411) | Fit differs from Home; visible/selected geometry framing; face/sketch alignment preserves command |
| Projection modes | [NAV-006](https://github.com/rje/Alloy/issues/412) | Orthographic/perspective and documented hybrid behavior; picking and framing remain correct |
| Named views and navigation preferences | [NAV-007](https://github.com/rje/Alloy/issues/413) | Create/restore/rename/delete, Home/front/up conventions, proper document/application persistence |
| Native constrained-orbit and pivot controls | [NAV-009](https://github.com/rje/Alloy/issues/428) | Visible mode/context/pick/drag/Escape/Reset with natural focus; pending desktop activation/window binding |
| Complete mouse and no-middle-button workflow | [NAV-008](https://github.com/rje/Alloy/issues/414) | Navigate, enter/exit sketch, return to view and select; ordinary input with no focus repair; short annotated demonstration |

Publicly documented primary Mac gestures need actual runtime qualification; a
support article or a synthetic Qt event is not evidence that a physical
trackpad behaves correctly. Keep usable explicit navigation tools as a fallback.

## Interaction, presentation and data quality

| Work to burn down | Owner | What counts as done |
| --- | --- | --- |
| Keyboard/focus/selection/popup workflow | [REL-012](https://github.com/rje/Alloy/issues/415), [FND-007](https://github.com/rje/Alloy/issues/7), [SOL-016](https://github.com/rje/Alloy/issues/154), [SOL-058](https://github.com/rje/Alloy/issues/403) | One ordinary sketch-to-export flow; correct event recipient, popup closure, Escape/Return/Tab and modal return |
| Consistent theme, docks, labels and constraint/selection visibility | [REL-013](https://github.com/rje/Alloy/issues/416), [FND-011](https://github.com/rje/Alloy/issues/11) | Actual representative app captures at useful minimum size/high DPI, plus changed-control input |
| Accessible sketch editing | [FND-015](https://github.com/rje/Alloy/issues/158) | Controls and selected-row history remain accessible with the deployed runtime; current CUA binding loss remains unqualified |
| New/Open/Save As/dirty-close | [DOC-013](https://github.com/rje/Alloy/issues/25) | Cancel preserves work, path changes only after successful save, clear unsaved decisions and correct tab identity |
| Crash/interruption recovery | [REL-014](https://github.com/rje/Alloy/issues/417) under [REL-002](https://github.com/rje/Alloy/issues/81) | Recover committed authored state without overwriting original; unnamed files, recover/discard and safe version refusal |
| Reference and failed-edit handling | [REL-015](https://github.com/rje/Alloy/issues/418) under [REL-001](https://github.com/rje/Alloy/issues/80) | Missing support, ambiguous topology, invalid downstream value and stale command preserve recoverable intent with actionable diagnostics |
| Import/export file interaction | [DOC-016](https://github.com/rje/Alloy/issues/167) | Native paths/dialogs, units, mesh settings and Cancel; retain independently checked STEP/STL results |
| Responsiveness | [REL-016](https://github.com/rje/Alloy/issues/419), [REL-011](https://github.com/rje/Alloy/issues/374) | Measured current startup/navigation/selection/solve/recompute/reopen/drag on representative existing models; optimize failed budgets only |
| Capability and known-limit documentation | [REL-017](https://github.com/rje/Alloy/issues/420) under [REL-006](https://github.com/rje/Alloy/issues/85) | Current supported commands/options/platforms, sample models, install/control instructions and evidence links; remove stale blocker claims |

Existing atomic QSaveFile saving and evaluate-before-open are foundations, not
crash recovery. Existing geometry and formula tests are evidence to reuse, not a
reason to rerun every tutorial whenever an unrelated UI field changes.

## Runtime packages and release gate

| Work | Owner | Evidence |
| --- | --- | --- |
| Reproduce the actual validated dependency runtime | [CI-004](https://github.com/rje/Alloy/issues/422), [macOS recipe CI-008](https://github.com/rje/Alloy/issues/426), [Windows/Linux CI-009](https://github.com/rje/Alloy/issues/427) | Required Qt patches, OCCT, assembly/solver libraries, plugins/fonts and notices pinned; no unexplained private SDK dependency |
| Relocatable macOS archive | [CI-005](https://github.com/rje/Alloy/issues/423) | Extract elsewhere, remove developer environment, inspect loaded libraries, launch GPU/API/model workflow; advertised architectures qualified |
| Relocatable Windows archive | [CI-006](https://github.com/rje/Alloy/issues/424) | DLL/resources/control client closure and native launch; transport evidence separate from desktop/GPU |
| Relocatable Linux archive | [CI-007](https://github.com/rje/Alloy/issues/425) | ELF paths, distro/ABI and display requirements, permissions and native launch |
| Installed hardware/platform qualification | [GATE-005](https://github.com/rje/Alloy/issues/134), [FND-012](https://github.com/rje/Alloy/issues/131), [CI-002](https://github.com/rje/Alloy/issues/133) | Actual extracted package on each declared desktop/GPU host, display scaling/resize, API, physical input and output |
| Candidate review | [REL-018](https://github.com/rje/Alloy/issues/421) | Same revision/archives, representative packaged workflows, recovery, readable controls, honest limitations and independent review |
| Explicit release promotion | [CI-003](https://github.com/rje/Alloy/issues/135), [REL-003](https://github.com/rje/Alloy/issues/82) | Promote the exact qualified bytes with checksums, notices and capability notes; version/manual CI only |

Windows/Linux desktop and GPU qualification will use external testers, as chosen
by the user on 2026-10-14. Supply candidate archives and a short repeatable test
[checklist](validation/external-package-checklist.md); record OS/GPU, package
revision, results and logs. Keep qualification
pending until returned evidence supports the advertised platform. Hosted software
rendering does not substitute for these checks. Signing,
notarization and minimum supported OS details must be explicit before distribution.

## Advanced tutorial chains remain required work

1. **Electronics enclosure:** GEO-017 is now accepted at `6d9b8dc` for the exact
   inset kernel and source-derived identities. SKT-076 is accepted at `c625649`
   for the authored inset and exact Join2/selected R0.5 lineage checks. Complete
   the cumulative alignment rim ([GEO-017](https://github.com/rje/Alloy/issues/347) → [SKT-076](https://github.com/rje/Alloy/issues/346) → [GEO-020](https://github.com/rje/Alloy/issues/429) → [TUT-023](https://github.com/rje/Alloy/issues/324)), then complete [TUT-016](https://github.com/rje/Alloy/issues/299) with both printable
   halves, retained fit/tool relationships and full source-stage evidence. Reuse
   the accepted board, ports and snaps checkpoints.
2. **Surface shoehorn:** finish exact R12 corner geometry ([GEO-019](https://github.com/rje/Alloy/issues/352)), generated
   Fillet endpoint references ([GEO-018](https://github.com/rje/Alloy/issues/350)), tangent-plane/hole support ([GEO-004](https://github.com/rje/Alloy/issues/286)),
   Browser organization ([ASM-012](https://github.com/rje/Alloy/issues/343)) and [TUT-017](https://github.com/rje/Alloy/issues/301). Do not replace the specified
   finishing operation with an easier look-alike.
3. **Surface iron:** correct exterior topology/adherence ([SRF-017](https://github.com/rje/Alloy/issues/361)), then retained
   handle bridge ([SRF-015](https://github.com/rje/Alloy/issues/359)), sheet-tool Trim ([SRF-016](https://github.com/rje/Alloy/issues/360)), Boundary Fill ([SRF-011](https://github.com/rje/Alloy/issues/289)) and
   [TUT-018](https://github.com/rje/Alloy/issues/302). Keep the prepared-sketch/source-dimension adaptation explicit.
4. **Piston and sliding lid:** accepted workflows become practical responsiveness
   and package fixtures. Their broader feature-family limitations remain tracked;
   no unchanged solver/tutorial reruns merely to produce another receipt.

Packaging can progress while these modeling blockers are investigated. They are
not silently dropped from the delivery queue if a limited preview is packaged
first. A successful ingredient does not close the cumulative tutorial.

## Delivery and verification policy

Alternate a bounded readiness improvement with a concrete tutorial blocker.
The first two slices, [free orbit](https://github.com/rje/Alloy/issues/407) and
[exact enclosure inset geometry](https://github.com/rje/Alloy/issues/347), are
accepted. The [macOS runtime recipe](validation/mac-runtime-recipe.md) is also
accepted, as is [authored enclosure inset](validation/trimmed-seam-inset.md).
Constrained orbit and pivot controls have accepted source/API/GPU evidence;
[NAV-009](https://github.com/rje/Alloy/issues/428) preserves the blocked native
route. The exact paired enclosure R0.5 refusal is isolated in GEO-020; the
cumulative rim/Cut/export gate remains Todo behind it. NAV-003 smooth zoom and
Qt trackpad routing is accepted with physical-device qualification preserved in
NAV-008. GEO-020 is the active bounded diagnostic, now informed by readable
public two-chain selection evidence. Reassess after each accepted
slice; never repeat an unchanged failing kernel/input probe without a new
hypothesis. Keep one private validation editor and retire completed worktrees.

Use small core tests for changed contracts, native/automation checks through the
real command path, and inspect actual GPU output. For a candidate, exercise a
representative sketch-to-print workflow plus an advanced model on each advertised
platform. Reuse prior geometry evidence; expand checks only for a material new
risk. Astra independently verifies coherent milestones. Complete and close the
active child, preserving remaining scope in Todo, before starting the next.
