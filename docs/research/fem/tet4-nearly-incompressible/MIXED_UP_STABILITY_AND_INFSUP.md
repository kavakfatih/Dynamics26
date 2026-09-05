# Mixed u-p Stability and the Numerical Inf-Sup Contract

Status: RESEARCHING
Date: 2026-09-05

## 1. Continuum mixed functional

Use the existing Dynamics26 V0.10 sign convention as the baseline.

For the simple volumetric law and isochoric hyperelastic energy:

    Psi_hat(u,p)
      = W_iso(F_bar)
        + p (J-1)
        - p^2/(2 K).

Here:
- F = I + Grad_0(u),
- J = det(F),
- p uses the existing Dynamics26 pressure sign convention,
- K is bulk modulus.

Pressure stationarity gives:

    J - 1 - p/K = 0

so for finite K:

    p = K (J-1).

Eliminating p recovers:

    W_vol = 1/2 K (J-1)^2.

In the formal K -> infinity limit, p becomes a Lagrange multiplier for:

    J = 1.

This is a saddle-point problem.

## 2. Linearized block system

After Newton linearization, the discrete system has the generic form:

    [ K_uu   K_up ] [Delta u] = -[R_u]
    [ K_pu   K_pp ] [Delta p]   [R_p].

For exact incompressibility:

    K_pp -> 0

apart from any explicit stabilization contribution.

Consequences:
- the global matrix is generally symmetric-indefinite when derived from a symmetric potential,
- an SPD-only linear solver is mathematically inappropriate,
- pressure null spaces and constraint handling must be explicit,
- block preconditioning becomes a first-class performance problem.

## 3. Discrete inf-sup condition

For displacement space V_h and pressure space Q_h, define the volumetric coupling bilinear form
b(v_h,q_h).

The discrete stability requirement is of the form:

    beta_h
      = inf_{q_h in Q_h*}
        sup_{v_h in V_h}
          b(v_h,q_h)
          / ( ||v_h||_V ||q_h||_Q )

with:

    beta_h >= beta_0 > 0

independently of mesh size h.

Q_h* removes the physically expected pressure null space where required.

If beta_h tends to zero under refinement, the pair is not uniformly stable.

## 4. Matrix form of the numerical inf-sup test

Let:
- B = assembled displacement-pressure coupling matrix,
- S = SPD matrix representing the selected displacement norm,
- M_p = SPD pressure mass/norm matrix after physical null-space treatment.

The Rayleigh quotient gives the generalized eigenproblem:

    B S^{-1} B^T q = lambda M_p q.

For admissible modes:

    beta_h = sqrt(lambda_min^+).

Interpretation over a refinement sequence:
- beta_h bounded away from zero -> evidence the pair passes the numerical test,
- beta_h -> 0 -> instability/locking risk,
- extra zero modes beyond the physical pressure null mode -> spurious pressure modes.

This follows the Chapelle-Bathe numerical inf-sup methodology.

The exact boundary conditions, norms and physical null-space filter are part of the test definition and
must be frozen before qualification; comparing eigenvalues from different norm definitions is invalid.

## 5. Pressure null space

For a fully incompressible problem with displacement boundary conditions that do not determine an
absolute hydrostatic pressure, pressure may be defined only up to a constant.

Then either:
- enforce zero-mean pressure,
- pin one mathematically appropriate pressure gauge,
- or project the constant mode out of the inf-sup eigenproblem.

Do not label the expected constant null mode a spurious pressure mode.

For nearly incompressible finite K, the -p^2/(2K) term regularizes pressure but the approximation pair
can still lock or produce poor pressure/stress behavior as K/G becomes large.

## 6. Why HEX8 Q1/P0 evidence cannot be copied to TET4

The current Dynamics26 V0.10 Q1/P0 HEX8 verification demonstrates:
- multi-field DOF mechanics,
- block residual/tangent architecture,
- a controlled locking comparison for that element path.

It does not prove a mesh-family inf-sup lower bound.

For tetrahedra, published large-strain studies explicitly identify simple P1-P0 as an unstable/locking
low-order pair in demanding nearly-incompressible applications.

Therefore:

    TET4 P1/P0

is a negative-control/reference candidate, not the default rubber formulation.

## 7. MINI tetra spaces

On the reference tetrahedron:

    V_hat = [ P1 + span{psi_B} ]^3
    Q_hat = P1

with one interior scalar bubble basis:

    psi_B = 256 xi eta zeta (1-xi-eta-zeta).

The bubble vanishes on every tetra face.

Therefore each element has:
- ordinary nodal displacement DOFs,
- three element-interior bubble displacement DOFs,
- ordinary nodal pressure DOFs.

Classical results support stability of this MINI pairing for almost-incompressible linear elasticity
on tetrahedral meshes.

## 8. Nonlinear MINI static condensation

The interior bubble has element-local support, so its Newton increment can be condensed.

Partition the linearized element unknowns:

    x = global-compatible nodal u/p increments
    b = local bubble displacement increments.

Element system:

    [K_xx K_xb] [Delta x] = -[R_x]
    [K_bx K_bb] [Delta b]   [R_b].

If K_bb is nonsingular:

    Delta b
      = -K_bb^{-1}(R_b + K_bx Delta x)

and the condensed global contribution is:

    K_cond = K_xx - K_xb K_bb^{-1} K_bx

    R_cond = R_x - K_xb K_bb^{-1} R_b.

Research requirement:
in finite strain the bubble is a real kinematic local unknown, not merely a matrix trick.

The element must retain/update its local bubble state consistently through:
- Newton iterations,
- load-step commit,
- cutback/revert,
- restart if later supported.

A code path that condenses K but forgets the nonlinear local bubble state is not acceptable.

## 9. Stabilized P1-P1

Equal-order nodal linear displacement and pressure are attractive but the raw pair is not accepted
without stabilization.

Karabelas et al. use a local pressure-projection concept:

    s_h(p,q)
      = sum_K integral_K
          alpha_K
          (p - Pi_K p)
          (q - Pi_K q) dV

where Pi_K is the elementwise constant pressure projection and alpha_K is the stabilization scaling
under the chosen formulation.

Dynamics26 should adopt the mathematical concept only after an independent dimensional/scaling
derivation.

Do not copy one application's fitted/scaled coefficient and call it universal.

The stabilized method changes the pressure block:

    K_pp <- K_pp + K_stab

with sign according to the committed residual convention.

## 10. Important test distinction

### MINI

The actual mixed pair should pass a mesh-refinement numerical inf-sup test.

### Unstabilized P1-P1

Expected to fail the raw pair stability test and serves as a negative control.

### Stabilized P1-P1

The raw spaces remain equal-order; classic pair-only beta_h is not the whole stability proof because
the formulation intentionally adds stabilization.

Qualification needs:
- consistency of stabilization,
- patch/homogeneous-state behavior,
- pressure-mode suppression,
- locking convergence,
- stress convergence,
- nonlinear tangent verification.

### F-bar

No global u-p pair exists, so the mixed inf-sup eigenproblem is not its qualification gate.

F-bar instead needs patch-volume consistency, locking benchmarks and exact nonlocal tangent checks.

## 11. Inf-sup test mesh sequence candidate

Use a deterministic unit-cube tetrahedral sequence independent of the unstructured mesher.

For refinement levels h_0, h_1, ...:
- preserve domain and boundary-condition pattern,
- build S, B, M_p consistently,
- remove rigid/gauge modes explicitly,
- solve for the smallest relevant generalized eigenvalues,
- report beta_h and zero-mode count.

Compare at minimum:
- P1/P0 negative control,
- unstabilized P1/P1 negative control,
- MINI P1+bubble/P1,
- later alternative stable/higher-order pairs.

Do not use a single mesh to claim inf-sup stability.
