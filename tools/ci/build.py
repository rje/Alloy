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
    if sys.platform != expected_os or platform.machine() != arch:
        raise RuntimeError(f'Expected {expected_os}/{arch}, observed {sys.platform}/{platform.machine()}')
    artifact = ROOT / 'build/ci-artifact'
    artifact.mkdir(parents=True, exist_ok=True)
    work = Path(os.environ.get('RUNNER_TEMP', ROOT / 'build/ci-deps')) / 'alloy-native'
    work.mkdir(parents=True, exist_ok=True)
    log_number = 0

    def run(*command, cwd=ROOT):
        nonlocal log_number
        log_number += 1
        command = list(map(str, command))
        print('Running: ' + ' '.join(command), flush=True)
        log_path = artifact / f'{log_number:02d}-{Path(command[0]).stem}.log'
        with log_path.open('w', encoding='utf-8') as log:
            log.write(json.dumps(command) + '\n')
            log.flush()
            result = subprocess.run(command, cwd=cwd, stdout=log, stderr=subprocess.STDOUT)
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

    try:
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
        options += [f'-DBUILD_MODULE_{module}=OFF' for module in
                    ('ApplicationFramework','DataExchange','Draw','DETools')]
        build(occt_source, 'occt', occt, options)
        directory = ROOT / 'build/ci-native'
        run('cmake', '-S', ROOT, '-B', directory, *generator, *shared,
            f'-DCMAKE_PREFIX_PATH={qt};{occt};{ft}', '-DBUILD_TESTING=OFF')
        run('cmake', '--build', directory, '--config', 'Release', '--parallel', '3')
        binary = directory / ('alloy.app' if sys.platform == 'darwin' else
                              'Release/alloy.exe' if sys.platform == 'win32' else 'alloy')
        if binary.is_dir():
            shutil.copytree(binary, artifact / binary.name, dirs_exist_ok=True)
            executable = binary / 'Contents/MacOS/alloy'
        else:
            shutil.copy2(binary, artifact / binary.name)
            executable = binary
        receipt = {'result':'compiled', 'target':args.target, 'observed_at':datetime.now(timezone.utc).isoformat(),
                   'source_commit':os.environ.get('GITHUB_SHA'), 'qt':QT_VERSION,
                   'occt_commit':OCCT, 'freetype_commit':FREETYPE,
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
