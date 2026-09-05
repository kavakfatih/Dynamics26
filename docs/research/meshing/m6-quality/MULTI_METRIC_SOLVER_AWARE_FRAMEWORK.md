# M6 Research — Multi-Metric, Solver-Aware Tetra Quality Framework

Status: RESEARCHING / engineering-contract candidate
Date: 2026-09-06

## 1. Research question

Why is one scalar quality metric insufficient for a tetrahedral FEM mesh, and how should Dynamics26
combine:

- mean ratio,
- weighted-Jacobian condition,
- dihedral angles,
- radius ratio,
- radius-edge ratio,

without confusing geometric validity, approximation error, stiffness conditioning, mesh grading,
large deformation or nearly-incompressible formulation stability?

The answer developed here is:

    do not build one opaque composite score.

Use a layered quality vector in which every quantity has a declared mathematical role.

Primary literature used in this derivation is registered as M6-TH-001, M6-TH-003, M6-TH-004 and
M6-FEM-004..006.

## 2. Weighted map and singular values

For a positively oriented affine TET4, let

    A0 = [X1-X0, X2-X0, X3-X0]

and let W be the fixed ideal-equilateral reference map.

Define

    T = A0 W^-1.

Let the singular values of T be

    sigma1 >= sigma2 >= sigma3 > 0

and define

    lambda_i = sigma_i^2.

A regular tetrahedron up to translation, rotation and positive uniform scale has

    sigma1 = sigma2 = sigma3.

Approach to geometric degeneracy is represented by a singular-value spread, especially

    sigma3 -> 0

after shape normalization.

Exact positive Orient3D remains the hard validity authority. Singular-value quality arithmetic never
replaces predicate truth.

## 3. Mean ratio in spectral form

For det(T)>0:

    q_MR(T)
      = 3 det(T)^(2/3) / ||T||_F^2
      = 3 (lambda1 lambda2 lambda3)^(1/3)
          / (lambda1 + lambda2 + lambda3).

Define the geometric mean

    g = (lambda1 lambda2 lambda3)^(1/3)

and normalized eigenvalues

    a_i = lambda_i / g.

Then

    a1 a2 a3 = 1

and, with

    S = a1 + a2 + a3,

we have

    q_MR(T) = 3/S.

Thus mean ratio is the ratio of the geometric mean to arithmetic mean of the squared singular values,
normalized so the ideal value is one.

It is:
- translation invariant,
- rigid-rotation invariant,
- uniform-scale invariant,
- sensitive to collapse,
- an isotropic shape metric rather than a complete solver metric.

## 4. Frobenius condition score and an exact Dynamics26 relation

Define

    kappa_F(T) = ||T||_F ||T^-1||_F

and

    q_kappa(T) = 3 / kappa_F(T).

For the normalized eigenvalues above, define

    P = 1/a1 + 1/a2 + 1/a3
      = a1 a2 + a1 a3 + a2 a3

because a1 a2 a3 = 1.

Then

    q_kappa(T)^2 = 9/(S P).

Applying the same mean-ratio definition to the inverse map gives

    q_MR(T^-1) = 3/P.

Therefore the exact identity is:

    q_kappa(T)^2
      = q_MR(T) q_MR(T^-1).

This makes the relationship between the two metrics stronger than a qualitative correlation.

### 4.1 Bounds

Since

    P >= 3

by AM-GM, and

    S^2 >= 3P,

we obtain:

    q_MR(T)^3
      <= q_kappa(T)^2
      <= q_MR(T),

or equivalently:

    q_MR(T)^(3/2)
      <= q_kappa(T)
      <= q_MR(T)^(1/2).

At the ideal regular tetrahedron all are one.

### 4.2 Relation to the spectral condition number

Let

    kappa_2(T) = sigma1/sigma3.

From the singular-value sums:

    1/kappa_2(T)
      <= q_kappa(T)
      <= min(
           1,
           3 / (kappa_2(T) + 1/kappa_2(T))
         ).

For a strongly ill-conditioned map:

    q_kappa = Theta(1/kappa_2).

### Dynamics26 consequence

q_MR and q_kappa must not be counted as two independent "votes".

Instead:
- q_MR is the primary smooth isotropic optimization score,
- q_kappa is a map-conditioning/distance-to-degeneracy diagnostic,
- their exact identity/bounds form an independent implementation consistency oracle.

## 5. Radius ratio is a different formula, not an independent shape theorem

Define

    q_RR = 3r/R.

Liu and Joe show that radius ratio, mean ratio and minimum-solid-angle families are equivalent in the
shape-regularity sense: if one collapses to zero along a degenerating tetrahedral family, the others
also collapse, up to measure-dependent bounds.

Therefore q_RR is valuable as:
- a geometrically different formula,
- a cross-check against coding mistakes,
- a diagnostic with different finite-range sensitivity,

but not as a statistically or mathematically independent second release vote against q_MR.

## 6. Radius-edge has a different job

Define

    rho_RE = R/l_min.

It is central to Delaunay-refinement/spacing reasoning.

It is not a complete FEM shape score because a sliver can have:
- all edges of moderate length,
- a bounded circumradius,
- nearly zero volume,
- disastrous dihedral angles,

while rho_RE remains moderate.

Dynamics26 therefore treats rho_RE as a refinement/spacing diagnostic, not a solver-suitability
certificate.

## 7. Dihedral angles are interpretable pathology sentinels

Store at least:

    theta_min
    theta_max

over the six interior dihedral angles.

These are especially useful for:
- sliver detection,
- near-flat wedge detection,
- user-facing geometry interpretation.

But dihedral extrema are not complete. The analytic needle family below demonstrates an element whose
volume and singular-value quality collapse while all interior dihedral angles remain bounded away
from both 0 and pi.

## 8. Analytic pathology family S — sliver

Reuse the committed family:

    A = ( 1,  0, 0)
    B = (-1,  0, 0)
    C = ( 0,  1, epsilon)
    D = ( 0, -1, epsilon)

for epsilon>0.

Previously derived:

    V = 2 epsilon / 3

    q_MR
      = 3 (2 epsilon)^(2/3)
        / (4 + epsilon^2)
      -> 0

    q_RR -> 0

    rho_RE -> 1/sqrt(2).

The important blind spot is:

    rho_RE does not diverge while the sliver becomes arbitrarily flat.

Angles and singular-value shape metrics detect the collapse.

## 9. Analytic pathology family N — needle

Define, epsilon>0:

    A = (0,0,0)
    B = (1,0,0)
    C = (0,epsilon,0)
    D = (0,0,epsilon).

Then:

    6V = epsilon^2

and the sum of squared edge lengths is

    sum l_ij^2 = 3 + 6 epsilon^2.

Therefore:

    q_MR(epsilon)
      = 2^(4/3) epsilon^(4/3)
        / (1 + 2 epsilon^2)
      -> 0.

The circumcenter is:

    (1/2, epsilon/2, epsilon/2)

so:

    R = 1/2 sqrt(1 + 2 epsilon^2).

For epsilon<1 the shortest edge is epsilon, hence:

    rho_RE
      = sqrt(1 + 2 epsilon^2)
        / (2 epsilon)
      -> infinity.

Also:

    q_RR -> 0
    q_kappa -> 0.

### Critical dihedral result

As epsilon -> 0, the six interior dihedral angles remain bounded in a benign-looking interval; their
limiting extrema are:

    theta_min -> 45 degrees
    theta_max -> 90 degrees.

Thus:

    dihedral extrema alone can miss a needle degeneration.

This family also illustrates why interpolation and conditioning are not the same question. Classical
maximum-angle theory allows certain degenerating tetrahedra while retaining linear interpolation
error estimates. A low isotropic q_MR is therefore not, by itself, a theorem that interpolation must
fail for every PDE/solution structure.

## 10. Analytic pathology family W — wedge / edge-collapse cap

Define:

    A = (0,0,0)
    B = (1,0,0)
    C = (0,1,0)
    D = (1/2,0,epsilon).

Then:

    6V = epsilon

and:

    sum l_ij^2
      = 23/4 + 3 epsilon^2.

Hence:

    q_MR(epsilon)
      = 12 (epsilon/2)^(2/3)
        / (23/4 + 3 epsilon^2)
      -> 0.

The circumcenter has z-coordinate

    z_c = epsilon/2 - 1/(8 epsilon),

so:

    R ~ 1/(8 epsilon).

The shortest edge tends to 1/2, therefore:

    rho_RE ~ 1/(4 epsilon) -> infinity.

For this family:

    q_RR -> 0
    q_kappa -> 0
    theta_min -> 0
    theta_max -> pi.

Thus wedge collapse is visible to the angle and radius-edge diagnostics as well as the singular-value
shape metrics.

## 11. Pathology / metric detection matrix

The table is qualitative; "detects" means the metric has a useful asymptotic signal, not that a
product threshold is already accepted.

| Pathology / context | Exact orientation | q_MR | q_kappa | Dihedral extrema | q_RR | rho_RE | Separate context required |
|---|---|---|---|---|---|---|---|
| positive sliver -> flat | remains positive until limit | detects | detects | detects strongly | detects | **can miss** | sliver treatment |
| needle N(epsilon) | remains positive until limit | detects | detects | **can miss** | detects | detects strongly | interpolation/conditioning split |
| wedge W(epsilon) | remains positive until limit | detects | detects | detects strongly | detects | detects strongly | none beyond metric suite |
| inverted/zero tet | **hard truth** | not authority | not authority | not authority | not authority | not authority | predicate/topology |
| graded, shape-regular mesh | passes | ideal/near ideal | ideal/near ideal | ideal/near ideal | ideal/near ideal | ideal/near ideal | size/gradation + global solver |
| intentionally anisotropic useful tet | passes | may penalize | may penalize | context dependent | may penalize | context dependent | PDE/metric tensor |
| nearly-incompressible locking | passes | may be excellent | may be excellent | may be excellent | may be excellent | may be excellent | element formulation |

This matrix is the central proof that one scalar cannot represent all required engineering questions.

## 12. Element stiffness conditioning: use the positive spectrum

For a scalar P1 diffusion/elliptic element, let the reference gradient matrix be G_hat.

The physical gradient matrix is:

    G = A0^-T G_hat.

For a constant SPD material/diffusion tensor D:

    K_e = V G^T D G.

K_e has a constant-mode null vector, so its ordinary full condition number is infinite and is not a
useful element metric.

The three positive eigenvalues of K_e equal the nonzero spectrum of a 3x3 operator of the form:

    V D^(1/2)
      A0^-T
      (G_hat G_hat^T)
      A0^-1
      D^(1/2).

After absorbing fixed reference-element constants into the weighted map, the geometry contribution to
the positive-spectrum condition is bounded by factors proportional to:

    kappa_2(T)^2,

together with the material condition and fixed reference constants.

For linear 3D elasticity a free TET4 has six rigid-body zero modes. The analogous diagnostic must
again operate on the positive/non-rigid spectrum.

### Dynamics26 rule

Never report:

    condition(K_e)

without specifying null-space treatment.

Use an explicitly named quantity such as:

    kappa_e_positive

or a local stiffness spectral proxy with:
- rigid/constant null modes removed,
- material tensor recorded,
- formulation recorded.

And never assert the false identity:

    kappa(K_global) = kappa(T)^2.

Assembly, BCs, size distribution, material contrast, scaling and preconditioning all matter.

## 13. Mesh grading is not shape quality

A mesh can consist entirely of regular tetrahedra and still have a large size range.

Then:

    q_MR = q_kappa = q_RR = 1

for every element, with ideal dihedral and radius-edge values, while the assembled linear system can
change substantially.

Conversely, Bank and Scott show that local refinement does not necessarily cause a severe condition
penalty when meshes remain nondegenerate and a natural basis scaling is used; in three dimensions
their bound has the same asymptotic form as for regular meshes.

Therefore:

    adjacent size ratio
    h_max/h_min
    global stiffness condition
    solver iterations

must not be collapsed into "element shape quality".

Research telemetry candidate for adjacent cells is:

    g_ij = max(h_i/h_j, h_j/h_i) >= 1,

where the authoritative h_T definition belongs to the M5/M6 sizing contract and is not frozen here.

Record raw, scaled and preconditioned solver behavior separately.

## 14. Interpolation error and stiffness conditioning can disagree

Križek's maximum-angle result proves that certain degenerating tetrahedral families can retain the
standard O(h) linear interpolation estimate in W_p^1 when the maximum-angle condition holds.

Shewchuk likewise emphasizes that element qualities for interpolation and stiffness conditioning do
not coincide.

Implications:

- no isotropic shape metric is a universal interpolation-error oracle,
- an anisotropic element can be useful when aligned with solution structure,
- stiffness conditioning can be sensitive to geometric features that do not destroy interpolation,
- future M9 anisotropic adaptation must assess shape in target metric space.

M6's Euclidean regular-tetra q_MR remains the isotropic baseline, not a permanent prohibition on
anisotropy.

## 15. Reference versus current nonlinear quality

For large deformation:

    A_t = F A0.

With the ideal map:

    T0 = A0 W^-1
    Tt = A_t W^-1 = F T0.

Therefore:

    kappa_2(Tt)
      <= kappa_2(F) kappa_2(T0).

This supports three separate observations:
- reference shape quality: T0,
- physical deformation: F and J_F=det(F),
- current distortion: Tt.

A low current q_MR can be caused by legitimate physical shear/stretch.

Hard nonlinear invalidity is instead tied to the constitutive/kinematic domain, especially:

    J_F <= 0

for ordinary orientation-preserving hyperelasticity.

Current quality is solver/adaptivity telemetry; it must not silently move nodes or alter physics.

## 16. Nearly-incompressible rubber: geometry cannot certify formulation stability

A regular TET4 can still lock severely in a low-order pure-displacement nearly-incompressible
formulation.

Conversely, a mixed/stabilized element can address the incompressibility constraint while still being
sensitive to poor geometric shape.

Dynamics26 therefore needs the crossed experiment:

    geometry family
      x
    formulation family
      x
    K/G or compressibility regime.

The current dedicated formulation package at:

    docs/research/fem/tet4-nearly-incompressible/

already defines this separation. In particular, T4-R09 crosses M6 geometry pathologies with
formulation behavior.

No mesh-quality metric may imply:

    rubber-ready = true.

That is a solver/formulation qualification result.

## 17. Dynamics26 layered quality-vector candidate

The research contract is:

### Layer 0 — hard validity

Authority:
- exact positive Orient3D,
- finite/distinct nodes,
- manifold/topology/provenance validity.

This layer is binary.

### Layer 1 — isotropic element shape

Primary:
- q_MR.

Diagnostics:
- q_kappa,
- theta_min/theta_max,
- q_RR.

Rules:
- q_MR and q_kappa are related, not independent votes,
- q_RR is a formula-independent cross-check but shape-regularity-equivalent,
- angle extrema are complementary pathology sentinels.

### Layer 2 — refinement / sizing / grading

Metrics:
- rho_RE,
- target-size compliance,
- local edge/element size distribution,
- gradation telemetry.

This layer answers point-spacing and sizing questions, not constitutive suitability.

### Layer 3 — FEM numerical context

Metrics/evidence:
- positive-spectrum local stiffness proxy,
- global condition estimate,
- iterative solver iterations,
- raw/scaled/preconditioned comparison,
- interpolation/discretization error against a reference solution.

Always record PDE/material/formulation/BC.

### Layer 4 — nonlinear current state

Metrics:
- J_F,
- kappa(F),
- q_MR(Tt),
- q_kappa(Tt),
- current angles,
- reference-to-current distortion ratios,
- Newton iterations/cutbacks/inversions.

### Layer 5 — formulation suitability

Examples:
- compressible P1 TET4 qualification,
- nearly-incompressible MINI qualification,
- stabilized P1/P1 qualification,
- F-bar patch qualification,
- later hybrid/mixed TET10 qualification.

This layer is not inferred from geometry.

## 18. Product reporting consequence

Do not expose one opaque status:

    Mesh Quality = 0.74
    => Good

as an engineering conclusion.

A future report should instead expose categories such as:

    Topology Validity
    Initial Shape Quality
    Sizing / Gradation
    FEM Conditioning Context
    Nonlinear Distortion
    Formulation Suitability

A UI may summarize categories, but the underlying engineering record must preserve the individual
metrics and benchmark domain.

## 19. Threshold consequence

Only mathematical invalidity is currently a hard universal gate.

Numerical quality limits must later be scoped to:
- element/formulation,
- material/compressibility regime,
- linear/nonlinear analysis class,
- size/gradation domain,
- solver scaling/preconditioner,
- verified benchmark ID.

No generic number such as:

    q_MR > 0.2

is accepted by this research.

## 20. Verification requirements created by this research

Add executable/oracle research gates for:
- exact q_MR/q_kappa identity and bounds over analytic/random singular spectra,
- needle N(epsilon) asymptotics and dihedral blind spot,
- wedge W(epsilon) asymptotics,
- sliver/needle/wedge metric detection matrix,
- positive-spectrum local stiffness conditioning with null modes removed,
- maximum-angle/interpolation versus isotropic-shape separation,
- graded shape-regular meshes with raw/scaled/preconditioned solver measurements,
- M6 geometry x TET4 formulation cross-correlation.

These are research gates only. They do not authorize M6 implementation.


## 21. Spectral metrics do not encode all tetra morphology

The weighted affine map has five continuous shape DOFs after physical rotation and uniform scale are
removed.

The singular-value spectrum carries only two of those DOFs.

Therefore q_MR and q_kappa, both functions of the singular values, cannot identify every tetra
morphology.

A fixed-spectrum family:

    T(phi) = Sigma R(phi)

keeps q_MR/q_kappa unchanged while angle and radius diagnostics can vary.

This is why the Layer-1 diagnostic set must retain geometry outside the singular spectrum.

See TETRA_PATHOLOGY_AND_ANGLE_CONDITIONS.md.

## 22. Angle telemetry must distinguish face and dihedral angles

For tetrahedral linear interpolation, maximum-angle theory has two independent components:
- triangular face angles,
- dihedral angles.

Therefore the research telemetry is extended conceptually with:

    face_angle_min
    face_angle_max.

Existing:

    theta_min
    theta_max

remains the dihedral pair.

Do not combine these into one undocumented "angle quality" value.

## 23. Solid angles are pathology telemetry

Research also tracks:

    solid_angle_min
    solid_angle_max

because cap/spike/needle morphology can contain solid-angle information not visible from a simple
dihedral-extrema report.

Solid angles are diagnostic only; they are not selected as the M6 primary optimization objective.
