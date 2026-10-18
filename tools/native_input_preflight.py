#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Observe macOS console readiness before launching a private native input gate.

Exit 0 means only that the session prerequisite passes. The application must
still establish native activation/key-window and Qt focus agreement itself.
Exit 2 is a blocked session; exit 1 means readiness could not be observed.
This tool does not launch, activate, unlock, or modify any application/session.
"""

import argparse
import datetime
import json
import os
from pathlib import Path
import plistlib
import subprocess
import sys


def assess(root, uid, manager):
    sessions = [
        {
            key: session.get(key)
            for key in (
                "kCGSSessionUserIDKey",
                "kCGSSessionOnConsoleKey",
                "kCGSessionLoginDoneKey",
                "CGSSessionScreenIsLocked",
                "CGSSessionScreenLockedTime",
            )
        }
        for session in root.get("IOConsoleUsers", [])
        if session.get("kCGSSessionUserIDKey") == uid
    ]
    blockers = []
    unknowns = []
    if manager != "Aqua":
        blockers.append("The process is outside the Aqua login manager.")
    if not sessions:
        blockers.append("No console session belongs to the current user.")
    if root.get("IOConsoleLocked") is True or any(
        session["CGSSessionScreenIsLocked"] is True for session in sessions
    ):
        blockers.append("The macOS console is locked.")
    if sessions and not any(
        session["kCGSSessionOnConsoleKey"] is True
        and session["kCGSessionLoginDoneKey"] is True
        for session in sessions
    ):
        blockers.append("The current user has no logged-in on-console session.")
    if not isinstance(root.get("IOConsoleLocked"), bool):
        unknowns.append("IOConsoleLocked was not reported as a Boolean.")
    if any(not isinstance(s["CGSSessionScreenIsLocked"], bool) for s in sessions):
        unknowns.append("A current-user session lock state is unknown.")
    return {
        "status": "blocked" if blockers else "unknown" if unknowns else "session-ready",
        "consoleLocked": root.get("IOConsoleLocked"),
        "currentUserSessions": sessions,
        "blockers": blockers,
        "unknowns": unknowns,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, help="Also save the JSON observation here")
    args = parser.parse_args()
    result = {
        "schema": "alloy.native-input-session-preflight.v1",
        "observedAtUtc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "platform": sys.platform,
        "userId": os.getuid(),
        "observationOnly": True,
        "scope": "Session prerequisite only; native AppKit/Qt readiness remains required.",
        "prerequisite": "The session owner must unlock the macOS console and keep that "
        "Aqua session active, then rerun this preflight before the unchanged strict "
        "native-input gate. Do not repair focus between user steps.",
    }
    try:
        if sys.platform != "darwin":
            raise RuntimeError("This preflight only supports macOS.")
        manager = subprocess.check_output(
            ["launchctl", "managername"], text=True, timeout=3
        ).strip()
        root = plistlib.loads(subprocess.check_output(
            ["ioreg", "-n", "Root", "-d1", "-a"], timeout=3
        ))
        # macOS versions may encode the single root as a dictionary or array.
        if isinstance(root, list):
            root = root[0]
        result["launchManager"] = manager
        result.update(assess(root, os.getuid(), manager))
    except (OSError, ValueError, IndexError, TypeError, RuntimeError,
            subprocess.SubprocessError) as error:
        result.update(status="unknown", error=str(error))
    rendered = json.dumps(result, indent=2) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(rendered)
    sys.stdout.write(rendered)
    return {"session-ready": 0, "blocked": 2, "unknown": 1}[result["status"]]


if __name__ == "__main__":
    sys.exit(main())
