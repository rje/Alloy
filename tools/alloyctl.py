#!/usr/bin/env python3
"""Small stdlib-only client for Alloy's private local control socket."""
import argparse
import json
import os
import socket
import sys
import uuid
import math
import subprocess
from pathlib import Path

PROTOCOL = 'alloy-control/1'
INSTANCE_DIR = Path.home() / '.alloy/instances'


class LocalEndpoint(str):
    """String-compatible endpoint carrying its verified transport."""
    def __new__(cls, address, transport):
        value = super().__new__(cls, address)
        value.transport = transport
        return value


LOCAL_PIPE_PREFIX = "\\\\.\\pipe\\"


def resolve_helper(configured=None):
    """Only caller configuration or known install/build locations can select code."""
    suffix = '.exe' if os.name == 'nt' else ''
    root = Path(__file__).resolve().parents[1]
    if configured or os.environ.get('ALLOY_LOCAL_CLIENT'):
        candidates = [Path(configured or os.environ['ALLOY_LOCAL_CLIENT'])]
    else:
        candidates = [Path(__file__).resolve().parent / ('alloy-local-client'+suffix),
                      root / ('alloy-local-client'+suffix),
                      Path(__file__).resolve().parent / 'alloy.app/Contents/MacOS/alloy-local-client',
                      root / 'build' / ('alloy-local-client'+suffix),
                      root / 'build/tools/local-control' / ('alloy-local-client'+suffix),
                      root / 'build/tools/local-control/Release' / ('alloy-local-client'+suffix),
                      root / 'build/Release' / ('alloy-local-client'+suffix)]
    for candidate in candidates:
        if candidate.is_file():
            return candidate.resolve()
    raise RuntimeError('Qt local transport helper unavailable; configure ALLOY_LOCAL_CLIENT or --transport-helper')


def exchange(endpoint, payload, timeout=30, transport=None, helper=None):
    if not math.isfinite(timeout) or timeout <= 0 or timeout*1000 > 2147483647:
        raise ValueError('Timeout must be finite and positive, at most 2147483 seconds')
    transport = transport or getattr(endpoint, 'transport', None) or ('windows-named-pipe' if os.name == 'nt' else 'unix-socket')
    if transport not in ('unix-socket', 'windows-named-pipe'):
        raise ValueError('Unsupported local transport')
    if transport == 'windows-named-pipe':
        if not isinstance(endpoint, str) or not endpoint.startswith(LOCAL_PIPE_PREFIX) or not endpoint[len(LOCAL_PIPE_PREFIX):] or any(c in endpoint[len(LOCAL_PIPE_PREFIX):] for c in '/\\'):
            raise ValueError('Only local Windows named pipes are supported')
        if os.name != 'nt':
            raise RuntimeError('Windows named pipes require a Windows client')
    if not isinstance(payload, bytes) or len(payload) > 65536 or not payload.endswith(b'\n') or b'\n' in payload[:-1]:
        raise ValueError('Request must be one newline frame at most 64 KiB')
    if transport == 'windows-named-pipe' or helper:
        executable = resolve_helper(helper)
        process = subprocess.Popen([str(executable), '--endpoint', endpoint, '--timeout-ms', str(max(1, math.ceil(timeout*1000)))],
                                   stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        try:
            output, errors = process.communicate(payload, timeout=timeout)
        except subprocess.TimeoutExpired:
            process.kill(); process.communicate()
            raise RuntimeError('Local transport deadline expired; command completion is unknown')
        if process.returncode:
            raise RuntimeError(errors.decode('utf-8', errors='replace').strip() or 'Local transport failed')
        line = output
    else:
        with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as client:
            client.settimeout(timeout)
            client.connect(endpoint)
            client.sendall(payload)
            with client.makefile('rb') as stream:
                line = stream.readline(1024*1024)
    if len(line) > 1024*1024 or not line.endswith(b'\n'):
        raise RuntimeError('Connection closed without a bounded complete response')
    return line


def request(endpoint, command, args=None, timeout=30, transport=None, helper=None):
    request_id = str(uuid.uuid4())
    message = {"id": request_id, "command": command, "args": args or {}}
    line = exchange(endpoint, json.dumps(message, allow_nan=False).encode()+b'\n', timeout, transport, helper)
    response = json.loads(line)
    if not isinstance(response, dict) or response.get('id') != request_id:
        raise RuntimeError('Response ID does not match request')
    return response


def record_endpoint(record):
    address = record.get('endpoint', record.get('socket'))
    transport = record.get('transport')
    if transport is None and os.name != 'nt':
        transport = 'unix-socket'  # Compatibility with old Unix discovery records.
    if not isinstance(address, str) or not address or transport not in ('unix-socket', 'windows-named-pipe'):
        raise RuntimeError('Unsupported discovery transport/address')
    return LocalEndpoint(address, transport), transport


def verify_instance(record, helper=None):
    """Check the live process/window/nonce before operating a discovered endpoint."""
    if record.get('protocol') != PROTOCOL or record.get('status') != 'ready':
        raise RuntimeError('Unsupported or stale instance discovery record')
    address, transport = record_endpoint(record)
    response = request(address, 'app.identity', timeout=2, transport=transport, helper=helper)
    if not response.get('ok'):
        raise RuntimeError('Endpoint did not identify as an Alloy instance')
    actual = response['result']
    if actual.get('application') != 'Alloy' or any(actual.get(key) != record.get(key)
        for key in ('protocol', 'pid', 'instanceId', 'windowId')):
        raise RuntimeError('Stale discovery record: live instance identity does not match')
    return actual


def discover(directory=INSTANCE_DIR, helper=None):
    result = []
    for path in sorted(Path(directory).glob('*.json')):
        record = {'path': str(path)}
        try:
            record.update(json.loads(path.read_text()))
            verify_instance(record, helper)
            record['discoveryStatus'] = 'live'
        except (OSError, ValueError, RuntimeError, KeyError, TypeError) as error:
            record['discoveryStatus'] = 'stale'
            record['discoveryError'] = str(error)
        result.append(record)
    return result


def choose_endpoint(socket_path=None, ready_file=None, instance=None,
                    instance_dir=INSTANCE_DIR, environment_endpoint=None, helper=None):
    if sum(value is not None for value in (socket_path, ready_file, instance)) > 1:
        raise RuntimeError('Use exactly one explicit selector: --socket, --ready-file or --instance')
    if ready_file:
        record = json.loads(Path(ready_file).read_text())
        verify_instance(record, helper)
        return record_endpoint(record)[0]
    if socket_path:
        return socket_path
    # An explicit UUID takes precedence over an environment fallback.
    if environment_endpoint and not instance:
        return environment_endpoint
    live = [record for record in discover(instance_dir, helper=helper) if record['discoveryStatus'] == 'live']
    if instance:
        live = [record for record in live if record['instanceId'] == instance]
    if len(live) != 1:
        raise RuntimeError('Choose one live instance with --instance (or supply --ready-file/--socket)')
    return record_endpoint(live[0])[0]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--transport-helper", help="Explicit trusted Qt helper path; never read from discovery")
    parser.add_argument("--timeout", type=float, default=30, help="Command response deadline in seconds (default: 30); timeout does not cancel a command")
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
            print(json.dumps(discover(options.instance_dir, helper=options.transport_helper), indent=2)); return 0
        endpoint = choose_endpoint(options.socket, options.ready_file, options.instance,
                                   options.instance_dir, os.environ.get('ALLOY_SOCKET'), helper=options.transport_helper)
        if options.action == "commands": command, args = "commands.list", {}
        elif options.action == "state": command, args = "app.state", {}
        elif options.action == "screenshot": command, args = "capture.screenshot", {"path": os.path.abspath(options.path)}
        else: command, args = options.command, json.loads(options.args)
        if not isinstance(args, dict):
            raise ValueError("--args must be a JSON object")
        if options.timeout <= 0:
            raise ValueError("--timeout must be positive")
        response = request(endpoint, command, args, timeout=options.timeout, helper=options.transport_helper)
        print(json.dumps(response, indent=2))
        return 0 if response.get("ok") else 1
    except (OSError, ValueError, RuntimeError, KeyError) as error:
        print(json.dumps({"ok": False, "completion": "failed", "error": {"code": "client_error", "message": str(error)}}), file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
