# TET4 Nearly-Incompressible Formulation Candidates

Status: RESEARCHING / NO WINNER
Date: 2026-09-05

## 1. Comparison axes

Each formulation is evaluated on:
- compressible and nearly/full incompressible consistency,
- locking resistance,
- pressure/stress quality,
- finite-strain objectivity,
- exact consistent tangent,
- global matrix properties,
- local state complexity,
- sparse assembly stencil,
- nonlinear rollback,
- solver/preconditioner demand,
- mesh distortion sensitivity,
- contact compatibility,
- computational cost,
- implementation auditability.

## 2. Candidate A — pure displacement P1 TET4

Fields:

    u: P1
    p: none.

For an affine TET4 with linear displacement:
- F is constant in the element,
- strain is constant for small-strain linear elasticity,
- one element cannot represent bending/strain gradients well.

With a large bulk modulus, pointwise/elementwise displacement control of J can cause severe volumetric
locking.

Research role:
- compressible baseline,
- constant-strain patch tests,
- negative-control nearly-incompressible benchmark,
- not a default rubber product candidate.

Public Abaqus guidance independently warns that ordinary linear tetrahedra generally require very
fine meshes for accurate continuum results. This is a product sanity check, not a Dynamics26 theorem.

## 3. Candidate B — MINI P1+bubble / P1

Fields:

    u: continuous nodal P1 + 3 local bubble displacement components per element
    p: continuous nodal P1.

Strengths:
- mathematically strong stable-pair lineage for tetrahedral almost-incompressible elasticity,
- allows exact incompressibility in mixed form,
- pressure is continuous/nodal,
- bubble increments can be statically condensed,
- explicit numerical inf-sup qualification is possible.

Costs/risks:
- global nodal pressure DOFs,
- indefinite block system,
- local nonlinear bubble state and condensation,
- additional quadrature/residual/tangent terms,
- follower/contact loads interacting with condensed internal modes need explicit derivation,
- block preconditioning is required at scale.

Leading role:
- Dynamics26 **mathematical mixed reference candidate**.

This is not yet an implementation selection.

## 4. Candidate C — pressure-projection stabilized P1-P1

Fields:

    u: continuous nodal P1
    p: continuous nodal P1.

Strengths:
- no bubble local kinematic state,
- simple nodal field model,
- published tetra/hexa finite-strain nearly/full incompressibility benchmarks,
- pressure/stress fields can be smoother than unstable low-order baselines.

Costs/risks:
- stabilization operator is part of the formulation and must be derived/tested,
- global pressure DOFs remain,
- matrix remains block/saddle-like,
- stability proof/verification differs from a naturally inf-sup-stable pair,
- stabilization scaling must respect units, mesh scale and constitutive convention.

Leading role:
- Dynamics26 **practical low-order mixed candidate**.

## 5. Candidate D — patch F-bar linear tetra

Fields:

    u: P1
    p: no global pressure field.

Concept:
replace element volumetric deformation by a patch-level volume change while retaining the element's
isochoric deformation.

Advantages:
- displacement-only global unknown structure,
- standard strain-driven constitutive interface can be retained,
- published linear tetra finite-strain near-incompressibility evidence,
- avoids a global pressure saddle-point solve.

Costs/risks:
- patch definition is an algorithmic object,
- residual of one element can depend on neighboring patch kinematics,
- consistent tangent acquires patch coupling,
- assembly stencil can widen,
- overlapping-patch ownership/determinism requires care,
- pressure is not an independent solution field,
- fully incompressible limit/stability semantics differ from true mixed methods,
- contact/boundary patch behavior requires dedicated study.

Leading role:
- independent **displacement-only engineering alternative**.

See F_BAR_PATCH_RESEARCH.md.

## 6. Candidate E — higher-order hybrid/mixed tetra

Later TET10 path.

Schönherr et al. derive quadratic tetrahedral hybrid/mixed elements from a three-field framework for
rubber-like materials and severe compression.

Public commercial guidance also tends to favor quadratic tetrahedra over ordinary linear tetrahedra
for general accuracy.

Potential strengths:
- higher interpolation accuracy,
- better bending/gradient representation,
- mature hybrid/mixed incompressibility path.

Costs:
- M8 high-order curved geometry/mesh ordering required,
- more DOFs and integration cost,
- contact behavior must be separately qualified,
- more complicated local state/tangent.

Leading role:
- likely important long-term **rubber accuracy reference/product candidate**, not an M2/M6 prerequisite.

## 7. Commercial sanity benchmarks

### ANSYS

Public SOLID285 documentation demonstrates that a 4-node tetra can be offered with:
- linear displacement,
- nodal hydrostatic pressure,
- pressure stabilization,
- large-strain/hyperelastic capability.

Dynamics26 does not infer or copy its proprietary stabilization.

The observation only confirms that a stabilized low-order mixed tetra is a viable product class.

### Abaqus

Public documentation states:
- pure displacement is inadequate for incompressible behavior,
- hybrid elements add pressure unknowns,
- ordinary linear tetrahedra have limited general accuracy,
- quadratic tetrahedral variants are generally preferred.

Again this is a capability/engineering sanity benchmark only.

## 8. Current research ranking

Not a product decision:

| Role | Candidate |
|---|---|
| negative control | pure P1 displacement TET4 |
| unstable mixed negative control | P1/P0 |
| stable mixed mathematical reference | MINI P1+bubble/P1 |
| practical mixed candidate | stabilized P1/P1 |
| displacement-only alternative | patch F-bar |
| later high-order accuracy path | hybrid/mixed TET10 |

The correct Dynamics26 choice should come from the verification matrix, not implementation convenience.
