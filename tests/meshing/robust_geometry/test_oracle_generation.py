#!/usr/bin/env python3
from __future__ import annotations

import filecmp
import hashlib
from pathlib import Path
import sys
import tempfile

REPO_ROOT = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else Path(__file__).resolve().parents[3]
sys.path.insert(0, str(REPO_ROOT / "tools" / "meshing_oracle"))

from exact_oracle import (  # noqa: E402
    PREDICATE_SPECS,
    canonical_cases,
    expected_canonical_signs,
    float_from_bits,
    generate_all,
    oracle_a,
    oracle_b,
    structured_random_cross_check,
)

FIXTURE_DIR = REPO_ROOT / "tests" / "meshing" / "robust_geometry" / "fixtures"


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> int:
    expected = expected_canonical_signs()
    signed_zero_seen = False

    for predicate, cases in canonical_cases().items():
        for case in cases:
            points = case["points"]
            sign_a = oracle_a(predicate, points)
            sign_b = oracle_b(predicate, points)
            if sign_a != sign_b:
                raise AssertionError(f"oracle mismatch: {predicate}/{case['id']}")
            if case["id"] in expected and sign_a != expected[case["id"]]:
                raise AssertionError(
                    f"hand truth mismatch: {case['id']} expected={expected[case['id']]} actual={sign_a}"
                )
            for point in points:
                for bits in point:
                    if bits == 0x8000000000000000:
                        signed_zero_seen = True
                    value = float_from_bits(bits)
                    if value == 0.0 and bits not in (0, 0x8000000000000000):
                        raise AssertionError("unexpected zero encoding")

    if not signed_zero_seen:
        raise AssertionError("signed-zero fixture coverage missing")

    checked = structured_random_cross_check(cases_per_predicate=256, seed=0xD26015)

    for nonfinite in (0x7FF0000000000000, 0xFFF0000000000000, 0x7FF8000000000001):
        malformed = [[0, 0], [0, 0], [nonfinite, 0]]
        for oracle in (oracle_a, oracle_b):
            try:
                oracle("orient2d", malformed)
            except ValueError:
                pass
            else:
                raise AssertionError("oracle accepted non-finite input")

    with tempfile.TemporaryDirectory(prefix="d26-oracle-a-") as first_tmp, tempfile.TemporaryDirectory(
        prefix="d26-oracle-b-"
    ) as second_tmp:
        first = Path(first_tmp)
        second = Path(second_tmp)
        generate_all(first)
        generate_all(second)

        for predicate in PREDICATE_SPECS:
            name = f"{predicate}.d26pred"
            committed = FIXTURE_DIR / name
            if not committed.is_file():
                raise AssertionError(f"missing committed fixture: {committed}")
            if not filecmp.cmp(first / name, second / name, shallow=False):
                raise AssertionError(f"generator is not deterministic: {name}")
            if not filecmp.cmp(first / name, committed, shallow=False):
                raise AssertionError(
                    f"committed fixture is stale: {name} generated={sha256(first/name)} committed={sha256(committed)}"
                )

    print(
        "M1.6 exact-oracle PASS "
        f"predicates={len(PREDICATE_SPECS)} random_cases={checked} "
        "dual_oracle=agree fixtures=bit-exact deterministic=yes"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
