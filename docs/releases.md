# Native builds and milestone releases

macOS, Windows and Linux are required Alloy targets. Milestones should produce
downloadable, independently usable builds for every declared target. The first
proposed architecture matrix is macOS arm64 and Intel x64, Windows x64, and Linux
x64. Minimum OS versions and additional architectures need explicit qualification.

## Current implementation and limits

The [Native builds workflow](../.github/workflows/native-build.yml) runs native
Release compilation on `macos-15`, `macos-15-intel`, `windows-2022`, and
`ubuntu-24.04`. It also supports manual dispatch. [tools/ci/build.py](../tools/ci/build.py)
downloads Qt 6.11.2 using a reviewed pinned aqt revision and builds exact OCCT
7.9.3 and FreeType 2.13.3 source revisions. CMake and Ninja/action versions are
pinned. Other downloader dependencies, hosted image revisions and system headers
still need the reproducibility audit in FND-009.

The initial matrix compiled Windows and both macOS architectures; Linux failed
on a missing Fontconfig development header. The pre-rewrite run and artifacts
were removed during publication cleanup. The workflow was temporarily disabled
for the rewrite and is re-enabled for a corrected run with the required Linux
Fontconfig headers. The current [matrix run](https://github.com/rje/Alloy/actions/runs/38190781624)
tests source d11ebf4; fresh complete-matrix results are pending. Artifacts carry
`unbundled` names and include compilation logs/receipts. They require development
libraries and are **not supported runnable release packages**. No tagged binary
release has been published. Only the local macOS hardware viewport is verified.
See [GPU evidence](validation/gpu-foundation.md).

CI compilation does not run the existing hardware smoke on hosted machines.
The strict GPU test remains intact. Linux Xw_Window currently requires X11/GLX;
initial Linux operation will use Qt xcb on X11/XWayland. Windows startup/client
still require named-pipe and access-control work. These are tracked requirements.

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
fail the candidate. As CAD features land, repeat all previously accepted workflows
from the packaged entrypoint on each supported OS.

macOS signing/notarization, Windows runtime/signing and installer policy, Linux
minimum distro/ABI and future AppImage/Flatpak options are open tasks. These do
not change the requirement for all three operating systems. FND-002's dependency
distribution audit remains a prerequisite for bundled public binaries; Alloy's
original code remains MIT.

The [public-source release research](research/cross-platform-releases.md) records
retrieval dates, Qt archive metadata, the verified Windows downloader discovery
fix, deployment references, and unknowns. The live [backlog](backlog/README.md)
and its GitHub links are authoritative for implementation status.
