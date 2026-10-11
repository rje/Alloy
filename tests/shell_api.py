#!/usr/bin/env python3
"""Real-entrypoint desktop smoke test, not a geometry or CAD-workflow test."""
import argparse
import hashlib
import json
from pathlib import Path
import socket
import struct
import subprocess
import sys
import tempfile
import time

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from alloyctl import request


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--app", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    output = Path(args.output).resolve()
    output.mkdir(parents=True, exist_ok=True)
    transcript = []
    # Use /tmp to keep the Unix socket path below macOS's sockaddr_un limit.
    with tempfile.TemporaryDirectory(prefix="alloy-test-", dir="/tmp") as directory:
        ready = Path(directory) / "ready.json"
        endpoint = str(Path(directory) / "control.sock")
        with (output / "app.log").open("w") as log:
            process = subprocess.Popen([str(Path(args.app).resolve()), "--control-socket", endpoint, "--ready-file", str(ready)], stdout=log, stderr=subprocess.STDOUT)
            try:
                deadline = time.monotonic() + 12
                while not ready.exists():
                    if process.poll() is not None:
                        raise AssertionError(f"App exited early: {process.returncode}; see {output / 'app.log'}")
                    if time.monotonic() >= deadline:
                        raise AssertionError("Native app did not become ready")
                    time.sleep(0.05)
                assert json.loads(ready.read_text())["status"] == "ready"

                def call(command, parameters=None, expected_error=None):
                    response = request(endpoint, command, parameters)
                    transcript.append({"command": command, "args": parameters or {}, "response": response})
                    if expected_error:
                        assert not response["ok"] and response["completion"] == "failed", response
                        assert response["error"]["code"] == expected_error, response
                    else:
                        assert response["ok"] and response["completion"] == "completed", response
                    return response.get("result", {})

                initial = call("app.state")
                assert initial["stage"] == "ui-shell" and not initial["document"]["loaded"]
                assert initial["document"]["bodies"] == 0 and not initial["kernel"]["integrated"]
                commands = {entry["id"]: entry for entry in call("commands.list")["commands"]}
                assert commands["view.orientation"]["available"]
                assert not commands["solid.extrude"]["available"]
                call("view.orientation", {"name": "top"})
                call("view.grid", {"visible": False})
                call("view.origin", {"visible": False})
                call("view.zoom", {"factor": 1.5})
                call("view.pan", {"x": 32, "y": -18})
                call("layout.browser", {"visible": False})
                call("layout.timeline", {"visible": False})
                call("layout.navigation", {"visible": False})
                changed = call("app.state")
                assert changed["viewport"]["orientation"] == "top"
                assert changed["viewport"]["zoom"] == 1.5
                assert changed["viewport"]["pan"] == {"x": 32, "y": -18}
                assert not changed["viewport"]["gridVisible"] and not changed["viewport"]["originVisible"]
                assert all(not changed["ui"][key] for key in ("browserVisible", "timelineVisible", "navigationVisible"))
                assert changed["revision"] == initial["revision"] + 8
                call("commands.search")
                assert call("app.state")["ui"]["commandSearchVisible"]
                call("commands.close")
                assert not call("app.state")["ui"]["commandSearchVisible"]
                changed = call("app.state")
                altered_path = output / "alloy-shell-altered.png"
                call("capture.screenshot", {"path": str(altered_path)})

                call("solid.extrude", expected_error="not_implemented")
                call("missing.command", expected_error="unknown_command")
                call("view.orientation", {"name": "invalid"}, "invalid_arguments")
                call("view.grid", {"visible": "false"}, "invalid_arguments")
                call("view.zoom", {"factor": 0}, "invalid_arguments")
                call("view.home", {"unexpected": True}, "invalid_arguments")
                assert call("app.state")["revision"] == changed["revision"]
                # Validate parse errors through the actual server, separately from client validation.
                with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as client:
                    client.settimeout(3)
                    client.connect(endpoint)
                    client.sendall(b"not JSON\n")
                    with client.makefile("rb") as stream:
                        bad = json.loads(stream.readline())
                    assert bad["error"]["code"] == "invalid_request" and not bad["ok"]

                call("layout.reset")
                call("view.home")
                call("view.grid", {"visible": True})
                call("view.origin", {"visible": True})
                final = call("app.state")
                assert all(final["ui"][key] for key in ("browserVisible", "timelineVisible", "navigationVisible"))
                assert final["viewport"]["orientation"] == "isometric" and final["viewport"]["zoom"] == 1
                assert final["viewport"]["pan"] == {"x": 0, "y": 0}
                capture = output / "alloy-shell.png"
                info = call("capture.screenshot", {"path": str(capture)})
                png = capture.read_bytes()
                assert png[:8] == b"\x89PNG\r\n\x1a\n"
                width, height = struct.unpack(">II", png[16:24])
                assert width == info["pixelWidth"] and height == info["pixelHeight"]
                assert width >= 1040 and height >= 640
                assert png != altered_path.read_bytes(), "State changes must affect the actual captured UI"
                call("app.quit")
                assert process.wait(timeout=5) == 0
                evidence = {"result": "passed", "scope": "Native shell and local API only; no geometry or CAD workflow", "initialState": initial, "finalState": final, "capture": info, "sha256": hashlib.sha256(png).hexdigest(), "transcript": transcript}
                (output / "shell-api.json").write_text(json.dumps(evidence, indent=2) + "\n")
                print(f"PASS: native shell, shared commands, state, errors, and real PNG capture ({width}x{height})")
                print(f"Evidence: {output}")
            finally:
                if process.poll() is None:
                    process.terminate()
                    try: process.wait(timeout=3)
                    except subprocess.TimeoutExpired: process.kill(); process.wait()


if __name__ == "__main__":
    main()
