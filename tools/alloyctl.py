#!/usr/bin/env python3
"""Small stdlib-only client for Alloy's private local control socket."""
import argparse
import json
import os
import socket
import sys
import uuid


def request(endpoint, command, args=None, timeout=5):
    request_id = str(uuid.uuid4())
    message = {"id": request_id, "command": command, "args": args or {}}
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as client:
        client.settimeout(timeout)
        client.connect(endpoint)
        client.sendall(json.dumps(message, allow_nan=False).encode() + b"\n")
        with client.makefile("rb") as stream:
            line = stream.readline(1024 * 1024)
    if not line.endswith(b"\n"):
        raise RuntimeError("Connection closed without a complete response")
    response = json.loads(line)
    if response.get("id") != request_id:
        raise RuntimeError("Response ID does not match request")
    return response


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--socket", default=os.environ.get("ALLOY_SOCKET"), help="Endpoint from the app's startup JSON or ALLOY_SOCKET")
    parser.add_argument("--ready-file", help="Read endpoint from startup discovery JSON")
    sub = parser.add_subparsers(dest="action", required=True)
    sub.add_parser("commands", help="Discover commands and argument schemas")
    sub.add_parser("state", help="Inspect the real application state")
    execute = sub.add_parser("exec", help="Execute a discovered command")
    execute.add_argument("command")
    execute.add_argument("--args", default="{}", help="JSON object")
    capture = sub.add_parser("screenshot", help="Capture the real main window to a PNG")
    capture.add_argument("path")
    options = parser.parse_args()
    try:
        endpoint = options.socket
        if options.ready_file:
            with open(options.ready_file, encoding="utf-8") as stream:
                endpoint = json.load(stream)["socket"]
        if not endpoint:
            parser.error("Supply --socket, --ready-file, or ALLOY_SOCKET")
        if options.action == "commands": command, args = "commands.list", {}
        elif options.action == "state": command, args = "app.state", {}
        elif options.action == "screenshot": command, args = "capture.screenshot", {"path": os.path.abspath(options.path)}
        else: command, args = options.command, json.loads(options.args)
        if not isinstance(args, dict):
            raise ValueError("--args must be a JSON object")
        response = request(endpoint, command, args)
        print(json.dumps(response, indent=2))
        return 0 if response.get("ok") else 1
    except (OSError, ValueError, RuntimeError, KeyError) as error:
        print(json.dumps({"ok": False, "completion": "failed", "error": {"code": "client_error", "message": str(error)}}), file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
