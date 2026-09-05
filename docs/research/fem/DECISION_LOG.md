# FEM Research Decision Log

## FEM-ADR-0001 — TET4 incompressibility requires independent formulation qualification

**Status:** ACCEPTED (RESEARCH RULE)
**Date:** 2026-09-05

### Decision

Dynamics26 does not infer nearly-incompressible/rubber TET4 suitability from:
- Delaunay correctness,
- geometric mesh quality,
- existing HEX8 mixed u-p tests.

TET4 receives a separate stability/locking/tangent/convergence qualification program.

---

## FEM-ADR-0002 — Do not adopt TET4 P1/P0 as the default rubber formulation

**Status:** PROPOSED
**Date:** 2026-09-05

### Leading evidence

Peer-reviewed nearly-incompressible studies identify simple P1-P0 as an unstable/locking low-order
pair in demanding tetrahedral applications and show stress/convergence deficiencies relative to
locking-free stabilized formulations.

### Research consequence

P1/P0 remains a negative-control/reference path unless later evidence overturns this conclusion.

---

## FEM-ADR-0003 — MINI is the leading stable mixed reference candidate

**Status:** PROPOSED
**Date:** 2026-09-05

### Candidate

    u = P1 + local bubble
    p = continuous P1.

Use:
- numerical inf-sup sequence,
- nonlinear bubble static-condensation verification,
- finite-strain tangent/locking benchmarks.

### Why reference, not product selection

It provides a strong mathematical comparison point but adds global pressure DOFs and local bubble
state. Performance/product selection remains open.

---

## FEM-ADR-0004 — Stabilized P1/P1 and patch F-bar remain independent candidate families

**Status:** PROPOSED
**Date:** 2026-09-05

### Stabilized P1/P1

Potential practical mixed low-order path. Requires its own stabilization derivation and cannot be
qualified by raw equal-order inf-sup alone.

### Patch F-bar

Potential displacement-only path. Requires patch topology, nonlocal exact tangent and locking
qualification. It is not equivalent to a true mixed pressure formulation.

Neither candidate is selected yet.

---

## FEM-ADR-0005 — TET4 is not automatically the final rubber accuracy element

**Status:** PROPOSED
**Date:** 2026-09-05

### Decision candidate

Even if a locking-free TET4 formulation is qualified, Dynamics26 will retain a higher-order
hybrid/mixed TET10 research path.

Reason:
- linear tetra interpolation has limited gradient/bending accuracy,
- public commercial guidance commonly prefers quadratic tetra for general accuracy,
- recent rubber-like mixed-element research includes robust quadratic tetrahedra under severe
  compression.

TET4 can be a robust arbitrary-mesh baseline without becoming the final accuracy ceiling.
