# M1.5 Commercial CAE Verification Benchmark

## 1. Scope

ANSYS, COMSOL and Marc are compared here for **observable mesh verification behavior**.

Their internal test suites and predicate implementations are proprietary/not publicly specified.

Dynamics26 therefore benchmarks:
- what metrics are exposed,
- what warnings/errors are surfaced,
- whether distributions/worst elements are inspectable,
- how mesh quality connects to solver/application validity.

## 2. ANSYS 2026 R1

Public ANSYS mesh evaluation exposes multiple metrics including:
- Element Quality,
- Aspect Ratio,
- Jacobian Ratio,
- Warping Factor,
- Parallel Deviation,
- Maximum Corner Angle,
- Skewness,
- Orthogonal Quality.

ANSYS also exposes:
- average,
- worst metric,
- warning/error limits,
- failed element counts/percentages in relevant quality workflows.

Structural defaults can monitor metrics such as Jacobian ratio, with configurable limits; contact documentation provides stricter practical quality guidance for contact regions.

### Dynamics26 lesson

Do not reduce mesh verification to one scalar.

A future `MeshQualityReport` should contain:
- metric name,
- definition/version,
- min/max/mean as appropriate,
- percentiles/distribution,
- warning count,
- hard-invalid count,
- location/element IDs of worst elements.

Hard topological invalidity and soft engineering-quality warnings remain separate.

## 3. COMSOL 6.4

COMSOL exposes particularly rich programmatic mesh statistics:

- element counts/types,
- minimum quality,
- mean quality,
- quality distribution/histogram,
- min/max/total volume,
- maximum/mean growth rate,
- status flags such as `hasproblems` and `iscomplete`.

Available quality measures include:
- skewness,
- maximum angle,
- volume vs circumradius,
- volume vs length,
- condition number,
- growth rate,
- curved skewness for higher-order contexts.

COMSOL's Free Tetrahedral operation also exposes quality-optimization effort; documented Basic/Medium levels aim at different minimum-quality targets while keeping the surface mesh fixed.

### Dynamics26 lesson

Verification should expose both:
- aggregate statistics,
- spatial identity of bad elements.

It should also keep:
```text
mesh generated successfully
!=
mesh quality acceptable for a given analysis
```

## 4. Marc / Mentat 2026.1

Current Marc public product material confirms continuing Mentat meshing improvements.

Public Hexagon support guidance for difficult nonlinear meshes emphasizes:
- local refinement in high-gradient regions,
- local adaptive remeshing,
- large-strain settings,
- solution sensitivity to mesh density/distortion.

Marc also distinguishes Global Remeshing and Local Adaptivity.

Detailed current public metric formulas comparable to ANSYS/COMSOL were not established in this research pass, so no unsupported formula/threshold is attributed to Marc.

### Dynamics26 lesson

Mesh verification eventually needs two levels:

### Geometric mesh quality
- shape,
- Jacobian/volume,
- sizing,
- gradation.

### Analysis adequacy
- mesh convergence,
- contact-region adequacy,
- distortion during nonlinear solve,
- remeshing/adaptation triggers.

A preprocessor quality score cannot prove solution convergence.

## 5. Cross-product benchmark

| Capability | ANSYS | COMSOL | Marc/Mentat | Dynamics26 target |
|---|---|---|---|---|
| Multiple quality metrics | Strong | Strong | public workflow evidence, exact formulas not used here | Required |
| Worst element inspection | Yes | Yes | Mentat inspection workflows | Required |
| Quality distribution/statistics | Yes | Yes | not used as current public spec | Required |
| Warning vs invalid distinction | Yes | Yes | diagnostic workflows | Typed |
| Quality optimization | Yes | Yes | remesh/adaptivity | M6/M9 |
| Solver-sensitive mesh guidance | Yes | Yes | Strong nonlinear guidance | Qualification stage |
| Internal predicate test method public | No | No | No | Independent M1 harness |

## 6. Dynamics26 report concept

Future immutable research/product report:

```text
MeshVerificationReport
- topologyStatus
- provenanceStatus
- elementCountByType
- qualityMetrics[]
- sizeFieldStatistics
- boundaryRecoveryStatus
- deterministicFingerprint
- warnings[]
- errors[]
```

This is not yet a production API.

## 7. Critical rule

Commercial threshold values are **benchmarks**, not automatic Dynamics26 truth.

Release thresholds must come from:
- element formulation qualification,
- solver sensitivity,
- mesh convergence studies,
- nonlinear/rubber/contact benchmarks.

For example, a generic tetra quality threshold cannot substitute for TET4 locking or large-deformation qualification.
