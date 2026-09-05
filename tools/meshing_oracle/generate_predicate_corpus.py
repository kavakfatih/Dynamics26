#!/usr/bin/env python3
from __future__ import annotations

import argparse
from pathlib import Path

from exact_oracle import generate_all


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate Dynamics26 exact predicate fixtures.")
    parser.add_argument("--output-dir", required=True, type=Path)
    args = parser.parse_args()
    generate_all(args.output_dir)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
