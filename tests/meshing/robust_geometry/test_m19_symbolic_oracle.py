#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import sys

REPO_ROOT = Path(sys.argv[1]).resolve()
sys.path.insert(0, str(REPO_ROOT / "tools" / "meshing_oracle"))

from exact_oracle import canonical_cases  # noqa: E402
from symbolic_oracle import symbolic_sign  # noqa: E402


def find_case(predicate: str, case_id: str):
    for case in canonical_cases()[predicate]:
        if case["id"] == case_id:
            return case
    raise KeyError(case_id)


def verify_degenerate(predicate: str, case_id: str) -> int:
    case = find_case(predicate, case_id)
    points = case["points"]
    assert isinstance(points, list)
    point_ids = list(range(1, len(points) + 1))

    sign = symbolic_sign(predicate, points, point_ids)
    if sign == 0:
        raise AssertionError(f"symbolic oracle left {predicate}/{case_id} unresolved")

    swapped_points = [list(point) for point in points]
    swapped_ids = list(point_ids)
    swapped_points[0], swapped_points[1] = swapped_points[1], swapped_points[0]
    swapped_ids[0], swapped_ids[1] = swapped_ids[1], swapped_ids[0]
    swapped_sign = symbolic_sign(predicate, swapped_points, swapped_ids)
    if swapped_sign != -sign:
        raise AssertionError(f"symbolic permutation parity failed for {predicate}")
    return sign


def main() -> int:
    o2 = verify_degenerate("orient2d", "o2_collinear")
    o3 = verify_degenerate("orient3d", "o3_coplanar")
    ic = verify_degenerate("incircle", "ic_cocircular")
    ins = verify_degenerate("insphere", "is_cospherical")

    nondegenerate = find_case("orient3d", "o3_positive_unit")
    points = nondegenerate["points"]
    assert isinstance(points, list)
    if symbolic_sign("orient3d", points, [4, 1, 3, 2]) != 1:
        raise AssertionError("symbolic oracle changed exact nonzero sign")

    duplicate = [list(points[0]), list(points[0]), list(points[2]), list(points[3])]
    try:
        symbolic_sign("orient3d", duplicate, [1, 2, 3, 4])
    except ValueError:
        pass
    else:
        raise AssertionError("symbolic oracle accepted uncanonicalized duplicate sites")

    print(
        "M1.9-B symbolic oracle PASS "
        f"orient2d={o2:+d} orient3d={o3:+d} incircle={ic:+d} insphere={ins:+d}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
