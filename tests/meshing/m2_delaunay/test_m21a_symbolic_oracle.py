#!/usr/bin/env python3
"""Independent exact polynomial oracle for M2.1-A D26LIFT1."""

from fractions import Fraction
from itertools import permutations


def parity(order):
    inversions = 0
    for i in range(len(order)):
        for j in range(i + 1, len(order)):
            inversions += order[i] > order[j]
    return -1 if inversions % 2 else 1


def poly_add(lhs, rhs):
    result = dict(lhs)
    for exponent, coefficient in rhs.items():
        result[exponent] = result.get(exponent, Fraction(0)) + coefficient
        if result[exponent] == 0:
            del result[exponent]
    return result


def poly_mul(lhs, rhs):
    result = {}
    for lhs_exp, lhs_coef in lhs.items():
        for rhs_exp, rhs_coef in rhs.items():
            exponent = lhs_exp + rhs_exp
            result[exponent] = (
                result.get(exponent, Fraction(0))
                + lhs_coef * rhs_coef
            )
            if result[exponent] == 0:
                del result[exponent]
    return result


def constant(value):
    value = Fraction(value)
    return {} if value == 0 else {0: value}


def monomial(exponent):
    return {exponent: Fraction(1)}


def determinant(rows):
    size = len(rows)
    total = {}
    for order in permutations(range(size)):
        term = constant(parity(order))
        for row, column in enumerate(order):
            term = poly_mul(term, rows[row][column])
        total = poly_add(total, term)
    return total


def leading_sign(polynomial):
    if not polynomial:
        return 0
    coefficient = polynomial[min(polynomial)]
    return 1 if coefficient > 0 else -1


def lift_rank(points):
    ids = sorted(point[0] for point in points)
    return {point_id: rank + 1 for rank, point_id in enumerate(ids)}


def insphere_polynomial(points):
    rank = lift_rank(points)
    rows = []
    for point_id, (x, y, z) in points:
        lift = poly_add(
            constant(x * x + y * y + z * z),
            monomial(rank[point_id]),
        )
        rows.append([
            constant(x),
            constant(y),
            constant(z),
            lift,
            constant(1),
        ])
    return determinant(rows)


def incircle_polynomial(points):
    rank = lift_rank(points)
    rows = []
    for point_id, (x, y) in points:
        lift = poly_add(
            constant(x * x + y * y),
            monomial(rank[point_id]),
        )
        rows.append([
            constant(x),
            constant(y),
            lift,
            constant(1),
        ])
    return determinant(rows)


def verify_insphere():
    sites = [
        (1, (0, 0, 0)),
        (2, (0, 0, 1)),
        (3, (0, 1, 0)),
        (4, (1, 0, 0)),
        (5, (1, 1, 1)),
    ]
    base = insphere_polynomial(sites)
    if 0 in base:
        raise AssertionError("co-spherical raw determinant is not exact zero")
    if leading_sign(base) != -1:
        raise AssertionError("five-site D26LIFT1 oracle sign mismatch")

    count = 0
    for order in permutations(range(5)):
        candidate = [sites[index] for index in order]
        expected = -1 * parity(order)
        if leading_sign(insphere_polynomial(candidate)) != expected:
            raise AssertionError(
                f"InSphere permutation parity mismatch: {order}"
            )
        count += 1
    if count != 120:
        raise AssertionError("five-site permutation count mismatch")
    return count


def verify_incircle():
    sites = [
        (1, (0, 0)),
        (3, (1, 0)),
        (2, (0, 1)),
        (4, (1, 1)),
    ]
    base = incircle_polynomial(sites)
    if 0 in base:
        raise AssertionError("co-circular raw determinant is not exact zero")
    if leading_sign(base) != -1:
        raise AssertionError("four-site D26LIFT1 oracle sign mismatch")

    count = 0
    for order in permutations(range(4)):
        candidate = [sites[index] for index in order]
        expected = -1 * parity(order)
        if leading_sign(incircle_polynomial(candidate)) != expected:
            raise AssertionError(
                f"InCircle permutation parity mismatch: {order}"
            )
        count += 1
    if count != 24:
        raise AssertionError("four-site permutation count mismatch")
    return count


def main():
    insphere_count = verify_insphere()
    incircle_count = verify_incircle()
    print(
        "M2.1-A symbolic polynomial oracle PASS "
        f"insphere_permutations={insphere_count} "
        f"incircle_permutations={incircle_count}"
    )


if __name__ == "__main__":
    main()
