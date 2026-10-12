#!/usr/bin/env python3
"""Small stdlib-only client for Alloy's private local control socket."""
import argparse
import json
import os
import socket
import sys
import uuid
from pathlib import Path

PROTOCOL = 'alloy-control/1'
INSTANCE_DIR = Path.home() / '.alloy/instances'


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


def verify_instance(record):
    """Check the live process/window/nonce before operating a discovered endpoint."""
    if record.get('protocol') != PROTOCOL or record.get('status') != 'ready':
        raise RuntimeError('Unsupported or stale instance discovery record')
    response = request(record['socket'], 'app.identity', timeout=2)
    if not response.get('ok'):
        raise RuntimeError('Endpoint did not identify as an Alloy instance')
    actual = response['result']
    if actual.get('application') != 'Alloy' or any(actual.get(key) != record.get(key)
        for key in ('protocol', 'pid', 'instanceId', 'windowId')):
        raise RuntimeError('Stale discovery record: live instance identity does not match')
    return actual


def discover(directory=INSTANCE_DIR):
    result = []
    for path in sorted(Path(directory).glob('*.json')):
        record = {'path': str(path)}
        try:
            record.update(json.loads(path.read_text()))
            verify_instance(record)
            record['discoveryStatus'] = 'live'
        except (OSError, ValueError, RuntimeError, KeyError, TypeError) as error:
            record['discoveryStatus'] = 'stale'
            record['discoveryError'] = str(error)
        result.append(record)
    return result


def choose_endpoint(socket_path=None, ready_file=None, instance=None,
                    instance_dir=INSTANCE_DIR, environment_endpoint=None):
    if sum(value is not None for value in (socket_path, ready_file, instance)) > 1:
        raise RuntimeError('Use exactly one explicit selector: --socket, --ready-file or --instance')
    if ready_file:
        record = json.loads(Path(ready_file).read_text())
        verify_instance(record)
        return record['socket']
    if socket_path:
        return socket_path
    # An explicit UUID takes precedence over an environment fallback.
    if environment_endpoint and not instance:
        return environment_endpoint
    live = [record for record in discover(instance_dir) if record['discoveryStatus'] == 'live']
    if instance:
        live = [record for record in live if record['instanceId'] == instance]
    if len(live) != 1:
        raise RuntimeError('Choose one live instance with --instance (or supply --ready-file/--socket)')
    return live[0]['socket']


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    selectors = parser.add_mutually_exclusive_group()
    selectors.add_argument("--socket", help="Explicit endpoint; ALLOY_SOCKET is a fallback when no selector is supplied")
    selectors.add_argument("--ready-file", help="Read and verify endpoint from startup discovery JSON")
    selectors.add_argument('--instance', help='Select a live instance UUID; required when multiple instances are running')
    parser.add_argument('--instance-dir', default=str(INSTANCE_DIR), help='Discovery registry directory')
    sub = parser.add_subparsers(dest="action", required=True)
    sub.add_parser("commands", help="Discover commands and argument schemas")
    sub.add_parser("state", help="Inspect the real application state")
    sub.add_parser('instances', help='List verified live instances and stale discovery records')
    execute = sub.add_parser("exec", help="Execute a discovered command")
    execute.add_argument("command")
    execute.add_argument("--args", default="{}", help="JSON object")
    capture = sub.add_parser("screenshot", help="Capture the real main window to a PNG")
    capture.add_argument("path")
    options = parser.parse_args()
    try:
        if options.action == 'instances':
            print(json.dumps(discover(options.instance_dir), indent=2)); return 0
        endpoint = choose_endpoint(options.socket, options.ready_file, options.instance,
                                   options.instance_dir, os.environ.get('ALLOY_SOCKET'))
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
