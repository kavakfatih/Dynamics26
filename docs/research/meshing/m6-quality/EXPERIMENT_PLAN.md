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


## Tier Q2-D — local-optimum and valley-crossing fixtures

### 4->4 valley fixture

Construct a four-tetra interior edge cavity where:
- the alternate 4->4 connectivity improves the primary quality objective,
- every elementary sequence realizing that 4->4 requires at least one first step that is not accepted
  by strict hill climbing.

Require:
- elementary strict hill climber stalls,
- direct edge-removal cavity search finds the improved final state,
- no non-improving intermediate state is committed.

### Operation-neighborhood nesting

For deterministic small point sets enumerate:
- 2<->3 candidates,
- edge-removal candidates,
- multi-face candidates,
- all legal fixed-cavity triangulations where tractable.

Record which operation family first reaches an improved state.

Purpose:
make "local optimum" relative to an explicit operation neighborhood.

## Tier Q2-E — fixed-cavity SPR oracle

For small cavities with independent exhaustive enumeration:

    q*_C
      =
    max_T min_t q_MR(t).

Verify:
- branch-and-bound result equals exhaustive oracle,
- positive orientation/intersection/protected constraints agree,
- pruning with the partial minimum never removes the true optimum,
- canonical tie result is deterministic.

Do not use copied external cavity lookup tables.

## Tier Q2-F — cavity-selection versus retriangulation optimality

Create nested cavities:

    C4 subset C5 subset ... subset Ck

around one poor tetrahedron.

Require cases where:
- the smallest cavity has exhaustive no-improvement,
- a larger cavity admits strict improvement.

Record separately:

    cavity_selection_result
    fixed_cavity_search_result.

Purpose:
prove that optimal retriangulation of one cavity is not global-mesh optimality.

## Tier Q2-G — strong-search effort semantics

Run bounded fixed-cavity search with deterministic count budgets:
- candidate tetra evaluations,
- branch nodes,
- cavity vertex count.

Require three distinct outcomes:

    Improved
    ExhaustiveNoImprovement
    BudgetExhausted.

BudgetExhausted must never be reported as proof of local optimality.

Wall-clock limits are excluded from deterministic reference qualification.

## Tier Q2-H — topology/smoothing alternation

Construct stars where:
1. edge removal fails before smoothing but succeeds after accepted smoothing,
2. smoothing stalls before reconnection but succeeds after connectivity change.

Compare:
- topology-only,
- smoothing-only,
- one-shot topology then smoothing,
- alternating topology/smoothing.

Purpose:
verify that the two search variables unlock one another.

## Additional M6 strong-reconnection research gates

| Gate | Requirement |
|---|---|
| M6-R37 | O-local optimum definition represented in deterministic reachability fixtures |
| M6-R38 | 4->4 valley fixture proves direct cavity operation can cross an elementary hill-climbing barrier transactionally |
| M6-R39 | edge-removal DP optimum is cross-checked against exhaustive link-polygon enumeration beyond N=3..7 spot cases |
| M6-R40 | multi-face removal small-cavity oracle demonstrates connectivity states unavailable to edge removal alone |
| M6-R41 | fixed-cavity SPR branch-and-bound matches exhaustive tetrahedralization oracle for tractable cavities |
| M6-R42 | branch-and-bound partial-minimum pruning never removes the true max-min optimum |
| M6-R43 | nested-cavity fixture separates ExhaustiveNoImprovement in C from improvement in a larger C' |
| M6-R44 | strong-search BudgetExhausted is distinct from ExhaustiveNoImprovement |
| M6-R45 | fixed-point connectivity-only accepted mutation sequence is cycle-free and terminates |
| M6-R46 | smoothing convergence policy closes continuous-state stalls independently of topology termination |
| M6-R47 | alternating smoothing/reconnection outperforms or equals one-shot schedules on unlock fixtures |
| M6-R48 | successful strong reconnection restarts cheap local passes and preserves exact boundary/provenance/size acceptance |

These are research/verification-design gates only.


## Tier Q2-I — QualityOrder / acceptance objective fixtures

### Pure max-min versus sorted quality vector

Use local vectors:

    old = [0.20,0.30,0.80]
    new = [0.20,0.31,0.32].

Require:
- max-min reports a first-component tie,
- QMRVector reports strict improvement.

### Aggregate worst-regression negative control

Use:

    old = [0.20,0.20]
    new = [0.19,0.50].

Require:
- arithmetic mean improves,
- harmonic mean / inverse-mean aggregate improves,
- QMRVector rejects because the worst tetrahedron degrades.

### Aggregate low-tail rejection negative control

Use:

    old = [0.20,0.30,0.80]
    new = [0.20,0.31,0.32].

Require:
- QMRVector accepts,
- arithmetic and harmonic means decrease.

Purpose:
prove aggregate and low-tail objectives answer different questions.

## Tier Q2-J — exact q_MR ordering oracle

For each positive tetra compute independently:

    q_MR
      =
    12(3V)^(2/3)/S

and exact-order ratio:

    R_MR
      =
    D^2/S^3.

Require:

    q_MR^3
      =
    432 R_MR

against high-precision reference arithmetic.

Pairwise exact ordering must use:

    sign(
      D_A^2 S_B^3
      -
      D_B^2 S_A^3
    ).

Test:
- regular tetra,
- sliver/needle/wedge families,
- dyadic scales,
- nearly equal qualities,
- exact quality ties,
- coordinate permutations preserving the same positive tet.

## Tier Q2-K — epsilon-comparator negative control

Choose:

    a = 0
    b = 0.75 delta
    c = 1.50 delta.

Demonstrate:

    a ~= b
    b ~= c
    a !~= c

under pairwise absolute-epsilon equality.

Require:
- no QualityVector implementation uses this relation as its sort equality.

## Tier Q2-L — local/global multiset order oracle

Enumerate finite quality multisets over a small exact rational alphabet.

For every:
- unchanged multiset U,
- old local A,
- new local B,

verify:

    B >_lex A

implies:

    sort(U union B)
      >_lex
    sort(U union A).

Include different old/new cell counts and exact-prefix cases.

## Tier Q2-M — lexicographic SPR branch bound

For tractable fixed cavities:
- enumerate all legal complete tetrahedralizations independently,
- compute the exact best QMRVector,
- run branch-and-bound using partial QMRVector padded with +infinity as upper bound.

Require exact agreement.

Also run the max-min oracle.

Require:
- both agree on best worst quality,
- lex search may select a better second/third-worst triangulation among equal-min candidates.

## Additional M6 quality-order research gates

| Gate | Requirement |
|---|---|
| M6-R49 | QMRVector strictly refines q_min on second/third-worst improvement fixtures |
| M6-R50 | variable-cell-count exact-prefix rule is deterministic and local/global composable |
| M6-R51 | q_MR^3 = 432 D^2/S^3 exact-order oracle matches independent high-precision evaluation |
| M6-R52 | adversarial near-equal q_MR pairs resolve through exact dyadic comparison, not pairwise epsilon |
| M6-R53 | epsilon-equality negative control demonstrates non-transitivity |
| M6-R54 | aggregate arithmetic/harmonic objectives demonstrate both worst-regression and low-tail-rejection counterexamples |
| M6-R55 | exhaustive finite-multiset oracle proves local QMRVector improvement implies global improvement |
| M6-R56 | connectivity-only replay with strict exact QMRVector improvement is cycle-free |
| M6-R57 | lexicographic SPR branch-and-bound matches exhaustive small-cavity QMRVector optimum |
| M6-R58 | max-min and lex SPR oracles agree on first component but expose equal-worst secondary differences |
| M6-R59 | smoothing proposal objectives commit only through exact validity + QMRVector acceptance |
| M6-R60 | Debug/Release repeated runs reproduce candidate selection and exact quality ties for fixed policy |

These remain research/verification-design gates.


## Tier Q2-N — D26QMR1 binary64 lattice oracle

### Bit-level decode corpus

Verify exact lattice integer reconstruction for:
- +0 / -0,
- minimum positive subnormal,
- maximum subnormal,
- minimum normal,
- adjacent normals,
- 1.0 and exact powers of two,
- maximum finite binary64,
- negative counterparts.

Reject:
- NaN,
- +infinity,
- -infinity.

Independent oracle must reconstruct:

    x = I 2^-1074.

### Primitive gcd normalization

For tetra lattice coordinates:
- compute all pairwise integer component differences,
- divide common positive gcd G,
- verify primitive gcd becomes one,
- repeat under every vertex permutation and multiple exact lattice translations.

Require identical:

    d^2/s^3.

### Exact mean-ratio order

For positive tetrahedra compare:

    sign(
      d_A^2 s_B^3
      -
      d_B^2 s_A^3
    ).

Cross-check against independent arbitrary-precision rational evaluation.

Include:
- regular/near-regular,
- sliver,
- needle,
- wedge,
- exact ties,
- adversarial near-ties.

## Tier Q2-O — exact-arithmetic width audit

For every reference corpus item record:

    primitive_component_bits
    determinant_bits
    edge_sum_bits
    comparison_product_bits.

Verify the derived conservative bounds:

    bitlen(s) <= 2B+5
    bitlen(|d|) <= 3B+3
    comparison magnitude < 12B+22 bits

using the experiment's exact definition of B.

Exercise wide exponent-span inputs approaching the binary64 theoretical limit.

## Tier Q2-P — D26QMR filtered-exact oracle

Reference exact-only comparator is authority.

Research a certified interval filter for:

    q3 = 432 D^2/S^3.

For each tetra:
- use independent exact power-of-two scaling to bound local floating dynamic range,
- evaluate outward intervals,
- certify only disjoint score intervals,
- otherwise fall back.

Require:
- no filter/exact disagreement,
- interval overlap never returns Equal,
- subnormal/underflow uncertainty goes to fallback,
- no overflow-driven decision,
- exact equality only from exact stage.

Record fallback rate by pathology and operation class.

## Tier Q2-Q — metamorphic comparator verification

Require D26QMR1 invariance under:
- all 24 tetra vertex permutations,
- exact lattice translations,
- exact power-of-two uniform scale,
- reflection of the geometric shape with orientation validity handled separately.

Cross-check:

    sign(integer determinant)
      ==
    M1 Orient3D exact sign

for identical ordered vertices.

## Additional D26QMR1 research gates

| Gate | Requirement |
|---|---|
| M6-R61 | binary64 bit patterns decode exactly to the common 2^-1074 integer lattice |
| M6-R62 | primitive gcd normalization is anchor- and permutation-invariant |
| M6-R63 | q_MR^3 = 432 d^2/s^3 passes an independent rational oracle |
| M6-R64 | derived bigint-width bounds are never exceeded by the adversarial corpus |
| M6-R65 | exact D26QMR1 comparator matches independent arbitrary-precision ordering |
| M6-R66 | all 24 vertex permutations preserve the exact quality key |
| M6-R67 | exact lattice translation and power-of-two scaling metamorphic tests pass |
| M6-R68 | inverted, degenerate and non-finite candidates cannot enter quality acceptance |
| M6-R69 | integer determinant sign agrees with M1 exact Orient3D sign |
| M6-R70 | certified interval filter never disagrees with exact D26QMR1 |
| M6-R71 | interval overlap/underflow is Uncertain and exact equality comes only from fallback |
| M6-R72 | wide exponent-span corpus exercises the bounded exact fallback path |
| M6-R73 | Debug/Release/replay produce the same final comparator result |
| M6-R74 | approximate displayed q_MR remains separate from authoritative exact ordering |

No M6 production comparator is authorized by these gates.


## Tier Q2-R — D26QMRF1 dynamic interval filter

Reference:
- exact D26QMR1 remains authority,
- every fast result is cross-checked during qualification.

### Cross-polynomial oracle

For positive tetrahedra A,B verify:

    F(A,B)
      =
    D_A^2 S_B^3
      -
    D_B^2 S_A^3

has the same sign as:
- exact D26QMR1 rational comparison,
- independent high-precision q_MR comparison.

Exercise:
- ordinary random tetrahedra,
- exact ties,
- near-ties,
- sliver/needle/wedge families.

### Independent scale metamorphism

Apply independent exact power-of-two scales:

    A -> 2^i A
    B -> 2^j B

over broad safe exponent ranges.

Require:

    sign(F)
      unchanged.

This directly verifies the bi-homogeneous factor:

    2^(6i+6j).

## Tier Q2-S — normalization and dynamic-range bounds

Using the certified interval upper bound M for one tetra:
- choose power-of-two normalization,
- verify normalized anchor-relative exact components remain within the documented bound,
- verify interval containment remains valid.

Independent exact reference must confirm:

    D^2 < 27
    S < 72
    |F| < 20,155,392

for successfully normalized mathematical inputs under the stated strict component bounds.

Boundary/equality cases around the normalization exponent must be included.

## Tier Q2-T — interval containment

For every interval primitive and full expression:
- reconstruct exact input values with arbitrary-precision rational arithmetic,
- verify exact subtraction result is enclosed,
- verify determinant D is enclosed,
- verify edge sum S is enclosed,
- verify comparison polynomial F is enclosed.

Include:
- cancellation-heavy coordinates,
- large translations,
- wide exponent spans,
- near-degenerate determinants,
- zero scalar edge components,
- subnormal boundary cases.

Any backend exception or unsupported range must return Uncertain.

## Tier Q2-U — fast-sign safety

For a large corpus:

    if lower(F_I) > 0
      fast result = Greater

    if upper(F_I) < 0
      fast result = Less

    otherwise
      fast result = Uncertain.

Require:

    every fast Less/Greater
      ==
    exact D26QMR1 result.

Forbidden:
- fast Equal,
- guessed sign after interval overlap.

Exact ties must reach fallback.

## Tier Q2-V — semi-static research bounds

For a frozen non-FMA expression tree and normal-range arithmetic, independently derive/check the
candidate bounds:

    determinant absolute error
      ~ gamma_10 * P_D

and:

    edge-sum relative error
      ~ gamma_20.

These constants are research targets, not accepted truth.

Qualification work must explicitly account for:
- coordinate subtraction,
- triple-product multiplication,
- signed six-term determinant summation,
- 18 squared scalar edge terms,
- 17 additions,
- underflow exclusions,
- exact compiler expression tree.

If the independent derivation gives a different safe bound, update the research document instead of
forcing gamma_10/gamma_20.

## Tier Q2-W — FMA/compiler contract matrix

Build/test separate configurations:

1. explicit non-contracted arithmetic,
2. explicit FMA candidate where mathematically derived,
3. unsafe-math negative control.

Require:
- exact fallback result identical in all safe configurations,
- certified filter only enabled for configurations covered by its proof,
- unsafe-math configuration cannot silently qualify.

Track:
- compiler version,
- FP contraction policy,
- rounding mode,
- subnormal handling.

## Tier Q2-X — fallback profile

Record D26QMRF1 outcomes by workload:

    CertifiedLess
    CertifiedGreater
    UncertainOverlap
    UncertainRange
    UncertainSubnormal
    ExactFallback
    ExactEqual.

Stratify by:
- 2->3,
- 3->2,
- edge removal,
- smoothing acceptance,
- SPR candidate search,
- pathology family.

Do not use one global fallback percentage as the only performance evidence.

## Additional D26QMRF1 research gates

| Gate | Requirement |
|---|---|
| M6-R75 | F sign agrees with exact D26QMR1 over analytic/random/adversarial fixtures |
| M6-R76 | independent per-tetra power-of-two scaling preserves F sign |
| M6-R77 | canonical filter vertex enumeration makes qualified telemetry replay-stable |
| M6-R78 | interval normalization preserves containment and the documented local range |
| M6-R79 | D, S and F intervals contain independent exact references |
| M6-R80 | normalized D^2/S/F mathematical bounds pass exact oracle checks |
| M6-R81 | every fast Less/Greater agrees with exact D26QMR1 |
| M6-R82 | interval overlap returns only Uncertain |
| M6-R83 | exact q_MR ties always reach exact fallback |
| M6-R84 | subnormal/overflow/environment uncertainty falls back without a guessed sign |
| M6-R85 | FMA and non-contracted paths are treated as distinct certified expression trees |
| M6-R86 | future CI detects unsafe floating-compiler policy drift |
| M6-R87 | candidate gamma_10/gamma_20 bounds are independently certified or corrected |
| M6-R88 | no semi-static filter constant is accepted without an expression-specific proof |
| M6-R89 | fallback telemetry is stratified by optimizer operation and pathology class |
| M6-R90 | Debug/Release/replay preserve the final exact result and qualified filter semantics |

These gates do not authorize production filter implementation.
