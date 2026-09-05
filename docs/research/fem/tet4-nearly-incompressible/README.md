# TET4 Nearly-Incompressible / Rubber Formulation Research

Program: Dynamics26 nonlinear FEM core
Status: RESEARCHING / NO FORMULATION SELECTED
Date: 2026-09-05

## Engineering question

Dynamics26 is developing an original tetrahedral mesher, but:

    good TET4 geometry
    !=
    stable nearly-incompressible TET4 mechanics.

Before a TET4 mesh can be called suitable for rubber-like materials, the element formulation must
have its own mathematical and numerical qualification.

## Current repository boundary

Dynamics26 V0.10 currently documents and verifies a mixed u-p HEX8/Q1-P0 baseline.

That is valuable solver/multi-field infrastructure evidence, but it is not evidence that:

    TET4 P1-P0 is stable.

Modern nearly-incompressible literature explicitly reports locking/inaccurate stress behavior for
simple unstable low-order pairs such as P1-P0 and states that mixed displacement-pressure spaces must
satisfy an LBB/inf-sup stability requirement or use a consistent stabilization.

Therefore no TET4 formulation is inherited automatically from HEX8.

## Candidate formulation families

Research candidates:

1. displacement-only P1 TET4 — negative-control/compressible baseline,
2. mixed MINI: P1+bubble displacement / continuous P1 pressure,
3. pressure-projection stabilized P1-P1,
4. patch F-bar linear tetrahedron,
5. later higher-order hybrid/mixed TET10.

No winner is selected yet.

## Current leading research roles

### Mathematical stable-reference candidate

MINI tetra:
- classical stable mixed pairing in the almost-incompressible linear setting,
- one element-interior scalar bubble basis enriched in all three displacement components,
- continuous nodal P1 pressure,
- local bubble increments can be statically condensed.

### Practical low-order mixed candidate

Projection-stabilized P1-P1:
- linear nodal displacement,
- linear nodal pressure,
- stabilization repairs equal-order instability,
- no element-interior bubble state,
- requires a carefully derived/versioned stabilization operator.

### Independent displacement-only candidate

Patch F-bar:
- no global pressure DOF,
- relaxes elementwise volume constraint over a patch,
- attractive C/material interface continuity,
- introduces patch/nonlocal coupling and requires exact consistent tangent.

### Higher-order later path

Hybrid/mixed TET10:
- strong literature/commercial motivation for rubber accuracy and large deformation,
- belongs after TET4 infrastructure and curved/high-order mesh support mature.

## Product-language rule

Until formulation-specific verification closes:

    TET4 mesh exists

must never imply:

    TET4 is rubber-ready.

Compressible TET4 support and nearly-incompressible TET4 support are separate capabilities.
