# M1.3 Proposed Decision Specification

**Status:** research proposal

## D1 — Preserve exact Zero
Candidate:
- robust predicate returns mathematical `Zero`,
- no epsilon or hidden sign fabrication.

## D2 — Exact duplicates become one site
Candidate:
```text
same exact coordinates
→ one canonical site
→ multiple provenance records
```

## D3 — Near duplicates remain distinct
Candidate for M1/M2:
- only explicit future geometry conditioning may merge them.

## D4 — Lower-dimensional input is explicit
Candidate:
- detect 0D/1D/2D affine dimension,
- requested 3D tetrahedralizer does not fabricate volume elements.

## D5 — SoS-style Delaunay tie
**PROPOSED**:
- distinct sites,
- valid 3D dimension,
- exact Delaunay predicate Zero,
- stable PointId-driven formal perturbation.

Not accepted until symbolic coefficient/oracle experiments pass.

## D6 — Stable identity, never pointer order
Candidate:
- no memory address,
- no unordered iteration,
- no transient insertion index,
- no thread scheduling in symbolic ordering.

## D7 — Canonical final topology
**PROPOSED target**:
```text
same sites + stable IDs + algorithm version
→ same canonical topology fingerprint
```
independent of supported input/insertion ordering.

## D8 — CAD invalidity is not symbolically repaired
Candidate:
- open, non-manifold or self-intersecting engineering boundaries remain explicit diagnostic/repair problems.

## Next proof work
1. formal PointId/component perturbation hierarchy,
2. exact symbolic coefficient oracle,
3. orientation permutation tests,
4. co-spherical Delaunay corpus,
5. insertion-order invariance experiments.
