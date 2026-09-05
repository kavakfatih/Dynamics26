# M6 — Tetrahedral Quality & FEM Suitability Research

Program: Dynamics26 Original Meshing Engine
Work package: M6 early research — quality mathematics before optimization implementation
Status: RESEARCHING / IMPLEMENTATION NOT STARTED
Date: 2026-09-05

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
- SLIVER_AND_DELAUNAY_LIMITS.md — why valid Delaunay meshes still contain slivers and why radius-edge alone is insufficient
- NONLINEAR_RUBBER_MESH_QUALITY.md — reference/current configuration quality, deformation gradient and incompressibility separation
- QUALITY_METRIC_POLICY.md — proposed Dynamics26 metric roles, aggregation and release-policy boundaries
- LOCAL_TOPOLOGY_IMPROVEMENT.md — 2<->3, 3<->2, 4<->4, protected topology and deterministic quality reconnection
- EDGE_REMOVAL_DYNAMIC_PROGRAMMING.md — general edge-star removal and original max-min link-polygon DP formulation
- SMOOTHING_UNTANGLING_AND_CAD_CONSTRAINTS.md — smart smoothing, feasible region, untangling separation and CAD mobility
- EXPERIMENT_PLAN.md — analytic shape families and solver-correlation campaign

## Leading research conclusions

- positive tetra volume is a validity condition, not a scale-independent quality score,
- Delaunay legality is not FEM-quality certification,
- radius-edge ratio is useful for Delaunay refinement but does not reliably detect slivers,
- mean ratio / weighted-Jacobian condition metrics are strong isotropic TET4 shape candidates,
- angle diagnostics remain valuable because one scalar metric can hide specific pathologies,
- one bad element can matter; report worst/low-percentile distributions rather than average alone,
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
- stronger edge/cavity reconnection can escape elementary-flip local maxima,
- sliver-specific finite-weight methods remain a separate later policy.

The first implementation candidate remains interior-only until CAD boundary/provenance motion is
qualified.
