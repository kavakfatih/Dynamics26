#!/usr/bin/env python3

import argparse
import math
import random
import struct
from fractions import Fraction
from pathlib import Path


def bits(value):
    packed = struct.pack(">d", float(value))
    return f"{struct.unpack('>Q', packed)[0]:016x}"


def tight_lower(value):
    rounded = float(value)
    if Fraction.from_float(rounded) > value:
        rounded = math.nextafter(rounded, -math.inf)
    return rounded


def tight_upper(value):
    rounded = float(value)
    if Fraction.from_float(rounded) < value:
        rounded = math.nextafter(rounded, math.inf)
    return rounded


def exact(value):
    return Fraction.from_float(float(value))


def determinant(tetra):
    points = [[exact(value) for value in point] for point in tetra]
    anchor, p1, p2, p3 = points
    relative = [
        [point[axis] - anchor[axis] for axis in range(3)]
        for point in (p1, p2, p3)
    ]

    a, b, c = relative[0]
    d, e, f = relative[1]
    g, h, i = relative[2]
    return (
        a * (e * i - f * h)
        - b * (d * i - f * g)
        + c * (d * h - e * g)
    )


def edge_sum(tetra):
    points = [[exact(value) for value in point] for point in tetra]
    total = Fraction(0)
    for lhs in range(4):
        for rhs in range(lhs + 1, 4):
            total += sum(
                (points[rhs][axis] - points[lhs][axis]) ** 2
                for axis in range(3)
            )
    return total


def comparison_polynomial(lhs, rhs):
    lhs_d = determinant(lhs)
    lhs_s = edge_sum(lhs)
    rhs_d = determinant(rhs)
    rhs_s = edge_sum(rhs)
    cross = lhs_d**2 * rhs_s**3 - rhs_d**2 * lhs_s**3
    return cross, lhs_d, lhs_s, rhs_d, rhs_s


def scale_tetra(tetra, exponent):
    return [
        tuple(math.ldexp(value, exponent) for value in point)
        for point in tetra
    ]


def translate_tetra(tetra, delta):
    return [
        tuple(point[axis] + delta[axis] for axis in range(3))
        for point in tetra
    ]


def positive_tetra(tetra):
    tetra = list(tetra)
    d = determinant(tetra)
    if d == 0:
        raise ValueError("degenerate tetra")

    # Dynamics26 M1 orient3d convention, standard p1-p0 determinantini
    # negates. Bu nedenle M1 Positive TET4 icin burada determinant < 0.
    if d > 0:
        tetra[2], tetra[3] = tetra[3], tetra[2]
    return tetra


def fixed_pairs():
    base = positive_tetra([
        (0.0, 0.0, 0.0),
        (1.0, 0.0, 0.0),
        (0.0, 1.0, 0.0),
        (0.0, 0.0, 1.0),
    ])
    regular = positive_tetra([
        (0.0, 0.0, 0.0),
        (1.0, 0.1, 0.2),
        (0.2, 1.1, 0.1),
        (0.1, 0.3, 1.2),
    ])
    sliver = positive_tetra([
        (0.0, 0.0, 0.0),
        (1.0, 0.2, 0.1),
        (0.1, 1.0, 0.2),
        (0.45, 0.45, 0.001),
    ])
    needle = positive_tetra([
        (0.0, 0.0, 0.0),
        (2.0, 0.1, 0.2),
        (0.3, 0.4, 0.1),
        (0.2, 0.3, 0.8),
    ])

    return [
        ("regular_vs_sliver", regular, sliver),
        ("sliver_vs_regular", sliver, regular),
        ("regular_vs_needle", regular, needle),
        ("exact_tie_self", regular, regular),
        ("exact_tie_scale", regular, scale_tetra(regular, 7)),
        (
            "exact_tie_translation",
            regular,
            translate_tetra(regular, (8.0, -4.0, 2.0)),
        ),
        (
            "wide_scale_compare",
            scale_tetra(regular, 300),
            scale_tetra(sliver, -300),
        ),
        ("base_vs_regular", base, regular),
    ]


def random_dyadic(rng):
    return math.ldexp(
        float(rng.randint(-31, 31)),
        rng.randint(-12, 12),
    )


def random_tetra(rng):
    while True:
        tetra = [
            tuple(random_dyadic(rng) for _ in range(3))
            for _ in range(4)
        ]
        if determinant(tetra) != 0 and edge_sum(tetra) > 0:
            return positive_tetra(tetra)


def random_pairs(count, seed):
    rng = random.Random(seed)
    return [
        (
            f"random_{index:04d}",
            random_tetra(rng),
            random_tetra(rng),
        )
        for index in range(count)
    ]


def append_tetra(fields, point_ids, tetra):
    fields.extend(str(point_id) for point_id in point_ids)
    for point in tetra:
        fields.extend(bits(value) for value in point)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True)
    parser.add_argument("--random-cases", type=int, default=128)
    parser.add_argument("--seed", type=int, default=26090603)
    args = parser.parse_args()

    pairs = fixed_pairs() + random_pairs(
        args.random_cases,
        args.seed,
    )

    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)

    with output.open("w", encoding="utf-8", newline="\n") as stream:
        stream.write(
            "# D26QMRF1 independent exact pair corpus v1\n"
        )

        for case_id, lhs, rhs in pairs:
            cross, lhs_d, lhs_s, rhs_d, rhs_s = (
                comparison_polynomial(lhs, rhs)
            )
            order = (cross > 0) - (cross < 0)

            fields = [case_id, str(order)]
            append_tetra(fields, (1, 2, 3, 4), lhs)
            append_tetra(fields, (11, 12, 13, 14), rhs)

            for value in (
                lhs_d,
                lhs_s,
                rhs_d,
                rhs_s,
                cross,
            ):
                fields.extend((
                    bits(tight_lower(value)),
                    bits(tight_upper(value)),
                ))

            stream.write("\t".join(fields) + "\n")

    print(
        f"D26QMRF1 oracle PASS fixed={len(fixed_pairs())} "
        f"random={args.random_cases} seed={args.seed}"
    )


if __name__ == "__main__":
    main()
