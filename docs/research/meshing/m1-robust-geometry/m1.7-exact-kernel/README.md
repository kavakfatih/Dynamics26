# M1.7 — Exact Robust Predicate Kernel

**State:** QUALIFIED  
**Exact-head evidence:** `4794d68c092b...`, macOS arm64 workflow #233 SUCCESS

## Implemented

Public C++ API:
- `PredicateSign`,
- `PredicateEvaluationPath`,
- orient2d,
- orient3d,
- incircle,
- insphere.

Exact path:
```text
finite binary64
→ exact dyadic decomposition
→ common power-of-two integer scale
→ Dynamics26 signed arbitrary-precision integer
→ exact determinant
→ PredicateSign
```

No Boost/CGAL/Gmsh/TetGen predicate runtime/source dependency is used.

M1.7 intentionally optimized correctness before speed.
