# M1 Final Closeout Candidate Audit

**Audit date:** 2026-09-05  
**Executable baseline audited:** `0e829f3efe765876155146f92bcd184020a16008`  
**Decision at document creation:** **CANDIDATE PASS — FINAL STATUS AWAITS THIS SYNCHRONIZATION COMMIT CI**

## 1. Gate review

| Gate | State | Final evidence |
|---|---|---|
| G01 CAD tolerance != predicate | PASS | separated contracts |
| G02 four predicate sign conventions | PASS | exact oracle + C++ |
| G03 dual exact oracle | PASS | Fraction + dyadic integer |
| G04 bit-exact fixture round-trip | PASS | D26PRED |
| G05 C++ exact kernel | PASS | M1.7, workflow #233 |
| G06 certified fast path | PASS | M1.8 + homogeneous proof |
| G07 compiler FP contract | PASS | source flags + guard |
| G08 Apple Silicon Debug/Release | PASS | workflows #234/#239 |
| G09 clean-room boundary | PASS | no external predicate runtime/source |
| G10 exact duplicate canonicalization | PASS | M1.9-B, #237 |
| G11 affine dimension | PASS | M1.9-B, #237 |
| G12 formal symbolic oracle | PASS | M1.9-B, #237 |
| G13 stable PointId hierarchy | PASS | canonicalization + symbolic ranking |
| G14 tetra primitive/validator | PASS | M1.9-C, #238 |
| G15 failure replay round-trip | PASS | M1.9-A, #236 |
| G16 adversarial/metamorphic corpus | PASS | M1.9-A, #236 |
| G17 telemetry/baseline | PASS | M1.9-D, #239 |
| G18 docs/status synchronization | PENDING THIS COMMIT CI | M1.9-E |
| G19 ANSYS/COMSOL/Marc continuity | PASS | public workflow benchmark only |
| G20 M2 blocked until final audit | PASS | ADR-MESH-0011 |

## 2. M1/M2 scope check

The following are intentionally M2, not hidden M1 blockers:
- point-location walk,
- cavity extraction,
- local retriangulation,
- production symbolic tie consumption,
- insertion-order topology determinism,
- super-domain/hull representation.

ADR-MESH-0012 freezes this boundary.

## 3. Commercial comparison check

M1 still uses ANSYS, COMSOL and Marc/Mentat only as engineering workflow/quality/diagnostic benchmarks.

No proprietary implementation is treated as predicate truth.

The accumulated comparison remains consistent:
- ANSYS: explicit repair, meshing methods, quality diagnostics and parallel-meshing controls,
- COMSOL: explicit repair tolerance, mesh sequence/status/statistics and tetra quality optimization,
- Marc/Mentat: nonlinear mesh-density, adaptivity/remeshing and closure/distortion workflows.

Nothing in M1 requires guessing their internal orientation/insphere arithmetic.

## 4. Candidate conclusion

All executable M1.9 blockers from the first audit are now closed.

M2 remains blocked until:
1. this synchronized documentation exact-head workflow is green,
2. a second final closeout record changes M1 to QUALIFIED.

No M2 code is authorized by this candidate document.
