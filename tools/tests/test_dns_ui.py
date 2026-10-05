#!/usr/bin/env python3
"""Run the actual UI snapshot function as a Node program with a DOM fixture.

Usage: python3 tools/tests/test_dns_ui.py

Like the C helper harness, this assembles a temporary test program from fixed
repository source and fixtures. It does not use JavaScript eval or a VM context.
Requires Node.js on PATH; no third-party packages or hardware are needed.
"""

from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


def extract_function(source: str) -> str:
    signature = "function snapshotNetListFromDOM()"
    start = source.find(signature)
    if start < 0:
        raise AssertionError("production UI snapshot function was not found")
    opening = source.index("{", start)
    depth = 0
    for index in range(opening, len(source)):
        if source[index] == "{":
            depth += 1
        elif source[index] == "}":
            depth -= 1
            if depth == 0:
                return source[start:index + 1]
    raise AssertionError("production UI snapshot function has no closing brace")


class DnsUiRegressionTest(unittest.TestCase):
    def test_dns_round_trip_in_both_address_modes(self) -> None:
        node = shutil.which("node")
        self.assertIsNotNone(node, "Node.js is required")
        source = (ROOT / "main/index.html").read_text(encoding="utf-8")
        fixture = (ROOT / "tools/tests/dns_ui/harness.js").read_text(encoding="utf-8")
        with tempfile.TemporaryDirectory(prefix="dns-ui-test-") as directory:
            program = Path(directory) / "test.cjs"
            program.write_text(extract_function(source) + "\n" + fixture, encoding="utf-8")
            result = subprocess.run(
                [node, str(program)], check=False, capture_output=True, text=True,
            )
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("PASS DHCP DNS persists, static DNS persists, and clearing persists", result.stdout)


if __name__ == "__main__":
    unittest.main(verbosity=2)
