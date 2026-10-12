#!/usr/bin/env python3
"""Authored-document API through the actual Alloy entrypoint; no geometry claim."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import time
import uuid
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from alloyctl import request


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', required=True)
    parser.add_argument('--output', required=True)
    parser.add_argument('--fixture', required=True)
    args = parser.parse_args()
    fixture = json.loads(Path(args.fixture).read_text())
    output = Path(args.output).resolve()
    output.mkdir(parents=True, exist_ok=True)
    transcript = []
    with tempfile.TemporaryDirectory(prefix='alloy-doc-', dir='/tmp') as directory:
        ready = Path(directory) / 'ready.json'
        with (output / 'app.log').open('w') as log:
            process = subprocess.Popen([str(Path(args.app).resolve()), '--ready-file', str(ready)], stdout=log, stderr=subprocess.STDOUT)
            try:
                deadline = time.monotonic() + 12
                while not ready.exists():
                    assert process.poll() is None, 'App exited before ready'
                    assert time.monotonic() < deadline, 'App startup timed out'
                    time.sleep(0.05)
                endpoint = json.loads(ready.read_text())['socket']
                def call(command, parameters=None, error=None):
                    response = request(endpoint, command, parameters)
                    transcript.append({'command': command, 'args': parameters or {}, 'response': response})
                    assert response['ok'] == (error is None), response
                    if error:
                        assert response['error']['code'] == error, response
                    return response.get('result', {})
                initial = call('app.state')
                original = call('document.inspect')
                assert original['loaded'] and original['features'] == 0 and original['bodies'] == 0
                assert str(uuid.UUID(original['id'])) == original['id']
                assert original['canonicalLengthUnit'] == 'mm' and original['units'] == 'mm'
                call('document.units', {'units': fixture['display_units'][0]})
                edited = call('document.inspect')
                assert edited['id'] == original['id'] and edited['units'] == 'in' and edited['dirty']
                call('document.new', {'discard': False}, 'unsaved_changes')
                call('document.units', {'units': fixture['unsupported_units']}, 'invalid_arguments')
                assert call('document.inspect') == edited
                call('document.units', {'units': fixture['display_units'][1]})
                call('document.new', {'discard': True})
                final = call('document.inspect')
                assert final['id'] != original['id'] and not final['dirty'] and final['undo']['count'] == 0
                call('app.quit')
                assert process.wait(timeout=5) == 0
                result = {'result': 'passed', 'fixtureSha256': hashlib.sha256(Path(args.fixture).read_bytes()).hexdigest(),
                          'verifiedCapabilities': ['document.identities', 'document.units', 'document.ownership_inspection'],
                          'initialState': initial, 'finalDocument': final, 'transcript': transcript,
                          'evidenceDimensions': {'api': {'status': 'passed'}, 'geometry': {'status': 'not_applicable', 'reason': 'No modeling operation is available in this case.'}}}
                (output / 'document-api.json').write_text(json.dumps(result, indent=2) + '\n')
                print('PASS: real-app authored identity, units, dirty replacement and error isolation')
            finally:
                if process.poll() is None:
                    process.terminate(); process.wait(timeout=5)


if __name__ == '__main__':
    main()
