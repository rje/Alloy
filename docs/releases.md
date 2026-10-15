# Native builds and milestone releases

macOS, Windows and Linux are required Alloy targets. Milestones should produce
downloadable, independently usable builds for every declared target. The first
proposed architecture matrix is macOS arm64 and Intel x64, Windows x64, and Linux
x64. Minimum OS versions and additional architectures need explicit qualification.

## Current implementation and limits

The [Native builds workflow](../.github/workflows/native-build.yml) runs native
Release compilation on `macos-15`, `macos-15-intel`, `windows-2022`, and
`ubuntu-24.04`. It runs only for `v*` version tags or explicit manual dispatch. Ordinary branch
commits and pull-request updates do not start the native matrix. On macOS,
[tools/ci/build.py](../tools/ci/build.py) builds the pinned Qt 6.11.2 source with
the validated accessibility patches and records the effective SDK recipe and
linked dependencies. Windows and Linux still use pinned `aqt` retrieval; CI-009
tracks their patched-runtime alignment. All targets build exact OCCT 7.9.3 and
FreeType 2.13.3 source revisions. CMake and Ninja/action versions are pinned.
FND-009 records tested compiler/image baselines and downloader dependencies;
see [native development baseline](dependencies/native-builds.md). Hosted OS
images and system headers remain rolling, with compiler drift guards.

The corrected [matrix run](https://github.com/rje/Alloy/actions/runs/38192218283)
compiled all four targets at source 12fe534; FND-009 completed the independently
reviewed toolchain/artifact audit. Later candidates need their own matrix result.
Artifacts carry
`unbundled` names and include compilation logs/receipts. They require development
libraries and are **not supported runnable release packages**. No tagged binary
release has been published. Only the local macOS hardware viewport is verified.
See [GPU evidence](validation/gpu-foundation.md) and the local
[editable primitive workflow](validation/primitives.md).

CI compilation does not run the existing hardware smoke on hosted machines.
The strict GPU test remains intact. Linux Xw_Window currently requires X11/GLX;
initial Linux operation will use Qt xcb on X11/XWayland. The bounded Windows named-pipe/access-control transport is accepted in FND-024;
FND-012 retains desktop and remaining platform qualification. Transport evidence
does not establish a working packaged GPU application.

## Milestone path

1. **Native candidates:** build every declared target from the same commit,
   retain exact toolchain/dependency/image versions and failure logs. CI-001 owns
   the complete passing matrix.
2. **Runtime packages:** CI-002 adds CMake/Qt deployment, OCCT/FreeType resources
   and shared libraries, platform plugins, control tools, notices/source-access
   inventory, and clean-host relocation. Begin with macOS `.app` ZIPs, Windows
   ZIP, and Linux tar.gz. Archive before upload to preserve Unix modes/symlinks.
3. **Hardware package gate:** GATE-005 runs installed packages on actual desktop
   GPU hosts for each target. Inspect API, physical inputs, displayed surface,
   pre-capture drawable sizing, scale/resize and screenshots. Receipts identify
   the exact package SHA. Hosted or software-rendered output stays separate.
4. **Explicit promotion:** CI-003 selects an existing version tag and applicable
   cumulative gate, checks license/package/hardware receipts and complete matrix,
   then publishes those identical tested archives to GitHub Releases using `gh`.
   Promotion must not rebuild different bytes or follow automatically from an
   issue closure. Foundation shells use honest prerelease capability notes;
   stable releases require the stable product gate.

Each published milestone carries platform archives, SHA256SUMS, source and
dependency manifests, notices/source-access materials, capability notes, and
fresh cumulative evidence. Missing platform assets or stale/mismatched receipts
fail the candidate. For a release candidate, exercise a representative sketch-to-print workflow and
an advanced assembly or surface fixture from the packaged entrypoint on each
supported OS. Reuse accepted geometry evidence and rerun changed or platform-risk
paths; do not replay every unchanged tutorial after each small edit.

macOS signing/notarization, Windows runtime/signing and installer policy, Linux
minimum distro/ABI and future AppImage/Flatpak options are open tasks. These do
not change the requirement for all three operating systems. FND-002's dependency
distribution audit remains a prerequisite for bundled public binaries; Alloy's
original code remains MIT.

The [public-source release research](research/cross-platform-releases.md) records
retrieval dates, Qt archive metadata, the verified Windows downloader discovery
fix, deployment references, and unknowns. The live [backlog](backlog/README.md)
and its GitHub links are authoritative for implementation status.

## Version cadence

Run proportionate local checks while developing. Cut an annotated semantic version
such as `v0.2.0-alpha.1` at a coherent, reviewed workflow milestone; align the
application/CMake version and record the source/capability notes before tagging.
Tag pushes intentionally start the full native matrix. Manual dispatch is reserved
for an explicit candidate/toolchain investigation. Do not start a replacement
matrix after every small commit or issue closure.

The automatic development-commit run 38199898011 was cancelled when this policy
was accepted. No version is cut by an editor/input fix alone. The earlier Qt extraction
failure was resolved by the accepted native matrix. Remaining release blockers
are runtime packaging, hardware/desktop qualification, dependency notices and
the initial-build readiness gate. CI-003 promotes the exact qualified milestone
archives without rebuilding them.
