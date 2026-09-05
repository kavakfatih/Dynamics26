# M1.6 — Executable Robust-Geometry Verification Prototype

**Program:** Dynamics26 Original Meshing System R&D  
**State:** VERIFYING  
**Implemented:** 2026-09-05  
**Production Delaunay code:** not started

## Purpose

M1.6 is the first executable evidence package built from M1.1–M1.5 research.

It implements test infrastructure only. It does not yet provide a production robust-predicate library or a Delaunay tetrahedralizer.

## Implemented components

### Dual exact predicate oracle

Path A:

\`\`\`text
binary64
→ float.as_integer_ratio()
→ Fraction
→ translated exact determinant
→ sign
\`\`\`

Path B:

\`\`\`text
binary64 bit decomposition
→ exact dyadic integer scaling
→ homogeneous integer determinant
→ sign
\`\`\`

The two arithmetic paths are intentionally structured differently.

Implemented predicates:

- orient2d,
- orient3d,
- incircle,
- insphere.

### Bit-exact fixture corpus

Schema:

\`\`\`text
D26PRED 1
\`\`\`

Coordinates are stored as raw IEEE-754 binary64 hexadecimal bit patterns.

Committed corpus currently covers:

- canonical positive/negative cases,
- exact zero,
- signed zero,
- large common offsets,
- tiny power-of-two scale,
- near-coplanar orient3d.

### Deterministic oracle cross-check

The Python verification script:

- checks hand-known canonical signs,
- cross-checks Oracle A/B,
- runs 256 deterministic randomized cases per predicate,
- rejects NaN/infinity,
- regenerates fixtures twice,
- compares regenerated fixtures bit-for-bit with committed fixtures.

Current local research-prototype run:

\`\`\`text
predicates=4
random_cases=1024
dual_oracle=agree
fixtures=bit-exact
deterministic=yes
\`\`\`

This local run is evidence, not a substitute for exact-head CI.

### C++ fixture reader

The test reader verifies:

- strict schema version,
- all required headers,
- exact coordinate field count,
- 16-digit hexadecimal encoding,
- finite binary64 values,
- bit-cast round trip,
- duplicate case-ID rejection,
- malformed sign rejection,
- non-finite fixture rejection,
- signed-zero normalization policy.

Current committed corpus:

\`\`\`text
25 cases
4 predicates
\`\`\`

Standalone local compile/run passed before repository commit.

## Repository locations

\`\`\`text
tools/meshing_oracle/
├── exact_oracle.py
└── generate_predicate_corpus.py

tests/meshing/robust_geometry/
├── test_oracle_generation.py
├── test_predicate_fixture_reader.cpp
└── fixtures/
    ├── orient2d.d26pred
    ├── orient3d.d26pred
    ├── incircle.d26pred
    └── insphere.d26pred
\`\`\`

## CTest integration

Tests:

\`\`\`text
unit_m16_exact_oracle
unit_m16_predicate_fixture_reader
\`\`\`

Labels include:

\`\`\`text
meshing
robust-geometry
predicate
exact-oracle
fixture
determinism
\`\`\`

The existing macOS arm64 Debug/Release workflow will execute them through the normal CTest run.

## Commercial benchmark continuity

M1.6 does not change the commercial benchmark conclusion from M1.5:

- ANSYS and COMSOL expose multi-metric mesh verification and warning/error workflows,
- Marc emphasizes nonlinear mesh adequacy/remeshing in public material,
- none of these products exposes a public internal predicate verification system that can serve as Dynamics26 truth.

Therefore exact predicate truth remains an independent Dynamics26 verification responsibility.

## Qualification boundary

M1.6 is not QUALIFIED until exact-head macOS arm64 CI confirms:

1. Python oracle test passes,
2. C++ fixture reader compiles and passes in Debug,
3. C++ fixture reader compiles and passes in Release,
4. full repository CTest suite remains green.

After that, the next executable step is the production/reference \`RobustPredicates\` kernel, followed by the M2 serial Delaunay reference prototype.
