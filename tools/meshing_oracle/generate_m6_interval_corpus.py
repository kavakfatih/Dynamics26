#!/usr/bin/env python3

import argparse
import math
import random
import struct
from fractions import Fraction
from pathlib import Path


def float_bits(value):
    return struct.unpack(">Q", struct.pack(">d", float(value)))[0]


def bits_hex(value):
    return f"{float_bits(value):016x}"


def tight_lower(value):
    rounded = float(value)
    rounded_exact = Fraction.from_float(rounded)
    if rounded_exact > value:
        rounded = math.nextafter(rounded, -math.inf)
    return rounded


def tight_upper(value):
    rounded = float(value)
    rounded_exact = Fraction.from_float(rounded)
    if rounded_exact < value:
        rounded = math.nextafter(rounded, math.inf)
    return rounded


def exact_interval(op, a_lo, a_hi, b_lo, b_hi, parameter):
    a0 = Fraction.from_float(a_lo)
    a1 = Fraction.from_float(a_hi)
    b0 = Fraction.from_float(b_lo)
    b1 = Fraction.from_float(b_hi)

    if op == "add":
        return a0 + b0, a1 + b1
    if op == "sub":
        return a0 - b1, a1 - b0
    if op == "mul":
        values = (a0 * b0, a0 * b1, a1 * b0, a1 * b1)
        return min(values), max(values)
    if op == "square":
        values = [a0 * a0, a1 * a1]
        lo = Fraction(0) if a0 <= 0 <= a1 else min(values)
        return lo, max(values)
    if op == "cube":
        if a0 <= 0:
            raise ValueError("cube corpus requires positive interval")
        return a0**3, a1**3
    if op == "scale":
        factor = Fraction(2) ** parameter
        return a0 * factor, a1 * factor
    raise ValueError(f"unknown op {op}")


def fixed_cases():
    min_sub = math.ldexp(1.0, -1074)
    return [
        ("add_exact", "add", 1.0, 1.0, 2.0, 2.0, 0),
        (
            "add_halfway",
            "add",
            1.0,
            1.0,
            math.ldexp(1.0, -53),
            math.ldexp(1.0, -53),
            0,
        ),
        ("add_subnormal", "add", min_sub, min_sub, min_sub, min_sub, 0),
        ("sub_cancel", "sub", 1.0, 1.0, 1.0, 1.0, 0),
        ("sub_interval", "sub", -2.0, 3.0, -4.0, 5.0, 0),
        ("mul_point", "mul", 1.1, 1.1, 1.3, 1.3, 0),
        ("mul_mixed_sign", "mul", -2.0, 3.0, -4.0, 5.0, 0),
        ("mul_subnormal", "mul", min_sub, min_sub, 2.0, 2.0, 0),
        ("square_cross_zero", "square", -1.0, 2.0, 0.0, 0.0, 0),
        ("square_negative", "square", -3.0, -2.0, 0.0, 0.0, 0),
        ("cube_positive", "cube", 0.5, 1.25, 0.0, 0.0, 0),
        ("scale_normal", "scale", 0.75, 0.75, 0.0, 0.0, 10),
        ("scale_subnormal", "scale", min_sub, min_sub, 0.0, 0.0, 1073),
    ]


def random_dyadic(rng, positive=False):
    mantissa = rng.randint(1 if positive else -16, 16)
    if mantissa == 0:
        mantissa = 1
    exponent = rng.randint(-40, 40)
    return math.ldexp(float(mantissa), exponent)


def random_cases(count, seed):
    rng = random.Random(seed)
    result = []
    operations = ("add", "sub", "mul", "square", "cube", "scale")

    for index in range(count):
        op = operations[index % len(operations)]
        parameter = 0

        if op == "cube":
            a0 = random_dyadic(rng, positive=True)
            a1 = random_dyadic(rng, positive=True)
            a_lo, a_hi = sorted((a0, a1))
            b_lo = b_hi = 0.0
        elif op == "scale":
            a0 = random_dyadic(rng)
            a1 = random_dyadic(rng)
            a_lo, a_hi = sorted((a0, a1))
            b_lo = b_hi = 0.0
            parameter = rng.randint(-20, 20)
        else:
            a0 = random_dyadic(rng)
            a1 = random_dyadic(rng)
            b0 = random_dyadic(rng)
            b1 = random_dyadic(rng)
            a_lo, a_hi = sorted((a0, a1))
            b_lo, b_hi = sorted((b0, b1))

        result.append(
            (
                f"random_{index:04d}_{op}",
                op,
                a_lo,
                a_hi,
                b_lo,
                b_hi,
                parameter,
            )
        )

    return result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True)
    parser.add_argument("--random-cases", type=int, default=128)
    parser.add_argument("--seed", type=int, default=26090602)
    args = parser.parse_args()

    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)

    all_cases = fixed_cases() + random_cases(
        args.random_cases,
        args.seed,
    )

    with output.open("w", encoding="utf-8", newline="\n") as stream:
        stream.write("# D26INT1 exact primitive containment corpus v1\n")
        stream.write(
            "# id op a_lo a_hi b_lo b_hi parameter "
            "expected_lo expected_hi\n"
        )

        for (
            case_id,
            op,
            a_lo,
            a_hi,
            b_lo,
            b_hi,
            parameter,
        ) in all_cases:
            exact_lo, exact_hi = exact_interval(
                op,
                a_lo,
                a_hi,
                b_lo,
                b_hi,
                parameter,
            )
            expected_lo = tight_lower(exact_lo)
            expected_hi = tight_upper(exact_hi)
            fields = [
                case_id,
                op,
                bits_hex(a_lo),
                bits_hex(a_hi),
                bits_hex(b_lo),
                bits_hex(b_hi),
                str(parameter),
                bits_hex(expected_lo),
                bits_hex(expected_hi),
            ]
            stream.write("\t".join(fields) + "\n")

    print(
        f"D26INT1 primitive oracle PASS fixed={len(fixed_cases())} "
        f"random={args.random_cases} seed={args.seed} output={output}"
    )


if __name__ == "__main__":
    main()
