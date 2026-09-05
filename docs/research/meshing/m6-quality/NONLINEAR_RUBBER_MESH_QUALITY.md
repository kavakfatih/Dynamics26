# Nonlinear / Rubber Analysis Implications of Tetrahedral Mesh Quality

Status: RESEARCHING
Date: 2026-09-05

## 1. Three Jacobians that must not be confused

For nonlinear solid mechanics, the word "Jacobian" is overloaded.

Dynamics26 documentation should separate:

### Reference geometry map

    A0 = dX/dxi
       = [X1-X0, X2-X0, X3-X0].

This describes initial TET4 mesh geometry.

### Deformation gradient

    F = dx/dX.

Its determinant:

    J_F = det(F)

is the physical local volume ratio.

### Current geometry map

    At = dx/dxi
       = F A0

for affine TET4 kinematics.

Therefore:

    det(At) = J_F det(A0).

These quantities answer different questions.

## 2. Initial mesh validity

Before solution:

    det(A0) > 0

under the implementation orientation convention.

This is a mesh validity requirement.

A small-but-positive determinant may still represent a very poor tetra, so use mean-ratio/condition/
angle metrics in addition.

## 3. Physical finite-deformation validity

Hyperelasticity normally requires orientation-preserving deformation:

    J_F = det(F) > 0.

For nearly incompressible rubber:

    J_F approximately 1

is expected physically.

This is **not** a mesh-quality score.

A regular tetrahedron under an incompressibility-locking displacement formulation can still produce
bad results even though its initial geometric quality is excellent.

## 4. Current element inversion

If initial det(A0)>0:

    det(At) <= 0
    <=> J_F <= 0

for the affine TET4 map.

Thus current geometric inversion and nonphysical deformation-gradient inversion coincide for a
linear tetrahedron.

This is a hard nonlinear failure state for the standard hyperelastic formulation, not merely a low
quality warning.

## 5. Shape under deformation

Let ideal-reference weighted shape maps be:

    T0 = A0 W^{-1}
    Tt = At W^{-1}.

Since At = F A0:

    Tt = F T0.

For any compatible matrix condition number:

    kappa(Tt) <= kappa(F) kappa(T0).

Interpretation:
- poor initial mesh shape can amplify numerical difficulty,
- strong physical deformation can distort even a good initial element,
- the two effects multiply rather than being interchangeable.

This motivates separate reporting of:
- reference mesh quality,
- current configuration distortion,
- deformation-gradient state.

## 6. Do not reject valid physical anisotropy blindly

A rubber component can experience legitimate large shear/stretch.

A low current equilateral-shape score may reflect real deformation rather than a bad initial mesh.

Therefore nonlinear solver policy should not simply say:

    current q_MR < threshold => physics invalid.

Hard physics/topology checks:
- J_F <= 0 -> invalid/inverted,
- non-finite stress/tangent -> invalid,
- failed constitutive domain -> invalid.

Distortion metrics:
- warn,
- correlate with Newton behavior/error,
- later drive adaptivity/remeshing,
- do not silently alter the mesh during a solve.

## 7. Useful runtime distortion observables

Research candidates:

    J_F = det(F)
    kappa(F)
    q_MR(Tt)
    q_MR(Tt) / q_MR(T0)
    kappa(Tt) / kappa(T0)
    min current dihedral angle
    max current dihedral angle.

No final runtime thresholds are frozen.

A dimensionless relative metric can help separate initial bad shape from deformation-induced
distortion, but it still requires solver correlation.

## 8. Nearly incompressible rubber and volumetric locking

Rubber-like solids have bulk stiffness much larger than shear stiffness.

As incompressibility is approached, a low-order pure-displacement finite element can become
artificially stiff because its discrete displacement space cannot represent the incompressibility
constraint adequately.

This is volumetric locking.

Key consequence:

    good tetra geometry
    != locking-free tetra formulation.

Mesh quality is necessary for reliable numerics but cannot replace:
- mixed displacement-pressure formulation,
- hybrid formulation,
- appropriate stabilization,
- a demonstrated inf-sup/stability strategy,
- formulation-specific convergence verification.

Published nearly-incompressible elasticity studies and rubber-element research confirm mixed/
stabilized formulations are used to address this problem.

## 9. Dynamics26 internal solver boundary

Current repository theory already documents mixed u-p nearly-incompressible hyperelasticity for the
existing HEX8-oriented baseline in docs/theory/V0.10_MIXED_UP_INCOMPRESSIBILITY.md.

That document explicitly does not establish arbitrary-mesh inf-sup stability.

Therefore future TET4 product qualification must not inherit "rubber ready" merely because:
- M6 mesh quality is good,
- M2 Delaunay is correct,
- HEX8 mixed tests pass.

A TET4 nearly-incompressible formulation needs its own:
- field/interpolation choice,
- stability reasoning,
- element patch tests,
- locking benchmarks,
- large-strain tangent checks,
- mesh convergence campaign.

## 10. Important P1/P0 warning

Modern stabilized nearly-incompressible literature notes that simple unstable approximation pairs,
including common low-order displacement/pressure combinations, can still lock or produce inaccurate
stress unless their stability is addressed.

Therefore a future tetra pressure field cannot be chosen only because the DOF count is convenient.

M6 records geometry quality; formulation stability belongs to solver/FEM qualification.

## 11. Large deformation benchmark implication

M7 TET4 product qualification should eventually include a crossed matrix:

### Geometry families
- regular tetra meshes,
- controlled mean-ratio degradation,
- controlled sliver families,
- anisotropic-but-valid meshes,
- graded meshes.

### Material/formulation families
- compressible linear elastic,
- compressible hyperelastic,
- nearly incompressible hyperelastic,
- future mixed/stabilized TET formulation.

### Loads
- tension,
- simple shear,
- bending,
- severe compression,
- torsion,
- combined distortion.

Measure:
- displacement error,
- stress error,
- reaction error,
- Newton iterations/cutbacks,
- tangent conditioning estimates,
- inversion count,
- runtime quality evolution.

Only such correlation can justify production quality thresholds.

## 12. Product language rule

Do not expose a generic green "Mesh Quality: Good" and infer analysis suitability.

Future reporting should distinguish:

    Topology Valid
    Geometric Shape Quality
    Sizing/Geometry Fidelity
    Solver/Formulation Suitability
    Current Nonlinear Distortion

A mesh can pass one category and fail another.
