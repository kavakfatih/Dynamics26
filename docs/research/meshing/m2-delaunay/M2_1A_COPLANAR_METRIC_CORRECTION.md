# P1 audit — coplanar metric freeze mismatch and correction

Date: 2026-09-06  
Audit baseline: `d2163476bd52d085e2efc0680ec1e5b4899b8460`  
Decision: ADR-MESH-0043  
Status: CORRECTED P1A SUBSCOPE QUALIFIED  
Source HEAD: `6962866dde18fa5dc17c48f49212594b091c2e32`  
CI: [macOS arm64 #298](https://github.com/kavakfatih/Dynamics26/actions/runs/34037763796)
completed/success; Debug and Release full CTest, M1/M2 tests and gui-fast passed.
Local supplementary GCC O0/O2 and address/undefined sanitizers passed (leak
checking disabled in the container). These local runs are not Mac qualification.

## Reproduced defect

The frozen recipe and implementation agreed with each other but used the wrong
metric on oblique planes. For A=(0,0,0), B=(1,0,1), C=(0,1,0), the Euclidean
circumcenter is (1/2,1/2,1/2), R²=3/4. Query P=(9/8,1/2,9/8) has squared
distance 25/32 > 24/32: strictly outside. XY InCircle instead sees squared
distance 25/64 < 1/2: inside. All inputs are exact binary64 dyadics.

The added C++ regression failed on the original source with:
`oblique 3D circle exterior was classified using the XY metric`.

## Independent derivation

For a circle center c in the facet plane, its equation is
`||p||² - 2 c·p + ||c||² - R² = 0`.
On a plane, choosing any nondegenerate coordinate pair (u,v) expresses the third
coordinate as an affine function of (u,v). Hence the circle equation is
`||p||² = A*u+B*v+C`. The lifted four-row determinant `[u v ||p||² 1]` is zero
exactly on the circle. With positive projected triangle orientation its sign is
positive inside and negative outside: subtracting the circle's affine lift plane
from the lift column gives orientation times `(R²-||query-c||²)`.

Projection is only a coordinate chart; it does not define the metric.
Use XY, then XZ, then YZ for the first exact nonzero projected orientation.
All three original coordinates remain in the norm. No normal normalization,
square root, rounded squared norm or numeric epsilon is used.

A shared positive dyadic coordinate scale alpha multiplies the two coordinate
columns by alpha and the norm column by alpha², so the determinant changes by
positive alpha⁴ only. M1's internal integer arithmetic mechanism can therefore
be reused without changing its public predicates or exposing BigInt.

On exact zero, the formal lift cofactors are the existing +,-,+,- projected
Orient2D minors. Ascending PointId priority is unchanged. The ordinary 2D raw
InCircle must not be re-evaluated during this tie branch: it may be nonzero.
No old D26DT1 constructor output is qualified; old P1A gate evidence is retained
as historical limited-corpus evidence, with G09/coplanar G10 reopened.

## Executable verification

`unit_m21a_delaunay_predicates` adds the failed baseline counterexample, a true
oblique-circle zero that is not an XY-circle zero, all six triangle permutations,
and 2601 exact squared-distance checks across three planes and scales 2^-500,
1, 2^500. Existing 120 InSphere and 24 InCircle permutation checks remain.
The C++ squared-distance oracle uses integer arithmetic independent of the
production determinant. The Python sparse-polynomial oracle also verifies the
oblique true-zero lift and orientation-normalized tie.

Production symbol: `classifyProjectedCoplanarCircumcircle` (private M2 API).
The name is retained; its header now states the preserved 3D lift explicitly.

## Scope

UI freeze stays active. No M1 semantic change, CAD mesher, FEM TET path, cavity
mutation or walk is introduced. M2 OVERALL = NOT QUALIFIED.
