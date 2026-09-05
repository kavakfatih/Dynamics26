# M6 Early Research — Quality / Solver Correlation Experiment Plan

Status: PROPOSED
Date: 2026-09-05

## 1. Goal

Determine which geometric tetra metrics actually predict failure, error and conditioning in
Dynamics26 FEM workflows.

The experiment sequence must separate:
- analytic metric correctness,
- geometric optimization behavior,
- linear-FEM conditioning,
- nonlinear distortion,
- nearly-incompressible formulation effects.

## 2. Tier Q0 — formula/golden verification

### Regular tetra

Expected:
- q_MR = 1,
- q_kappa = 1,
- q_RR = 1,
- rho_RE = sqrt(6)/4,
- all interior dihedral angles = arccos(1/3),
- positive volume.

### Uniform scale

Scale regular tetra by:
- 2^-20,
- 2^-5,
- 1,
- 2^5,
- 2^20.

Expected:
- normalized shape metrics invariant,
- volume scales cubically,
- topology/orientation unchanged.

### Rigid transformations

Rotations/translations/reflections with reorientation as needed.

Expected:
- normalized shape metrics invariant,
- signed orientation handled according to vertex ordering.

## 3. Tier Q1 — analytic pathology families

### Sliver family S(epsilon)

    A=(1,0,0)
    B=(-1,0,0)
    C=(0,1,epsilon)
    D=(0,-1,epsilon)

Sweep epsilon by powers of two toward zero.

Expected:
- V -> 0,
- q_MR -> 0,
- q_RR -> 0,
- condition metric -> 0,
- rho_RE -> 1/sqrt(2), demonstrating radius-edge blind spot.

### Needle family

Construct one vertex approaching another/local feature while preserving a nontrivial dihedral profile.

Purpose:
- demonstrate minimum dihedral alone is not complete,
- compare q_MR/q_kappa/q_RR.

### Flat-face/wedge family

Drive one altitude/face area toward zero.

Purpose:
- cross-check metric equivalence/asymptotics.

### Graded but shape-regular family

Scale neighboring regular/near-regular tetrahedra over large size ratios.

Purpose:
- separate shape quality from mesh-size/global conditioning.

## 4. Tier Q2 — local optimization experiments

For each local star/cavity:
- Laplacian move,
- smart Laplacian,
- q_MR optimization move,
- 2<->3 / 3<->2 / 4<->4 candidate reconnections where valid.

Measure:
- worst q_MR before/after,
- harmonic/percentile quality,
- min/max dihedral,
- radius-edge,
- boundary/sizing violations,
- deterministic operation result.

No production optimizer is selected yet.

## 5. Tier Q3 — Delaunay versus quality

Generate identical point sets and compare:
- ordinary Delaunay,
- legal local quality swaps,
- smoothed mesh,
- later weighted/regular-Delaunay prototype.

Questions:
- how many slivers survive ordinary Delaunay?
- which operations change Delaunay legality?
- which metrics improve?
- does solver behavior improve?

This test makes explicit that solver quality can justify leaving ordinary Delaunay after M2/M4.

## 6. Tier Q4 — linear FEM correlation

Use controlled tetra meshes for a simple isotropic linear-elastic or scalar elliptic problem.

Vary one pathology at a time.

Record:
- element mapping kappa,
- local stiffness spectral condition proxy,
- global matrix condition estimate where practical,
- iterative solver iterations,
- displacement/energy/stress error against reference,
- q_MR/q_kappa/angle/radius metrics.

Goal:
derive correlation, not universal threshold.

## 7. Tier Q5 — nonlinear geometry correlation

For TET4-capable future solver path, use:
- simple shear,
- uniaxial tension/compression,
- bending,
- torsion.

Track per increment:

    J_F
    q_MR(reference)
    q_MR(current)
    kappa(F)
    kappa(T_current)/kappa(T_reference)
    theta_min/current
    theta_max/current
    Newton iterations
    cutbacks
    inversion events.

Goal:
identify which current-configuration metrics predict solver difficulty without confusing legitimate
physical deformation with bad initial mesh.

## 8. Tier Q6 — nearly incompressible rubber separation

Use at least two formulations once available:
- a known locking-prone displacement baseline,
- a verified mixed/stabilized tetra formulation.

Run the same high-quality geometric mesh across increasing K/G or nu -> 0.5.

Expected research finding:
- geometric quality held fixed,
- locking behavior changes primarily with formulation/compressibility.

Then hold formulation fixed and degrade mesh quality.

This two-axis experiment separates:

    geometry error
    from
    incompressibility/formulation error.

## 9. Tier Q7 — automotive/rubber geometries

After simple benchmarks:
- rubber annulus,
- simplified crank-pulley rubber volume,
- engine-mount rubber block/void geometry,
- bonded rubber-metal interface.

Measure:
- quality distributions,
- solver convergence,
- reaction/stiffness convergence,
- local stress sensitivity,
- deformation distortion.

Customer/proprietary geometry is not committed without permission.

## 10. Candidate M6 research gates

| Gate | Requirement |
|---|---|
| M6-R01 | metric definitions/source registry committed |
| M6-R02 | regular-tetra exact/near-exact golden values |
| M6-R03 | scale/rotation invariance tests |
| M6-R04 | analytic sliver family proves radius-edge blind spot |
| M6-R05 | q_MR and q_kappa equivalence/correlation numerically cross-checked |
| M6-R06 | angle/edge-ratio blind spots represented by fixtures |
| M6-R07 | quality-report distribution semantics fixed |
| M6-R08 | no product thresholds accepted without solver-correlation evidence |
| M6-R09 | linear FEM conditioning correlation campaign passes/reports |
| M6-R10 | nonlinear reference/current quality separation benchmarked |
| M6-R11 | incompressible locking experiment separates geometry from formulation |
| M6-R12 | D26LIFT1 and future finite regular-Delaunay weights remain separate policies |

These are early research gates, not M6 implementation qualification.

## 11. Decision outputs expected later

Research should eventually answer:
1. primary TET4 optimization metric,
2. supporting diagnostics,
3. warning thresholds,
4. hard product thresholds by solver capability,
5. smoothing versus flip schedule,
6. whether/where weighted sliver treatment is justified,
7. runtime nonlinear distortion monitoring policy,
8. how quality results appear in MeshGenerationReport/solver preflight.
