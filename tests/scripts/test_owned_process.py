"""Actual subprocess lifetime tests; no mocked process signaling."""
import pathlib
import subprocess
import sys
import unittest

SCRIPT = pathlib.Path(__file__).resolve().parents[2] / "scripts/github/run_owned_process.py"


class OwnedProcessTest(unittest.TestCase):
    def invoke(self, code, timeout=3):
        return subprocess.run([sys.executable, str(SCRIPT), "--timeout", str(timeout),
                               "--", sys.executable, "-c", code], capture_output=True, text=True, timeout=8)

    def test_success_and_failure_propagate(self):
        self.assertEqual(self.invoke("raise SystemExit(0)").returncode, 0)
        self.assertEqual(self.invoke("raise SystemExit(7)").returncode, 7)

    def test_timeout_does_not_kill_unrelated_process(self):
        unrelated = subprocess.Popen([sys.executable, "-c", "import time; time.sleep(8)"])
        try:
            result = self.invoke("import time; time.sleep(8)", 0.1)
            self.assertEqual(result.returncode, 124)
            self.assertIn("PROCESS TIMEOUT", result.stderr)
            self.assertIsNone(unrelated.poll())
        finally:
            unrelated.terminate()
            unrelated.wait(timeout=3)

    def test_cancellation_reaps_owned_child(self):
        wrapper = subprocess.Popen([sys.executable, str(SCRIPT), "--timeout", "8", "--",
                                    sys.executable, "-c", "import time; time.sleep(8)"],
                                   stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        try:
            self.assertIn("PROCESS START", wrapper.stdout.readline())
            wrapper.terminate()
            output, error = wrapper.communicate(timeout=5)
            self.assertEqual(wrapper.returncode, 143, error)
            self.assertIn("PROCESS END", output)
        finally:
            if wrapper.poll() is None:
                wrapper.kill()
                wrapper.wait()


if __name__ == "__main__":
    unittest.main()
