# M5 Design Freeze — Isotropic Size Field and Refinement

Status: **RESEARCH COMPLETE / DESIGN FROZEN / IMPLEMENTATION NOT STARTED**  
Freeze ID: **D26-M5-FREEZE-1**  
Research package: **M5 — Size Fields / Refinement**  
Date: 2026-09-06

## 1. Scope

The first M5 scope is an isotropic scalar physical size field. An anisotropic tensor metric is deferred to M9 adaptation/remeshing research.

M3's first-fundamental-form mapping is chart guidance for a scalar physical target and does not promote M5 to anisotropic sizing.

## 2. Field composition and active criteria

A criterion that is inactive at physical location `x` contributes no finite restriction:

```text
inactive criterion at x = +infinity

h_req(x)
= min over criteria active at x
```

Equivalently, the familiar form

```text
h_req(x) =
min(
  h_global,
  h_scoped,
  h_curvature,
  h_proximity,
  ...
)
```

is interpreted with every inactive term equal to `+infinity`. Overlapping active criteria compose
by mathematical minimum, independent of rule enumeration order.

Normal product usage requires `h_global` to be a finite, strictly positive fallback criterion so
every declared meshing-domain point has a finite requested size unless a typed invalid/unsupported
state is returned.

Named Selection sizing rules resolve to persistent `GeometryEntityId` scopes before the
mathematical meshing kernel starts. The kernel does not interpret UI selection objects.

## 3. Frozen gradation semantics

For `k >= 0`, let `D` be the declared **physical meshing domain**. For `x,y in D`:

```text
h(x) = inf_y [ h_req(y) + k ||x-y||_3D ]
```

where `||x-y||_3D` is physical 3D Euclidean distance. This is the greatest `k`-Lipschitz
minorant of `h_req` on the active physical domain; UV/chart distance is not the authority.

First-scope disconnected-body policy is explicit: `D` is evaluated per connected physical
Body/Region component, and gradation does **not** propagate from one disconnected component to
another. Any future cross-body coupling is a separately versioned policy and must enter the settings
fingerprint. Silent global propagation across disconnected bodies is forbidden.

The implementation algorithm may evolve; the mathematical meaning may not silently evolve. If the
GUI exposes a growth ratio or another control instead of k, its mapping to k is versioned and
included in the settings fingerprint together with the disconnected-component propagation policy.

## 4. Minimum size is a limitation control

If required geometric size is below configured `h_min`, Dynamics26 must not silently clamp and report criterion success.

A minimum-size protection may stop refinement, but the outcome must expose a typed state such as `MinimumSizeLimited` and identify the unsatisfied criteria/features.

`h_min` protects resources; it is not proof of geometric correctness.

## 5. Curvature predictors

For local curvature radius R and allowed sagitta delta:

```text
h_delta = 2 sqrt(2 R delta - delta^2)
```

For maximum normal change theta:

```text
h_theta = 2 R sin(theta/2)
```

Candidate:

```text
h_curvature = min(h_delta, h_theta)
```

Validity domains for the first minor-arc predictors are explicit:

```text
R > 0
0 <= delta <= R
0 <= theta <= pi
```

These are predictors only. Final authority is actual 3D CAD-vs-mesh deviation and normal error.
Outside these domains the kernel returns a typed failure, or a documented criterion-disabled state
only when policy explicitly declares that criterion inactive. NaN propagation is forbidden.

## 6. Proximity and thickness

Nearest-geometry search excludes self/owned or incident topology where appropriate. The same Face, its own Edge or an incident feature may not create artificial near-zero proximity and uncontrolled refinement.

Reference candidate:

```text
h_proximity = gap / N
```

where N is a versioned policy/configuration value and the gap is to meaningful nonincident/non-owned geometry.

## 7. Determinism and fingerprinting

Minimum composition is commutative. Remaining violations are processed by a deterministic total order based on versioned criterion class, persistent geometry identity and canonical site/spatial identity.

Topology/termination-affecting settings are fingerprinted, including global/scoped sizes, CAD-error controls, proximity policy, gradation mapping, minimum size and resource/element budgets.

## 8. Typed outcomes

At minimum:
- `CriteriaSatisfied`,
- `MinimumSizeLimited`,
- `ConstraintUnsatisfied`,
- `ElementBudgetExceeded`,
- `ResourceFailure`,
- `UnsupportedGeometry`.

Budget/resource termination is neither geometric failure of another kind nor successful criterion satisfaction.

## 9. Refinement and provenance

Refinement preserves source Body/Face/Edge/Vertex provenance for constrained entities. New interior nodes/elements receive region provenance through explicit construction rules, not coordinate matching.

Uncontrolled point growth is bounded before unbounded work and terminates with a truthful typed status.

## 10. Qualification boundary

D26-M5-FREEZE-1 freezes scalar size-field semantics and qualification gates. It does not claim sizing/refinement implementation or arbitrary-CAD mesh readiness.

Qualification authority: `M5_QUALIFICATION_GATE_MATRIX.md`.
