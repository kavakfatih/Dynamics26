#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import sys

REPO_ROOT = Path(sys.argv[1]).resolve()
OUTPUT_DIR = Path(sys.argv[2]).resolve()
sys.path.insert(0, str(REPO_ROOT / "tools" / "meshing_oracle"))

from exact_oracle import (  # noqa: E402
    PREDICATE_SPECS,
    adversarial_cases,
    float_from_bits,
    generate_adversarial,
    oracle_a,
    oracle_b,
    point_bits,
    render_replay_fixture,
)


def negated(sign: int) -> int:
    return -sign if sign else 0


def verify_metamorphic(predicate: str) -> int:
    cases = adversarial_cases(predicate)
    checked = 0
    for case in cases:
        points = case["points"]
        assert isinstance(points, list)
        a = oracle_a(predicate, points)
        b = oracle_b(predicate, points)
        if a != b:
            raise AssertionError(f"dual oracle mismatch: {predicate}/{case['id']}")
        checked += 1

    base = cases[1]["points"]
    assert isinstance(base, list)
    swapped = [list(point) for point in base]
    swapped[0], swapped[1] = swapped[1], swapped[0]
    if oracle_a(predicate, swapped) != negated(oracle_a(predicate, base)):
        raise AssertionError(f"swap parity failed for {predicate}")

    dimension, _ = PREDICATE_SPECS[predicate]
    decoded = [[float_from_bits(bits) for bits in point] for point in base]

    scaled = point_bits([[value * 8.0 for value in point] for point in decoded])
    if oracle_a(predicate, scaled) != oracle_a(predicate, base):
        raise AssertionError(f"positive scale invariance failed for {predicate}")

    translation = [16.0] * dimension
    translated = point_bits([
        [value + translation[axis] for axis, value in enumerate(point)]
        for point in decoded
    ])
    if oracle_a(predicate, translated) != oracle_a(predicate, base):
        raise AssertionError(f"exact-representable translation failed for {predicate}")
    return checked


def main() -> int:
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    total = 0
    for predicate in PREDICATE_SPECS:
        total += verify_metamorphic(predicate)

    generate_adversarial(OUTPUT_DIR)

    replay_case = adversarial_cases("orient3d")[3]
    replay = OUTPUT_DIR / "replay_orient3d.d26pred"
    replay.write_text(render_replay_fixture("orient3d", replay_case), encoding="utf-8")

    if replay.read_text(encoding="utf-8").count("\n") < 8:
        raise AssertionError("replay fixture is incomplete")

    print(
        "M1.9-A adversarial PASS "
        f"cases={total} metamorphic=yes replay={replay.name}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
