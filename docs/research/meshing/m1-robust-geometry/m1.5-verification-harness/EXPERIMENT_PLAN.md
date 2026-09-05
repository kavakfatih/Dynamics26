# M1.5 Verification Harness Experiment Plan

## EXP-MESH-0150 — Oracle A/B executable agreement

Implement test-only Python reference paths.

Acceptance:
- exact sign agreement for all generated cases,
- generator aborts on any disagreement,
- seed/case replay is deterministic.

## EXP-MESH-0151 — Bit-exact round trip

Pipeline:
```text
Python binary64
→ raw uint64 hex fixture
→ C++ parse
→ std::bit_cast<double>
```

Acceptance:
- every reconstructed bit pattern equals source,
- +0/-0 preserved in serialization,
- site canonicalization test applies the explicit signed-zero policy.

## EXP-MESH-0152 — Golden predicate reader

Feed committed canonical/adversarial fixtures to a placeholder/reference C++ reader.

Acceptance:
- strict schema parsing,
- corrupt input rejection,
- stable case IDs,
- exact expected sign loaded correctly.

## EXP-MESH-0153 — Generated corpus reproducibility

Generate the same corpus twice in separate directories.

Acceptance:
- normalized fixture SHA-256 identical,
- summary counts identical.

## EXP-MESH-0154 — Failure replay

Inject a deliberate wrong sign in a research test.

Acceptance:
- test emits one-case replay record,
- replay record can be passed directly to the C++ test runner,
- seed/generator/case index is sufficient to regenerate original.

## EXP-MESH-0155 — Test-tier timing

Measure T0/T1/T2 on hosted/target macOS.

Goal:
- keep every-push golden corpus fast,
- choose deterministic generated-corpus size from measured runtime, not guesswork.

## EXP-MESH-0156 — Topology corruption corpus

Hand-author:
- single tetra,
- two tetra sharing face,
- small tetra ball.

Inject:
- stale neighbor,
- wrong reciprocal face,
- duplicate vertex,
- negative orientation,
- duplicate tetra,
- non-manifold face.

Acceptance:
- validator identifies exact corruption class.

## EXP-MESH-0157 — Walk vs brute-force reference

Once point-location prototype exists:
- query exact cell/facet/edge/vertex/outside cases,
- compare walk locator to full scan.

Acceptance:
- identical typed result,
- zero infinite walks,
- replay on mismatch.

## EXP-MESH-0158 — Commercial mesh-report comparison

When tool access is available, run the same simple tetra geometry in:
- ANSYS,
- COMSOL,
- Marc/Mentat.

Record:
- element count,
- exposed quality metrics,
- min/mean/worst/distribution when available,
- warning/error behavior,
- optimization/repair controls.

Purpose:
- design Dynamics26 reporting UX,
- not numerical mesh parity.

## EXP-MESH-0159 — Mesh convergence qualification template

Prepare a generic later-stage experiment template:
- mesh sequence,
- DOF/element count,
- target response,
- relative response change,
- quality distribution,
- solver convergence.

This becomes important when TET4/TET10 enters solver qualification.
