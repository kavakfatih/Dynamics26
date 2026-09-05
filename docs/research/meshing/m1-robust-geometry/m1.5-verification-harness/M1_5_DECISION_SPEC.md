# M1.5 Proposed Decision Specification

**Status:** research proposal

## D1 — Existing CTest infrastructure

Candidate ACCEPT:
- use current CMake/CTest architecture,
- extend `add_femcae_meshing_test`,
- do not introduce a new C++ unit-test framework for M1/M2.

## D2 — Python standard-library oracle

Candidate ACCEPT:
- test-only Python oracle,
- no production Python dependency,
- no third-party arbitrary precision package required initially.

## D3 — Raw binary64 fixture encoding

Candidate ACCEPT:
- 16-hex-digit uint64 pattern is canonical coordinate serialization,
- C++ reconstructs via bit-cast,
- ordinary decimal text is not fixture truth.

## D4 — Two oracle paths

Candidate ACCEPT:
- Fraction path,
- dyadic-integer path,
- fixture generated only when both signs agree.

## D5 — Committed + generated corpus

Candidate ACCEPT:
- small permanent golden/regression corpus in Git,
- larger deterministic seeded corpus generated in build/test space.

## D6 — Every failure replayable

Candidate ACCEPT:
- seed,
- generator version,
- case index,
- raw bits,
- expected/actual
are emitted for a mismatch.

## D7 — Test tiers

Candidate ACCEPT:
- fast golden corpus every relevant CI push,
- larger deterministic corpus in meshing verification gates,
- deep research stress separated from routine GUI-only changes,
- Release performance separate from correctness.

## D8 — Commercial mesh quality is benchmark, not oracle

Candidate ACCEPT:
- ANSYS/COMSOL/Marc reporting behavior informs Dynamics26 quality/report UX,
- commercial threshold values do not define predicate correctness or universal solver qualification.

## D9 — Multi-metric mesh verification

Candidate ACCEPT for later product report:
- topology validity,
- geometric quality,
- sizing conformity,
- provenance,
- determinism,
- analysis qualification are separate axes.

## D10 — No M2 optimization without independent reference

Candidate ACCEPT:
- walk compared to brute-force,
- fast predicate compared to exact oracle,
- optimized adjacency compared to topology validator,
- spatial ordering compared under same canonical topology checks.

## Remaining implementation decisions

1. exact on-disk header details,
2. CTest fixture setup vs build-time generated corpus,
3. generator script/module names,
4. committed corpus size,
5. canonical topology digest format.
