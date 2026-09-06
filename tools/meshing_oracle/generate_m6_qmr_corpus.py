#!/usr/bin/env python3

import argparse
import itertools
import math
import random
import struct
from fractions import Fraction
from math import gcd
from pathlib import Path


def sign(value):
    return (value > 0) - (value < 0)


def float_bits(value):
    return struct.unpack(">Q", struct.pack(">d", float(value)))[0]


def bits_hex(value):
    return f"{float_bits(value):016x}"


def determinant3(r0, r1, r2):
    return (
        r0[0] * (r1[1] * r2[2] - r1[2] * r2[1])
        - r0[1] * (r1[0] * r2[2] - r1[2] * r2[0])
        + r0[2] * (r1[0] * r2[1] - r1[1] * r2[0])
    )


def fraction_key(tetra):
    points = [
        tuple(Fraction.from_float(float(component)) for component in point)
        for point in tetra
    ]
    e01 = tuple(points[1][i] - points[0][i] for i in range(3))
    e02 = tuple(points[2][i] - points[0][i] for i in range(3))
    e03 = tuple(points[3][i] - points[0][i] for i in range(3))
    d = abs(determinant3(e01, e02, e03))
    if d == 0:
        raise ValueError("degenerate tetrahedron")

    s = Fraction(0)
    for i, j in itertools.combinations(range(4), 2):
        s += sum((points[j][k] - points[i][k]) ** 2 for k in range(3))
    return d * d, s * s * s


def decode_binary64(value):
    bits = float_bits(value)
    negative = (bits >> 63) != 0
    exponent_bits = (bits >> 52) & 0x7FF
    fraction_bits = bits & ((1 << 52) - 1)

    if exponent_bits == 0:
        if fraction_bits == 0:
            return 0, 0
        significand = fraction_bits
        exponent = -1074
    else:
        significand = (1 << 52) | fraction_bits
        exponent = exponent_bits - 1023 - 52

    return -significand if negative else significand, exponent


def integer_points(tetra):
    decoded = [decode_binary64(c) for p in tetra for c in p]
    nonzero_exponents = [e for m, e in decoded if m != 0]
    common_exponent = min(nonzero_exponents) if nonzero_exponents else 0

    values = []
    for mantissa, exponent in decoded:
        if mantissa == 0:
            values.append(0)
        else:
            values.append(mantissa << (exponent - common_exponent))

    return [tuple(values[3 * i : 3 * i + 3]) for i in range(4)]


def integer_key(tetra, primitive=False):
    points = integer_points(tetra)
    edges = []
    for i, j in itertools.combinations(range(4), 2):
        edges.append(tuple(points[j][k] - points[i][k] for k in range(3)))

    d = abs(determinant3(edges[0], edges[1], edges[2]))
    # combinations order starts (0,1),(0,2),(0,3), so first three are anchor edges.
    if d == 0:
        raise ValueError("degenerate tetrahedron")
    s = sum(component * component for edge in edges for component in edge)

    if primitive:
        content = 0
        for edge in edges:
            for component in edge:
                content = gcd(content, abs(component))
        if content > 1:
            g2 = content * content
            g3 = g2 * content
            if d % g3 != 0 or s % g2 != 0:
                raise AssertionError("primitive normalization divisibility failure")
            d //= g3
            s //= g2

    return d * d, s * s * s


def compare_keys(lhs, rhs):
    return sign(lhs[0] * rhs[1] - rhs[0] * lhs[1])


def compare_oracles(lhs, rhs):
    fraction_result = compare_keys(fraction_key(lhs), fraction_key(rhs))
    integer_result = compare_keys(integer_key(lhs), integer_key(rhs))
    primitive_result = compare_keys(
        integer_key(lhs, primitive=True),
        integer_key(rhs, primitive=True),
    )
    if fraction_result != integer_result or fraction_result != primitive_result:
        raise AssertionError(
            f"oracle mismatch fraction={fraction_result} "
            f"integer={integer_result} primitive={primitive_result}"
        )
    return fraction_result


def fixed_cases():
    base = [
        (0.0, 0.0, 0.0),
        (1.0, 0.0, 0.0),
        (0.0, 1.0, 0.0),
        (0.0, 0.0, 1.0),
    ]
    scale8 = [tuple(8.0 * x for x in p) for p in base]
    translated = [tuple(x + 16.0 for x in p) for p in base]
    permuted = [base[i] for i in (2, 0, 3, 1)]

    sliver10 = [
        (1.0, 0.0, 0.0),
        (-1.0, 0.0, 0.0),
        (0.0, 1.0, math.ldexp(1.0, -10)),
        (0.0, -1.0, math.ldexp(1.0, -10)),
    ]
    sliver20 = [
        (1.0, 0.0, 0.0),
        (-1.0, 0.0, 0.0),
        (0.0, 1.0, math.ldexp(1.0, -20)),
        (0.0, -1.0, math.ldexp(1.0, -20)),
    ]
    needle10 = [
        (0.0, 0.0, 0.0),
        (1.0, 0.0, 0.0),
        (0.0, math.ldexp(1.0, -10), 0.0),
        (0.0, 0.0, math.ldexp(1.0, -10)),
    ]
    needle20 = [
        (0.0, 0.0, 0.0),
        (1.0, 0.0, 0.0),
        (0.0, math.ldexp(1.0, -20), 0.0),
        (0.0, 0.0, math.ldexp(1.0, -20)),
    ]
    wedge10 = [
        (0.0, 0.0, 0.0),
        (1.0, 0.0, 0.0),
        (0.0, 1.0, 0.0),
        (0.5, 0.0, math.ldexp(1.0, -10)),
    ]
    wedge20 = [
        (0.0, 0.0, 0.0),
        (1.0, 0.0, 0.0),
        (0.0, 1.0, 0.0),
        (0.5, 0.0, math.ldexp(1.0, -20)),
    ]
    scale_hi = [tuple(math.ldexp(x, 500) for x in p) for p in base]
    scale_lo = [tuple(math.ldexp(x, -500) for x in p) for p in base]

    perturb_down = list(base)
    perturb_down[3] = (0.0, 0.0, math.nextafter(1.0, 0.0))
    perturb_up = list(base)
    perturb_up[3] = (0.0, 0.0, math.nextafter(1.0, math.inf))

    return [
        ("scale_equal", base, scale8),
        ("translation_equal", base, translated),
        ("permutation_equal", base, permuted),
        ("base_gt_sliver", base, sliver10),
        ("sliver10_gt_sliver20", sliver10, sliver20),
        ("needle10_gt_needle20", needle10, needle20),
        ("wedge10_gt_wedge20", wedge10, wedge20),
        ("sliver20_lt_base", sliver20, base),
        ("needle20_lt_needle10", needle20, needle10),
        ("scale_hi_equal", base, scale_hi),
        ("scale_lo_equal", base, scale_lo),
        ("scale_hi_lo_equal", scale_hi, scale_lo),
        ("pert_down_vs_base", perturb_down, base),
        ("pert_up_vs_base", perturb_up, base),
    ]


def random_dyadic(rng):
    mantissa = rng.randint(-8, 8)
    exponent = rng.randint(-300, 300)
    return math.ldexp(float(mantissa), exponent)


def random_tetra(rng):
    for _ in range(10000):
        tetra = [
            tuple(random_dyadic(rng) for _ in range(3))
            for _ in range(4)
        ]
        try:
            fraction_key(tetra)
            return tetra
        except ValueError:
            continue
    raise RuntimeError("failed to generate non-degenerate tetrahedron")


def validate_permutations():
    base = fixed_cases()[0][1]
    reference_fraction = fraction_key(base)
    reference_integer = integer_key(base)
    for permutation in itertools.permutations(range(4)):
        candidate = [base[i] for i in permutation]
        if fraction_key(candidate) != reference_fraction:
            raise AssertionError("Fraction permutation invariance failure")
        if integer_key(candidate) != reference_integer:
            raise AssertionError("integer permutation invariance failure")
        if integer_key(candidate, primitive=True) != integer_key(
            base, primitive=True
        ):
            raise AssertionError("primitive permutation invariance failure")


def write_case(stream, case_id, lhs, rhs):
    result = compare_oracles(lhs, rhs)
    expected = {-1: "-1", 0: "0", 1: "+1"}[result]
    fields = [case_id, expected]
    fields.extend(bits_hex(value) for tetra in (lhs, rhs) for point in tetra for value in point)
    stream.write("\t".join(fields) + "\n")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True)
    parser.add_argument("--random-cases", type=int, default=128)
    parser.add_argument("--seed", type=int, default=260906)
    args = parser.parse_args()

    validate_permutations()

    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)

    rng = random.Random(args.seed)
    with output.open("w", encoding="utf-8", newline="\n") as stream:
        stream.write("# D26QMR1 generated exact comparator corpus v1\n")
        stream.write("# case_id expected 24x_binary64_hex\n")
        for case_id, lhs, rhs in fixed_cases():
            write_case(stream, case_id, lhs, rhs)
        for index in range(args.random_cases):
            write_case(
                stream,
                f"random_{index:04d}",
                random_tetra(rng),
                random_tetra(rng),
            )

    print(
        f"D26QMR1 oracle PASS fixed={len(fixed_cases())} "
        f"random={args.random_cases} seed={args.seed} output={output}"
    )


if __name__ == "__main__":
    main()
