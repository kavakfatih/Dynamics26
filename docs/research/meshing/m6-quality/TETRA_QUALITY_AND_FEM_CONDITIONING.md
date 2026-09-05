# Tetra Quality, Jacobian Geometry and FEM Conditioning

Status: RESEARCHING / derivation record
Date: 2026-09-05

## 1. Why "good tetrahedron" is application-dependent

Shewchuk's linear-element analysis distinguishes at least three different objectives:
- interpolation accuracy,
- discretization behavior,
- stiffness-matrix conditioning.

The element shape that is best for one objective need not be best for another, especially for
anisotropic physics.

Therefore M6 does not define quality as simply "distance from equilateral" in every future context.
For the first **isotropic TET4 baseline**, however, a regular tetrahedron is a useful ideal reference.

## 2. Reference geometry map

Let a TET4 reference-configuration geometry be

    X0, X1, X2, X3.

Define the affine incidence/Jacobian matrix from the standard simplex:

    A0 = [ X1-X0  X2-X0  X3-X0 ].

Then:

    det(A0) = 6 V0

up to the chosen local vertex orientation.

Dynamics26 stores finite tetrahedra with a fixed positive orientation convention, so the corresponding
signed determinant must be positive after mapping the implementation convention consistently.

Important:
- determinant sign -> validity/orientation,
- determinant magnitude -> element size/volume,
- determinant alone -> not a scale-independent shape measure.

A large distorted tetrahedron can have large positive volume and still be poorly shaped.

## 3. Weighted shape map

To compare with an **equilateral** ideal rather than the right standard simplex, introduce a fixed
ideal-reference matrix W and define:

    T = A0 W^{-1}.

If the physical tetrahedron is a translated, rotated and uniformly scaled regular tetrahedron:

    T = s R,

where s>0 and R is orthogonal.

The singular values of T are then equal.

For an arbitrary tetrahedron let singular values be:

    sigma1 >= sigma2 >= sigma3 > 0.

Shape degeneracy is associated with sigma3 -> 0.

## 4. Mean-ratio quality

A convenient normalized isotropic TET4 shape score is:

    q_MR(T) = 3 det(T)^(2/3) / ||T||_F^2,

for det(T)>0.

Properties:
- 0 < q_MR <= 1,
- q_MR = 1 for a regular tetrahedron up to rotation/uniform scale,
- q_MR -> 0 as the element approaches degeneracy,
- invariant under translation, rigid rotation and uniform positive scaling.

For tetrahedra an equivalent common edge/volume form is:

    q_MR = 12 (3 V)^(2/3) / sum_{i<j} l_ij^2.

The normalization gives q_MR=1 for a regular tetrahedron.

Liu/Joe analyze relationships among tetrahedral shape measures, and Knupp shows the algebraic
mean-ratio/condition family is closely tied to singular values and distance from degeneracy.

## 5. Condition-number shape score

Using the Frobenius matrix condition number:

    kappa_F(T) = ||T||_F ||T^{-1}||_F.

For a 3D similarity transform the minimum is 3.

Define:

    q_kappa = 3 / kappa_F(T).

Then:
- 0 < q_kappa <= 1,
- q_kappa=1 for regular/similar ideal tets,
- q_kappa -> 0 as T approaches singularity.

Knupp proves the condition-number shape metric is equivalent, in the shape-measure sense, to the
mean-ratio family and that inverse condition measures distance to singular/degenerate mappings.

### Dynamics26 implication

q_MR and q_kappa are mathematically related and partly redundant.

Leading policy:
- q_MR as primary smooth isotropic shape/optimization metric,
- q_kappa as a solver-facing diagnostic/cross-check,
- do not pretend they provide two independent votes.

## 6. Why poor shape affects element matrices

For linear basis functions:

    grad_X N = A0^{-T} grad_xi N.

So physical shape-function gradients are amplified by the inverse geometry map.

For a scalar elliptic element:

    K_e = integral (grad N)^T D (grad N) dV

and for linear elasticity:

    K_e = integral B^T C B dV,

where B contains the same inverse-map geometry factors.

As sigma_min(A0) approaches zero:
- ||A0^{-1}|| grows,
- element gradients grow,
- large stiffness entries/eigenvalues can appear,
- matrix conditioning can deteriorate.

This does **not** mean geometry alone determines the global solver condition number.

Global conditioning also depends on:
- element sizes,
- connectivity,
- boundary conditions,
- material stiffness and anisotropy,
- bulk/shear contrast,
- formulation,
- scaling/preconditioning.

Mesh quality is a geometric contributor, not the whole linear-system diagnosis.

## 7. One bad element matters

The modern mesh-quality survey and classical FEM analyses emphasize that badly shaped elements can
strongly influence stiffness conditioning; an average quality value can hide this.

Dynamics26 should therefore report, per metric:
- invalid count,
- minimum / maximum where meaningful,
- low percentiles such as p0.1 / p1 / p5,
- median,
- distribution/histogram,
- count below research/validated bands.

A mesh-wide arithmetic mean is supplementary only.

## 8. Radius ratio

For tetrahedron inradius r and circumradius R:

    q_RR = 3 r / R.

For a regular tetrahedron r/R = 1/3, hence q_RR=1.

As a flat tetrahedron approaches zero volume, its inradius tends to zero, so q_RR detects many
degenerate/sliver shapes.

Liu/Joe show radius ratio, mean ratio and solid-angle-based measures are equivalent in a mathematical
shape-regularity sense, but equivalence does not make them equally useful for optimization or
diagnostics.

## 9. Radius-edge ratio

Define:

    rho_RE = R / l_min,

with circumradius R and shortest edge l_min.

For a regular tetrahedron:

    rho_RE = sqrt(6)/4 ~= 0.612372.

Smaller is better in the usual Delaunay-refinement convention, with the regular tetrahedron attaining
the lower bound.

rho_RE is important because Delaunay refinement algorithms can control it and relate it to point
spacing/feature size.

But it is not a complete FEM shape metric because slivers can have bounded/good rho_RE.

## 10. Angle diagnostics

Store all six interior dihedral angles or at least:

    theta_min
    theta_max.

A regular tetrahedron has interior dihedral angle:

    arccos(1/3) ~= 70.5288 degrees.

Slivers typically drive some dihedral angles toward 0 or pi.

However angle metrics also have blind spots:
- minimum dihedral angle alone can miss some needle-like tetrahedra,
- edge ratio alone can miss flat tetrahedra,
- radius-edge ratio alone can miss slivers.

This is direct evidence for a metric suite rather than one universal number.

## 11. Scaled/signed Jacobian

For linear TET4 the affine Jacobian is constant in the element.

A signed/normalized Jacobian is valuable for:
- inversion/orientation,
- degeneration margin,
- later consistency with HEX/TET10 reporting.

But for TET4 the primary isotropic shape score should be derived from a weighted map/singular-value
family rather than raw det(A0), because determinant is scale-dependent.

For TET10/curved elements the Jacobian varies spatially and M8 must define sampling/optimization over
the element domain.

## 12. Isotropic versus anisotropic quality

Equilateral-reference metrics are appropriate for an isotropic baseline.

They are **not** a universal rule that long/thin elements are bad.

For anisotropic PDEs or metric-field adaptation, the desired element may be stretched and aligned
with the solution/metric tensor. Shewchuk explicitly analyzes cases where anisotropic elements are
superior.

Future M9 should evaluate shape in transformed metric space, conceptually replacing the Euclidean
ideal W by the local target metric.

M6 must preserve this extension path.

## 13. Proposed TET4 geometric metric roles

| Quantity | Role |
|---|---|
| exact positive orientation / signed volume | hard validity |
| q_MR | primary isotropic shape score / optimization candidate |
| q_kappa | conditioning/degeneracy diagnostic |
| theta_min, theta_max | sliver/angle diagnostic |
| q_RR | independent geometric shape cross-check |
| rho_RE | Delaunay refinement / spacing quality |
| edge ratio | telemetry only; not sufficient as release gate |
| element volume | size/sizing telemetry; not normalized shape |

No final threshold is accepted in M6 early research.


## 14. Exact q_MR / q_kappa singular-value relation

Let:

    lambda_i = sigma_i^2
    g = (lambda1 lambda2 lambda3)^(1/3)
    a_i = lambda_i/g

so that a1 a2 a3=1.

Define:

    S = a1+a2+a3
    P = 1/a1+1/a2+1/a3.

Then:

    q_MR(T) = 3/S
    q_MR(T^-1) = 3/P

and:

    q_kappa(T)^2
      = q_MR(T) q_MR(T^-1).

Therefore:

    q_MR^(3/2)
      <= q_kappa
      <= q_MR^(1/2).

This identity is a useful implementation oracle and also proves that q_MR and q_kappa must not be
treated as independent release votes.

For kappa_2=sigma_max/sigma_min:

    1/kappa_2
      <= q_kappa
      <= min(1, 3/(kappa_2 + 1/kappa_2)).

See MULTI_METRIC_SOLVER_AWARE_FRAMEWORK.md for the derivation and pathology consequences.

## 15. Local stiffness condition needs null-space semantics

For scalar affine P1:

    K_e = V G^T D G
    G = A0^-T G_hat.

The element has a constant null mode. Its useful local conditioning diagnostic is therefore based on
the positive spectrum, not the ordinary full condition number.

For an unconstrained linear-elastic TET4 there are six rigid-body null modes and the same rule
applies.

After removing the physical null space, geometry enters the local spectral bounds through inverse-map
factors and can contribute approximately/quadratically in kappa_2(T), up to fixed reference and
material constants.

This is not an equality for the assembled global stiffness matrix.

## 16. Interpolation quality is not stiffness quality

Križek's tetrahedral maximum-angle result permits certain degenerating tetrahedra while preserving
the standard linear interpolation error order.

Shewchuk independently shows that interpolation and stiffness-conditioning quality objectives do not
coincide.

Therefore:
- q_MR is not a universal interpolation-error oracle,
- low q_MR can be unacceptable for the isotropic M6 baseline while a future metric-aligned
  anisotropic element may still be approximation-efficient,
- stiffness/solver evidence must be measured separately.

## 17. Grading is a separate solver-context axis

A shape-regular graded mesh can have ideal per-element q_MR/q_kappa/q_RR while containing a large
element-size range.

Bank/Scott show that local refinement need not significantly degrade conditioning when the mesh is
nondegenerate and natural basis scaling is used, especially in 3D asymptotic bounds.

Dynamics26 must therefore record:
- shape distribution,
- size/gradation distribution,
- raw matrix condition/iterations,
- scaling/preconditioner,

as distinct observations.
