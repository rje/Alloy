#!/usr/bin/env python3
"""Inspect actual retained native compilation; current app API is separate."""
import argparse
import hashlib
import json
from pathlib import Path
from app_harness import application

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', required=True)
    parser.add_argument('--output', required=True)
    parser.add_argument('--fixture', required=True)
    args = parser.parse_args()
    fixture = json.loads(Path(args.fixture).read_text())
    record = json.loads((ROOT / fixture['record']).read_text())
    checked = {}
    for target in fixture['targets']:
        item = record['targets'][target]
        folder = ROOT / item['artifact_directory']
        for name, expected in item['checked_files_sha256'].items():
            observed = hashlib.sha256((folder / name).read_bytes()).hexdigest()
            assert observed == expected, f'{target}: changed artifact {name}'
        build = json.loads((folder / 'build.json').read_text())
        assert build['result'] == 'compiled' and build['source_commit'] == record['source_commit']
        for name in ('qt', 'occt_commit', 'freetype_commit'):
            assert build[name] == record[name]
        compiler = f"{item['compiler_id']} {item['compiler_version']}"
        assert f'The CXX compiler identification is {compiler}' in (folder / '16-cmake.log').read_text()
        assert not build['runtime_tested'] and not build['gpu_tested'] and not build['dependencies_bundled']
        checked[target] = {'source_commit':build['source_commit'], 'compiler':compiler,
            'binary_sha256':build['binary_sha256'], 'runtime_tested':False}
    with application(args.app, args.output) as (call, transcript):
        initial = call('app.state')
        assert call('app.identity')['application'] == 'Alloy'
        assert initial['runtime']['qtVersion'] == record['qt']
    result = {'result':'passed', 'fixtureSha256':hashlib.sha256(Path(args.fixture).read_bytes()).hexdigest(),
        'nativeRecordSha256':hashlib.sha256((ROOT / fixture['record']).read_bytes()).hexdigest(),
        'verifiedCapabilities':['build.native_baseline'], 'initialState':initial, 'transcript':transcript,
        'nativeBuilds':checked, 'evidenceDimensions':{'build':{'status':'not_run',
        'reason':'Inspected four prior clean native compilation artifacts at the explicitly recorded source commit; current local binary was not rebuilt by this case.'}}}
    (Path(args.output) / 'native-builds.json').write_text(json.dumps(result,indent=2)+'\n')
    print('PASS: four retained native compiler/dependency/binary records; runtime limits preserved')


if __name__ == '__main__':
    main()
