# Linux archive preparation (not package acceptance)

2026-10-18, CI-007/#425. CI-004/#422 remains its open prerequisite. The
successful unbundled Linux x64 compile artifact `12138153247` from manual run
[`39125613075`](https://github.com/rje/Alloy/actions/runs/39125613075)
contains Alloy, its local client, manifests and notices, but not the Qt, OCCT,
FreeType or solver runtime libraries. Its `build.json` explicitly records
`dependencies_bundled: false` and `runtime_tested: false`. This artifact cannot
be repacked as a relocatable application after the CI build workspace is gone.

Once CI-004 admits the complete build/runtime input inventory, construct
CI-007's archive **inside the Linux build job**. Stage the app and helper,
`alloyctl.py`, the matching private Qt libraries and required plugins (including
`xcb`), OCCT and resources, FreeType, PlaneGCS/assembly/Ondsel libraries,
fonts, lib3mf notices and other matching licenses/source records. Add explicit
install/layout rules. Preserve symlinks and modes; include exact component,
archive and per-file hashes. Select a reviewed system-library allowlist instead
of bundling glibc, its loader or host GPU drivers.

Use relative `$ORIGIN` lookup on the executable **and each bundled ELF shared
object/plugin**; `DT_RUNPATH` is not inherited for indirect dependencies.
Reject absolute build-workspace RPATH/RUNPATH and missing `DT_NEEDED` or plugin
loads. Extract the archive at a different path with development `LD_*`/Qt
environment cleared, then inspect actual loaded paths and exercise local
control, document open/save and export. A hosted Xvfb/Mesa smoke is a
software-rendered packaging diagnostic only. External testers must separately
establish actual GPU/vendor, display scale/resize and physical input through
the [platform checklist](external-package-checklist.md).

The current proposed floor is Ubuntu 24.04 x64, glibc 2.39, X11 or XWayland
with working GLX; verify the final ABI and system-library allowlist from the
actual archive. This does not advertise other distributions or establish
hardware rendering. Public primary references retrieved 2026-10-18:
[Qt 6.11 Linux deployment](https://doc.qt.io/qt-6.11/linux-deployment.html),
[Qt Linux requirements](https://doc.qt.io/qt-6.11/linux-requirements.html),
[Qt CMake deployment](https://doc.qt.io/qt-6.11/cmake-deployment.html),
[`qt_generate_deploy_app_script`](https://doc.qt.io/qt-6.11/qt-generate-deploy-app-script.html),
and [glibc `ld.so(8)`](https://man7.org/linux/man-pages/man8/ld.so.8.html).
