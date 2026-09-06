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


def numeric_determinant_sign(rows):
    polynomial_rows = [
        [constant(value) for value in row]
        for row in rows
    ]
    return leading_sign(determinant(polynomial_rows))


def orient3d_sign(a, b, c, d):
    return numeric_determinant_sign([
        [*a, 1],
        [*b, 1],
        [*c, 1],
        [*d, 1],
    ])


def insphere_sign(a, b, c, d, query):
    rows = []
    for x, y, z in (a, b, c, d, query):
        rows.append([x, y, z, x * x + y * y + z * z, 1])
    return numeric_determinant_sign(rows)


def incircle_sign(a, b, c, query):
    rows = []
    for x, y in (a, b, c, query):
        rows.append([x, y, x * x + y * y, 1])
    return numeric_determinant_sign(rows)


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


def verify_finite_and_ghost_semantics():
    tetra = [
        (Fraction(1), Fraction(0), Fraction(0)),
        (Fraction(0), Fraction(1), Fraction(0)),
        (Fraction(0), Fraction(0), Fraction(1)),
        (Fraction(0), Fraction(0), Fraction(0)),
    ]
    inside = (Fraction(1, 4), Fraction(1, 4), Fraction(1, 4))
    outside = (Fraction(2), Fraction(2), Fraction(2))

    if orient3d_sign(*tetra) != 1:
        raise AssertionError("positive tetra orientation oracle mismatch")
    if insphere_sign(*tetra, inside) != 1:
        raise AssertionError("positive tetra interior InSphere oracle mismatch")
    if insphere_sign(*tetra, outside) != -1:
        raise AssertionError("positive tetra exterior InSphere oracle mismatch")

    facet = [
        (Fraction(0), Fraction(0), Fraction(0)),
        (Fraction(1), Fraction(0), Fraction(0)),
        (Fraction(0), Fraction(1), Fraction(0)),
    ]
    inside_witness = (Fraction(0), Fraction(0), Fraction(1))
    exterior_query = (Fraction(1, 4), Fraction(1, 4), Fraction(-1))
    interior_query = (Fraction(1, 4), Fraction(1, 4), Fraction(1, 4))

    if orient3d_sign(*facet, inside_witness) != -1:
        raise AssertionError("outward hull-facet orientation oracle mismatch")
    if orient3d_sign(*facet, exterior_query) != 1:
        raise AssertionError("ghost exterior half-space oracle mismatch")
    if orient3d_sign(*facet, interior_query) != -1:
        raise AssertionError("ghost triangulation-side oracle mismatch")

    tri2 = [
        (Fraction(0), Fraction(0)),
        (Fraction(1), Fraction(0)),
        (Fraction(0), Fraction(1)),
    ]
    if incircle_sign(*tri2, (Fraction(1, 4), Fraction(1, 4))) != 1:
        raise AssertionError("projected circumdisk interior oracle mismatch")
    if incircle_sign(*tri2, (Fraction(2), Fraction(2))) != -1:
        raise AssertionError("projected circumdisk exterior oracle mismatch")
    if incircle_sign(*tri2, (Fraction(1), Fraction(1))) != 0:
        raise AssertionError("projected circumcircle exact-zero oracle mismatch")


def main():
    verify_finite_and_ghost_semantics()
    insphere_count = verify_insphere()
    incircle_count = verify_incircle()
    # ADR-MESH-0043: z=x oblique plane, genuine radius-3 circle.
    # Full polynomial expansion is independent of production cofactor code.
    sites = [(10, (0, 3, 0)), (80, (2, 1, 2)),
             (30, (-2, 1, -2)), (90, (0, -3, 0))]
    rank = lift_rank(sites)
    for order in permutations(range(3)):
        ordered = [sites[i] for i in order] + [sites[3]]
        rows = [[constant(x), constant(y),
                 poly_add(constant(x*x+y*y+z*z), monomial(rank[pid])),
                 constant(1)] for pid, (x, y, z) in ordered]
        poly = determinant(rows)
        orientation = numeric_determinant_sign(
            [[x, y, 1] for _, (x, y, _) in ordered[:3]])
        if 0 in poly or leading_sign(poly) * orientation != -1:
            raise AssertionError("oblique Euclidean lift tie mismatch")
    print(
        "M2.1-A symbolic polynomial oracle PASS "
        f"insphere_permutations={insphere_count} "
        f"incircle_permutations={incircle_count}"
    )


if __name__ == "__main__":
    main()
