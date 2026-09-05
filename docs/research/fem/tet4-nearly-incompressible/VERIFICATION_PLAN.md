# TET4 Nearly-Incompressible Verification Program

Status: PROPOSED RESEARCH PLAN
Date: 2026-09-05

## 1. Principle

A TET formulation becomes a Dynamics26 product capability only after:

    field theory
    -> element residual/tangent
    -> stability evidence
    -> mesh-family convergence
    -> nonlinear material tests
    -> distortion tests
    -> solver integration
    -> cross-formulation/cross-code sanity
    -> product workflow verification.

Mesh generation success is not element qualification.

## 2. T4-R01 — compressible P1 TET4 baseline

Use moderate compressibility.

Verify:
- partition of unity,
- rigid body zero energy,
- constant-strain patch,
- homogeneous finite deformation,
- element residual,
- tangent symmetry where expected,
- finite-difference tangent.

Purpose:
establish the geometry/constitutive baseline before incompressibility complications.

## 3. T4-R02 — locking negative-control sweep

Use identical high-quality tetra meshes.

Sweep:

    K/G = 10, 10^2, 10^3, 10^4

or equivalent Poisson ratios approaching 0.5.

For linear isotropic material:

    K/G = 2(1+nu) / [3(1-2nu)].

Measure:
- displacement/reaction error,
- apparent stiffness,
- stress,
- mesh convergence.

Pure displacement P1 and simple P1/P0 are negative controls, not target formulations.

## 4. T4-R03 — numerical inf-sup mesh sequence

Build deterministic unit-cube tetra refinement levels.

For each true mixed pair:
- assemble B, displacement norm S and pressure norm M_p,
- remove physical rigid/gauge modes,
- solve the generalized eigenproblem,
- track beta_h = sqrt(lambda_min^+),
- count unexpected zero/spurious pressure modes.

Primary positive candidate:
- MINI P1+bubble/P1.

Negative controls:
- unstable low-order pairs.

Do not claim stabilized P1/P1 passes by applying the raw pair test alone; its stabilization is part of
the discrete operator.

## 5. T4-R04 — MINI static-condensation equivalence

For a small mesh compare:

### Full system
Keep bubble displacement unknowns globally for test only.

### Condensed system
Condense bubble increments elementwise.

Require:
- same Newton increment for nodal u/p to tolerance,
- recovered bubble increment matches,
- same residual after update,
- same converged state,
- same reaction/stress.

Then exercise:
- load cutback,
- line search,
- failed step revert,
- repeated solve.

This validates local nonlinear bubble lifecycle.

## 6. T4-R05 — mixed tangent verification

For MINI and stabilized P1/P1:
- element block tangent versus finite difference,
- assembled global residual Jacobian versus finite difference,
- K_up/K_pu consistency under conservative load/material conditions,
- stabilization block signs/scaling,
- exact-incompressible and nearly-incompressible branches.

Follower/contact loads are separate later gates.

## 7. T4-R06 — pressure mode / stress quality

Benchmarks must inspect fields, not only displacement.

Measure:
- pressure null/spurious modes,
- checkerboard patterns,
- pressure L2 error where reference exists,
- stress convergence,
- nodal/element pressure location metadata.

Karabelas et al. report cases where simple P1/P0 outputs can look acceptable in global response while
stress fields remain inaccurate/non-smooth. Dynamics26 must therefore include stress/pressure in
qualification.

## 8. T4-R07 — stabilized P1/P1 consistency

Verify:
- stabilization vanishes for pressure fields in the projection space as designed,
- constant-pressure consistency,
- rigid/homogeneous state consistency,
- mesh-scale behavior,
- no user-tuned magic coefficient hidden in code,
- locking/stress convergence.

If a stabilization parameter remains, expose it as a versioned formulation constant with derivation,
not a generic user quality knob.

## 9. T4-R08 — F-bar patch qualification

Verify all items in F_BAR_PATCH_RESEARCH.md.

Especially:
- homogeneous Fbar=F,
- exact patch volume relation,
- global residual/tangent finite-difference agreement including cross-element derivatives,
- deterministic patch construction,
- K/G locking sweep,
- comparison with MINI/stabilized mixed references.

## 10. T4-R09 — mesh-shape cross test

Cross formulation with M6 analytic geometry families:
- regular tetra,
- controlled q_MR degradation,
- sliver family,
- needle/wedge,
- graded shape-regular meshes.

Measure:
- solver error,
- pressure/stress,
- Newton behavior,
- matrix conditioning proxies.

Goal:
separate formulation instability from bad geometry.

## 11. T4-R10 — finite-strain hyperelastic campaign

At minimum:
- Neo-Hookean,
- Mooney-Rivlin,
- Yeoh,
- Ogden after spectral tangent path is available.

Deformations:
- uniaxial tension/compression,
- simple shear,
- biaxial/planar mode where reference is available,
- severe compression,
- torsion/block distortion.

For each:
- W/stress/tangent consistency,
- J behavior,
- pressure,
- reaction,
- Newton iterations/cutbacks,
- mesh convergence.

## 12. T4-R11 — exact incompressibility branch

For formulations claiming full incompressibility:
- K_pp/compressibility term removed consistently,
- pressure gauge/null space treated,
- block linear backend supports indefinite system,
- volume constraint converges,
- inf-sup/stabilized stability evidence remains valid,
- no artificial pressure regularization silently remains.

## 13. T4-R12 — TET4 versus TET10 accuracy

Once M8 supports high-order tetra geometry:
compare TET4 candidates to a verified quadratic hybrid/mixed tetra path.

Include:
- bending,
- stress gradients,
- severe compression,
- contact later.

This prevents low-order tetra convenience from becoming an unexamined product ceiling.

## 14. Qualification matrix

| Gate | Requirement |
|---|---|
| T4-G01 | compressible P1 TET4 patch/tangent baseline green |
| T4-G02 | locking negative controls demonstrate expected degradation |
| T4-G03 | MINI refinement inf-sup sequence bounded and spurious modes understood |
| T4-G04 | MINI full vs static-condensed Newton equivalence |
| T4-G05 | mixed element/global tangent finite-difference verification |
| T4-G06 | stabilized P1/P1 consistency + pressure/stress convergence |
| T4-G07 | F-bar homogeneous/patch/tangent verification |
| T4-G08 | cross-formulation K/G sweep |
| T4-G09 | M6 quality x formulation distortion matrix |
| T4-G10 | hyperelastic finite-strain benchmark campaign |
| T4-G11 | exact incompressible pressure-null/backend semantics verified where claimed |
| T4-G12 | TET4 vs higher-order reference comparison before broad rubber product claim |

## 15. Current research recommendation

Do **not** pick one formulation yet.

Recommended evidence order:

    pure P1 / P1-P0 negative controls
        ->
    MINI as stable mixed reference
        ->
    stabilized P1-P1 as practical mixed candidate
        ->
    patch F-bar as displacement-only alternative
        ->
    later hybrid/mixed TET10 accuracy reference.

This order gives Dynamics26 an independent mathematical reference before performance-driven choices.
