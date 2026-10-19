"""Small real-entrypoint harness for feature workflows."""
from contextlib import contextmanager
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import time
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from alloyctl import request, record_endpoint


@contextmanager
def application(app, output, *, command_timeout=30):
    output = Path(output)
    output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='alloy-case-') as directory:
        ready = Path(directory) / ('ready/ready.json' if sys.platform == 'win32' else 'ready.json')
        with (output / 'app.log').open('w') as log:
            process = subprocess.Popen([str(Path(app).resolve()), '--ready-file', str(ready),
                '--instance-dir', str(Path(directory) / 'instances')], stdout=log, stderr=subprocess.STDOUT)
            try:
                deadline = time.monotonic() + 12
                while not ready.exists():
                    assert process.poll() is None, 'App exited before ready'
                    assert time.monotonic() < deadline, 'App startup timed out'
                    time.sleep(0.05)
                endpoint, transport = record_endpoint(json.loads(ready.read_text()))
                helper = str(Path(app).resolve().parent/"alloy-local-client.exe") if sys.platform == "win32" else None
                transcript = []
                def call(command, parameters=None, error=None):
                    started = time.monotonic()
                    response = request(endpoint, command, parameters, timeout=command_timeout, transport=transport, helper=helper)
                    transcript.append({'command': command, 'args': parameters or {}, 'response': response,
                                       'elapsedMs': round((time.monotonic()-started)*1000, 1)})
                    initial_result = response.get('result', {})
                    deadline = started + command_timeout
                    while response.get('completion') == 'pending':
                        assert time.monotonic() < deadline, response
                        time.sleep(0.025)
                        response = request(endpoint, 'command.status', {'session': initial_result['id'], 'generation': initial_result['previewGeneration']},
                                           timeout=max(0.1, deadline-time.monotonic()), transport=transport, helper=helper)
                        transcript.append({'command': 'command.status', 'args': {'session': initial_result['id'], 'generation': initial_result['previewGeneration']},
                                           'response': response, 'elapsedMs': round((time.monotonic()-started)*1000, 1)})
                    assert response['ok'] == (error is None), response
                    if error:
                        assert response['error']['code'] == error, response
                    return {**initial_result, **response.get('result', {})} if initial_result else response.get('result', {})
                call.endpoint = endpoint
                call.transport = transport
                call.ready_file = ready
                call.discovery = json.loads(ready.read_text())
                yield call, transcript
                call('app.quit', {'discard':True})
                # Closing a large imported assembly can outlast the command
                # response while Qt drains its native window and worker state.
                assert process.wait(timeout=20) == 0
            finally:
                if process.poll() is None:
                    process.terminate()
                    process.wait(timeout=5)
