# Four-target native compile milestone

2026-10-13. CI-001 / #132 candidate. Source
`d07253470a346f1b29faa87b33fcced51416ea0c`, manually dispatched
[run 38396495940](https://github.com/rje/Alloy/actions/runs/38396495940).
All four clean jobs passed: macOS arm64, macOS x64, Windows x64 and Linux x64.
The complete matrix was dispatched once after the tutorial integrations, using
fresh dependency directories without restored caches. Historical native success
at source12fe5345 is separately preserved; no duplicate current-source matrix
was run merely to repeat successful compilation.

Root downloaded every named artifact to
`build/evidence/native-matrix-38396495940/<target>`. Checked binary hashes against
build receipts, actual architecture with `file`, compiler and required-module
configure logs, real-app build/link logs, pinned dependencies and solver
source/build contracts against the selected Git revision. Both Mac deployment
headers remain15.0/SDK15.5. Exact record and hashes are in
[the target manifest](../dependencies/native-targets.json); the old record is
preserved in native-targets-2026-10-12.json.

| Target | Result | Compiler | Binary SHA-256 prefix |
| --- | --- | --- | --- |
| macOS arm64 | compiled | AppleClang17.0.0.17000013 | 73dee2f0a37520a2 |
| macOS x64 | compiled | AppleClang17.0.0.17000013 | d6ab7236448bc72a |
| Windows x64 | compiled | MSVC19.44.35229.0 | e83f9f21c991fb92 |
| Linux x64 | compiled | GCC13.3.0 | d56af6c887ca3f6b9 |

The first Windows text-contract comparison used LF Git blob bytes and failed.
Its exact checked-out CRLF equivalents match both hashes in the build receipt;
the executable hash matched independently. No artifact was altered or check
weakened. Source-contract line endings are now explicit in the manifest limits.

ApplicationFramework and DataExchange are enabled alongside the four earlier
required OCCT modules. The same source/configure logs confirm Qt6.11.2,
OCCT7.9.3, shared PlaneGCS, Eigen5.0.1 and Boost1.90.0. All original run logs are
retained, including hosted-action Node runtime migration annotations; these did
not cause a build failure. Ordinary commits do not trigger this workflow.

This source includes the five accepted tutorial workflows, Intersect and nested
profile-region work. It predates Windows client transport #220 and Sweep #223.
The separate passing Qt-only Windows transport run38401749556 belongs to its own
receipt; it does not make this full-app compile source newer.

No remote app launch, CAD workflow, GPU hardware, runtime dependency bundle or
minimum-OS test was performed. Those flags remain false. No release/tag was made.
CI-002, platform runtime gates and FND-012 retain the remaining scope. Existing
local real-app evidence is reused; no unchanged GUI was relaunched for a compile
receipt. Independent Astra artifact/claim review is pending.
