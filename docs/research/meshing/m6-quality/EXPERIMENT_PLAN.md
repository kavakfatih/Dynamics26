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
| M6-R13 | 2->3 / 3->2 generic cavity validator rejects non-conformal/non-positive candidates |
| M6-R14 | N=3..7 edge-removal DP matches exhaustive polygon triangulation oracle |
| M6-R15 | edge-removal candidate preserves exact cavity boundary and volume |
| M6-R16 | smart Laplacian never commits a lower-quality or inverted local star |
| M6-R17 | max-min signed-volume untangling fixture is separate from shape-improvement acceptance |
| M6-R18 | q_MR optimization fallback improves controlled low-tail stars without inversion |
| M6-R19 | combined reconnection+smoothing compared against either mechanism alone |
| M6-R20 | boundary mobility classes preserve CAD/provenance/feature constraints in future surface tests |
| M6-R21 | deterministic operation ordering reproduces identical optimized topology/coordinates for fixed policy |

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


## Tier Q2-A — exact local reconnection fixtures

### 2->3
Construct:
- legal convex five-site cavity,
- complementary-edge-already-present rejection,
- coplanar/zero candidate rejection,
- protected-face rejection.

Require exact boundary/volume preservation.

### 3->2
Construct valence-3 edge star and invalid variants.

Require exact inverse relationship with 2->3 where the same five-site cavity is used.

### General edge removal
For N=3..7:
- enumerate all Catalan polygon triangulations independently,
- compare exhaustive best max-min q_MR with dynamic-programming result,
- verify canonical tie choice,
- inject invalid triangle contributions and confirm they are excluded.

### 4->4
Use N=4 edge star and verify the two link-polygon diagonal alternatives.

No implementation table is source authority.


## Tier Q2-B — smoothing and untangling fixtures

For a free interior vertex star:
- ordinary centroid proposal that would invert -> smart smoother must reject,
- centroid proposal that improves worst q_MR -> accept,
- case where centroid stalls but optimization can improve,
- exact feasible-region half-space checks,
- deterministic vertex-order permutations.

Untangling fixtures:
- one local inverted star with a feasible positive solution,
- one impossible/constraint-blocked star.

Untangling success must not be interpreted as permission to hide an invalid M2/M4 generated mesh.


## Tier Q2-C — CAD mobility research fixtures

When M3/M4 surface contracts exist:
- interior 3-DOF vertex,
- Face 2-DOF vertex,
- Edge 1-DOF vertex,
- CAD Vertex fixed,
- protected shared/bonded interface.

Verify:
- no off-manifold motion,
- Edge node ordering,
- persistent GeometryEntityId provenance,
- size-field constraints,
- deterministic result.

Until these pass, the M6 reference optimizer keeps boundary vertices fixed.


## Tier Q1-B — metric-dependency and blind-spot fixtures

### Singular-spectrum oracle

Generate positive singular triples over many decades.

Require:

    q_kappa(T)^2
      = q_MR(T) q_MR(T^-1)

and:

    q_MR^(3/2)
      <= q_kappa
      <= q_MR^(1/2).

Also cross-check q_kappa against kappa_2 bounds.

Purpose:
- catch formula/normalization mistakes,
- prove q_MR/q_kappa are related diagnostics rather than two independent votes.

### Needle family N(epsilon)

    A=(0,0,0)
    B=(1,0,0)
    C=(0,epsilon,0)
    D=(0,0,epsilon).

Expected:

    q_MR
      = 2^(4/3) epsilon^(4/3)
        /(1+2 epsilon^2)
      -> 0

    q_kappa -> 0
    q_RR -> 0
    rho_RE -> infinity

while:

    theta_min -> 45 degrees
    theta_max -> 90 degrees.

Purpose:
prove dihedral extrema alone are incomplete.

### Wedge family W(epsilon)

    A=(0,0,0)
    B=(1,0,0)
    C=(0,1,0)
    D=(1/2,0,epsilon).

Expected:

    q_MR -> 0
    q_kappa -> 0
    q_RR -> 0
    rho_RE -> infinity
    theta_min -> 0
    theta_max -> pi.

Purpose:
cross-check a collapse detected by both angle and radius-edge diagnostics.

## Tier Q4-B — stiffness positive-spectrum correlation

For scalar P1 and later elastic TET4:

1. explicitly remove physical element null modes,
2. compute positive-spectrum local stiffness condition/proxy,
3. correlate with kappa_2(T), q_MR and q_kappa,
4. sweep material condition separately,
5. assemble controlled global meshes and record raw/scaled/preconditioned condition/iterations.

Do not use the ordinary full element matrix condition number because it contains physical zero modes.

## Tier Q4-C — interpolation versus conditioning separation

Use scaled versions of:
- regular tetra,
- needle family,
- controlled anisotropic families.

Measure:
- linear interpolation error for analytic fields,
- local stiffness positive-spectrum condition,
- q_MR/q_kappa,
- face/dihedral maximum-angle conditions.

Purpose:
demonstrate experimentally that approximation fitness and stiffness conditioning can rank the same
tetrahedron differently.

## Tier Q4-D — graded shape-regular meshes

Build meshes whose tetrahedra remain shape-regular while the size range increases.

Record:
- q_MR/q_kappa/angles/q_RR,
- adjacent size/gradation distribution,
- h_max/h_min,
- raw condition estimate,
- naturally scaled condition estimate,
- preconditioned iterative count.

Purpose:
avoid attributing all global linear-system behavior to element shape.

## Additional M6 research gates

| Gate | Requirement |
|---|---|
| M6-R22 | exact q_MR/q_kappa identity and bounds pass analytic/random singular-spectrum oracle |
| M6-R23 | needle family proves dihedral-extrema blind spot with committed analytic golden values |
| M6-R24 | wedge family proves complementary angle/radius-edge response with committed analytic goldens |
| M6-R25 | sliver/needle/wedge/graded/anisotropic pathology matrix is represented by independent fixtures |
| M6-R26 | local stiffness condition diagnostics remove physical null modes and correlate with map condition |
| M6-R27 | interpolation experiment demonstrates that maximum-angle/approximation and conditioning objectives differ |
| M6-R28 | graded shape-regular meshes record raw/scaled/preconditioned solver behavior separately from shape |
| M6-R29 | M6 geometry families are crossed with TET4 formulation/compressibility evidence in T4-R09/T4-G09 |

These remain research/verification-design gates, not implementation authorization.


## Tier Q1-C — classical pathology taxonomy and angle conditions

Build deterministic coordinate fixtures for:

    spire/needle
    splinter
    spindle
    spear
    spike
    wedge
    spade
    cap
    sliver.

For each fixture record:

    q_MR
    q_kappa
    q_RR
    rho_RE
    theta_min/max
    face_angle_min/max
    solid_angle_min/max.

No pathology name is inferred from one threshold.

### Maximum-angle split

Verify separately:
- all triangular face angles bounded away from pi,
- all dihedral angles bounded away from pi.

Include families demonstrating:
- needle/splinter/wedge can satisfy both,
- spike can violate face-angle while retaining a dihedral upper bound,
- cap/sliver can retain acceptable face maxima while dihedral angles approach pi,
- spindle/spear/spade can violate both.

Purpose:
reproduce the interpolation-theory distinction without turning it into a mesher acceptance threshold.

## Tier Q1-D — spectral collapse classification

### Flat spectrum

    Sigma_F = diag(1,1,epsilon)

Expected:

    q_MR
      = 3 epsilon^(2/3)/(2+epsilon^2).

### Needle spectrum

    Sigma_N = diag(1,epsilon,epsilon)

Expected:

    q_MR
      = 3 epsilon^(4/3)/(1+2 epsilon^2).

Require both to produce the identical:

    q_kappa
      = 3 epsilon
        /sqrt((2+epsilon^2)(1+2epsilon^2)).

Also require:

    q_MR_inv(F) = q_MR(N)
    q_MR_inv(N) = q_MR(F).

Purpose:
prove that condition score alone cannot identify the collapse dimension.

## Tier Q1-E — fixed-spectrum orientation sweep

Use:

    Sigma = diag(3.0,1.2,0.4)
    T(phi) = Sigma R_z(phi).

Sweep phi deterministically.

Require:
- q_MR invariant,
- q_kappa invariant,
- at least one of q_RR / dihedral / face-angle / solid-angle observations changes away from tetrahedral
  symmetry rotations.

Committed research goldens include phi=0 and phi=30 degrees from
TETRA_PATHOLOGY_AND_ANGLE_CONDITIONS.md.

Purpose:
prove singular-value metrics do not encode all five tetra shape DOFs.

## Additional M6 pathology research gates

| Gate | Requirement |
|---|---|
| M6-R30 | classical skinny/flat pathology fixture library exists with quantitative metrics |
| M6-R31 | face-angle and dihedral maximum-angle conditions are verified as separate observables |
| M6-R32 | flat/needle spectral families have identical q_kappa and analytically distinct q_MR asymptotics |
| M6-R33 | q_MR_inv duality oracle swaps flat/needle canonical spectra |
| M6-R34 | fixed-spectrum right-rotation sweep preserves q_MR/q_kappa but changes non-spectral geometry diagnostics |
| M6-R35 | face/dihedral atan2 and solid-angle formulas pass regular/pathology golden checks |
| M6-R36 | named pathology labels remain explanatory and never bypass quantitative/solver acceptance evidence |

These gates extend M6-R22..R29 and remain research-only.
