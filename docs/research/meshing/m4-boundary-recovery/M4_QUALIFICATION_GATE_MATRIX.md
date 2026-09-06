# M4 Qualification Gate Matrix

Status: **CONTRACT FROZEN / EXECUTABLE EVIDENCE PENDING**  
Applies to: **D26-M4-FREEZE-1**

| Gate | Frozen requirement | Minimum evidence | Current state |
|---|---|---|---|
| M4-G01 | valid/watertight surface input | reject open/non-manifold/inconsistent PLC | DEFINED / PENDING |
| M4-G02 | segment recovery | every source segment represented by recovered edges | DEFINED / PENDING |
| M4-G03 | segment subcomplex coverage | exact ordered child chain equals source segment | DEFINED / PENDING |
| M4-G04 | facet recovery | every source facet represented by recovered child TRI3 | DEFINED / PENDING |
| M4-G05 | facet subcomplex coverage | coverage proof shows no gaps, overlaps or leakage | DEFINED / PENDING |
| M4-G06 | CAD provenance preservation | recovered children inherit source GeometryEntityId | DEFINED / PENDING |
| M4-G07 | exact constructed-point semantics | SegmentLNC construction is authoritative | DEFINED / PENDING |
| M4-G08 | constructed-site symbolic determinism | D26SITE2/D26LIFT2 stable across allocation/thread/order variation | DEFINED / PENDING |
| M4-G09 | exact predicate agreement for constructed sites | production signs agree with independent exact SegmentLNC oracle | DEFINED / PENDING |
| M4-G10 | no rounded-coordinate topology authority | adversarial realization cannot change accepted topology silently | DEFINED / PENDING |
| M4-G11 | cavity recovery | local/ordinary cavity strategy qualifies declared corpus | DEFINED / PENDING |
| M4-G12 | robust fallback recovery | bounded fallback succeeds or fails explicitly | DEFINED / PENDING |
| M4-G13 | outside classification | ghost/unbounded traversal labels simple solids correctly | DEFINED / PENDING |
| M4-G14 | cavity/hollow-body classification | nested shells/cavities/multiple shells classify correctly | DEFINED / PENDING |
| M4-G15 | binary64 realization revalidation | exact topology and coverage revalidated after realization | DEFINED / PENDING |
| M4-G16 | exact-positive output tetra | every accepted output tetra has positive authoritative orientation | DEFINED / PENDING |
| M4-G17 | explicit unsupported/failure states | invalid shell/recovery/construction failures are typed | DEFINED / PENDING |
| M4-G18 | deterministic fingerprint/replay | topology/provenance fingerprint stable under replay | DEFINED / PENDING |
| M4-G19 | count/resource termination | bounded recovery terminates with truthful resource status | DEFINED / PENDING |

## Release rule

M4 is not qualified until all mandatory M4-G01..M4-G19 gates have exact-head executable evidence. Resource termination is never geometric success.
