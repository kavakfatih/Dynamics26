# Dynamics26 Tetra Quality Metric Policy — Early Research

Status: PROPOSED / thresholds not accepted
Date: 2026-09-06

## 1. Quality layers

Dynamics26 should separate five layers:

1. Topology validity
2. Initial geometric shape quality
3. CAD/sizing fidelity
4. Solver/formulation suitability
5. Current-configuration nonlinear distortion

A single scalar cannot faithfully represent all five.

## 2. Proposed M6 TET4 metric roles

### Hard validity

- exact positive Orient3D / signed volume,
- finite coordinates,
- distinct nodes,
- valid topology/provenance.

Failure is binary and blocks solver use.

### Primary isotropic shape

    q_MR in (0,1]

Mean ratio is the leading M6 optimization/release-shape candidate because it is:
- normalized,
- scale/rotation invariant,
- smooth for positive elements,
- tied to Jacobian singular values,
- zero at degeneracy,
- computationally practical.

### Solver-facing shape diagnostic

    q_kappa = 3/kappa_F(T).

Use to expose mapping conditioning/distance-to-singularity interpretation.

Do not double-count q_MR and q_kappa as independent metrics.

### Sliver/angle diagnostics

    theta_min
    theta_max.

Angles are interpretable and catch important sliver behavior, but are not a complete sole metric.

### Delaunay-refinement diagnostic

    rho_RE = R/l_min.

Useful for M5/M6 refinement studies, not a final standalone FEM gate.

### Formula-independent geometric cross-check

    q_RR = 3r/R.

Useful for shape/sliver validation and independent formula testing.

Liu/Joe show radius-ratio and mean-ratio families are equivalent in the asymptotic shape-regularity
sense. Therefore q_RR is **not** an independent release vote; its value is cross-formula validation
and different diagnostic sensitivity.

## 3. Metrics deliberately not primary

### Raw volume / det(A)

Reason:
- scale-dependent,
- necessary for validity/size,
- not enough for shape.

### Edge ratio

Reason:
- can remain acceptable for flat/sliver-like elements,
- telemetry only.

### Minimum dihedral alone

Reason:
- can miss needle-like degeneration.

### Radius-edge alone

Reason:
- can be good for a sliver.

### Vendor composite "element quality"

Reason:
- useful product benchmark,
- not a transparent Dynamics26 mathematical contract.

## 4. Mesh aggregation

Never report only average q_MR.

Minimum research report:

    invalid_count
    min(q_MR)
    p0.1(q_MR)
    p1(q_MR)
    p5(q_MR)
    median(q_MR)
    min(theta)
    max(theta)
    p95(rho_RE)
    max(rho_RE)
    finite_tet_count

Large meshes may use streaming/select/approximate percentile methods later, but report semantics stay
fixed.

Why low-tail emphasis:
one extreme shape can have disproportionate stiffness-conditioning effects.

## 5. Threshold policy

M6 early research intentionally does **not** freeze values such as:

    q_MR >= 0.2
    theta_min >= 10 degrees

as product gates.

Generic thresholds vary by:
- PDE,
- element formulation,
- material,
- solver,
- mesh size/gradation,
- linear versus nonlinear use.

Threshold states should eventually be:

### HARD INVALID
Derived from mathematics:
- inverted/zero-volume,
- non-finite,
- corrupted topology.

### VERIFIED RELEASE LIMIT
Derived from Dynamics26 solver correlation:
- threshold has a benchmark ID,
- supported element/formulation/material domain,
- measured failure/error rationale.

### USER/EXPERIMENT WARNING
Advisory, not a correctness theorem.

## 6. Optimization objective policy

Leading M6 optimizer priority:

1. preserve exact positive validity,
2. preserve CAD boundary/provenance and size constraints,
3. improve the worst/low-tail q_MR,
4. improve aggregate/harmonic quality without sacrificing the worst tail,
5. monitor dihedral/radius metrics,
6. accept a topology/movement operation only if all hard constraints remain valid.

This follows literature evidence that average-only and worst-only objectives have different behavior
and combined strategies can perform better.

## 7. Smoothing acceptance

A node movement is not accepted merely because local q_MR average improves.

Require:
- no inversion,
- no protected CAD/provenance violation,
- no forbidden size-field degradation,
- chosen objective improves according to deterministic comparison,
- boundary node movement respects CAD constraint manifold.

## 8. Connectivity-change acceptance

For flips/cavity reconnection:
- all candidate tetra positive,
- local topology valid,
- boundary/provenance preserved,
- deterministic tie if objective is exactly equal,
- quality objective improves,
- Delaunay property may be preserved or deliberately relaxed only according to the optimizer policy.

M6 quality optimization is allowed to produce a high-quality non-Delaunay tetrahedralization if that
policy is explicitly chosen and validated; ordinary Delaunay is not itself a solver requirement.

## 9. Weighted/regular-Delaunay policy boundary

Finite vertex weights used for quality are a separate versioned M6 algorithm.

They cannot reuse D26LIFT1 semantics.

Suggested conceptual names:

    D26LIFT1
    = infinitesimal symbolic tie policy

    D26REGQ*
    = future finite regular-Delaunay quality policy

The latter is not designed/accepted yet.

## 10. Analysis suitability

Future mesh report should never infer:

    q_MR high => suitable for nearly incompressible rubber.

Instead solver suitability must check:
- element type,
- formulation,
- material compressibility regime,
- verification status,
- nonlinear limits.

This is especially important for Dynamics26's rubber/nonlinear target.

## 11. Anisotropy extension

The initial q_MR baseline measures shape against an equilateral Euclidean target.

Future anisotropic adaptation should generalize to a target metric tensor and evaluate element shape
in metric space.

Do not hard-code "regular in Euclidean space" into the generic mesh-quality architecture.


## 12. Layered quality-vector contract

Do not collapse the quality system to one composite scalar.

Research candidate layers:

1. **Hard validity** — exact orientation/topology/provenance.
2. **Initial isotropic shape** — q_MR primary; q_kappa, dihedral extrema and q_RR diagnostics.
3. **Sizing/refinement/gradation** — rho_RE, target-size compliance, local size ratios.
4. **FEM numerical context** — positive-spectrum local stiffness proxy, global condition estimate,
   interpolation/discretization error, solver iterations, scaling/preconditioner.
5. **Current nonlinear distortion** — J_F, kappa(F), current q_MR/q_kappa/angles.
6. **Formulation suitability** — element/material-specific verified capability.

A UI summary may aggregate categories, but the engineering record must preserve the vector.

## 13. Metric dependency policy

The suite intentionally contains mathematically related quantities because they serve different
diagnostic/oracle roles.

Do not count correlated quantities as independent evidence:

    q_MR <-> q_kappa
    q_MR <-> q_RR shape-regularity equivalence.

Use:
- q_MR for isotropic shape optimization,
- q_kappa for mapping-condition interpretation and consistency checks,
- q_RR for geometric formula cross-check,
- dihedral extrema for angle pathology,
- rho_RE for Delaunay-refinement/spacing behavior.

The exact relation:

    q_kappa(T)^2
      = q_MR(T) q_MR(T^-1)

and its bounds are documented in MULTI_METRIC_SOLVER_AWARE_FRAMEWORK.md.

## 14. Grading and stiffness policy

Shape quality does not encode mesh grading.

A regular-tetra mesh may have perfect element shape metrics while neighboring/global element sizes
vary strongly.

Future reports must keep:
- element shape,
- local size/gradation,
- global stiffness conditioning,
- raw/scaled/preconditioned solver behavior

as separate fields.

No local size-ratio threshold is accepted in this research package yet.

## 15. Positive-spectrum stiffness diagnostic

A free scalar P1 tetra stiffness matrix has a constant null mode; an unconstrained 3D elastic TET4 has
six rigid-body null modes.

Therefore an element "condition number" is meaningful only after the physical null space is removed.

Any future field must name this explicitly, for example:

    kappa_e_positive.

Do not compare it numerically across different:
- materials,
- formulations,
- BC/norm conventions,

without recording that context.


## 16. Extended angle/pathology telemetry

M6 research now distinguishes:

### Dihedral-angle diagnostics

    theta_min
    theta_max

Role:
- sliver/flatness interpretation,
- stiffness/pathology correlation.

### Face-angle diagnostics

    face_angle_min
    face_angle_max

Role:
- independent tetrahedral maximum-angle interpolation condition,
- spike/spear/spade-type morphology research.

### Solid-angle diagnostics

    solid_angle_min
    solid_angle_max

Role:
- cap/spike/needle morphology interpretation,
- independent geometry telemetry.

None of these is currently a universal release gate.

## 17. Spectral-duality diagnostic

Optional research-only:

    q_MR_inv = q_kappa^2/q_MR.

For canonical spectral families:

    diag(1,1,epsilon)
    diag(1,epsilon,epsilon)

q_kappa is identical while q_MR and q_MR_inv swap roles.

This is useful for analytic classification of flat versus needle-like spectral collapse.

Do not expose q_MR_inv as an independent quality vote.

## 18. Pathology labels are explanatory metadata

Names such as:

    sliver
    wedge
    cap
    spire
    needle
    splinter
    spindle
    spear
    spike
    spade

may be used in research fixtures and diagnostics.

They must not become the authority for acceptance.

Production decisions remain based on:
- exact validity,
- quantitative metric vector,
- CAD/sizing constraints,
- solver/formulation evidence.
