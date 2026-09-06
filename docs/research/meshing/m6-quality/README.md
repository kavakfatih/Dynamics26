# M6 — Tetrahedral Quality & FEM Suitability Research

Program: Dynamics26 Original Meshing Engine
Work package: M6 tetra quality / optimizer research closeout
Status: RESEARCH COMPLETE / DESIGN FROZEN / IMPLEMENTATION NOT QUALIFIED
Date: 2026-09-06

## Freeze authority

The authoritative M6 research closeout is:

    M6_DESIGN_FREEZE.md.

Qualification planning is summarized by:

    M6_QUALIFICATION_GATE_MATRIX.md.

If an older research-candidate paragraph conflicts with the freeze document, the freeze document and
accepted M6 ADRs take precedence.

Design freeze means:
- mathematics/architecture/verification contracts are frozen for first implementation,
- production M6 code is not yet implemented/qualified,
- M6-R01..M6-R170 are qualification contracts, not a claim of passed executable evidence.

## Purpose

This package builds the scientific bridge between:

    geometrically correct tetrahedralization
    -> mesh shape quality
    -> finite-element numerical conditioning
    -> nonlinear / rubber analysis suitability

M2 answers whether a tetrahedralization is topologically and Delaunay-correct.
M6 will later answer whether the tetrahedra are numerically useful and how to improve them without
breaking geometry/provenance.

These are not the same problem.

## Research rule

No single commercial "element quality" number becomes Dynamics26 mathematical truth.

For each metric, record:
- exact definition and normalization,
- invariance properties,
- degeneracies it detects,
- degeneracies it can miss,
- FEM/numerical meaning,
- optimization suitability,
- computational cost,
- eventual acceptance evidence.

Final release thresholds are not chosen from generic tables. They must be correlated against
Dynamics26 solver benchmarks in M7.

## Document map

- TETRA_QUALITY_AND_FEM_CONDITIONING.md — Jacobian/singular-value mechanics, mean ratio and stiffness-conditioning bridge
- MULTI_METRIC_SOLVER_AWARE_FRAMEWORK.md — exact metric relations, pathology matrix, grading/interpolation/conditioning separation and layered quality-vector contract
- TETRA_PATHOLOGY_AND_ANGLE_CONDITIONS.md — classical pathology taxonomy, spectral blind spots, face/dihedral maximum-angle conditions and solid-angle diagnostics
- SLIVER_AND_DELAUNAY_LIMITS.md — why valid Delaunay meshes still contain slivers and why radius-edge alone is insufficient
- NONLINEAR_RUBBER_MESH_QUALITY.md — reference/current configuration quality, deformation gradient and incompressibility separation
- QUALITY_METRIC_POLICY.md — proposed Dynamics26 metric roles, aggregation and release-policy boundaries
- QUALITY_ORDER_AND_ACCEPTANCE_POLICY.md — sorted mean-ratio acceptance order, local-to-global theorem and proposal/commit separation
- EXACT_QMR_COMPARATOR.md — binary64 lattice oracle, exact order identity and bounded integer comparison
- EXACT_QMR_BACKEND_SELECTION.md — M1 BigInt reuse, no-GCD exact construction, expansion/Boost alternatives and local exact-key caching policy
- QUALITY_KEY_LIFECYCLE_AND_DETERMINISTIC_SCHEDULING.md — exact-key validity domain, cache invalidation, proposal footprints, conflict-free rounds and deterministic parallel planning
- PARALLEL_ROUND_MONOTONICITY_AND_TERMINATION.md — batch-union quality theorem, binary64 finite-state termination and active-set reference semantics
- OPTIMIZER_OPERATION_SCHEDULE.md — frozen D26OPS1 first schedule: cheap smoothing/topology cycle, edge-removal DP, optimization smoothing and bounded SPR escalation
- M6_DESIGN_FREEZE.md — authoritative frozen architecture, supersession rules, deferred scope and implementation boundary
- M6_QUALIFICATION_GATE_MATRIX.md — complete M6-R01..M6-R170 qualification-contract grouping and pending-evidence status
- M6_IMPLEMENTATION_SPEC.md — repository-specific private module/API/CMake/state/operation contract for executable M6 reference implementation
- M6_IMPLEMENTATION_SEQUENCE.md — I0..I10 phased implementation and qualification order
- FILTERED_QMR_COMPARATOR.md — dynamic interval cross-polynomial filter, power-of-two normalization, semi-static bound research and compiler/FP contract
- INTERVAL_ARITHMETIC_BACKEND.md — C++20/macOS arm64 outward interval primitives, adjacent-representable widening, fenv/FMA/subnormal contract and backend ranking
- INTERVAL_DOWNSCALE_POLICY_EXPERIMENT.md — D26INT1 subnormal downscale policy: the frozen policy was unsound at the min-normal tie and cost D26QMRF1 all coverage on structured meshes; unconditional outward widening adopted
- LOCAL_TOPOLOGY_IMPROVEMENT.md — 2<->3, 3<->2, 4<->4, protected topology and deterministic quality reconnection
- LOCAL_OPTIMIZATION_TRAPS_AND_STRONG_RECONNECTION.md — reachability/local-optimum theory, multi-face/SPR escalation, transactional valley crossing and termination
- EDGE_REMOVAL_DYNAMIC_PROGRAMMING.md — general edge-star removal and original max-min link-polygon DP formulation
- SMOOTHING_UNTANGLING_AND_CAD_CONSTRAINTS.md — smart smoothing, feasible region, untangling separation and CAD mobility
- EXPERIMENT_PLAN.md — analytic shape families and solver-correlation campaign

## Leading research conclusions

- positive tetra volume is a validity condition, not a scale-independent quality score,
- Delaunay legality is not FEM-quality certification,
- radius-edge ratio is useful for Delaunay refinement but does not reliably detect slivers,
- mean ratio / weighted-Jacobian condition metrics are strong isotropic TET4 shape candidates, but their singular-value relation means they are not independent votes,
- angle diagnostics remain valuable because one scalar metric can hide specific pathologies; the analytic needle family shows dihedral extrema can remain benign while q_MR collapses,
- face-angle and dihedral maximum-angle conditions are independent FEM interpolation diagnostics; dihedral-only telemetry is insufficient,
- singular-value metrics encode only part of tetra shape: fixed q_MR/q_kappa can coexist with different dihedral, radius-ratio and solid-angle behavior,
- one bad element can matter; report worst/low-percentile distributions rather than average alone,
- optimizer commit acceptance now leads toward a sorted worst-to-best q_MR vector rather than pure max-min or an aggregate mean; smooth aggregate objectives remain proposal tools,
- D26QMR1 is an exact **ordering** contract, not an exact irrational q_MR-value evaluator; binary64 coordinates reduce to a bounded integer-rational comparison with no quality epsilon,
- the leading exact fallback backend is now the already-qualified M1 dyadic BigInt arithmetic extracted into a shared internal kernel; expansion arithmetic remains an experimental alternative rather than assumed M1 infrastructure,
- D26QMRF1 now leads with a certified dynamic interval filter over the homogeneous comparison polynomial; semi-static magic-constant filters are deferred until their expression tree and floating environment are formally/auditably certified,
- D26INT1 now leads with round-to-nearest primitive evaluation plus adjacent-representable outward widening; directed-rounding and EFT/FMA backends remain secondary research paths,
- exact quality caches are coordinate-state artifacts rather than TetHandle artifacts; first parallelism should evaluate immutable-snapshot proposals concurrently but select/commit a deterministic conflict-free round,
- a non-empty conflict-free round of strict local D26QV1 improvements is globally strict-monotone; with fixed finite PointIds and finite canonical binary64 coordinates this also gives finite accepted-mutation history,
- D26OPS1 now leads toward smart smoothing -> 2-to-3 -> general edge-removal DP -> optimization smoothing, repeating cheap cycles before bounded SPR escalation; numeric activation thresholds remain unfrozen,
- mesh grading, interpolation error, stiffness conditioning and nearly-incompressible formulation stability remain separate quality/evidence layers,
- M2 D26LIFT1 infinitesimal symbolic perturbation is **not** sliver exudation,
- large-deformation runtime distortion is separate from initial mesh quality,
- nearly incompressible rubber locking is an element-formulation problem that good geometry alone
  cannot cure,
- isotropic regular-tetra metrics must not later be misused to reject intentionally anisotropic
  elements generated in a metric field.

## Scope

This is early M6 research performed in parallel with M2 implementation planning.

It does not authorize:
- M6 smoothing/flips,
- weighted regular-triangulation production changes,
- sliver exudation implementation,
- arbitrary thresholds,
- TET4 rubber product claims,
- M9 anisotropic adaptation.


## Early optimizer research direction

Current evidence favors a **combined** quality strategy rather than a single operation:
- connectivity changes remove bad local topology,
- smoothing improves vertex positions,
- stronger edge/cavity reconnection can escape elementary-flip local maxima; fixed-cavity optimality must remain separate from cavity-selection and global-mesh optimality,
- sliver-specific finite-weight methods remain a separate later policy.

The first implementation candidate remains interior-only until CAD boundary/provenance motion is
qualified.
