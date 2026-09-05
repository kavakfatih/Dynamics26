# M6 Early Research — Local Topology Improvement

Status: RESEARCHING / implementation not started
Date: 2026-09-06

## 1. Engineering question

Once M2/M4 have produced a valid tetrahedralization, which local connectivity changes should
Dynamics26 use to improve FEM-oriented tetrahedral shape without silently changing:
- the point set,
- CAD boundary topology,
- protected interfaces,
- size-field intent,
- geometry provenance?

The answer is not "keep the mesh Delaunay at all costs". Classical experiments show that an
in-sphere/Delaunay face-swap pass can be useful as an intermediate connectivity cleanup, but a final
in-sphere-only objective can leave very poor extremal dihedral angles. Swapping and smoothing are
more effective when combined.

Primary sources:
- Barry Joe, Three-Dimensional Triangulations from Local Transformations, 1989.
- Freitag & Ollivier-Gooch, Tetrahedral Mesh Improvement Using Swapping and Smoothing, 1997.
- Marot & Remacle, Quality tetrahedral mesh generation with HXT, 2020.
- Shewchuk, Two Discrete Optimization Algorithms for the Topological Improvement of Tetrahedral
  Meshes, 2002 research manuscript.

## 2. Topology operations and point-set semantics

The fundamental 3D bistellar/Pachner operations are:
- 1 -> 4: add a vertex inside one tetrahedron,
- 4 -> 1: remove a degree-4 interior vertex,
- 2 -> 3: replace two tetrahedra sharing a face by three sharing a new edge,
- 3 -> 2: inverse of 2 -> 3.

For a quality stage that should initially preserve the point distribution and size field:

    first candidates = 2<->3 and their generalizations.

1<->4 / 4<->1 change the point set and therefore belong behind an explicit refinement/coarsening
policy, not the first M6 quality pass.

A 4->4 operation is useful in practice but is not a fundamental 3D Pachner move; it can be viewed as
a composition of 2->3 followed by 3->2, or as the N=4 case of a general edge-removal cavity.

## 3. 2 -> 3 local reconnection

Current cavity:

    T0 = (a,b,c,d)
    T1 = (a,c,b,e)

with common internal face {a,b,c} and opposite vertices d,e on opposite sides.

Candidate:

    (a,b,d,e)
    (b,c,d,e)
    (c,a,d,e)

sharing new interior edge {d,e}.

A flip is not legal merely because the connectivity can be written.

Reference validation must require:
1. old two-cell cavity is a valid topological 3-ball,
2. shared face is not protected,
3. complementary edge {d,e} is not already present elsewhere in a way that violates the simplicial
   complex,
4. all three candidate tetrahedra have exact non-zero orientation and can be stored positive,
5. candidate internal faces pair exactly twice,
6. candidate exterior boundary is exactly the old cavity boundary,
7. exact total signed/absolute cavity volume is conserved,
8. no CAD/interface/provenance constraint is modified,
9. quality acceptance policy approves the candidate.

Using a generic local-retriangulation validator is safer than relying on a diagram-specific
"convexity epsilon".

## 4. 3 -> 2 local reconnection

Current cavity contains exactly three tetrahedra around an interior edge {d,e} whose link is the
triangle (a,b,c).

Candidate removes {d,e} and creates:

    (a,b,c,d)
    (a,c,b,e)

The move requires:
- the edge star relevant to the move has valence exactly three,
- complementary face {a,b,c} is not already an incompatible simplex,
- same boundary / volume / positive-candidate invariants as 2->3,
- protected feature checks,
- quality improvement.

This is the N=3 edge-removal case.

## 5. 4 -> 4 as a composite quality operation

An interior edge with four incident tetrahedra has a quadrilateral link.

There are two triangulations of that link polygon, so removing the old edge and choosing the other
diagonal yields four replacement tetrahedra.

This is the quality-relevant 4->4 operation.

Dynamics26 should not hard-code a separate topological universe for it. It falls naturally out of
the general edge-removal formulation in EDGE_REMOVAL_DYNAMIC_PROGRAMMING.md.

## 6. Delaunay legality versus FEM quality

For a reconfigurable five-point 2/3 cavity, the in-sphere criterion chooses the locally Delaunay
configuration.

But M6's final objective is FEM-oriented quality, not ordinary Delaunay identity.

Therefore:
- M2 owns Delaunay correctness for construction,
- M6 may deliberately accept a non-Delaunay local reconnection if it improves the versioned quality
  objective and preserves all hard constraints,
- no M6 operation is allowed to weaken M1 exact orientation/topology validity.

This separation is supported experimentally by Freitag & Ollivier-Gooch: in-sphere swapping was
useful as an early pass, but using in-sphere as the final face-swap criterion performed poorly for
extremal angles.

## 7. Cavity quality comparison

For a local cavity C with positive TET4s and primary element score q_MR:

    q_min(C) = min_{T in C} q_MR(T).

A conservative first comparison is:

    q_min(new) > q_min(old).

This cannot sacrifice the old worst element for a better average.

However pure max-min hill climbing can stall.

Research candidate for deterministic secondary comparison:

    QualityKey(C)
      = (
          q_min,
          harmonic_mean(q_MR),
          arithmetic_mean(q_MR),
          -cell_count
        )

compared lexicographically after hard validity.

This is not accepted yet; Q2 experiments must compare it against:
- pure max-min,
- combined worst + average objectives,
- angle-based objectives.

Freitag/Knupp and Freitag/Ollivier-Gooch provide evidence that worst-element and aggregate objectives
have complementary behavior.

## 8. Numerical quality comparison is not a geometric predicate

The exact predicate contract remains:

    Orient3D / topology truth -> filtered-exact M1 decision.

Quality values such as q_MR use floating arithmetic and are optimization scores.

If two candidate quality scores are too close to distinguish robustly under the chosen numerical
policy:
- do not invent a geometry epsilon,
- treat the quality decision as an optimization tie,
- prefer no-op or a deterministic canonical candidate according to the versioned M6 policy.

A quality-comparison tolerance/margin, if later used, is an optimizer policy parameter and must never
be reused for orientation, manifoldness or CAD identity.

## 9. Protected topology policy

Early M6 connectivity changes operate on interior tetrahedral topology only.

Blocked initially:
- CAD boundary triangle replacement,
- CAD feature edge replacement,
- seam/interface triangle changes,
- bonded/shared interface connectivity changes,
- topology with unresolved GeometryEntityId provenance.

A future surface optimizer may allow a boundary 2->2 triangle diagonal change only when:
- both triangles belong to the same authoritative CAD Face,
- no protected CAD Edge is changed,
- the new triangle geometry satisfies surface/chord/normal constraints,
- provenance is rebuilt exactly.

This is separate future research.

## 10. Deterministic operation scheduling

Local optimization is path-dependent: two legal quality-improving flips applied in a different order
can lead to different local maxima.

M6 therefore needs deterministic scheduling independent of container iteration.

Research candidate:
1. active poor-tet queue ordered by (q_MR ascending, CanonicalTetKey),
2. generate legal local operations in a fixed operation-type order,
3. compare candidate QualityKey,
4. break exact/optimization ties by canonical connectivity key,
5. after commit, invalidate/recompute only the affected neighborhood,
6. repeat until no strict accepted improvement or a versioned pass/effort budget is reached.

Parallel M6 optimization must preserve a separately specified deterministic policy or explicitly carry
a different algorithm version.

## 11. Research candidate schedule

Not accepted as production yet:

    valid M2/M4 mesh
      -> interior 2<->3 quality flips
      -> general edge removal on worst edge stars
      -> smart interior smoothing
      -> optimization-based smoothing on remaining low-tail stars
      -> repeat local topology + smoothing
      -> optional later sliver-specific finite-weight/cavity treatment
      -> final validator + quality distribution

Why interleave?
- smoothing cannot fix bad connectivity,
- connectivity changes cannot optimize vertex positions,
- literature repeatedly reports better results from combinations than either mechanism alone.

## 12. Operations deliberately deferred

Not part of the first M6 implementation candidate:
- point insertion/deletion for quality,
- edge contraction,
- surface movement,
- weighted sliver exudation,
- Small Polyhedron Reconnection / exhaustive large cavity search,
- parallel quality optimization,
- anisotropic metric-space reconnection.

They remain high-value research tracks.


## 13. Reachability definition of a local optimum

For operation family O:

    N_O(M)
      = one-operation legal neighbors of M.

A mesh is O-locally optimal when no M' in N_O(M) strictly improves the versioned quality order.

Expanding the operation family enlarges the reachable neighborhood.

Therefore:

    elementary-flip local optimum
    != edge-removal local optimum
    != general-cavity local optimum.

See LOCAL_OPTIMIZATION_TRAPS_AND_STRONG_RECONNECTION.md.

## 14. Composite operations must be transactional

A final 4->4 or stronger reconnection can be representable as a sequence of elementary flips while
requiring a non-improving intermediate state.

Dynamics26 must not commit such an intermediate state.

Strong/composite search follows:

    Plan -> Search -> Validate -> Compare final -> Commit/Discard.

This preserves monotone committed mesh quality while permitting non-monotone private exploration.

## 15. Multi-face / SPR research position

Multi-face removal is the inverse family of edge removal and adds connectivity moves not covered by
edge removal alone.

Small Polyhedron Reconnection searches a much larger fixed-cavity triangulation space and can subsume
edge/multi-face final states for the same cavity.

Current direction:
- keep multi-face removal as a research oracle/middle-tier candidate,
- use bounded SPR only as a later last-resort experiment,
- do not implement either before elementary/edge/smoothing gates exist.

## 16. Termination distinction

For fixed coordinates and a fixed finite point set, strictly improving point-set-preserving
connectivity mutations terminate because only finitely many tetrahedralizations exist.

This proof does not extend directly to:
- smoothing, because coordinates vary continuously,
- unrestricted point insertion/deletion, because the point set changes.

Those paths require separate convergence/resource stop policies.
