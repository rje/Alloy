#!/usr/bin/env python3
"""Native compilation evidence, not runtime packaging or a CAD acceptance gate."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import shutil
import subprocess
import sys
from datetime import datetime, timezone

ROOT = Path(__file__).resolve().parents[2]
OCCT = 'a016080bf6738d6aeae020badee4e888ad1540a5'  # V7_9_3
FREETYPE = '42608f77f20749dd6ddc9e0536788eaad70ea4b5'  # VER-2-13-3
QT_VERSION = '6.11.2'
OCCT_REQUIRED_MODULES = ('FoundationClasses', 'ModelingData', 'ModelingAlgorithms',
                         'Visualization', 'ApplicationFramework', 'DataExchange')
OCCT_DISABLED_MODULES = ('Draw', 'DETools')
TARGETS = {
    'macos-arm64': ('darwin', 'arm64', 'mac', 'clang_64'),
    'macos-x64': ('darwin', 'x86_64', 'mac', 'clang_64'),
    'windows-x64': ('win32', 'AMD64', 'windows', 'win64_msvc2022_64'),
    'linux-x64': ('linux', 'x86_64', 'linux', 'linux_gcc_64'),
}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--target', choices=TARGETS, required=True)
    args = parser.parse_args()
    expected_os, arch, host, qt_arch = TARGETS[args.target]
    baseline = json.loads((ROOT / 'docs/dependencies/native-targets.json').read_text())['targets'][args.target]
    if sys.platform != expected_os or platform.machine() != arch:
        raise RuntimeError(f'Expected {expected_os}/{arch}, observed {sys.platform}/{platform.machine()}')
    artifact = ROOT / 'build/ci-artifact'
    artifact.mkdir(parents=True, exist_ok=True)
    work = Path(os.environ.get('RUNNER_TEMP', ROOT / 'build/ci-deps')) / 'alloy-native'
    work.mkdir(parents=True, exist_ok=True)
    log_number = 0

    def run(*command, cwd=ROOT, env=None):
        nonlocal log_number
        log_number += 1
        command = list(map(str, command))
        print('Running: ' + ' '.join(command), flush=True)
        log_path = artifact / f'{log_number:02d}-{Path(command[0]).stem}.log'
        with log_path.open('w', encoding='utf-8') as log:
            log.write(json.dumps(command) + '\n')
            log.flush()
            result = subprocess.run(command, cwd=cwd, env=env, stdout=log, stderr=subprocess.STDOUT)
        if result.returncode:
            print(log_path.read_text(encoding='utf-8', errors='replace')[-16000:])
            raise RuntimeError(f'Command failed ({result.returncode}); see {log_path}')

    def source(name, repository, revision):
        folder = work / name
        run('git', 'init', folder)
        run('git', '-C', folder, 'remote', 'add', 'origin', repository)
        run('git', '-C', folder, 'fetch', '--depth', '1', 'origin', revision)
        run('git', '-C', folder, 'checkout', '--detach', 'FETCH_HEAD')
        return folder

    generator = ['-G', 'Visual Studio 17 2022', '-A', 'x64'] if sys.platform == 'win32' else ['-G', 'Ninja']
    shared = ['-DCMAKE_BUILD_TYPE=Release']
    if sys.platform == 'darwin':
        shared += [f'-DCMAKE_OSX_ARCHITECTURES={arch}']

    def build(source_dir, name, prefix, options):
        directory = work / f'{name}-build'
        run('cmake', '-S', source_dir, '-B', directory, *generator, *shared,
            f'-DCMAKE_INSTALL_PREFIX={prefix}', *options)
        run('cmake', '--build', directory, '--config', 'Release', '--parallel', '3')
        run('cmake', '--install', directory, '--config', 'Release')

    def mac_link_inventory(executable, directory, qt, occt, ft):
        """Read linked development outputs; this does not prove loaded runtime closure."""
        def one(folder, pattern):
            matches = sorted({path.resolve() for path in folder.rglob(pattern) if path.is_file()})
            if len(matches) != 1:
                raise RuntimeError(f'Expected one {pattern} under {folder}, found {matches}')
            return matches[0]
        files = {
            'alloy': executable,
            'alloy_planegcs': one(directory, 'liballoy_planegcs.0.dylib'),
            'alloy_assembly_solver': one(directory, 'liballoy_assembly_solver.0.dylib'),
            'OndselSolver': one(directory, 'libOndselSolver.1.dylib'),
            'QtCore': qt / 'lib/QtCore.framework/Versions/A/QtCore',
            'QtGui': qt / 'lib/QtGui.framework/Versions/A/QtGui',
            'QtWidgets': qt / 'lib/QtWidgets.framework/Versions/A/QtWidgets',
            'QtNetwork': qt / 'lib/QtNetwork.framework/Versions/A/QtNetwork',
            'Cocoa_platform_plugin': qt / 'plugins/platforms/libqcocoa.dylib',
            'OCCT_TKernel': one(occt / 'lib', 'libTKernel*.dylib'),
            'OCCT_TKService': one(occt / 'lib', 'libTKService*.dylib'),
            'FreeType': one(ft / 'lib', 'libfreetype*.dylib'),
        }
        inventory = {}
        for name, path in files.items():
            if not path.is_file():
                raise RuntimeError(f'Missing linked dependency {name}: {path}')
            inventory[name] = {'path': str(path.resolve()),
                               'sha256': hashlib.sha256(path.read_bytes()).hexdigest(),
                               'otool_L': subprocess.check_output(['otool', '-L', str(path)], text=True)}
        if 'liballoy_assembly_solver' not in inventory['alloy']['otool_L'] or \
           'liballoy_planegcs' not in inventory['alloy']['otool_L'] or \
           'QtWidgets' not in inventory['alloy']['otool_L'] or \
           'libTK' not in inventory['alloy']['otool_L'] or \
           'OndselSolver' not in inventory['alloy_assembly_solver']['otool_L'] or \
           'freetype' not in inventory['OCCT_TKService']['otool_L'].lower():
            raise RuntimeError('Expected app/solver/OCCT/FreeType links not found')
        return inventory

    try:
        qt_manifest = None
        if sys.platform == 'darwin':
            qt_work = work / 'qt-private-runtime'
            qt_archive = work / 'qtbase-v6.11.2.tar.gz'
            qt_env = os.environ.copy()
            qt_env.update(ALLOY_PRIVATE_QT_WORK=str(qt_work), ALLOY_QTBASE_ARCHIVE=str(qt_archive))
            run('bash', ROOT / 'tools/dependencies/build-private-qt.sh', env=qt_env)
            qt = qt_work / 'install'
            qt_manifest = qt_work / 'private-qt-build.json'
            if not qt_manifest.is_file():
                raise RuntimeError('Coherent private Qt build has no success manifest')
            shutil.copy2(qt_manifest, artifact / 'private-qt-build.json')
        else:
            # CI-009 retains cross-platform source-patch parity. Stock aqt is
            # compilation evidence only and is not the validated macOS runtime.
            run(sys.executable, '-m', 'aqt', 'install-qt', host, 'desktop', QT_VERSION,
                qt_arch, '--outputdir', work / 'Qt')
            configs = list((work / 'Qt').rglob('Qt6Config.cmake'))
            if len(configs) != 1:
                raise RuntimeError(f'Expected one native Qt config, found {configs}')
            qt = configs[0].parents[3]
        ft = work / 'freetype-install'
        ft_source = source('freetype', 'https://github.com/freetype/freetype.git', FREETYPE)
        build(ft_source, 'freetype', ft, ['-DBUILD_SHARED_LIBS=ON',
            *[f'-DFT_DISABLE_{feature}=TRUE' for feature in ('ZLIB','BZIP2','PNG','HARFBUZZ','BROTLI')]])
        occt = work / 'occt-install'
        occt_source = source('occt', 'https://github.com/Open-Cascade-SAS/OCCT.git', OCCT)
        options = [f'-DINSTALL_DIR={occt}', '-DBUILD_LIBRARY_TYPE=Shared',
                   f'-D3RDPARTY_FREETYPE_DIR={ft}', '-DUSE_FREETYPE=ON', '-DUSE_OPENGL=ON',
                   '-DUSE_TBB=OFF', '-DUSE_VTK=OFF', '-DUSE_FREEIMAGE=OFF',
                   '-DUSE_FFMPEG=OFF', '-DUSE_RAPIDJSON=OFF', '-DUSE_DRACO=OFF']
        options += [f'-DBUILD_MODULE_{module}=ON' for module in OCCT_REQUIRED_MODULES]
        options += [f'-DBUILD_MODULE_{module}=OFF' for module in OCCT_DISABLED_MODULES]
        build(occt_source, 'occt', occt, options)
        directory = ROOT / 'build/ci-native'
        run('cmake', '-S', ROOT, '-B', directory, *generator, *shared,
            f'-DCMAKE_PREFIX_PATH={qt};{occt};{ft}', '-DBUILD_TESTING=OFF',
            f'-DALLOY_EXPECTED_COMPILER_ID={baseline["compiler_id"]}',
            f'-DALLOY_EXPECTED_COMPILER_VERSION={baseline["compiler_version"]}')
        run('cmake', '--build', directory, '--config', 'Release', '--parallel', '3')
        binary = directory / ('alloy.app' if sys.platform == 'darwin' else
                              'Release/alloy.exe' if sys.platform == 'win32' else 'alloy')
        if binary.is_dir():
            shutil.copytree(binary, artifact / binary.name, dirs_exist_ok=True)
            executable = binary / 'Contents/MacOS/alloy'
        else:
            shutil.copy2(binary, artifact / binary.name)
            executable = binary
        helper = directory / ('Release/alloy-local-client.exe' if sys.platform == 'win32' else 'alloy-local-client')
        shutil.copy2(helper, artifact / helper.name)
        shutil.copy2(ROOT / 'tools/alloyctl.py', artifact / 'alloyctl.py')
        link_inventory = None
        if sys.platform == 'darwin':
            link_inventory = mac_link_inventory(executable, directory, qt, occt, ft)
            (artifact / 'linked-dependencies.json').write_text(json.dumps(link_inventory, indent=2) + '\n')
        receipt = {'result':'compiled', 'target':args.target, 'observed_at':datetime.now(timezone.utc).isoformat(),
                   'source_commit':os.environ.get('GITHUB_SHA'), 'qt':QT_VERSION,
                   'qt_source': 'patched-source-macos' if qt_manifest else 'stock-aqt-unqualified',
                   'qt_build_manifest_sha256': hashlib.sha256(qt_manifest.read_bytes()).hexdigest() if qt_manifest else None,
                   'local_client_sha256':hashlib.sha256(helper.read_bytes()).hexdigest(),
                   'occt_commit':OCCT, 'freetype_commit':FREETYPE,
                   'occt_required_modules':list(OCCT_REQUIRED_MODULES),
                   'occt_disabled_modules':list(OCCT_DISABLED_MODULES),
                   'solver_sources_manifest_sha256':hashlib.sha256(
                       (ROOT / 'third_party/planegcs/sources.json').read_bytes()).hexdigest(),
                   'solver_build_contract_sha256':hashlib.sha256(
                       (ROOT / 'cmake/PlaneGCS.cmake').read_bytes()).hexdigest(),
                   'assembly_adapter_source_sha256':hashlib.sha256(
                       (ROOT / 'src/AssemblySolver.cpp').read_bytes()).hexdigest(),
                   'assembly_solver_build_contract_sha256':hashlib.sha256(
                       (ROOT / 'cmake/OndselSolver.cmake').read_bytes()).hexdigest(),
                   'assembly_solver_sources_manifest_sha256':hashlib.sha256(
                       (ROOT / 'third_party/ondsel/sources.json').read_bytes()).hexdigest(),
                   'ondsel_solver_commit':'4be80eef02a3486cda0d78f3ccbb308d207a9639',
                   'ondsel_solver_archive_sha256':'701a5b6814114d3bd358700b4308e0c75c8fe86d24400d5accde0f3c11e3580a',
                   'link_inventory_sha256':hashlib.sha256((artifact / 'linked-dependencies.json').read_bytes()).hexdigest() if link_inventory else None,
                   'compiler_baseline': {key:baseline[key] for key in ('compiler_id','compiler_version')},
                   'host':platform.platform(), 'python':platform.python_version(),
                   'runner_image':os.environ.get('ImageOS'), 'runner_image_version':os.environ.get('ImageVersion'),
                   'binary_sha256':hashlib.sha256(executable.read_bytes()).hexdigest(),
                   'runtime_tested':False, 'gpu_tested':False, 'dependencies_bundled':False}
        (artifact / 'build.json').write_text(json.dumps(receipt,indent=2)+'\n')
    finally:
        (artifact / 'README.txt').write_text(
            'Compilation evidence only. These binaries are unbundled and require development libraries.\n'
            'This artifact is not a supported milestone release or proof of runtime/GPU/CAD behavior.\n'
            'Native runtime packaging and validation remain blocking backlog tasks.\n')


if __name__ == '__main__':
    main()
