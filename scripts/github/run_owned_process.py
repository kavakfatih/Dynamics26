#!/usr/bin/env python3
"""Bound one CI subprocess; never terminate another runner's application by name."""
import argparse
import os
import signal
import subprocess
import sys
import time


def run(command, timeout):
    started = time.monotonic()
    child = subprocess.Popen(command, start_new_session=True)

    def interrupted(signum, _frame):
        raise InterruptedError(signum)

    previous = {sig: signal.signal(sig, interrupted) for sig in (signal.SIGTERM, signal.SIGINT)}
    print(f"PROCESS START pid={child.pid} timeout={timeout}s", flush=True)
    try:
        try:
            status = child.wait(timeout=timeout)
            return status if status >= 0 else 128 - status
        except subprocess.TimeoutExpired:
            print(f"PROCESS TIMEOUT pid={child.pid}", file=sys.stderr, flush=True)
            return 124
        except InterruptedError as exc:
            return 128 + int(exc.args[0])
    finally:
        # Only our new process group is owned. Parallel GUI jobs and the user's
        # open Dynamics26 application must survive cleanup and cancellation.
        for sig in previous:
            signal.signal(sig, signal.SIG_IGN)
        try:
            os.killpg(child.pid, signal.SIGTERM)
        except ProcessLookupError:
            pass
        try:
            child.wait(timeout=2)
        except subprocess.TimeoutExpired:
            os.killpg(child.pid, signal.SIGKILL)
            child.wait()
        for sig, handler in previous.items():
            signal.signal(sig, handler)
        print(f"PROCESS END pid={child.pid} elapsed={time.monotonic()-started:.3f}s", flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--timeout", type=float, required=True)
    parser.add_argument("command", nargs=argparse.REMAINDER)
    args = parser.parse_args()
    command = args.command[1:] if args.command[:1] == ["--"] else args.command
    if not command or args.timeout <= 0:
        parser.error("positive timeout and executable command required")
    return run(command, args.timeout)


if __name__ == "__main__":
    sys.exit(main())
