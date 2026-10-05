#!/usr/bin/env python3
"""Compile and run the STA DNS override regression harness.

Usage:
    python3 tools/tests/test_dns_override.py

The test extracts the production helpers from ``main/main.c``, surrounds them
with host mocks, and compiles the result with the system C compiler. This keeps
the regression test tied to the firmware implementation without requiring an
ESP-IDF build or hardware.
"""

from __future__ import annotations

import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
FIXTURES = Path(__file__).resolve().parent / "dns_override"


def extract_c_function(source: str, name: str) -> str:
    """Return one complete static C function, including its closing brace."""
    markers = (f"static void {name}(", f"static bool {name}(")
    starts = [source.find(marker) for marker in markers]
    starts = [start for start in starts if start >= 0]
    if not starts:
        raise AssertionError(f"production function {name!r} was not found")

    start = min(starts)
    opening = source.find("{", start)
    if opening < 0:
        raise AssertionError(f"production function {name!r} has no body")

    depth = 0
    for index in range(opening, len(source)):
        char = source[index]
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                return source[start : index + 1]
    raise AssertionError(f"production function {name!r} has an incomplete body")


class DnsOverrideRegressionTest(unittest.TestCase):
    def test_production_helpers_with_host_mocks(self) -> None:
        compiler = shutil.which("cc")
        self.assertIsNotNone(compiler, "a host C compiler named 'cc' is required")

        production = (ROOT / "main" / "main.c").read_text(encoding="utf-8")
        generated = "\n\n".join(
            (
                (FIXTURES / "harness_prefix.c").read_text(encoding="utf-8"),
                extract_c_function(production, "wifi_clear_fallback_dns"),
                extract_c_function(production, "wifi_apply_dns_override"),
                (FIXTURES / "harness_tests.c").read_text(encoding="utf-8"),
            )
        )

        with tempfile.TemporaryDirectory(prefix="dns-override-test-") as temp:
            temp_path = Path(temp)
            source = temp_path / "dns_override_test.c"
            executable = temp_path / "dns_override_test"
            source.write_text(generated, encoding="utf-8")

            compile_result = subprocess.run(
                [
                    compiler,
                    "-std=c11",
                    "-Wall",
                    "-Wextra",
                    "-Werror",
                    str(source),
                    "-o",
                    str(executable),
                ],
                check=False,
                capture_output=True,
                text=True,
            )
            self.assertEqual(
                compile_result.returncode,
                0,
                compile_result.stdout + compile_result.stderr,
            )

            run_result = subprocess.run(
                [str(executable)],
                check=False,
                capture_output=True,
                text=True,
            )
            self.assertEqual(
                run_result.returncode,
                0,
                run_result.stdout + run_result.stderr,
            )
            expected = (
                "PASS initial apply",
                "PASS renewal overwrite repair",
                "PASS explicit-to-inherited transition",
                "PASS no override",
                "PASS no-op",
                "PASS partial set failure",
                "PASS fallback clear failure",
            )
            for marker in expected:
                self.assertIn(marker, run_result.stdout)


if __name__ == "__main__":
    unittest.main(verbosity=2)
