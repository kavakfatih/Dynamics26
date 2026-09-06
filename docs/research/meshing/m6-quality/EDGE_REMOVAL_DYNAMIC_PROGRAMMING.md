# M6 Early Research — General Edge Removal by Link-Polygon Optimization

Status: DESIGN FROZEN / D26OPS1-O3 algorithm contract
Date: 2026-09-06

## 1. Why general edge removal matters

Elementary 2<->3 flips are cheap but can get trapped in local maxima.

A stronger local operation removes an interior edge and retriangulates its complete incident cavity.
Freitag & Ollivier-Gooch used edge swapping as an important supplement to face swaps. Shewchuk later
described dynamic-programming optimization for edge removal, and HXT treats edge removal as one of
its most useful topological improvement operations.

Dynamics26 can derive a compact original formulation from the link polygon of an interior edge.

## 2. Interior-edge star

Let the interior edge be:

    e = {a,b}.

Assume its incident tetrahedra form a valid cyclic star with ring/link vertices:

    v0, v1, ..., v_{N-1}

in topological order.

Old cells:

    (a,b,v_i,v_{i+1})     i mod N.

There are N old tetrahedra.

The link of an interior edge in a valid tetrahedral 3-manifold is a closed polygonal cycle.

Before any quality optimization, verify:
- one connected cyclic edge star,
- each ring edge occurs in the expected old cell pair,
- no protected feature semantics on {a,b},
- candidate cavity is internal and its boundary is unchanged by the operation.

## 3. Removing the edge

After removing edge {a,b}, triangulate the N-gon:

    P = (v0, v1, ..., v_{N-1})

into N-2 triangles.

For each link triangle:

    (v_i, v_k, v_j)

create two tetrahedra, one attached to each pole:

    (a, v_i, v_k, v_j)
    (b, v_i, v_j, v_k)

with local ordering normalized to positive exact Orient3D.

Therefore every complete edge removal produces:

    2(N-2) = 2N - 4

new tetrahedra.

Examples:
- N=3 -> 2 tets: ordinary 3->2,
- N=4 -> 4 tets: a 4->4 alternative,
- N=5 -> 6 tets,
- N=6 -> 8 tets,
- N=7 -> 10 tets.

## 4. Combinatorial search size

The number of triangulations of a convex combinatorial N-gon is the Catalan number:

    Cat_{N-2} = 1/(N-1) * binomial(2N-4, N-2).

Thus:
- N=3: 1,
- N=4: 2,
- N=5: 5,
- N=6: 14,
- N=7: 42.

Not every combinatorial triangulation is a geometrically valid 3D edge-removal patch.

A candidate link triangle is invalid if either of its associated pole tetrahedra is flat/inverted or
if the assembled patch fails the generic cavity validator.

## 5. Link-triangle exact quality contribution

For proposed link triangle:

    tau=(v_i,v_k,v_j)

define the two pole tetrahedra:

    T_a(tau)
    T_b(tau).

First exact-validity gate:
- distinct vertices,
- exact Orient3D non-zero,
- local ordering normalized positive,
- protected/boundary legality satisfied.

Define the exact two-element contribution:

    W(i,k,j)
      =
    sort_ascending(
      q_MR(T_a),
      q_MR(T_b)
    )

under D26QMR1.

If either pole tetra is invalid:

    W(i,k,j)=Invalid.

No floating q_MR threshold or approximate equality enters the authoritative DP.

## 6. Frozen D26QV1 dynamic programming recurrence

The historical scalar max-min recurrence is retained only as an independent first-component oracle.

Authoritative O3 must optimize the full D26QV1 quality multiset.

Let:

    V[i,j]

be the best achievable sorted exact q_MR multiset for triangulating polygon chain:

    v_i,...,v_j.

Base for adjacent vertices:

    V[i,i+1]
      =
    empty multiset.

For split i<k<j, define candidate:

    C(i,k,j)
      =
    merge_sorted(
      V[i,k],
      V[k,j],
      W(i,k,j)
    ).

If:
- W is invalid,
- either subproblem is invalid,

the split is invalid.

Recurrence:

    V[i,j]
      =
    max_D26QV1 over valid k
      C(i,k,j).

Store the deterministic maximizing split k for reconstruction.

### Optimal-substructure proof

D26QV1 has union compatibility:

    A > B
      =>
    A union C > B union C.

Therefore for a fixed split k:
- replacing a subpolygon completion by a strictly better D26QV1 subcompletion can never make the
  complete candidate worse after union with the unchanged other subpolygon and W contribution.

Hence each subproblem may safely retain only its D26QV1-optimal completion.

This proves the recurrence optimizes the full quality vector over the edge-removal triangulation space.

### Complexity

There are O(N^2) subproblems and O(N) splits each.

With a transparent vector representation:
- sorted-vector merge/compare costs O(N),
- reference worst-case time is O(N^4),
- stored vector payload is O(N^3) in the simplest table representation.

Typical edge valence is expected to be small, but no production bound is assumed.

A later compact/persistent representation may recover lower practical cost without changing the
D26QV1 recurrence semantics.

## 7. Deterministic exact tie handling

If two complete split candidates have exactly equal D26QV1 vectors:
- compare their complete canonical diagonal/connectivity representation,
- choose the lexicographically smallest versioned canonical candidate.

This tie chooses a proposal representation only.

Commit still requires:

    new cavity D26QV1
      >
    old cavity D26QV1.

An exact quality tie versus the old cavity never mutates authoritative topology.

Do not use:
- floating tolerance,
- hash iteration,
- allocation order.

## 8. Candidate patch validation remains mandatory

Dynamic programming optimizes scores; it does not prove the 3D embedding by itself.

After reconstructing the candidate:
1. every new finite tetra exact-positive,
2. every internal face has incidence two,
3. exterior cavity boundary exactly matches old boundary,
4. no duplicate tetra or illegal pre-existing diagonal/face relation,
5. exact old/new cavity volume matches,
6. no protected CAD/interface/provenance change,
7. size-field acceptance passes if applicable,
8. quality policy confirms strict improvement.

Any failure rejects the operation.

## 9. Why no precomputed N<=7 table is required

Historical implementations often precompute or enumerate small edge-star possibilities because
typical valence is modest.

Dynamics26's first reference research path should favor:
- one transparent recurrence,
- deterministic reconstruction,
- generic candidate validation,
- no copied lookup tables.

If profiling later shows N is overwhelmingly small and DP cost matters, a generated cache can be
derived from the same recurrence and tested against it.

## 10. Beyond one edge

The inverse family, multi-face removal, and more general Small Polyhedron Reconnection search a
larger local triangulation space.

HXT documents that basic 2<->3 hill climbing can stall and uses stronger cavity reconnection
operations to escape local maxima.

Dynamics26 should defer these until:
- elementary flips,
- edge removal DP,
- smoothing

have executable quality/solver evidence.

## 11. Exact topology versus exact quality ordering

Use M1 exact predicates for:
- candidate orientation,
- flatness,
- cavity/topology legality.

Use D26QMR1 for:
- exact ordering of already-valid tetra qualities.

Use D26QV1 for:
- exact local patch/cavity quality-vector ordering.

Displayed q_MR may be floating diagnostic data only.

This preserves the M1/M6 ownership boundary without approximate optimizer equality.

## 12. Exact scope of the D26QV1 DP optimum

Let T_e be all geometrically/constraint-valid cavity retriangulations that remove selected edge e.

The frozen DP solves:

    max_D26QV1 over R in T_e
      QMRVector(R).

This is an exact D26QV1 optimum over T_e when:
- every legal link-polygon triangulation is represented,
- invalid W contributions are rejected correctly,
- subproblem recurrence uses exact D26QV1 ordering,
- reconstructed candidate passes the generic cavity validator.

It is not an optimum over every triangulation of the surrounding mesh.

Historical max-min DP remains an independent oracle for the first vector component and must agree with
the first element of the D26QV1 optimum where applicable.

## 13. Why multi-face removal is not redundant

Edge removal asks whether one edge should disappear.

Its inverse family, multi-face removal, asks whether a connected family of sandwiched faces should
disappear and the complementary edge should appear.

An m-face removal replaces:

    2m -> m+2

tetrahedra.

The 2->3 flip is the m=1 special case.

Therefore edge removal alone does not span the entire point-set-preserving local reconnection
neighborhood.

## 14. DP versus general cavity search

The edge-link structure reduces the candidate problem to a polygon triangulation and enables:

    O(N^3) time
    O(N^2) memory.

A general SPR cavity lacks this one-dimensional cyclic-link reduction.

That is why edge removal should remain a cheap strong local primitive even if SPR is researched later.
