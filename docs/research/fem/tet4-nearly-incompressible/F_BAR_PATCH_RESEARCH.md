# Patch F-bar Tetrahedral Research

Status: RESEARCHING
Date: 2026-09-05

## 1. Purpose

Patch F-bar is investigated because it offers a low-order linear tetra route for finite-strain
near-incompressibility without adding a global pressure field.

The original de Souza Neto/Pires/Owen work defines near-incompressibility over a patch of simplex
elements rather than enforcing it independently in every linear tetrahedron and derives an exact
Newton tangent.

Dynamics26 will rederive its own equations before implementation.

## 2. TET4 kinematics

For each linear tetra e:

    F_e = constant over the element
    J_e = det(F_e).

Let a predefined patch P contain elements e.

Reference patch volume:

    V_P^0 = sum_e V_e^0.

Current patch volume for affine TET4s:

    V_P^t = sum_e J_e V_e^0.

Define patch volume ratio:

    J_P
      = V_P^t / V_P^0
      = (sum_e V_e^0 J_e) / (sum_e V_e^0).

A natural 3D F-bar map is then:

    Fbar_e
      = (J_P / J_e)^(1/3) F_e.

Hence:

    det(Fbar_e) = J_P.

This preserves the element's isochoric direction/distortion while replacing its local volumetric
ratio with the patch ratio.

## 3. Homogeneous deformation consistency

If all elements in a patch experience the same homogeneous deformation:

    F_e = F*
    J_e = J*

then:

    J_P = J*

and:

    Fbar_e = F*.

So the modification disappears for a homogeneous state.

This must be a golden patch test.

## 4. Objectivity sanity

Under a superposed rigid rotation R:

    F_e' = R F_e
    J_e' = J_e
    J_P' = J_P.

Therefore:

    Fbar_e' = R Fbar_e.

So the kinematic modification is compatible with frame indifference provided the constitutive law
itself is objective.

## 5. The tangent is nonlocal over the patch

This is the key architecture issue.

Because:

    J_P
      = (1/V_P^0) sum_m V_m^0 J_m,

its variation is:

    delta J_P
      = (1/V_P^0) sum_m V_m^0 delta J_m.

Therefore Fbar_e and the residual associated with element e depend on displacement increments in all
elements contributing to patch P.

Consequences:
- a mathematically consistent Newton tangent can couple nodes that are not in element e,
- standard "element owns only its node stencil" assembly may be insufficient,
- patch coupling must be explicit in the assembly graph,
- omitting cross-patch derivative terms produces a quasi/inconsistent tangent and changes Newton
  behavior.

The exact tangent must be verified against finite differences at the patch/global residual level.

## 6. Patch definition is part of the formulation

Open research questions:
- vertex-centered star patch,
- deterministic element-cluster patch,
- boundary/truncated patch behavior,
- material/interface boundaries,
- multi-region patches,
- patch overlap,
- patch ownership and parallel assembly,
- adaptive/remesh stability.

Patch topology cannot cross:
- different material regions unless explicitly formulated,
- contact interfaces,
- bonded interfaces with distinct kinematics/material ownership,
- unsupported CAD/body boundaries.

The first experiment should use simple interior patches only.

## 7. Pressure/result semantics

F-bar does not naturally provide the same independent nodal pressure unknown as a true mixed u-p
formulation.

Any hydrostatic pressure result is:
- constitutively recovered,
- patch/element-derived,
- not a primary DOF.

GUI/result metadata must not label it as an independent pressure solution field.

## 8. Fully incompressible limit

F-bar is primarily researched here as a nearly-incompressible locking-relief technique.

Do not assume it is equivalent to:
- a stable mixed saddle-point discretization,
- exact global incompressibility with a Lagrange multiplier,
- an inf-sup-qualified pressure field.

Those are different mathematical formulations.

## 9. Candidate verification

F-bar research must include:
1. rigid motion,
2. homogeneous deformation,
3. patch constant-J state,
4. simple shear with J=1,
5. uniaxial finite strain,
6. K/G locking sweep,
7. patch topology permutation/determinism,
8. patch-size sensitivity,
9. boundary-patch sensitivity,
10. global exact tangent finite difference,
11. comparison to mixed reference formulation,
12. distorted/sliver tetra interaction.

## 10. Solver architecture implication

F-bar is not automatically cheaper merely because it has no pressure DOFs.

Potential trade:

    fewer global unknown fields
    versus
    wider/nonlocal displacement coupling.

Dynamics26 benchmarks must measure:
- nonzero matrix count,
- assembly cost,
- factorization/iterative cost,
- Newton iterations,
- memory,
- accuracy.

The formulation decision must be based on total solver behavior.
