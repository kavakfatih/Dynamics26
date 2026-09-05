from __future__ import annotations

from fractions import Fraction
from itertools import permutations
import math
import random
import struct
from typing import Iterable, Sequence, Union

PREDICATE_SPECS = {
    "orient2d": (2, 3),
    "orient3d": (3, 4),
    "incircle": (2, 4),
    "insphere": (3, 5),
}
GENERATOR_VERSION = "m1.6-1"


def bits_from_float(value: float) -> int:
    return struct.unpack(">Q", struct.pack(">d", float(value)))[0]


def float_from_bits(bits: int) -> float:
    if not 0 <= bits <= 0xFFFFFFFFFFFFFFFF:
        raise ValueError("binary64 bits out of range")
    return struct.unpack(">d", struct.pack(">Q", bits))[0]


def fraction_from_bits(bits: int) -> Fraction:
    value = float_from_bits(bits)
    if not math.isfinite(value):
        raise ValueError("predicate coordinates must be finite")
    numerator, denominator = value.as_integer_ratio()
    return Fraction(numerator, denominator)


def dyadic_from_bits(bits: int) -> tuple[int, int]:
    """Return exact (integer significand, exponent) with value = n * 2**e."""
    sign_value = -1 if (bits >> 63) else 1
    exponent_bits = (bits >> 52) & 0x7FF
    fraction_bits = bits & ((1 << 52) - 1)
    if exponent_bits == 0x7FF:
        raise ValueError("predicate coordinates must be finite")
    if exponent_bits == 0:
        return sign_value * fraction_bits, -1074
    significand = (1 << 52) | fraction_bits
    exponent = int(exponent_bits) - 1023 - 52
    return sign_value * significand, exponent


def sign(value: Union[int, Fraction]) -> int:
    return (value > 0) - (value < 0)


def _det_fraction(matrix: Sequence[Sequence[Fraction]]) -> Fraction:
    size = len(matrix)
    if size == 0 or any(len(row) != size for row in matrix):
        raise ValueError("determinant matrix must be non-empty and square")
    if size == 1:
        return matrix[0][0]
    total = Fraction(0)
    for column in range(size):
        minor = [list(row[:column]) + list(row[column + 1 :]) for row in matrix[1:]]
        total += (1 if column % 2 == 0 else -1) * matrix[0][column] * _det_fraction(minor)
    return total


def _permutation_sign(order: Sequence[int]) -> int:
    inversions = 0
    for i in range(len(order)):
        for j in range(i + 1, len(order)):
            inversions += order[i] > order[j]
    return -1 if inversions % 2 else 1


def _det_integer_permutations(matrix: Sequence[Sequence[int]]) -> int:
    size = len(matrix)
    if size == 0 or any(len(row) != size for row in matrix):
        raise ValueError("determinant matrix must be non-empty and square")
    total = 0
    for order in permutations(range(size)):
        term = _permutation_sign(order)
        for row, column in enumerate(order):
            term *= matrix[row][column]
        total += term
    return total


def _validate_shape(predicate: str, points_bits: Sequence[Sequence[int]]) -> tuple[int, int]:
    try:
        dimension, arity = PREDICATE_SPECS[predicate]
    except KeyError as exc:
        raise ValueError(f"unsupported predicate: {predicate}") from exc
    if len(points_bits) != arity or any(len(point) != dimension for point in points_bits):
        raise ValueError(f"{predicate} requires {arity} points in {dimension}D")
    return dimension, arity


def oracle_a(predicate: str, points_bits: Sequence[Sequence[int]]) -> int:
    """Exact Fraction oracle using translated predicate formulas."""
    _validate_shape(predicate, points_bits)
    points = [[fraction_from_bits(bits) for bits in point] for point in points_bits]

    if predicate == "orient2d":
        a, b, c = points
        value = (a[0] - c[0]) * (b[1] - c[1]) - (a[1] - c[1]) * (b[0] - c[0])
        return sign(value)

    if predicate == "orient3d":
        a, b, c, d = points
        ad = [a[i] - d[i] for i in range(3)]
        bd = [b[i] - d[i] for i in range(3)]
        cd = [c[i] - d[i] for i in range(3)]
        value = (
            ad[0] * (bd[1] * cd[2] - bd[2] * cd[1])
            - ad[1] * (bd[0] * cd[2] - bd[2] * cd[0])
            + ad[2] * (bd[0] * cd[1] - bd[1] * cd[0])
        )
        return sign(value)

    if predicate == "incircle":
        a, b, c, d = points
        ad = [a[i] - d[i] for i in range(2)]
        bd = [b[i] - d[i] for i in range(2)]
        cd = [c[i] - d[i] for i in range(2)]
        alift = ad[0] * ad[0] + ad[1] * ad[1]
        blift = bd[0] * bd[0] + bd[1] * bd[1]
        clift = cd[0] * cd[0] + cd[1] * cd[1]
        value = (
            alift * (bd[0] * cd[1] - bd[1] * cd[0])
            + blift * (cd[0] * ad[1] - cd[1] * ad[0])
            + clift * (ad[0] * bd[1] - ad[1] * bd[0])
        )
        return sign(value)

    a, b, c, d, e = points
    rows: list[list[Fraction]] = []
    for point in (a, b, c, d):
        relative = [point[i] - e[i] for i in range(3)]
        lift = sum(component * component for component in relative)
        rows.append(relative + [lift])
    return sign(_det_fraction(rows))


def oracle_b(predicate: str, points_bits: Sequence[Sequence[int]]) -> int:
    """Independent exact dyadic-integer oracle using homogeneous determinants."""
    _validate_shape(predicate, points_bits)
    dyadic_points = [[dyadic_from_bits(bits) for bits in point] for point in points_bits]
    nonzero_exponents = [
        exponent
        for point in dyadic_points
        for significand, exponent in point
        if significand != 0
    ]
    common_exponent = min(nonzero_exponents) if nonzero_exponents else 0
    integer_points = [
        [
            0 if significand == 0 else significand << (exponent - common_exponent)
            for significand, exponent in point
        ]
        for point in dyadic_points
    ]

    if predicate == "orient2d":
        matrix = [[x, y, 1] for x, y in integer_points]
    elif predicate == "orient3d":
        matrix = [[x, y, z, 1] for x, y, z in integer_points]
    elif predicate == "incircle":
        matrix = [[x, y, x * x + y * y, 1] for x, y in integer_points]
    else:
        matrix = [[x, y, z, x * x + y * y + z * z, 1] for x, y, z in integer_points]
    return sign(_det_integer_permutations(matrix))


def point_bits(points: Iterable[Iterable[float]]) -> list[list[int]]:
    return [[bits_from_float(value) for value in point] for point in points]


def canonical_cases() -> dict[str, list[dict[str, object]]]:
    zneg = -0.0
    p2m100 = math.ldexp(1.0, -100)
    p2m200 = math.ldexp(1.0, -200)
    base2 = math.ldexp(1.0, 40)
    base3 = math.ldexp(1.0, 30)

    return {
        "orient2d": [
            {"id": "o2_ccw_unit", "class": "canonical", "seed": 0,
             "points": point_bits([(0.0, 0.0), (1.0, 0.0), (0.0, 1.0)])},
            {"id": "o2_cw_unit", "class": "canonical", "seed": 0,
             "points": point_bits([(0.0, 0.0), (0.0, 1.0), (1.0, 0.0)])},
            {"id": "o2_collinear", "class": "exact-zero", "seed": 0,
             "points": point_bits([(0.0, 0.0), (1.0, 1.0), (2.0, 2.0)])},
            {"id": "o2_signed_zero", "class": "signed-zero", "seed": 0,
             "points": point_bits([(zneg, 0.0), (1.0, zneg), (0.0, 1.0)])},
            {"id": "o2_large_offset", "class": "large-offset", "seed": 0,
             "points": point_bits([(base2, base2), (base2 + 1.0, base2), (base2, base2 + 1.0)])},
            {"id": "o2_tiny_scale", "class": "scale", "seed": 0,
             "points": point_bits([(0.0, 0.0), (p2m100, 0.0), (0.0, p2m100)])},
        ],
        "orient3d": [
            {"id": "o3_positive_unit", "class": "canonical", "seed": 0,
             "points": point_bits([(1.0, 0.0, 0.0), (0.0, 1.0, 0.0), (0.0, 0.0, 1.0), (0.0, 0.0, 0.0)])},
            {"id": "o3_negative_swap", "class": "canonical", "seed": 0,
             "points": point_bits([(0.0, 1.0, 0.0), (1.0, 0.0, 0.0), (0.0, 0.0, 1.0), (0.0, 0.0, 0.0)])},
            {"id": "o3_coplanar", "class": "exact-zero", "seed": 0,
             "points": point_bits([(0.0, 0.0, 0.0), (1.0, 0.0, 0.0), (0.0, 1.0, 0.0), (1.0, 1.0, 0.0)])},
            {"id": "o3_signed_zero", "class": "signed-zero", "seed": 0,
             "points": point_bits([(1.0, zneg, 0.0), (zneg, 1.0, 0.0), (0.0, zneg, 1.0), (zneg, 0.0, 0.0)])},
            {"id": "o3_large_offset", "class": "large-offset", "seed": 0,
             "points": point_bits([(base3 + 1.0, base3, base3), (base3, base3 + 1.0, base3),
                                   (base3, base3, base3 + 1.0), (base3, base3, base3)])},
            {"id": "o3_tiny_scale", "class": "scale", "seed": 0,
             "points": point_bits([(p2m100, 0.0, 0.0), (0.0, p2m100, 0.0),
                                   (0.0, 0.0, p2m100), (0.0, 0.0, 0.0)])},
            {"id": "o3_near_coplanar", "class": "near-degenerate", "seed": 0,
             "points": point_bits([(1.0, 0.0, 0.0), (0.0, 1.0, 0.0),
                                   (0.0, 0.0, 0.0), (0.0, 0.0, p2m200)])},
        ],
        "incircle": [
            {"id": "ic_inside", "class": "canonical", "seed": 0,
             "points": point_bits([(1.0, 0.0), (0.0, 1.0), (-1.0, 0.0), (0.0, 0.0)])},
            {"id": "ic_outside", "class": "canonical", "seed": 0,
             "points": point_bits([(1.0, 0.0), (0.0, 1.0), (-1.0, 0.0), (0.0, -2.0)])},
            {"id": "ic_cocircular", "class": "exact-zero", "seed": 0,
             "points": point_bits([(1.0, 0.0), (0.0, 1.0), (-1.0, 0.0), (0.0, -1.0)])},
            {"id": "ic_reverse_inside", "class": "canonical", "seed": 0,
             "points": point_bits([(-1.0, 0.0), (0.0, 1.0), (1.0, 0.0), (0.0, 0.0)])},
            {"id": "ic_signed_zero", "class": "signed-zero", "seed": 0,
             "points": point_bits([(1.0, zneg), (zneg, 1.0), (-1.0, zneg), (zneg, zneg)])},
            {"id": "ic_tiny_scale", "class": "scale", "seed": 0,
             "points": point_bits([(p2m100, 0.0), (0.0, p2m100), (-p2m100, 0.0), (0.0, 0.0)])},
        ],
        "insphere": [
            {"id": "is_inside", "class": "canonical", "seed": 0,
             "points": point_bits([(1.0, 0.0, 0.0), (0.0, 1.0, 0.0), (0.0, 0.0, 1.0),
                                   (0.0, 0.0, 0.0), (0.5, 0.5, 0.5)])},
            {"id": "is_cospherical", "class": "exact-zero", "seed": 0,
             "points": point_bits([(1.0, 0.0, 0.0), (0.0, 1.0, 0.0), (0.0, 0.0, 1.0),
                                   (0.0, 0.0, 0.0), (1.0, 1.0, 1.0)])},
            {"id": "is_outside", "class": "canonical", "seed": 0,
             "points": point_bits([(1.0, 0.0, 0.0), (0.0, 1.0, 0.0), (0.0, 0.0, 1.0),
                                   (0.0, 0.0, 0.0), (2.0, 2.0, 2.0)])},
            {"id": "is_reverse_inside", "class": "canonical", "seed": 0,
             "points": point_bits([(0.0, 1.0, 0.0), (1.0, 0.0, 0.0), (0.0, 0.0, 1.0),
                                   (0.0, 0.0, 0.0), (0.5, 0.5, 0.5)])},
            {"id": "is_signed_zero", "class": "signed-zero", "seed": 0,
             "points": point_bits([(1.0, zneg, 0.0), (zneg, 1.0, 0.0), (0.0, zneg, 1.0),
                                   (zneg, 0.0, zneg), (0.5, 0.5, 0.5)])},
            {"id": "is_tiny_scale", "class": "scale", "seed": 0,
             "points": point_bits([(p2m100, 0.0, 0.0), (0.0, p2m100, 0.0), (0.0, 0.0, p2m100),
                                   (0.0, 0.0, 0.0), (p2m100 / 2.0, p2m100 / 2.0, p2m100 / 2.0)])},
        ],
    }


def expected_canonical_signs() -> dict[str, int]:
    return {
        "o2_ccw_unit": 1, "o2_cw_unit": -1, "o2_collinear": 0,
        "o3_positive_unit": 1, "o3_negative_swap": -1, "o3_coplanar": 0,
        "ic_inside": 1, "ic_outside": -1, "ic_cocircular": 0, "ic_reverse_inside": -1,
        "is_inside": 1, "is_cospherical": 0, "is_outside": -1, "is_reverse_inside": -1,
    }


def _format_case(predicate: str, case: dict[str, object]) -> str:
    points = case["points"]
    assert isinstance(points, list)
    sign_a = oracle_a(predicate, points)
    sign_b = oracle_b(predicate, points)
    if sign_a != sign_b:
        raise RuntimeError(f"oracle disagreement for {case['id']}: {sign_a} vs {sign_b}")
    flat = [bits for point in points for bits in point]
    fields = [
        str(case["id"]), str(case["class"]),
        f"{sign_a:+d}" if sign_a else "0", str(case["seed"]),
        *(f"{bits:016x}" for bits in flat),
    ]
    return "\t".join(fields)


def render_fixture(predicate: str) -> str:
    dimension, arity = PREDICATE_SPECS[predicate]
    lines = [
        "# D26PRED 1",
        f"# predicate={predicate}",
        "# encoding=ieee754-binary64-bits-hex",
        f"# dimension={dimension}",
        f"# arity={arity}",
        f"# coordinate_count={dimension * arity}",
        f"# generator={GENERATOR_VERSION}",
    ]
    lines.extend(_format_case(predicate, case) for case in canonical_cases()[predicate])
    return "\n".join(lines) + "\n"


def generate_all(output_dir) -> None:
    from pathlib import Path
    output = Path(output_dir)
    output.mkdir(parents=True, exist_ok=True)
    for predicate in PREDICATE_SPECS:
        (output / f"{predicate}.d26pred").write_text(render_fixture(predicate), encoding="utf-8")


def structured_random_cross_check(cases_per_predicate: int = 128, seed: int = 0xD26) -> int:
    rng = random.Random(seed)
    checked = 0
    for predicate, (dimension, arity) in PREDICATE_SPECS.items():
        for _ in range(cases_per_predicate):
            points = []
            for _point in range(arity):
                coordinates = []
                for _axis in range(dimension):
                    mantissa = rng.randint(-4096, 4096)
                    exponent = rng.randint(-30, 30)
                    coordinates.append(math.ldexp(float(mantissa), exponent))
                points.append(coordinates)
            bits = point_bits(points)
            if oracle_a(predicate, bits) != oracle_b(predicate, bits):
                raise AssertionError(f"random oracle mismatch in {predicate}")
            checked += 1
    return checked


def structured_random_cases(predicate: str, count: int, seed: int) -> list[dict[str, object]]:
    if count < 0:
        raise ValueError("random case count cannot be negative")
    dimension, arity = PREDICATE_SPECS[predicate]
    predicate_offsets = {
        "orient2d": 0x102,
        "orient3d": 0x103,
        "incircle": 0x201,
        "insphere": 0x301,
    }
    rng = random.Random(seed + predicate_offsets[predicate])
    cases: list[dict[str, object]] = []
    for case_index in range(count):
        points = []
        for _point in range(arity):
            coordinates = []
            for _axis in range(dimension):
                mantissa = rng.randint(-4096, 4096)
                exponent = rng.randint(-80, 80)
                coordinates.append(math.ldexp(float(mantissa), exponent))
            points.append(coordinates)
        cases.append({
            "id": f"rnd_{predicate}_{case_index:06d}",
            "class": "random-structured",
            "seed": seed,
            "points": point_bits(points),
        })
    return cases


def render_fixture_with_random(predicate: str, random_cases: int, seed: int) -> str:
    dimension, arity = PREDICATE_SPECS[predicate]
    lines = [
        "# D26PRED 1",
        f"# predicate={predicate}",
        "# encoding=ieee754-binary64-bits-hex",
        f"# dimension={dimension}",
        f"# arity={arity}",
        f"# coordinate_count={dimension * arity}",
        f"# generator={GENERATOR_VERSION}",
    ]
    cases = list(canonical_cases()[predicate])
    cases.extend(structured_random_cases(predicate, random_cases, seed))
    lines.extend(_format_case(predicate, case) for case in cases)
    return "\n".join(lines) + "\n"


def generate_all_with_random(output_dir, random_cases: int, seed: int) -> None:
    from pathlib import Path
    output = Path(output_dir)
    output.mkdir(parents=True, exist_ok=True)
    for predicate in PREDICATE_SPECS:
        (output / f"{predicate}.d26pred").write_text(
            render_fixture_with_random(predicate, random_cases, seed),
            encoding="utf-8",
        )
