# Native development baseline

All four jobs in [milestone run 38396495940](https://github.com/rje/Alloy/actions/runs/38396495940)
clean-built the real Release entrypoint at `d07253470a346f1b29faa87b33fcced51416ea0c`
on 2026-10-13. This is the accepted tutorial, profile-region and Intersect milestone;
it predates the later portable-control and Sweep integrations.
[native-targets.json](native-targets.json) retains downloaded binary/configure/build
hashes, compiler and runner-image versions, exact sources and dependency options.
These are unbundled compile outputs. They do not qualify remote desktop operation,
hardware rendering, installed minimum OS versions or distributable packages.

| Development target | Observed compiler | Proposed runtime floor |
| --- | --- | --- |
| macOS arm64 and x64 | AppleClang 17.0.0.17000013, Xcode 16.4 / SDK 15.5 | macOS 13 |
| Windows x64 | MSVC 19.44.35229.0, VS2022 / SDK 10.0.26100.0 | Windows 10 1809 |
| Ubuntu 24.04 x64 | GCC 13.3.0 | Ubuntu 24.04, glibc 2.39, X11/GLX |

The proposed floors come from the public [Qt 6.11 support table](https://doc.qt.io/qt-6.11/supported-platforms.html)
and Alloy's Linux build host, retrieved 2026-10-11. Both actual Mac executables
still declare Mach-O `minos 15.0` / SDK 15.5. They do not demonstrate macOS 13
support. CI-002 must set and qualify deployment targets across Alloy, OCCT and
FreeType before lower-OS packaging. Other architectures and distributions remain
unqualified; the Qt page's older Linux archive ABI is not an Alloy runtime claim.

## Verified build contract

All targets use Qt 6.11.2 public SDKs, exact OCCT 7.9.3 and FreeType 2.13.3 source
commits, shared libraries, CMake 3.31.6 and Ninja 1.11.1.4. Pinned actions and the
reviewed downloader revision and dependency constraints remain in `tools/ci/`.
Each job used a fresh `RUNNER_TEMP/alloy-native` directory without a dependency
cache. OCCT FoundationClasses, ModelingData, ModelingAlgorithms, Visualization,
ApplicationFramework and DataExchange are explicitly enabled; only Draw and
DETools are disabled. Optional TBB, VTK, FreeImage, FFmpeg, RapidJSON and Draco
are disabled. Configure logs confirm the app links the complete required module
set and shared PlaneGCS `ec6852e6caadf55045881a6003cc7ad038c8d2a7`, Eigen 5.0.1
and Boost 1.90.0.

Downloaded binaries match their build receipts and expected Mach-O/PE/ELF
architectures. Logs retain actual compiler identification, source options and
successful real-app link steps. Solver source-manifest/build-contract checksums
match the selected source revision. Windows receipts hash CRLF checkout bytes;
Unix receipts hash LF bytes. This explained byte difference is not source drift.
Every receipt explicitly leaves runtime, GPU and dependency-bundling flags false.

## Reproduce and inspect

Install Python 3.12.10 on macOS/Windows or 3.12.15 on Linux and the compiler above,
clone Alloy at the intended candidate revision, then run:

```sh
python -m pip install -c tools/ci/constraints.txt "aqtinstall @ git+https://github.com/miurahr/aqtinstall.git@076e1659807d0b362a3ed684d54c2e9c775eb9c7" "cmake==3.31.6" "ninja==1.11.1.4"
python tools/ci/build.py --target macos-arm64
# Other native hosts: macos-x64, windows-x64, linux-x64.
```

Linux needs the GL/X11/font development packages listed in the workflow. Windows
uses Visual Studio 17 2022 x64; Mac/Linux use Ninja. The compiler guard detects
baseline drift. Ordinary local CMake builds may use another C++20 compiler
without claiming that reviewed baseline. CI runs only on manual dispatch or
version tags; this milestone dispatched one complete matrix, not one per commit.

For each target, download the matching named artifact from run 38396495940 to
`build/evidence/native-matrix-38396495940/<target>` as recorded in the manifest.
The existing FND-009 workflow case can inspect these retained artifacts and
separately probe the current local app API. It does not compile its current local
candidate remotely. This milestone reused existing local API evidence instead
of launching another unchanged editor just to inspect compilation artifacts.

## Historical evidence and remaining work

The prior four-target run 38192218283 compiled source `12fe5345d2a165eaea54fac95cd2079b37ac37f1`.
Its exact record is preserved in [native-targets-2026-10-12.json](native-targets-2026-10-12.json);
[the historical case receipt](../validation/native-baseline.json) remains unchanged.
That earlier app did not require ApplicationFramework/DataExchange. The fresh
milestone above verifies the expanded dependency graph and current tutorial code.
The separate Python setup failure in run 38195276854 used 3.12.15 on platforms
without matching public archives; the corrected platform-specific pins pass here.

Hosted images and OS packages still roll. Compiler guards and recorded image
versions expose drift; they do not reproduce the entire host or guarantee
bit-identical binaries. Archive mirroring, complete source/package inventories,
relocatable bundles and installed minimum-OS tests remain CI-002/GATE-005.
No tag, release or supported downloadable package follows from compile acceptance.
