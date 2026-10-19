#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Read-only session assessment, including macOS's unlocked omitted lock key."""
import importlib.util
from pathlib import Path
import unittest

spec = importlib.util.spec_from_file_location(
    "native_input_preflight", Path(__file__).parents[1] / "tools/native_input_preflight.py")
preflight = importlib.util.module_from_spec(spec)
spec.loader.exec_module(preflight)


class NativeInputPreflightTest(unittest.TestCase):
    def assess(self, root_locked=False, **session_fields):
        session = {"kCGSSessionUserIDKey": 501, "kCGSSessionOnConsoleKey": True,
                   "kCGSessionLoginDoneKey": True, **session_fields}
        return preflight.assess({"IOConsoleLocked": root_locked,
                                 "IOConsoleUsers": [session]}, 501, "Aqua")["status"]

    def test_unlocked_console_omits_session_lock_key(self):
        self.assertEqual(self.assess(), "session-ready")

    def test_either_explicit_lock_blocks(self):
        self.assertEqual(self.assess(True), "blocked")
        self.assertEqual(self.assess(CGSSessionScreenIsLocked=True), "blocked")

    def test_unknown_root_or_malformed_session_lock_does_not_pass(self):
        self.assertEqual(self.assess(None), "unknown")
        self.assertEqual(self.assess(CGSSessionScreenIsLocked="false"), "unknown")

    def test_session_must_be_logged_in_on_console(self):
        self.assertEqual(self.assess(kCGSSessionOnConsoleKey=False), "blocked")
        self.assertEqual(self.assess(kCGSessionLoginDoneKey=False), "blocked")


if __name__ == "__main__":
    unittest.main()
