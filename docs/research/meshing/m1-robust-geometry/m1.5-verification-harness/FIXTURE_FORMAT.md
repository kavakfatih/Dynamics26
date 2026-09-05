# Bit-Exact Predicate Fixture Format

## 1. Requirement

Fixture serialization must preserve every finite binary64 input exactly across:

```text
Python generator
→ Git/build file
→ C++ test process
```

Ordinary decimal formatting is not the canonical representation.

## 2. Canonical coordinate encoding

Use raw IEEE binary64 bit patterns as 16 hexadecimal digits.

Examples:

```text
3ff0000000000000   # +1.0
0000000000000000   # +0.0
8000000000000000   # -0.0
```

Python:
- packs/unpacks using `struct`.

C++:
- parse as `uint64_t`,
- reconstruct via `std::bit_cast<double>`.

This avoids floating parser differences.

## 3. Text format

Proposed line-oriented format:

Header:

```text
# D26PRED 1
# predicate=orient3d
# encoding=ieee754-binary64-bits-hex
# coordinates_per_case=12
```

Data line:

```text
CASE_ID<TAB>CLASS<TAB>EXPECTED<TAB>SEED<TAB>HEX0<TAB>HEX1...
```

Expected:

```text
-1
0
+1
```

Classes:

```text
canonical
exact-zero
near-degenerate
large-offset
scale
subnormal-boundary
regression
random-structured
```

The exact extension may be `.d26pred`.

## 4. Why not JSON for the core fixture

JSON is readable, but Dynamics26 currently has no need to add a JSON dependency to the low-level meshing test executable.

A strict TSV-like format:
- is trivial to parse,
- has no numeric conversion ambiguity,
- is easy to diff,
- remains language neutral.

A separate manifest may use another human-readable format if needed.

## 5. Signed zero

Raw bits preserve +0 and -0 as distinct encodings.

Predicate mathematics treats both as the same real zero.

Canonical site policy proposal:
- normalize signed zero to +0 before exact-coordinate duplicate grouping.

Reason:
- +0 and -0 compare equal as real coordinates,
- allowing them to become distinct Delaunay sites would create artificial duplicate geometry.

This policy receives explicit tests.

## 6. NaN/infinity

Non-finite patterns belong in **invalid-input tests**, not exact predicate truth fixtures.

Expected outcome:
- typed invalid input / rejection.

Do not assign them a geometric sign.

## 7. Fixture provenance

Committed corpus should contain a sidecar manifest or header metadata:

- fixture schema version,
- oracle generator version,
- source commit,
- deterministic seed set,
- generation timestamp optional,
- SHA-256 of normalized data.

Timestamp must not participate in reproducibility hash.

## 8. Schema evolution

Breaking fixture changes increment:

```text
D26PRED 1
→ D26PRED 2
```

The C++ reader rejects unknown major schema versions explicitly.

## 9. Parser strictness

Reject:
- incorrect field count,
- malformed hex,
- unexpected predicate name,
- duplicate case ID,
- unsupported expected sign,
- extra non-comment tokens.

Test data corruption must fail loudly.
