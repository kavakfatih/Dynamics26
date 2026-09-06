# M8 Future Direction — TET10 and Curved Boundaries

Status: **FUTURE DIRECTION DEFINED / NO IMPLEMENTATION / NOT A TET4 BLOCKER**  
Direction ID: **D26-M8-DIRECTION-1**  
Research package: **M8 — TET10 / Curved Boundaries**  
Date: 2026-09-06

## 1. Boundary

M8 follows the TET4 product path and dedicated TET4 solver/formulation qualification. It must not block V1.2 TET4 development.

## 2. Curved-element validity

A positive straight-sided TET4 does not imply that a curved TET10 obtained by moving midside nodes is valid.

For curved isoparametric mapping `x(xi)`, mathematical validity requires:

```text
det J(xi) > 0
for every xi in the reference tetrahedron
```

Checking only interpolation nodes or a finite set of Gauss points is not a complete certificate.

## 3. Frozen future research direction

Preferred direction:
1. express the Jacobian-determinant polynomial in a Bernstein/Bézier basis,
2. use coefficient bounds to certify positivity when possible,
3. subdivide the reference domain when bounds are inconclusive,
4. perform curved-mesh optimization/untangling while preserving CAD constraints,
5. accept only after continuous validity certification or explicit bounded failure.

This is a research direction, not a production algorithm freeze.

## 4. Relationship to M6 and solver research

M6 straight-TET4 quality mathematics remains unchanged. Curved-element validity/quality will require separate M8 policies.

Higher-order TET10 mechanical formulation qualification is likewise distinct from geometric TET10 validity.

## 5. Development rule

DEV-MESH-P7 begins only after the TET4 path is mature enough to justify curved/high-order work. No M8 implementation is authorized by this closeout.
