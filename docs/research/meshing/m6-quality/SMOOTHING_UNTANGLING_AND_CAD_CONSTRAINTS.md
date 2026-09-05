# M6 Early Research — Smoothing, Untangling and CAD-Constrained Motion

Status: RESEARCHING / implementation not started
Date: 2026-09-06

## 1. Smoothing changes geometry, not connectivity

For a free vertex x with fixed incident topology, smoothing changes x to improve the quality of its
incident tetrahedra.

Three conceptually different tasks must remain separate:
- cheap relocation heuristic,
- constrained quality optimization,
- untangling invalid geometry.

A mesh-generation optimizer should not routinely create inverted cells and then ask an untangler to
repair them.

## 2. Ordinary Laplacian smoothing

The classic Laplacian proposal moves a vertex toward the centroid of its graph neighbors:

    x_L = (1/m) sum_j x_j.

Advantages:
- very cheap,
- local,
- often useful as a first proposal.

Disadvantages:
- no inherent quality objective,
- can decrease local worst quality,
- can invert elements,
- can move points away from sizing/CAD intent.

Therefore Dynamics26 should never commit a raw Laplacian move unconditionally.

## 3. Smart Laplacian

Research candidate:

1. compute centroid proposal,
2. optionally line-search on segment x -> x_L,
3. exact-check all incident candidate tetrahedra positive,
4. evaluate local M6 quality objective,
5. accept only if the versioned objective improves and constraints remain satisfied.

Freitag & Ollivier-Gooch found smart Laplacian better than unconditional Laplacian and much cheaper
than full optimization smoothing; a combined smart-Laplacian + optimization strategy achieved high
quality at lower cost in their experiments.

HXT also uses an accepted/improved Laplacian relocation and line search rather than blindly moving to
the centroid.

## 4. Feasible region for one interior vertex

Fix all neighbors of a vertex x.

For each incident tetrahedron, its signed six-times-volume is an affine function of x:

    D_i(x) = alpha_i dot x + beta_i.

Positive validity requires:

    D_i(x) > 0.

Therefore the local non-inversion feasible region is the intersection of open half-spaces:

    F = intersection_i { x | D_i(x) > 0 }.

Its closure is convex.

This is a very useful property:
- exact predicate truth determines which side is valid,
- a line segment between two strictly feasible points remains feasible,
- validity constraints can be treated separately from the nonlinear quality objective.

## 5. Untangling by max-min signed volume

Freitag & Plassmann exploit the affine volume property.

For a possibly tangled local star, define:

    phi(x) = min_i D_i(x).

Maximizing phi is a linear-programming style problem because all D_i are affine.

If an optimum has:

    phi(x*) > 0,

the local star has a position making every incident simplex positive.

This is a strong diagnostic/recovery method.

But maximizing minimum volume is not a good final shape-quality objective: it can distort a small,
well-shaped element simply to increase its absolute volume.

### Dynamics26 policy direction

Untangling is not the normal M6 improvement path.

If M2/M4 produced an invalid initial mesh:
- treat it as construction/geometry failure,
- do not silently "heal" it with M6.

An explicit future untangling mode may be useful for:
- imported meshes,
- controlled remeshing/adaptation recovery,
- diagnostic experiments.

Its provenance/failure semantics must be explicit.

## 6. q_MR local optimization

For a valid local star:

    q_i(x) = q_MR(T_i(x)).

Natural worst-element objective:

    maximize min_i q_i(x)
    subject to D_i(x) > 0.

The objective is nonsmooth where the active worst tetrahedron changes.

Possible research solvers:
- active-set/nonsmooth local optimization,
- barrier-constrained smooth approximations,
- line-search/trust-region methods,
- inverse-mean-ratio aggregate optimization.

No optimizer is selected yet.

Hard requirement:
every trial accepted for topology mutation must remain inside the exact-positive feasible region.

## 7. Inverse mean-ratio global/aggregate research

Munson formulates mesh shape improvement as optimization of inverse mean ratio and develops a
large-scale nonlinear optimization method.

This is valuable as an independent research direction because:
- it provides a smooth aggregate objective family,
- it can improve overall shape quality,
- it complements worst-element max-min logic.

But average/aggregate optimization alone may fail to prioritize the one disastrous element that
matters most to a solver.

Dynamics26 should benchmark:
- local max-min q_MR,
- harmonic/inverse-mean-ratio aggregation,
- combined worst + aggregate schedule.

## 8. ODT/CVT research track

Optimal Delaunay Triangulation (ODT) couples vertex positions and Delaunay connectivity through a
Delaunay-consistent energy.

Published 3D ODT work reports strong sliver suppression compared with CVT and graded-mesh
extensions, while also documenting nonconvex/nonsmooth optimization difficulties and the possible
need for additional sliver postprocessing.

ODT is scientifically valuable but is not automatically the first Dynamics26 M6 optimizer because:
- M6 wants direct FEM-quality correlation,
- boundary/CAD constraints are central,
- M2 ordinary-Delaunay identity and M6 final quality are intentionally separate,
- an ODT energy may optimize a different objective than q_MR/solver conditioning.

Keep ODT as an independent experiment.

## 9. CAD motion has a mobility dimension

A generated mesh vertex does not always have three free spatial DOFs.

Proposed classification:

| Vertex class | Mobility | Candidate coordinates |
|---|---:|---|
| volume interior | 3 DOF | x,y,z |
| CAD Face interior | 2 DOF | authoritative Face parameter coordinates / constrained surface motion |
| CAD Edge interior | 1 DOF | authoritative curve parameter |
| CAD Vertex | 0 DOF | fixed |
| protected bonded/shared interface | policy-dependent | normally shared constrained motion or fixed |

This is a geometric constraint domain, not a GUI concept.

## 10. First M6 implementation should freeze boundaries

Until M3/M4 have qualified:
- surface triangulation,
- CAD projection,
- feature protection,
- boundary provenance,
- chord/normal fidelity,

the safest first M6 optimizer is:

    interior connectivity + interior vertex motion only.

Boundary improvement is a later extension.

This follows the same engineering reason mature implementations sometimes freeze surface meshes:
boundary motion needs CAD access and can break interfaces or geometric fidelity.

## 11. Future CAD Face smoothing

A Face node cannot be optimized in unrestricted XYZ and projected back as an afterthought.

A future candidate should:
1. carry authoritative GeometryEntityId and Face parameter state,
2. optimize in a valid local parameter/manifold representation,
3. evaluate element quality in physical 3D,
4. enforce trimmed-domain/seam/feature restrictions,
5. evaluate CAD surface point/normal through GeometryMeshingView,
6. preserve surface connectivity unless a separately authorized surface flip is performed,
7. verify chordal/normal/provenance constraints.

Display tessellation is never the projection authority.

## 12. CAD Edge smoothing

For a node on a CAD Edge:
- optimize only the curve parameter,
- preserve monotonic node order,
- never cross protected CAD vertices,
- honor edge size/gradation target,
- update all incident Face/volume cells consistently.

This prevents a "quality improvement" from moving the node off the real engineering edge.

## 13. Size-field acceptance

Even an interior smoothing move can damage local size distribution.

Future acceptance should inspect:
- target h(x),
- incident edge-length ratios,
- gradation limits,
- proximity/curvature constraints where relevant.

Quality optimization is subordinate to hard geometry/provenance constraints and coordinated with the
size field.

Point insertion/deletion requires even stronger size-field semantics and is deferred.

## 14. Nonlinear solver connection

Generation-time smoothing acts on the reference geometry A0.

It is not a runtime node-relaxation algorithm for a nonlinear solve.

During physical deformation:
- current positions x are solution unknowns,
- moving them for "mesh quality" changes the physics,
- remeshing/adaptation requires field/state transfer and belongs to M9.

M6 generation smoothing must never be invoked inside a solve as an invisible geometry fix.

## 15. Candidate interior smoothing schedule

Research-only candidate:

    for poor interior vertex stars in deterministic order:
        propose smart Laplacian / line-search move
        if hard-valid + objective improves:
            accept
        else if star remains below quality activation level:
            run q_MR optimization fallback
        commit only deterministic strict improvement

Then rerun local reconnection because smoothing can make new topological swaps beneficial.

This schedule is consistent with classical evidence that smoothing and swapping reinforce one another.

## 16. Determinism

Vertex-update order affects the resulting local optimum.

Research candidate:
- stable vertex key order within quality buckets,
- deterministic active queue,
- deterministic line-search evaluation sequence,
- canonical fallback/tie policy,
- update only affected stars.

Parallel smoothing is later research; it must specify conflict coloring/independent sets and
reproducibility policy explicitly.


## 17. Alternating optimization and local traps

Smoothing and connectivity optimization act on different variables:

    smoothing:
      coordinates change, connectivity fixed

    reconnection:
      connectivity changes, coordinates fixed.

A connectivity state that is locally optimal before smoothing can become improvable after smoothing.

A smoothing star that is locally stuck can become improvable after reconnection.

Therefore the M6 search should be researched as alternating optimization rather than a single pass of
one operation class.

## 18. Smoothing termination is not implied by finite topology

Connectivity-only strict hill climbing on a fixed finite point set has a finite-state termination
argument.

Smoothing does not.

Coordinates vary continuously, so an unbounded sequence of arbitrarily small improvements is
conceptually possible.

Future smoothing convergence policy must explicitly define combinations of:
- minimum accepted quality gain,
- minimum displacement,
- line-search limit,
- pass/effort limit,
- active-set stall.

These are optimizer-policy values, not geometric tolerances.


## 19. Proposal objective versus authoritative acceptance

Smooth inverse-mean-ratio / condition-number objectives are attractive because they support
differentiable nonlinear optimization.

They are now treated as **proposal objectives**.

A proposal does not become mesh authority merely because its aggregate objective improves.

Leading commit rule:

    exact positive local star
      +
    QMRVector(new star)
      >_lex
    QMRVector(old star).

This combines the numerical convenience of aggregate smoothing with worst-tail protection.

## 20. Aggregate-objective negative controls

Research fixtures must include at least:

### Worst regression accepted by harmonic mean

    old = [0.20,0.20]
    new = [0.19,0.50].

Harmonic mean improves while the worst tetrahedron degrades.

### Lex improvement rejected by aggregate mean

    old = [0.20,0.30,0.80]
    new = [0.20,0.31,0.32].

The second worst improves, so the lexicographic low tail improves, while arithmetic/harmonic means
decrease.

These fixtures prove proposal and commit objectives must remain distinct.
