#!/usr/bin/env python3
from __future__ import annotations

import argparse
from pathlib import Path

from exact_oracle import generate_all, generate_all_with_random


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate Dynamics26 exact predicate fixtures.")
    parser.add_argument("--output-dir", required=True, type=Path)
    parser.add_argument("--random-cases", type=int, default=0)
    parser.add_argument("--seed", type=int, default=0xD26)
    args = parser.parse_args()
    if args.random_cases < 0:
        parser.error("--random-cases cannot be negative")
    if args.random_cases == 0:
        generate_all(args.output_dir)
    else:
        generate_all_with_random(args.output_dir, args.random_cases, args.seed)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
