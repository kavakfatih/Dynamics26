#!/usr/bin/env python3

import argparse
import itertools
from fractions import Fraction
from pathlib import Path


def exact(value):
    return Fraction.from_float(float(value))


def determinant(tetra):
    points = [[exact(value) for value in point] for point in tetra]
    anchor = points[0]
    r = [
        [points[row][axis] - anchor[axis] for axis in range(3)]
        for row in (1, 2, 3)
    ]
    a, b, c = r[0]
    d, e, f = r[1]
    g, h, i = r[2]
    return (
        a * (e * i - f * h)
        - b * (d * i - f * g)
        + c * (d * h - e * g)
    )


def edge_sum(tetra):
    points = [[exact(value) for value in point] for point in tetra]
    return sum(
        sum(
            (points[j][axis] - points[i][axis]) ** 2
            for axis in range(3)
        )
        for i in range(4)
        for j in range(i + 1, 4)
    )


# Tetra'lar Dynamics26 M1 orient3d convention'inda Positive olacak bicimde
# onceden siralanmistir: standard p1-p0 determinant burada negatiftir.
ALPHABET = [
    [
        (0.0, 0.0, 0.0),
        (1.0, 0.0, 0.0),
        (0.0, 0.0, 1.0),
        (0.0, 1.0, 0.0),
    ],
    [
        (0.0, 0.0, 0.0),
        (1.0, 0.1, 0.2),
        (0.1, 0.3, 1.2),
        (0.2, 1.1, 0.1),
    ],
    [
        (0.0, 0.0, 0.0),
        (1.0, 0.2, 0.1),
        (0.1, 1.0, 0.2),
        (0.45, 0.45, 0.001),
    ],
    [
        (0.0, 0.0, 0.0),
        (2.0, 0.1, 0.2),
        (0.2, 0.3, 0.8),
        (0.3, 0.4, 0.1),
    ],
]


def quality_key(tetra):
    d = determinant(tetra)
    s = edge_sum(tetra)
    if not d < 0 or not s > 0:
        raise RuntimeError("oracle alphabet is not Dynamics26-positive TET4")
    return d * d / (s * s * s)


QUALITIES = [quality_key(tetra) for tetra in ALPHABET]
if len(set(QUALITIES)) != len(QUALITIES):
    raise RuntimeError("quality alphabet requires distinct exact qualities")


def sorted_quality_symbols(symbols):
    return sorted(symbols, key=lambda symbol: QUALITIES[symbol])


def compare_vectors(lhs, rhs):
    lhs_sorted = sorted_quality_symbols(lhs)
    rhs_sorted = sorted_quality_symbols(rhs)

    for left, right in zip(lhs_sorted, rhs_sorted):
        if QUALITIES[left] < QUALITIES[right]:
            return -1
        if QUALITIES[left] > QUALITIES[right]:
            return 1

    if len(lhs_sorted) == len(rhs_sorted):
        return 0

    # Frozen exact-prefix semantics: shorter wins.
    return 1 if len(lhs_sorted) < len(rhs_sorted) else -1


def encode(symbols):
    return "-" if not symbols else ",".join(str(value) for value in symbols)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True)
    args = parser.parse_args()

    vectors = []
    for length in range(4):
        vectors.extend(
            itertools.combinations_with_replacement(
                range(len(ALPHABET)),
                length,
            )
        )

    vector_id = {
        tuple(symbols): index
        for index, symbols in enumerate(vectors)
    }

    pair_count = 0
    union_count = 0
    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)

    with output.open("w", encoding="utf-8", newline="\n") as stream:
        stream.write("# D26QV1 exhaustive finite-multiset oracle v1\n")
        for index, symbols in enumerate(vectors):
            stream.write(
                f"V\t{index}\t{encode(symbols)}\n"
            )

        for lhs_id, lhs in enumerate(vectors):
            for rhs_id, rhs in enumerate(vectors):
                expected = compare_vectors(lhs, rhs)
                stream.write(
                    f"P\t{lhs_id}\t{rhs_id}\t{expected}\n"
                )
                pair_count += 1

        # Exhaustive union compatibility over every strict local new>old
        # pair and every common multiset C in the same finite alphabet.
        for old_id, old in enumerate(vectors):
            for new_id, new in enumerate(vectors):
                local = compare_vectors(new, old)
                if local <= 0:
                    continue

                for common_id, common in enumerate(vectors):
                    old_union = tuple(old) + tuple(common)
                    new_union = tuple(new) + tuple(common)
                    union_order = compare_vectors(
                        new_union,
                        old_union,
                    )
                    if union_order <= 0:
                        raise RuntimeError(
                            "D26QV1 union compatibility oracle failed"
                        )

                    stream.write(
                        "U\t"
                        f"{old_id}\t{new_id}\t{common_id}\t"
                        f"{local}\t{union_order}\n"
                    )
                    union_count += 1

    print(
        "D26QV1 oracle PASS "
        f"vectors={len(vectors)} pairs={pair_count} "
        f"strict_union_cases={union_count}"
    )


if __name__ == "__main__":
    main()
