#!/usr/bin/env python3
"""Build the pinned Windows/Linux Qtbase candidate; runtime qualification is separate."""
# SPDX-License-Identifier: MIT
import argparse
import json
import os
from pathlib import Path
import platform
import shutil
import subprocess
import sys
import tarfile
import urllib.request

from private_qt_recipe import ARCHIVE_SHA256, ARCHIVE_URL, VERSION, HERE, sha, tree_sha, cache, write_json

ITEMVIEW = 'src/widgets/accessible/itemviews.cpp'
ITEMVIEW_SHA256 = '1c1b20c1c4d4c0a73c22036ca656466558b015ec8eb226fd59534e82d0eba7c5'
PATCH = HERE / 'qt-itemview-cache/itemviews-accessible-cache.patch'
INPUTS = (Path(__file__).resolve(), HERE / 'private_qt_recipe.py', PATCH,
          HERE / 'qt-itemview-cache/CMakeLists.txt', HERE / 'qt-itemview-cache/tst_itemviewcache.cpp')


def inventory(root):
    return {path.relative_to(root).as_posix(): sha(path)
            for path in sorted(root.rglob('*')) if path.is_file()}


def verify(work, manifest, key):
    if manifest.get('recipe_key') != key or manifest.get('status') != 'complete':
        raise RuntimeError('Qt cache recipe mismatch; select an empty work root')
    if tree_sha(work / 'source') != manifest['source_tree'] or \
            inventory(work / 'install') != manifest['installed_files_sha256'] or \
            sha(work / 'build/CMakeCache.txt') != manifest['cmake_cache_sha256'] or \
            inventory(work / 'cache-test-evidence') != manifest['regression_evidence_sha256']:
        raise RuntimeError('Qt cache source/install/configuration/regression evidence changed')


def extract_and_patch(archive, source):
    source.mkdir(parents=True)
    # Python 3.12's data filter also rejects escaping links and absolute paths.
    with tarfile.open(archive) as stream:
        for member in stream.getmembers():
            parts = Path(member.name).parts
            if len(parts) < 2:
                continue
            member.name = str(Path(*parts[1:]))
            stream.extract(member, source, filter='data')
    subprocess.run(['git', 'apply', '--check', str(PATCH)], cwd=source, check=True)
    subprocess.run(['git', 'apply', str(PATCH)], cwd=source, check=True)
    if sha(source / ITEMVIEW) != ITEMVIEW_SHA256:
        raise RuntimeError('Patched Qt item-view source does not match the admitted input')


def configuration(target, prefix):
    options = ['-DCMAKE_BUILD_TYPE=Release', f'-DCMAKE_INSTALL_PREFIX={prefix.as_posix()}',
               '-DBUILD_SHARED_LIBS=ON', '-DQT_BUILD_TESTS=OFF', '-DQT_BUILD_EXAMPLES=OFF',
               '-DQT_BUILD_BENCHMARKS=OFF', '-DFEATURE_sql=OFF', '-DFEATURE_printsupport=OFF',
               '-DFEATURE_accessibility=ON', '-DFEATURE_widgets=ON', '-DFEATURE_itemviews=ON',
               '-DFEATURE_freetype=ON', '-DFEATURE_network=ON',
               '-DFEATURE_opengl=ON', '-DFEATURE_testlib=ON']
    if target == 'linux-x64':
        options += ['-DFEATURE_xcb=ON', '-DFEATURE_xcb_glx_plugin=ON',
                    '-DFEATURE_fontconfig=ON', '-DFEATURE_dbus=ON', '-DFEATURE_accessibility_atspi_bridge=ON']
    return options


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--target', required=True, choices=('windows-x64', 'linux-x64'))
    parser.add_argument('--work', required=True, type=Path)
    parser.add_argument('--archive', required=True, type=Path)
    parser.add_argument('--jobs', type=int, default=3)
    args = parser.parse_args()
    windows = args.target == 'windows-x64'
    if (sys.platform, platform.machine()) != (('win32', 'AMD64') if windows else ('linux', 'x86_64')):
        raise RuntimeError('Build must execute on its declared native target')
    compiler = shutil.which('cl' if windows else 'c++')
    if not compiler:
        raise RuntimeError('Native compiler missing; Windows requires an x64 MSVC developer environment')
    work, archive = args.work.resolve(), args.archive.resolve()
    prefix = work / 'install'
    options = configuration(args.target, prefix)
    compiler_version = subprocess.run([compiler] if windows else [compiler, '--version'],
                                     capture_output=True, text=True)
    recipe = {'schema': 'alloy-native-qt-input/1', 'target': args.target, 'qt_version': VERSION,
              'archive_url': ARCHIVE_URL, 'archive_sha256': ARCHIVE_SHA256,
              'input_sha256': {str(path.relative_to(HERE)): sha(path) for path in INPUTS},
              'patch_policy': {'itemview_cache': 'applied-shared-QtWidgets-source',
                               'cocoa_accessibility': 'not-applicable-macOS-only-platform-plugin'},
              'cmake_args': options, 'generator': 'Ninja', 'host': platform.platform(),
              'compiler': {'path': compiler, 'sha256': sha(compiler),
                           'version': compiler_version.stdout + compiler_version.stderr},
              'tool_versions': {tool: subprocess.check_output([tool, '--version'], text=True)
                                for tool in ('cmake', 'ninja')},
              'windows_sdk_environment': {key: os.environ.get(key) for key in
                                          ('WindowsSdkDir', 'WindowsSDKVersion', 'VCToolsInstallDir',
                                           'VCToolsVersion', 'VSCMD_ARG_TGT_ARCH')},
              'host_packages': None if windows else subprocess.check_output(
                  ['dpkg-query', '-W', '-f=${binary:Package}\t${Version}\n'], text=True)}
    import hashlib
    key = hashlib.sha256(json.dumps(recipe, sort_keys=True).encode()).hexdigest()
    manifest_path = work / 'private-qt-build.json'
    if work.exists() and any(work.iterdir()):
        if not archive.is_file() or sha(archive) != ARCHIVE_SHA256:
            raise RuntimeError('Cached Qt archive missing or changed')
        if not manifest_path.is_file():
            raise RuntimeError('Unmanifested/incomplete Qt cache; select an empty work root')
        verify(work, json.loads(manifest_path.read_text()), key)
        print(f'Verified native Qt cache: {key}')
        return
    work.mkdir(parents=True, exist_ok=True)
    write_json(work / 'private-qt-input.json', recipe | {'recipe_key': key})
    archive.parent.mkdir(parents=True, exist_ok=True)
    if not archive.exists():
        temporary = archive.with_suffix('.download')
        try:
            with urllib.request.urlopen(ARCHIVE_URL, timeout=120) as response, temporary.open('wb') as output:
                shutil.copyfileobj(response, output)
            if sha(temporary) != ARCHIVE_SHA256:
                raise RuntimeError('Downloaded Qt archive hash mismatch')
            os.replace(temporary, archive)
        finally:
            temporary.unlink(missing_ok=True)
    if sha(archive) != ARCHIVE_SHA256:
        raise RuntimeError('Qt archive hash mismatch')
    source = work / 'source'
    extract_and_patch(archive, source)
    def run(*command, env=None):
        subprocess.run(list(map(str, command)), check=True, env=env)
    run('cmake', '-S', source, '-B', work / 'build', '-G', 'Ninja', *options)
    values = cache(work / 'build/CMakeCache.txt')
    for option in options:
        name, value = option[2:].split('=', 1)
        if values.get(name) != value:
            raise RuntimeError(f'Effective Qt option differs: {name}: {values.get(name)!r} != {value!r}')
    run('cmake', '--build', work / 'build', '--parallel', args.jobs)
    run('cmake', '--install', work / 'build')
    run('cmake', '-S', HERE / 'qt-itemview-cache', '-B', work / 'cache-test', '-G', 'Ninja',
        '-DCMAKE_BUILD_TYPE=Release', f'-DCMAKE_PREFIX_PATH={prefix.as_posix()}',
        f'-DQt6_DIR={prefix.as_posix()}/lib/cmake/Qt6')
    run('cmake', '--build', work / 'cache-test', '--parallel', args.jobs)
    env = os.environ.copy()
    env.update(QT_QPA_PLATFORM='offscreen', QT_PLUGIN_PATH=str(prefix / 'plugins'))
    env['PATH'] = str(prefix / 'bin') + os.pathsep + env['PATH']
    executable = work / 'cache-test' / ('itemview_cache_test.exe' if windows else 'itemview_cache_test')
    evidence = work / 'cache-test-evidence'
    evidence.mkdir()
    run(executable, '-o', evidence / 'itemview-cache.txt,txt', '-o', evidence / 'itemview-cache.xml,junitxml', env=env)
    shutil.copy2(executable, evidence / executable.name)
    files = inventory(prefix)
    required_plugin = 'plugins/platforms/qwindows.dll' if windows else 'plugins/platforms/libqxcb.so'
    if required_plugin not in files:
        raise RuntimeError(f'Missing required native platform plugin: {required_plugin}')
    if sha(source / ITEMVIEW) != ITEMVIEW_SHA256:
        raise RuntimeError('Patched Qt source changed during build')
    manifest = {'schema': 'alloy-native-qt-build/1', 'status': 'complete', 'recipe_key': key,
                'input': recipe, 'source_tree': tree_sha(source), 'source_patched_sha256': {ITEMVIEW: ITEMVIEW_SHA256},
                'installed_files_sha256': files, 'installed_plugins_sha256': {
                    name: digest for name, digest in files.items() if name.startswith('plugins/')},
                'source_licenses_sha256': inventory(source / 'LICENSES'),
                'cmake_cache_sha256': sha(work / 'build/CMakeCache.txt'), 'effective_cmake_cache': values,
                'effective_toolchain': {path.relative_to(work / 'build').as_posix(): path.read_text()
                                        for path in sorted((work / 'build/CMakeFiles').rglob('CMake*Compiler.cmake'))},
                'qtpaths_query': subprocess.check_output([str(prefix / 'bin' / ('qtpaths.exe' if windows else 'qtpaths')), '--query'], text=True),
                'regression_evidence_sha256': inventory(evidence), 'cache_regression': 'passed-offscreen-itemview-cache',
                'runtime_loaded_tested': False, 'gpu_tested': False, 'dependencies_bundled': False}
    write_json(manifest_path, manifest)
    print(f'Native source Qt candidate complete: {key}; Alloy runtime/GPU qualification remains separate')


if __name__ == '__main__':
    main()
