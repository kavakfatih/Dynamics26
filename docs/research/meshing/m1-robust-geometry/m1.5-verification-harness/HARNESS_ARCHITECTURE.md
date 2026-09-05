# Verification Harness Architecture

## 1. Test layers

The harness is intentionally layered.

### H0 — Platform arithmetic contract

Checks:
- binary64 assumptions,
- IEEE/IEC 559 support where required,
- radix 2,
- precision,
- rounding mode,
- forbidden fast-math macro,
- approved compiler path.

### H1 — Exact predicate truth

Inputs:
- committed bit-exact fixtures.

Checks:
- orient2d,
- orient3d,
- incircle,
- insphere.

Authority:
- M1.1 independent exact oracle.

### H2 — Certified-filter safety

Checks:
- every fast-certified sign matches exact expected sign,
- fallback is always allowed,
- fast-path Zero is forbidden,
- range gate sends unsafe arithmetic to fallback.

### H3 — Degeneracy

Checks:
- exact duplicate canonicalization,
- +0/-0 policy,
- affine dimension,
- exact coplanar/cospherical cases,
- symbolic tie oracle,
- input permutation invariance.

### H4 — Tetra topology

Checks:
- positive orientation,
- reciprocal neighbors,
- canonical face equality,
- oriented-face consistency,
- generation/stale handle behavior,
- deliberate corruption detection.

### H5 — Point location

Checks:
- walk result vs exact brute-force scan,
- Vertex/Edge/Facet/Cell classification,
- outside hull states,
- seed quality,
- walk cycle prevention.

### H6 — M2 Delaunay

Later checks:
- all stored tetra positive,
- empty-sphere condition under selected perturbation policy,
- reciprocal topology,
- no duplicate tetra,
- valid convex-hull boundary,
- deterministic canonical topology fingerprint.

### H7 — CAD/meshing product verification

Later:
- CAD Face provenance,
- boundary recovery,
- quality metrics,
- local sizing response,
- TET4 solver qualification,
- mesh convergence.

## 2. Repository layout proposal

```text
tools/
└── meshing_oracle/
    ├── generate_predicate_corpus.py
    ├── exact_rational.py
    ├── exact_dyadic_integer.py
    └── README.md

tests/
└── meshing/
    └── robust_geometry/
        ├── fixtures/
        │   ├── orient2d.d26pred
        │   ├── orient3d.d26pred
        │   ├── incircle.d26pred
        │   └── insphere.d26pred
        ├── test_predicates.cpp
        ├── test_filter_certification.cpp
        ├── test_degeneracy.cpp
        ├── test_tet_topology.cpp
        └── test_point_location.cpp
```

Exact filenames remain implementation-stage decisions, but ownership boundaries are the research result.

## 3. No external runtime dependency

The first oracle generator uses only Python standard library:

- `fractions.Fraction`,
- `struct`,
- `random`,
- `itertools`,
- `hashlib`.

Reasons:
- Python is already present in current CI,
- exact rational arithmetic is independent from C++ production code,
- no property-test or arbitrary-precision package dependency is required.

A later research-only tool may add another external oracle, but production verification must not depend on it.

## 4. Small committed truth vs large generated stress

Use two corpus classes.

### Committed corpus

Contains:
- canonical sign cases,
- exact zero,
- adversarial near-zero,
- every discovered regression,
- representative scale/offset cases.

Purpose:
- fast CI,
- permanent replay,
- reviewable source truth.

### Deterministic generated corpus

Generated from recorded seeds into the build directory.

Contains:
- thousands/millions of structured randomized cases,
- broad exponent ranges,
- near-degenerate families.

Purpose:
- deeper verification without bloating Git history.

The generator itself must be deterministic for a given version+seed.

## 5. Failure artifact

Every generated mismatch must print or write a single-case replay record containing:

- predicate,
- expected sign,
- actual sign/filter outcome,
- raw binary64 bits,
- corpus class,
- seed,
- generator version,
- case index.

The replay record must be directly ingestible by the C++ fixture reader.

## 6. Regression promotion

Any generated failure that reveals a real bug is promoted into the committed corpus.

Workflow:

```text
generated failure
→ minimize/reduce if useful
→ assign stable case ID
→ commit fixture
→ fix implementation
→ permanent regression
```

## 7. Deterministic summaries

Tests report counters, not large success logs.

Example:

```text
orient3d:
cases=125000
certified=124812
fallback=188
zero=4
mismatch=0
```

CI failure prints only:
- first N failures,
- replay records,
- summary.

## 8. Reference implementation principle

Slow reference code is intentionally simple.

Examples:
- brute-force point location,
- permutation determinant oracle,
- full topology validator.

It is not used in production hot loops.

Its purpose is to make optimized code independently checkable.
