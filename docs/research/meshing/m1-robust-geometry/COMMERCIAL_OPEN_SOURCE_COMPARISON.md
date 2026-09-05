# M1 Comparison — Commercial and Open-Source Geometry Robustness

## 1. Scope limitation

Commercial vendors generally document geometry repair, defeaturing and meshing controls, but they do not publish the source-level implementation of orientation/insphere predicates.

Therefore this comparison does not claim to know proprietary predicate algorithms.

## 2. ANSYS

Public ANSYS documentation exposes:

- mesh defeaturing,
- defeature size,
- selective topology protection,
- topology repair operations,
- local/global size controls.

Interpretation:

- geometry simplification is explicit policy,
- important engineering topology can be protected,
- meshing robustness is not presented to the user as one arbitrary epsilon.

**Dynamics26 adaptation:** explicit geometry cleanup/simplification controls later; do not silently merge topology.

## 3. COMSOL

COMSOL documents geometry repair tolerance as:

- automatic,
- relative,
- absolute.

Geometry operations can merge entities separated by less than the repair tolerance.

Interpretation:

- CAD/model repair tolerance is a first-class geometry-operation concept,
- it is distinct from the FEM mesh discretization itself.

**Dynamics26 adaptation:** keep repair tolerance in geometry policy, separate from robust predicate exactness.

## 4. Marc / Mentat

Public Marc/Mentat release notes expose several tolerance-related user behaviors, for example auto-calculated display/geometry tolerances and sweep-node merging within tolerance.

No public robust-predicate implementation contract was found in this M1 research pass.

**Dynamics26 adaptation:** do not infer undocumented internals.

## 5. CGAL

Public CGAL documentation explicitly supports a kernel concept with:

- exact geometric predicates,
- inexact geometric constructions.

It also states why predicates are crucial to control-flow correctness.

This is a strong architecture reference for separating discrete decisions from metric constructions.

No CGAL code/API will be copied.

## 6. Gmsh source study

High-level public source-tree observations:

- robust predicates are isolated under a numerical subsystem,
- the 3D Delaunay implementation consumes robust orientation/insphere decisions,
- the Delaunay code contains an explicit degeneracy/tie-handling path,
- geometry/mesh data structures remain separate from the predicate module.

Lesson:

```text
predicate kernel
!=
Delaunay algorithm
!=
mesh model
```

No constants, code, class layout or tie-break implementation are adopted from Gmsh.

## 7. TetGen source study

High-level observations:

- TetGen carries robust orientation/incircle/insphere infrastructure as a separate concern,
- its public headers describe Delaunay, constrained Delaunay, refinement and boundary preservation as separate meshing capabilities,
- recent source also makes license boundaries visible.

TetGen includes a copy/adaptation of public-domain robust predicate code, but the Dynamics26 original-engine strategy explicitly chooses not to copy it.

Lesson:

- predicate robustness is foundational,
- constrained boundary recovery is separate from unconstrained Delaunay.

## 8. Netgen study

Netgen is an important advancing-front/mesh-optimization reference.

This M1 source search did not establish a simple public `orient3d/insphere` architecture comparable to Gmsh/TetGen, so no unsupported claim is recorded.

Lesson:

> absence of a quick source-search match is not evidence that robust geometry is unimportant.

## 9. Cross-comparison

| Concern | Commercial products | Open-source study | Dynamics26 direction |
|---|---|---|---|
| Geometry repair tolerance | Public/user-visible | varies | explicit GeometryTolerancePolicy |
| Topological predicate exactness | undocumented internal | strong public examples/literature | original RobustPredicates |
| Degeneracy | mostly hidden from user | explicit algorithm concern | predicate Zero + higher-level policy |
| Exact predicates vs constructions | not publicly specified | CGAL explicitly separates | separate modules |
| Fast vs exact path | undocumented | filtering/adaptive literature | research leading design |
| Source reuse | unavailable/proprietary | licenses vary | no source copy |

## 10. Main conclusion

The robust-geometry architecture should be justified by computational-geometry literature, not by attempting to imitate a vendor implementation.

Commercial products tell us what a professional user should experience:
- geometry problems are diagnosable,
- tolerances are explicit,
- important topology can be protected.

Open-source studies tell us what algorithmic concerns repeatedly appear:
- certified predicates,
- degeneracy,
- topology data structures,
- separation of generation stages.
