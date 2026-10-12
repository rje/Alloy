# Pinned PlaneGCS shared library

`cmake/PlaneGCS.cmake` builds `alloy_planegcs` as a separate shared library from
the thirteen exact public files in [sources.json](sources.json). The manifest
records source URLs, revision, retrieval date and SHA-256 values. Originals are
downloaded into the build directory and checked on every configure. Existing
verified downloads permit offline reconfiguration; the first configure requires
network access unless that cache is already populated.

The copied build source receives one C++20 portability change, visibly marked
with its date: `std::unreachable()` becomes `std::abort()` and `<cstdlib>` is
included. Numerical solver code is unchanged. The original files and notices
remain intact in `_deps/planegcs/original`; compiled copies are in `source`.
Independent MIT shims provide DLL export/import declarations on Windows, a
thread-safe diagnostic stream and elapsed timing. They replace only application
configuration/logging glue, not constraint equations. The private C++ library
boundary requires compatible compiler/runtime/headers when replacing the library.

Eigen **5.0.1** and Boost **1.90.0** headers are required. Matching installed
headers are selected using `ALLOY_EIGEN_INCLUDE_DIR` and
`ALLOY_BOOST_INCLUDE_DIR`. If absent or different, CMake fetches source archives
whose SHA-256 values are pinned in the build script and dependency inventory.
`-DALLOY_FETCH_SOLVER_HEADERS=OFF` instead fails with the required versions.
`EIGEN_MPL2_ONLY` is applied to the library and its consumers. No Eigen/Boost
compiled libraries are needed for this adapter.

The public Boost archive digest comes from its
[release metadata](https://archives.boost.io/release/1.90.0/source/boost_1_90_0.tar.bz2.json).
The Eigen digest was computed from the official
[5.0.1 archive](https://gitlab.com/libeigen/eigen/-/archive/5.0.1/eigen-5.0.1.tar.gz).
Both were retrieved 2026-10-11. Public source availability and version checks
do not replace a complete deployed-package inventory.

PlaneGCS and its support files retain their LGPL-2.1-or-later SPDX declarations,
copyright headers and older upstream license boilerplate. See the retained
[upstream license](../../LICENSES/PlaneGCS-LGPL-2.1-or-later.txt),
[Eigen MPL terms](../../LICENSES/MPL-2.0.txt) and
[Boost license](../../LICENSES/BSL-1.0.txt). Alloy's original adapter remains MIT.

Before distributing a runnable package, include the corresponding original and
patched source, shims/build recipe, header dependencies, copyright notices and
licenses alongside the package's source/replacement instructions. Library
replacement, platform signing and relocated runtime tests remain CI-002/GATE-005
work. An unbundled build is not package acceptance. Native Windows/Linux solver
build and runtime qualification happens at a version cut or explicit manual CI
dispatch, not each commit.
