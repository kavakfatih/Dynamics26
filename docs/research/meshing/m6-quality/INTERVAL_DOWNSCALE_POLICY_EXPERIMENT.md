# M6 Research — D26INT1-E1 Subnormal Downscale Policy Experiment

Status: MEASURED / policy decision open
Date: 2026-09-06

Harness: `tests/meshing/m6_quality/experiment_m6_interval_downscale.cpp`
(ctest `experiment_m6_interval_downscale`, label `experiment`)

## 1. Engineering question

`scalePowerOfTwo` refuses to scale an interval whose endpoint would underflow or
land in the subnormal range, and reports `Range` so D26QMRF1 drops to D26QMRB1.
The policy is frozen and asserted by `test_m6_interval_backend`:

    scalePowerOfTwo({minNormal, minNormal}, -1)
      ->
    Range   // "subnormal downscale must conservatively return range uncertainty"

Should that policy stay, or should the primitive take an outward step like every
other D26INT1 primitive already does?

## 2. Why the question came up

D26QMRF1 was wired into the D26QV1 comparison path. Once it was actually being
called, its certificate rate on Dynamics26's own mesh output measured zero.

The mechanism is reachable and ordinary, not pathological:

    subtract(0, 0)          -> [-denorm_min, +denorm_min]   // outward step
    scalePowerOfTwo(_, -1)  -> Range                        // both ends underflow

A relative coordinate is exactly zero whenever the anchor vertex shares an axis
value with another vertex of the cell. That is an axis-aligned edge, or a face
lying on a coordinate plane. The normalization step then has to scale that
interval down, both endpoints underflow, and the cell loses its certificate.

Today's product meshing path is `StructuredHexMesher` over parametric box or
box-compatible CAD, so this is not an edge case in this repository; it is the
normal case.

## 3. Proposed policy

`scalbn` is exact while the result stays normal, so a normal result needs no
step at all. Otherwise:

- result subnormal: `scalbn` is correctly rounded, so one outward step covers
  the rounding;
- result underflowed to zero from a non-zero input: the true magnitude is below
  `denorm_min`, so one outward step still encloses it.

Widening an enclosure is always sound. The current policy discards the enclosure
rather than widening it, which is conservative in the sense of never being
wrong, but not in the sense of being free.

This makes `scalePowerOfTwo` consistent with the rest of D26INT1 instead of an
exception to it.

## 4. Measurement

4000 cells per population. `current` and `proposed` are certificate rates for a
single cell; `certified` counts consecutive pairs the proposed policy could
order; `checked` counts those verified against D26QMRB1.

| population | current | proposed | certified | agreed |
| --- | --- | --- | --- | --- |
| random coordinates | 100.0% | 100.0% | 3999 | 3999 |
| structured lattice corner | **0.0%** | **100.0%** | 3874 | 3874 |
| three vertices on a plane | **0.0%** | **100.0%** | 3999 | 3999 |
| one shared axis value | 11.6% | 100.0% | 3999 | 3999 |
| graded anisotropic | 100.0% | 100.0% | 3999 | 3999 |

Total: 19870 certified orders, 19870 agreeing with D26QMRB1, 0 disagreements.

Reading the table:

- The current policy is not weak in general. On coordinate populations with no
  exact coincidences it already certifies everything, including the graded
  anisotropic family.
- It collapses precisely on exact coordinate coincidence, which is what
  structured meshing produces by construction.
- "one shared axis value" is the mildest form: a single shared coordinate
  between two vertices already drops the rate to 11.6%.
- The lattice population does not reach 3999 certified pairs because some
  neighbouring cells are exactly equal in quality. An exact tie correctly yields
  no certificate; there is no fast Equal.

## 5. What the experiment does and does not establish

Establishes:

- the coverage difference is real, large, and concentrated on this repository's
  own mesh output;
- across 19870 certified orders the proposed policy never disagreed with the
  exact backend.

Does not establish:

- a soundness proof. The argument in section 3 is the proof obligation; the
  measurement is corroboration, not a substitute.
- behaviour under the full M6-R75..R108 gate, which asserts the current policy
  directly and would need updating together with any change.

## 6. Decision required

Adopting the proposal is a change to a frozen contract. It requires:

1. replacing the rejection in `scalePowerOfTwo` with the outward step,
2. updating the `Range` assertion in `test_m6_interval_backend`,
3. re-running M6-R75..R108 with the subnormal probes rewritten around the new
   policy,
4. recording the coverage change in the qualification evidence.

Leaving the policy frozen is coherent too, but then D26QMRF1 should be
documented as effective only on meshes without exact coordinate coincidence,
because on structured output it is currently pure overhead: it always runs, it
never certifies, and D26QMRB1 does all the work anyway.

Production code is unchanged pending this decision.
