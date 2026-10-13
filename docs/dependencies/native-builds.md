# Native development baseline

Observed 2026-10-11 local date / 2026-10-12 UTC. All four jobs in
[run 38192218283](https://github.com/rje/Alloy/actions/runs/38192218283) clean-built
the real Release entrypoint at `12fe5345d2a165eaea54fac95cd2079b37ac37f1`.
[native-targets.json](native-targets.json) retains exact binary/configure hashes,
compiler versions, runner-image versions and source options.

| Development target | Observed compiler | Proposed runtime floor |
| --- | --- | --- |
| macOS arm64 and x64 | AppleClang 17.0.0.17000013, Xcode 16.4 / SDK 15.5 | macOS 13 |
| Windows x64 | MSVC 19.44.35229.0, VS2022 / SDK 10.0.26100.0 | Windows 10 1809 |
| Ubuntu 24.04 x64 | GCC 13.3.0 | Ubuntu 24.04, glibc 2.39, X11/GLX |

These runtime floors are candidates from the current
[Qt 6.11 support table](https://doc.qt.io/qt-6.11/supported-platforms.html)
and Alloy's Linux build host, retrieved 2026-10-11. They have not been tested as
installed Alloy runtime minima. Other architectures/distros are unqualified.
Both retained Mac executables actually declare Mach-O `minos 15.0` / SDK 15.5.
They do not demonstrate the proposed macOS 13 floor. CI-002 must set and qualify
deployment targets across Alloy, OCCT and FreeType before lower-OS packaging.
The Qt page gives different Arm/x64 archive ABI guidance; this project initially
targets the observed Ubuntu 24.04 host rather than claiming older-distro support.

All targets use Qt 6.11.2 public SDKs, exact OCCT 7.9.3 / FreeType 2.13.3 source
commits, shared libraries, CMake 3.31.6 and Ninja 1.11.1.4. The reviewed downloader
revision and its observed dependency constraints are in `tools/ci/`. The historical
run disabled ApplicationFramework, DataExchange, Draw and DETools. The current app
requires ApplicationFramework and DataExchange for its complete native dependency
graph and STEP/STL exchange; the milestone script now explicitly enables all six
required OCCT modules and disables only Draw/DETools. Optional TBB, VTK, FreeImage,
FFmpeg, RapidJSON and Draco remain disabled. Current CMake also verifies the pinned
PlaneGCS sources and Eigen/Boost headers; build receipts identify its source manifest
and build contract. These option changes are pending a new native matrix, not
retroactive claims about the historical artifacts.

On the declared native development host, install Python 3.12.10 on macOS/Windows
or 3.12.15 on Linux and the compiler above, clone Alloy, then run:

```sh
python -m pip install -c tools/ci/constraints.txt "aqtinstall @ git+https://github.com/miurahr/aqtinstall.git@076e1659807d0b362a3ed684d54c2e9c775eb9c7" "cmake==3.31.6" "ninja==1.11.1.4"
python tools/ci/build.py --target macos-arm64
# Other native hosts: macos-x64, windows-x64, linux-x64.
```

Linux additionally needs the GL/X11/font development packages listed verbatim in
the native workflow. Windows uses the Visual Studio 17 2022 x64 generator; Mac
and Linux use Ninja. A fresh work directory is required; each source checkout is
detached at its recorded revision. The build reports versions/components and
rejects compiler drift from the reviewed CI baseline. Ordinary local CMake builds
can use another C++20 compiler and report it without claiming that baseline.

`python3 tests/workflows/run.py --case FND-009 --evidence build/evidence/FND-009`
passed against downloaded, hash-checked records.
The candidate-bound [case receipt](../validation/native-baseline.json) preserves
the historical compilation/current API distinction in its machine statuses.
For the retained artifact check, a fresh checkout must first
download that named run's artifacts into the paths in the target record using
`gh run download 38192218283 --dir build/evidence/native-matrix-38192218283`.
This case inspects **prior compilation at the recorded commit** and separately
checks the current local app API. It does not compile the current local candidate
on remote machines or claim remote runtime/hardware success.

Hosted image labels, OS packages and SDK installation remain rolling. Captured
image versions plus compiler guards make drift visible; they do not make the
entire host reproducible or binaries bit-identical. Archive mirroring, full source
and package inventories, installed minimum-OS testing and relocatable runtime
bundles remain CI-002/GATE-005. CI-001 verifies the strengthened workflow and a
repeat clean matrix. No supported binary release follows from this receipt.

The uniform Python 3.12.15 pin failed setup on macOS/Windows in
[run 38195276854](https://github.com/rje/Alloy/actions/runs/38195276854);
Linux compiled successfully. The public
[Actions Python archive manifest](https://github.com/actions/python-versions/blob/main/versions-manifest.json),
retrieved 2026-10-11, lists 3.12.15 Linux archives and 3.12.10 archives for all
declared Mac/Windows architectures. The corrected matrix uses those explicit
platform pins; its new run must pass before CI-001 closes. This setup failure
does not invalidate the prior four-target compile receipt or qualify the new
primitive candidate across platforms.
