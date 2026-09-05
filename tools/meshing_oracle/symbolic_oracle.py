from __future__ import annotations

from fractions import Fraction
from typing import Dict, Iterable, Sequence

from exact_oracle import PREDICATE_SPECS, fraction_from_bits, oracle_a


Polynomial = Dict[int, Fraction]


def _clean(poly: Polynomial) -> Polynomial:
    return {exp: coeff for exp, coeff in poly.items() if coeff}


def _add(lhs: Polynomial, rhs: Polynomial) -> Polynomial:
    result = dict(lhs)
    for exp, coeff in rhs.items():
        result[exp] = result.get(exp, Fraction(0)) + coeff
    return _clean(result)


def _neg(poly: Polynomial) -> Polynomial:
    return {exp: -coeff for exp, coeff in poly.items()}


def _mul(lhs: Polynomial, rhs: Polynomial) -> Polynomial:
    result: Polynomial = {}
    for exp_a, coeff_a in lhs.items():
        for exp_b, coeff_b in rhs.items():
            exp = exp_a + exp_b
            result[exp] = result.get(exp, Fraction(0)) + coeff_a * coeff_b
    return _clean(result)


def _constant(value: Fraction) -> Polynomial:
    return {} if value == 0 else {0: value}


def _det(matrix: Sequence[Sequence[Polynomial]]) -> Polynomial:
    size = len(matrix)
    if size == 0 or any(len(row) != size for row in matrix):
        raise ValueError("symbolic determinant matrix must be square")
    if size == 1:
        return dict(matrix[0][0])

    result: Polynomial = {}
    for column in range(size):
        minor = [
            list(row[:column]) + list(row[column + 1 :])
            for row in matrix[1:]
        ]
        term = _mul(matrix[0][column], _det(minor))
        if column % 2:
            term = _neg(term)
        result = _add(result, term)
    return result


def _canonical_coordinate_tuple(points_bits: Sequence[Sequence[int]]) -> tuple[tuple[int, ...], ...]:
    def normalize(bits: int) -> int:
        return 0 if (bits & 0x7FFFFFFFFFFFFFFF) == 0 else bits
    return tuple(tuple(normalize(bits) for bits in point) for point in points_bits)


def _validate_distinct_sites(points_bits: Sequence[Sequence[int]]) -> None:
    normalized = _canonical_coordinate_tuple(points_bits)
    if len(set(normalized)) != len(normalized):
        raise ValueError("symbolic perturbation requires canonical distinct sites")


def symbolic_sign(
    predicate: str,
    points_bits: Sequence[Sequence[int]],
    point_ids: Sequence[int],
) -> int:
    dimension, arity = PREDICATE_SPECS[predicate]
    if len(points_bits) != arity or len(point_ids) != arity:
        raise ValueError("symbolic predicate arity mismatch")
    if len(set(point_ids)) != len(point_ids) or any(point_id <= 0 for point_id in point_ids):
        raise ValueError("PointIds must be unique positive integers")
    _validate_distinct_sites(points_bits)

    point_rank = {point_id: rank for rank, point_id in enumerate(sorted(point_ids))}
    coordinates: list[list[Polynomial]] = []

    for point, point_id in zip(points_bits, point_ids):
        if len(point) != dimension:
            raise ValueError("symbolic predicate dimension mismatch")
        row: list[Polynomial] = []
        for component, bits in enumerate(point):
            exact = fraction_from_bits(bits)
            rank = point_rank[point_id] * dimension + component
            # Base 4 keeps perturbation exponent digits collision-free for the
            # degree-2 lifted coordinates used by incircle/insphere.
            perturbation_exp = 4 ** rank
            poly = _constant(exact)
            poly[perturbation_exp] = poly.get(perturbation_exp, Fraction(0)) + Fraction(1)
            row.append(_clean(poly))
        coordinates.append(row)

    one = _constant(Fraction(1))

    if predicate == "orient2d":
        matrix = [[row[0], row[1], one] for row in coordinates]
    elif predicate == "orient3d":
        matrix = [[row[0], row[1], row[2], one] for row in coordinates]
    elif predicate == "incircle":
        matrix = []
        for row in coordinates:
            lift = _add(_mul(row[0], row[0]), _mul(row[1], row[1]))
            matrix.append([row[0], row[1], lift, one])
    else:
        matrix = []
        for row in coordinates:
            lift = _add(
                _add(_mul(row[0], row[0]), _mul(row[1], row[1])),
                _mul(row[2], row[2]),
            )
            matrix.append([row[0], row[1], row[2], lift, one])

    polynomial = _det(matrix)
    if not polynomial:
        raise RuntimeError("symbolic perturbation failed to resolve degeneracy")

    first_exp = min(polynomial)
    first_coeff = polynomial[first_exp]
    result = (first_coeff > 0) - (first_coeff < 0)

    exact = oracle_a(predicate, points_bits)
    if exact != 0 and result != exact:
        raise AssertionError("symbolic perturbation changed nondegenerate predicate sign")
    return result
