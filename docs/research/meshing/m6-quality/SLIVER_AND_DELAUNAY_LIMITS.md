# Sliver Tetrahedra and the Limits of Ordinary Delaunay Quality

Status: RESEARCHING
Date: 2026-09-05

## 1. Delaunay correctness does not prevent bad tetrahedra

An ordinary 3D Delaunay tetrahedralization satisfies an empty-circumsphere condition.

That condition is topological/geometric and says nothing by itself about:
- volume relative to edge lengths,
- minimum/maximum dihedral angle,
- stiffness conditioning,
- nonlinear distortion resistance.

The classic 3D pathology is the sliver.

## 2. Sliver geometry

A sliver has four vertices close to one plane while its edge lengths can all remain moderate.

Qualitatively:
- no necessarily tiny edge,
- nearly zero volume,
- some very poor dihedral angles,
- bad Jacobian singular-value spread,
- possible deceptively acceptable radius-edge ratio.

The recent mesh-quality survey classifies slivers separately from needles and wedges because
tetrahedra have 3D solid/dihedral-angle pathologies not present in triangles.

## 3. Dynamics26 analytic sliver family

Use the exact one-parameter family, epsilon > 0:

    A = ( 1,  0, 0)
    B = (-1,  0, 0)
    C = ( 0,  1, epsilon)
    D = ( 0, -1, epsilon).

With the current M1 row-determinant orientation ordering:

    Orient3D(A,B,C,D) = 4 epsilon > 0.

Physical volume magnitude:

    V = 2 epsilon / 3.

Squared edge lengths:
- two opposite edges have length^2 = 4,
- the other four have length^2 = 2 + epsilon^2.

Hence:

    sum l_ij^2 = 16 + 4 epsilon^2.

Mean ratio:

    q_MR(epsilon)
      = 12 (3V)^(2/3) / sum l_ij^2
      = 3 (2 epsilon)^(2/3) / (4 + epsilon^2).

Therefore:

    epsilon -> 0+  => q_MR -> 0.

## 4. Why radius-edge can be fooled

For this family the circumsphere center is:

    (0,0,epsilon/2)

and:

    R = sqrt(1 + epsilon^2/4).

For small epsilon the shortest edge is:

    l_min = sqrt(2 + epsilon^2).

Thus:

    rho_RE = R/l_min
           -> 1/sqrt(2)
           ~= 0.707107

as epsilon -> 0.

The regular-tetra optimum is approximately 0.612372.

So an arbitrarily flat, vanishing-volume sliver can keep a radius-edge value not far from the ideal.

TetGen documentation makes the same practical point: most bad tetrahedra have large radius-edge ratio,
but slivers are the important exception.

### Dynamics26 conclusion

rho_RE is a refinement metric, not a standalone FEM-quality gate.

## 5. Radius ratio catches the collapse

All four triangular faces of the analytic family have area:

    sqrt(1 + epsilon^2).

Total surface area:

    S = 4 sqrt(1 + epsilon^2).

Inradius:

    r = 3V/S
      = epsilon / (2 sqrt(1 + epsilon^2)).

Normalized radius ratio:

    q_RR = 3r/R -> 0

as epsilon -> 0.

This is consistent with mean-ratio degeneration.

## 6. Delaunay refinement and slivers

A 3D Delaunay refinement process can eliminate tetrahedra with poor radius-edge ratio and still leave
slivers.

That is why the literature separates:
1. radius-edge / spacing refinement,
2. sliver elimination.

Sliver exudation is one important weighted-Delaunay method for the second step.

## 7. Sliver exudation uses finite quality weights

Cheng, Dey, Edelsbrunner, Facello and Teng introduced sliver exudation using weighted/regular
Delaunay triangulations.

The conceptual operation deliberately changes vertex weights so sliver configurations stop being
regular/Delaunay cells.

This is **not** the same as M2's D26LIFT1.

### D26LIFT1

    infinitesimal formal lift terms
    used only when exact InSphere/InCircle == Zero
    goal: deterministic tie topology
    ordinary non-degenerate Delaunay topology unchanged

### Sliver-exudation / quality weights

    finite algorithmic weights
    may alter regular triangulation even without exact degeneracy
    goal: element-quality improvement

Conflating these two weight domains would destroy the meaning of M2 topology policy.

## 8. Architecture consequence

Future weighted quality optimization needs a separate module/policy, conceptually:

    M2 ordinary Delaunay reference
        ↓
    M4/M5 boundary + sizing conformity
        ↓
    M6 quality optimizer
        ├── smoothing
        ├── flips/cavity reconnection
        ├── finite weighted/regular-Delaunay experiment
        └── sliver-specific treatment

D26LIFT1 remains a predicate tie policy and cannot be repurposed as a quality knob.

## 9. Sliver optimization objective

A quality optimizer should not optimize radius-edge alone.

Leading candidate objective family:
- hard constraint: no inverted tetra,
- improve worst q_MR / inverse mean ratio,
- monitor theta_min and theta_max,
- preserve CAD boundary/provenance,
- preserve acceptable sizing,
- optionally use finite regular-Delaunay weights only under a separately versioned algorithm.

Freitag/Knupp report that combining average-quality and worst-element objectives was more effective
than using either goal alone on their test meshes.

## 10. Modern research watch

Recent 2026 work on solver-side treatment of sliver tetrahedra suggests that sliver clusters/sheets
can be particularly damaging and explores modifying the finite-element treatment rather than removing
every geometric sliver.

Dynamics26 should track this as research, but it does not change the current architecture:
- mesher should still seek robust shape quality,
- solver formulations must be independently robust,
- a new solver-side sliver treatment would require its own theory/verification program.

Do not use a recent preprint as a substitute for M6 geometric quality qualification.
