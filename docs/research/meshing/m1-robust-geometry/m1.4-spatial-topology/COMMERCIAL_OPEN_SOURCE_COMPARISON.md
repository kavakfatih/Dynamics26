# M1.4 Comparison — Commercial CAE and Open-Source Scalability/Search

## 1. Commercial-source limitation

ANSYS, COMSOL and Marc do not publish their internal point-location or tetra adjacency data structures in the reviewed documentation.

Therefore:
- no claim is made that they use a kd-tree, BVH, walk, BRIO or any other internal search algorithm,
- comparison is restricted to public scalability/meshing behavior.

## 2. ANSYS

ANSYS Meshing publicly supports:
- Parallel Part Meshing,
- parallel execution for selected methods including Patch Independent Tetra and MultiZone,
- explicit CPU allocation,
- memory-aware guidance.

The documentation warns against oversubscribing cores/memory and gives a default planning figure of at least roughly 2 GB per CPU core for parallel part meshing.

### Dynamics26 lesson

A scalable mesher needs:
- explicit memory telemetry,
- thread-count controls later,
- no assumption that more cores always improve meshing.

This does not specify M2 search internals.

## 3. COMSOL 6.4

COMSOL states that meshing benefits from shared-memory parallelism.

Its 3D tetra mesher parallelizes over geometry faces and domains. COMSOL explicitly notes:
- a geometry with only one domain, such as one imported CAD part, may see almost no parallel tet-meshing speedup,
- multi-domain assemblies can see significant speedup,
- parallel execution can require more memory.

### Dynamics26 lesson

Our automotive use case often includes one complicated solid/rubber volume.

Therefore:
- domain-level parallelism alone is not enough as a long-term strategy,
- but parallelizing the first Delaunay kernel before serial correctness is still premature.

## 4. Marc / Mentat

Marc public documentation emphasizes:
- global remeshing,
- local adaptivity,
- mesh-density control,
- hard/soft nodes/edges/faces,
- user-defined mesh density hooks,
- current Mentat meshing improvements.

Public Marc sources reviewed here do not expose point-location or tetra storage internals.

### Dynamics26 lesson

Long-term search/data structures must remain compatible with:
- protected topology,
- density fields,
- remeshing,
- field/state transfer.

M2 can be much narrower.

## 5. CGAL

Public CGAL 3D triangulation design separates:
- geometric traits/predicates,
- combinatorial triangulation data structure,
- optional location policy.

Point location:
- accepts a start cell/vertex hint,
- can use a faster additional location structure,
- reports typed location states,
- supports concurrency locking in advanced configurations.

### Dynamics26 lesson

Keep geometry decisions, combinatorial topology and point-location policy separable.

## 6. TetGen

The 2015 TetGen paper describes:
- efficient tetrahedral mesh data structure,
- incremental vertex insertion,
- spatial point sorting,
- filtered exact predicates.

It specifically states that large-data incremental efficiency depends on insertion order and that spatial sorting enables very efficient point location in practice.

### Dynamics26 lesson

Insertion order is a first-class performance parameter, not just input formatting.

## 7. Gmsh sequential Delaunay source study

High-level observations:
- tetrahedra store four vertices and four neighbor references,
- point insertion uses neighbor walking,
- spatial/Hilbert-style ordering appears in the insertion pipeline,
- cavity and boundary buffers are reused,
- storage avoids moving live tetra objects during growth.

No source layout or code is copied.

## 8. HXT/Gmsh parallel source + paper study

The HXT line of research demonstrates:
- compact dedicated tetra arrays,
- walk-to-cavity,
- breadth-first cavity traversal,
- spatial curve ordering,
- space-filling-curve partitioning for parallel insertion,
- conflict handling between partitions.

Marot–Pellerin–Remacle report that dedicated data structure + point sorting + insertion optimization gave large serial speed gains before parallelization, and then use Moore-curve partitioning for multithreading.

### Dynamics26 lesson

Correct optimization order:

```text
serial topology correctness
→ spatial locality
→ compact memory
→ profile
→ parallel partitioning
```

not the reverse.

## 9. Netgen

Netgen's advancing-front meshing architecture queries local front faces around a selected base face and maintains spatial/local-size structures.

This is a different volume-meshing family but reinforces a general lesson:
- local candidate extraction/search is a core mesher subsystem,
- it should be isolated from geometry identity and final FEM storage.

## 10. Comparison matrix

| Concern | ANSYS | COMSOL | Marc | Open-source evidence | Dynamics26 M2 |
|---|---|---|---|---|---|
| Search internals public | No | No | No | Yes in research/source | own design |
| Parallel meshing | Yes | Yes | not used as internal spec | HXT strong | later |
| Memory sensitivity | explicit | explicit | large nonlinear/remesh workflows | compact arrays matter | measure from start |
| Point-location hint | internal unknown | internal unknown | internal unknown | CGAL/TetGen/Gmsh | walk + hint |
| Spatial ordering | internal unknown | internal unknown | internal unknown | BRIO/HXT/TetGen | benchmark |
| Typed locate state | internal UI abstraction | geometry/mesh status | meshing diagnostics | CGAL explicit | required |
