# macOS private Qt recipe and native CI provenance

CI-008 admits the macOS build recipe for the coherent patched Qt 6.11.2 SDK.
The recipe pins the public archive and the two accessibility patches, builds
Qt frameworks and plugins together, installs them, and runs the offscreen
item-view cache regression before writing a success manifest. Native CI uses
that recipe on macOS. Windows and Linux used stock `aqt` when CI-008 was
accepted; CI-009 now has an unqualified patched-source candidate awaiting its
native matrix. This is a build and dependency-provenance result, not a packaged
or loaded-runtime qualification.

The isolated clean build used
`../Alloy-dependencies/qt-private-runtime-ci008` and completed the 5/5
offscreen regression. Its `private-qt-build.json` has recipe key
`def1e74f5837364a99e4fb055514a23bd75b6a3d2dc04249178ed5b55ba70566`
and records a digest of 22,856 patched source files, 11 installed plugins,
38 source-license files, compiler/SDK identity, effective CMake features and
`qtpaths` output. An identical recipe admitted that cache; a changed
deployment target refused it. The manifest records effective system library
choices (including system zlib), so this does not claim bit-for-bit output
reproducibility or fully pinned host dependencies.

An isolated Release Alloy candidate completed 444/444 Ninja steps against this SDK.
Read-only link inspection found its Qt RPATH at the candidate installation and
verified 12 hashed app/Qt/OCCT/FreeType/solver binaries with `otool -L`. The
app was not launched for this check. The candidate still links local Homebrew
OCCT/FreeType, and no package relocation or GPU test was performed. The
existing private SDK and ordinary development build were preserved.

Exact logs and manifests are outside the repository under
`../Alloy-reference-records/deliverables/runtime-reproducibility-2026-10-14/`:
`clean-private-qt-build.log`, `cache-admission-check.log`,
`alloy-candidate-build.log`, and `linked-dependencies.json`. The isolated
success manifest is at
`../Alloy-dependencies/qt-private-runtime-ci008/private-qt-build.json`.
The standalone candidate app has SHA-256
`3e42941b9ad6e160bee7b45c417859e5d40d55c8c62ae33b30fdd8050a18a3b3`;
its link inventory has SHA-256
`d5ba2db769c4dfdceeea1e91b8672813bcabc9ac600a41cd3278b02ef5ccff33`.

The current SDK remains needed for the accepted native editor workflows.
The isolated candidate build directory and Qt candidate source/build/cache-test
directories are disposable after the manifest and evidence are archived; keep
the candidate `install` if comparing the CI recipe against a later package.
CI-002 still owns bundling, notices, loaded-library verification and clean-host
relocation. Hardware operation remains a separate gate.

After acceptance, the obsolete isolated `alloy-ci008-build` directory was
removed on 2026-10-14 (107 MB). Its logs and link inventory remain external;
the two SDKs, candidate source cache and current development build remain intact.
The historical candidate binary is no longer available at its recorded path.
