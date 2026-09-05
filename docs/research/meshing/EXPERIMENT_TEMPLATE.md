# Meshing R&D Experiment Template

Copy this template for each substantial experiment.

---

# EXP-MESH-XXXX — Title

**Date:** YYYY-MM-DD  
**Researcher/session:**  
**Branch:** main  
**Dynamics26 HEAD:**  
**Algorithm version/commit:**  
**Status:** PLANNED / RUNNING / PASS / FAIL / INCONCLUSIVE

## 1. Question

What engineering or algorithmic question is this experiment answering?

## 2. Hypothesis

State the expected result before running the experiment.

## 3. Theory sources

List IDs from \`SOURCE_REGISTRY.md\`.

- TH-...
- OS-... if an external architecture/test comparison is used.

## 4. Input

### Geometry
- name:
- source:
- geometry hash:
- units:
- topology summary:

### Meshing settings
- global size:
- local size:
- curvature:
- proximity:
- growth:
- seed:
- other:

## 5. Algorithm configuration

- surface algorithm:
- volume algorithm:
- boundary recovery:
- optimization:
- predicate mode:
- deterministic ordering:

## 6. Acceptance criteria

Define pass/fail before results.

Example:

- zero inverted tetrahedra,
- all CAD Face boundaries recovered,
- min dihedral > target,
- max geometry deviation < target,
- deterministic fingerprint stable across 10 runs.

## 7. Results

### Counts
- nodes:
- surface triangles:
- tetrahedra:

### Correctness
- inverted:
- missing boundary:
- non-manifold:
- orphan nodes:

### Quality
- min dihedral:
- p01 / p05 / p50:
- max radius-edge:
- condition metric:

### Geometry fidelity
- max distance:
- normal error:

### Performance
- time:
- peak RAM:

### Reproducibility
- fingerprint:
- repeated runs:

## 8. External comparison

If Netgen/Gmsh/MMG/TetGen/CGAL/commercial software is used, record only the independent comparison result and version.

No source code is copied.

## 9. Failure analysis

Describe exact failure mechanism or uncertainty.

## 10. Decision

- KEEP
- MODIFY
- REJECT
- MORE RESEARCH

Explain why.

## 11. Follow-up

List next experiment IDs or ADR impact.
