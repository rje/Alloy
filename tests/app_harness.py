"""Small real-entrypoint harness for feature workflows."""
from contextlib import contextmanager
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import time
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from alloyctl import request


@contextmanager
def application(app, output):
    output = Path(output)
    output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='alloy-case-') as directory:
        ready = Path(directory) / 'ready.json'
        with (output / 'app.log').open('w') as log:
            process = subprocess.Popen([str(Path(app).resolve()), '--ready-file', str(ready),
                '--instance-dir', str(Path(directory) / 'instances')], stdout=log, stderr=subprocess.STDOUT)
            try:
                deadline = time.monotonic() + 12
                while not ready.exists():
                    assert process.poll() is None, 'App exited before ready'
                    assert time.monotonic() < deadline, 'App startup timed out'
                    time.sleep(0.05)
                endpoint = json.loads(ready.read_text())['socket']
                transcript = []
                def call(command, parameters=None, error=None):
                    response = request(endpoint, command, parameters)
                    transcript.append({'command': command, 'args': parameters or {}, 'response': response})
                    assert response['ok'] == (error is None), response
                    if error:
                        assert response['error']['code'] == error, response
                    return response.get('result', {})
                yield call, transcript
                call('app.quit')
                assert process.wait(timeout=5) == 0
            finally:
                if process.poll() is None:
                    process.terminate()
                    process.wait(timeout=5)
