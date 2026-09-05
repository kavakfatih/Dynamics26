# M6 Early Research — General Edge Removal by Link-Polygon Optimization

Status: RESEARCHING / mathematical algorithm candidate
Date: 2026-09-05

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

## 5. Triangle contribution quality

For a proposed link triangle tau=(v_i,v_k,v_j), define its two pole cells:

    T_a(tau)
    T_b(tau).

First exact-validity gate:
- both cells have distinct vertices,
- exact Orient3D != Zero,
- both can be oriented positive.

Then define candidate triangle score:

    w(i,k,j)
      = min(
          q_MR(T_a),
          q_MR(T_b)
        ).

If either cell is invalid:

    w(i,k,j) = -infinity / invalid.

The use of q_MR is a current M6 research candidate, not a production threshold.

## 6. Max-min dynamic programming

We want the polygon triangulation whose worst generated tetrahedron is as good as possible.

Let:

    Q[i,j]

be the best achievable worst quality for the polygon chain from v_i to v_j.

Base:
- adjacent polygon vertices contain no triangle, so treat their neutral score as +infinity.

Recurrence:

    Q[i,j]
      = max_{i<k<j}
          min(
              Q[i,k],
              Q[k,j],
              w(i,k,j)
          ).

Store the maximizing split k for reconstruction.

Complexity:

    time  O(N^3)
    space O(N^2).

This is a standard polygon-triangulation dynamic-programming structure applied to a Dynamics26
max-min tetra quality objective.

It avoids copying implementation tables from external meshers and naturally covers arbitrary
reasonable edge valence.

## 7. Deterministic tie handling

If multiple k values give numerically equal accepted quality under the M6 comparison policy:
- compare the complete canonical diagonal set / resulting canonical tetra connectivity,
- select the lexicographically smallest versioned candidate.

Do not use hash iteration order.

This gives reproducible local topology for a fixed site set and M6 quality policy.

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

## 11. Exact topology versus quality arithmetic

Use M1 exact predicates for:
- candidate orientation,
- flatness,
- local geometric legality.

Use q_MR floating calculations only for:
- ranking already-valid candidates.

This separation is essential to preserve M1/M2 robustness contracts.
