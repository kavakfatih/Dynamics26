# M6 Research — Tetra Pathology Taxonomy, Angle Conditions and Spectral Blind Spots

Status: RESEARCHING / verification-design candidate
Date: 2026-09-06

## 1. Purpose

This document deepens the Dynamics26 TET4 quality framework by separating three different ideas:

1. classical **morphological pathology names** such as sliver, wedge, cap, needle and spike,
2. **spectral degeneration** of the weighted affine map,
3. **FEM angle conditions** used in interpolation theory.

These are related but they are not interchangeable.

The main engineering conclusion is:

    named pathology
    != quantitative quality metric
    != FEM interpolation theorem
    != solver suitability.

Dynamics26 should keep quantitative metrics as authority and use pathology names only as explanatory
classification/diagnostic labels.

Primary research references are registered as:
- M6-TH-005,
- M6-TH-007,
- M6-FEM-004,
- M6-ANG-001,
- M6-ANG-002,
- M6-GEO-001.

## 2. Classical tetra pathology taxonomy

The computational-geometry / finite-element literature commonly distinguishes two broad visual
families of degenerating tetrahedra.

### Skinny / line-like family

- spire / needle,
- splinter,
- spindle,
- spear,
- spike.

### Flat / plane-like family

- wedge,
- spade,
- cap,
- sliver.

Hannukainen, Korotov and Krizek reproduce this classification from the classical sliver/mesh
literature.

Sorgente et al. independently summarize common practical pathologies:
- needle: strongly line-like/local solid-angle collapse,
- wedge: a face becomes very small,
- sliver: one vertex approaches the plane of its opposite face while ordinary edge lengths can remain
  moderate.

### Important policy

These names are descriptive, not mathematically disjoint equivalence classes.

A continuous tetra shape can move between qualitative categories and a real element may exhibit more
than one pathology signal.

Therefore Dynamics26 must not implement logic such as:

    if type == SLIVER then bad
    else good.

The authority remains the metric vector and solver evidence.

## 3. Sliver has a special Delaunay meaning

Cheng et al. define a sliver more specifically than merely "flat":

- four vertices lie close to a plane,
- their orthogonal projection to that plane forms a convex quadrilateral,
- there is no necessarily short edge.

This explains the classic Delaunay problem:

    ordinary radius-edge refinement can remove many short-edge pathologies
    while leaving slivers.

The existing Dynamics26 analytic sliver family is consistent with this behavior:

    q_MR -> 0
    q_RR -> 0
    theta_min -> 0
    theta_max -> pi

while:

    rho_RE -> 1/sqrt(2).

So "flat" and "sliver" must not be treated as synonyms. Sliver is the important flat subfamily with a
radius-edge blind spot.

## 4. Tetrahedral maximum-angle condition has two independent parts

For linear tetrahedral interpolation, the classical tetrahedral maximum-angle condition requires
uniform upper bounds strictly below pi for:

1. all **triangular face angles**,
2. all **dihedral angles**.

These two requirements are independent.

This is a major extension to the current M6 angle telemetry, which previously emphasized only
dihedral extrema.

The 2022 Korotov-Krizek-Kucera analysis gives useful pathology examples:

| Pathology family | Face maximum-angle condition | Dihedral maximum-angle condition | Linear interpolation implication in the cited family |
|---|---|---|---|
| needle / spire | passes | passes | can retain optimal order |
| splinter | passes | passes | can retain optimal order |
| wedge | passes | passes | can retain optimal order |
| spike | can fail | can pass | interpolation can lose optimal behavior |
| cap | can pass | can fail | interpolation can lose optimal behavior |
| sliver | can pass | can fail | maximum-angle theorem does not protect it |
| spindle | can fail | can fail | not protected |
| spear | can fail | can fail | not protected |
| spade | can fail | can fail | not protected |

This table is a research taxonomy, not a universal element-by-element theorem.

The important Dynamics26 conclusion is:

    dihedral-only telemetry is incomplete for interpolation research.

## 5. Why dihedral bounds alone are not a complete geometric quality test

Independent computational-geometry examples show:

- a spear can keep a lower dihedral bound while developing a very large dihedral angle,
- a splinter can keep an upper dihedral bound while developing very small dihedral angles,
- a needle/spire can keep both minimum and maximum dihedral angles apparently benign while its
  radius-edge ratio becomes arbitrarily bad.

The committed Dynamics26 needle family already demonstrates the last case analytically:

    theta_min -> 45 degrees
    theta_max -> 90 degrees

while:

    q_MR -> 0
    q_kappa -> 0
    q_RR -> 0
    rho_RE -> infinity.

Therefore angle extrema are pathology sentinels, not complete shape coordinates.

## 6. A first-principles dimensional argument: singular values cannot encode full tetra morphology

Let:

    T = A0 W^-1

be the weighted affine map from an equilateral reference tetrahedron.

T contains 9 scalar degrees of freedom.

For shape only:
- remove 3 physical rotation DOFs,
- remove 1 uniform scale DOF.

A general tetrahedron therefore has:

    9 - 3 - 1 = 5

continuous similarity-shape degrees of freedom.

Now write:

    T = U Sigma V^T.

After removing physical rotation U and uniform scale:
- the singular-value ratios in Sigma contribute only 2 DOFs,
- the right-singular orientation V contributes 3 additional DOFs.

Hence any metric depending only on singular values, including q_MR and q_kappa, cannot distinguish all
possible tetra morphologies.

This is not a weakness of those metrics. It is a statement about information content.

### Dynamics26 consequence

q_MR and q_kappa are excellent spectral shape diagnostics, but angle/radius/solid-angle observations
remain necessary because they probe geometry not fully encoded by the singular-value spectrum.

## 7. General spectral degeneration family

Normalize the largest singular value and consider:

    Sigma_(a,b)(epsilon)
      = diag(1, epsilon^a, epsilon^b)

with:

    0 <= a <= b
    epsilon -> 0+.

Then:

    det Sigma = epsilon^(a+b).

For a>0:

    q_MR
      ~ 3 epsilon^(2(a+b)/3).

The Frobenius condition score is controlled asymptotically by the smallest singular value:

    q_kappa
      = Theta(epsilon^b).

Interpretation:
- q_MR responds to the **combined volume-collapse exponent** a+b,
- q_kappa responds primarily to the **worst directional collapse exponent** b.

This explains why the metrics are related but not interchangeable.

## 8. Canonical flat versus needle spectral duality

Two especially useful goldens are:

### Flat/rank-two limit

    Sigma_F(epsilon) = diag(1,1,epsilon).

Then:

    q_MR,F
      = 3 epsilon^(2/3)
        /(2 + epsilon^2).

### Needle/rank-one limit

    Sigma_N(epsilon) = diag(1,epsilon,epsilon).

Then:

    q_MR,N
      = 3 epsilon^(4/3)
        /(1 + 2 epsilon^2).

But both have exactly the same Frobenius condition score:

    q_kappa,F
      = q_kappa,N
      = 3 epsilon
        /sqrt((2+epsilon^2)(1+2 epsilon^2)).

Therefore:

    q_kappa alone cannot distinguish flat collapse from needle collapse.

This is a stronger result than saying the metrics are merely correlated.

## 9. Inverse mean-ratio as a research spectral-duality diagnostic

From the previously derived identity:

    q_kappa(T)^2
      = q_MR(T) q_MR(T^-1),

define for research only:

    q_MR_inv = q_MR(T^-1)
             = q_kappa^2 / q_MR.

For the two canonical families:

    q_MR_inv(Sigma_F) = q_MR(Sigma_N)
    q_MR_inv(Sigma_N) = q_MR(Sigma_F).

A possible non-product classification coordinate is:

    chi_spec
      = log(q_MR / q_MR_inv).

Then asymptotically:
- flat spectral collapse -> chi_spec -> +infinity,
- needle spectral collapse -> chi_spec -> -infinity.

This is not proposed as a release metric or user threshold.

Its current value is:
- analytic understanding,
- fixture generation,
- distinguishing two collapse spectra that q_kappa alone cannot separate.

## 10. Same singular values can still give different angles

Let a fixed anisotropic spectrum be:

    Sigma = diag(3.0, 1.2, 0.4)

and define a right-rotation family:

    T(phi) = Sigma R_z(phi).

Because R_z is orthogonal:

    singular_values(T(phi)) = singular_values(Sigma)

for every phi.

Therefore q_MR and q_kappa remain exactly constant.

Using the committed equilateral reference W, a numerical research fixture gives approximately:

| phi | q_MR | q_kappa | q_RR | theta_min | theta_max |
|---:|---:|---:|---:|---:|---:|
| 0 deg | 0.3609029103 | 0.3468987334 | 0.1481713744 | 29.8338 deg | 147.0837 deg |
| 30 deg | 0.3609029103 | 0.3468987334 | 0.1544161430 | 20.6626 deg | 131.7716 deg |

So two tetrahedra can have the same singular spectrum, q_MR and q_kappa while having materially
different angle and radius-ratio diagnostics.

This is the executable-style counterexample required by the dimensional argument.

## 11. Face-angle telemetry

For a triangular face and two edge vectors u,v from the same face vertex:

    alpha = atan2(
              ||u x v||,
              u dot v
            ).

Using atan2 rather than acos preserves the acute/obtuse branch and behaves better near 0 and pi.

Research telemetry candidates:

    face_angle_min
    face_angle_max.

The maximum is particularly important for interpolation-condition studies.

No product threshold is accepted yet.

## 12. Dihedral-angle telemetry

For an edge e shared by faces f and g, let n_f and n_g be outward unit normals of a positive
tetrahedron.

The interior dihedral angle is:

    cos(theta_e)
      = - n_f dot n_g.

If A_f and A_g are the adjacent face areas, l_e is the common-edge length and V>0 is tetra volume:

    sin(theta_e)
      = 3 V l_e /(2 A_f A_g).

Therefore a stable research evaluation can use:

    theta_e
      = atan2(
          3 V l_e /(2 A_f A_g),
          -n_f dot n_g
        ).

This separates:
- exact orientation/volume truth,
- floating angle diagnostic.

The exact predicate remains the authority for zero/inversion.

## 13. Solid-angle telemetry

The classical pathology taxonomy also distinguishes behavior that dihedral extrema alone can obscure,
especially cap/spike-like large solid angles and needle-like very small solid angles.

For three edge vectors a,b,c emanating from one tetra vertex, Van Oosterom and Strackee give a robust
triangle-solid-angle expression:

    Omega
      = 2 atan2(
          |a dot (b x c)|,
          |a||b||c|
          + (a dot b)|c|
          + (b dot c)|a|
          + (c dot a)|b|
        ).

Use steradians.

Research telemetry candidates:

    solid_angle_min
    solid_angle_max.

These are not promoted to primary optimization metrics.

Their role is pathology interpretation and independent geometry checking.

## 14. Extended pathology / telemetry matrix

| Phenomenon | q_MR | q_kappa | dihedral min/max | face angles | solid angles | q_RR | rho_RE |
|---|---|---|---|---|---|---|---|
| sliver | detects | detects | strong | may look acceptable on faces | detects collapse | detects | can miss |
| needle/spire | detects | detects | can look acceptable | relevant | very small angle signal possible | detects | strong |
| spike | detects degeneration | detects | may remain bounded above | can expose near-pi face angle | large/small signal possible | detects | usually poor |
| cap | detects | detects | can expose near-pi dihedral | faces may pass max-angle | large solid angle | detects | usually poor |
| wedge | detects | detects | strong | can still satisfy max-angle theorem | diagnostic | detects | strong |
| same spectral Sigma, different V | identical possible | identical | can differ | can differ | can differ | can differ | can differ |

This table explains why the Dynamics26 quality layer is a vector rather than a single scalar.

## 15. Implications for optimizer architecture

Pathology classification can help select experiments, but it must not become a brittle production
decision tree.

Safe current direction:

### Ordinary short-edge / radius-edge pathologies

Research:
- sizing/refinement,
- smart smoothing,
- topology reconnection.

### Sliver-like pathology

Research separately:
- smoothing,
- stronger cavity reconnection,
- finite weighted/regular-Delaunay treatment,
- sliver exudation family.

### Interpolation-risk pathology

Use:
- face maximum-angle telemetry,
- dihedral maximum-angle telemetry,
- explicit interpolation benchmarks.

Do not assume q_MR alone predicts interpolation error.

### Nonlinear/rubber suitability

Still separate:
- no pathology class or geometric metric establishes locking freedom,
- formulation qualification remains in the dedicated TET4 FEM package.

## 16. Dynamics26 reporting policy candidate

Future TET4 diagnostic data should be capable of retaining:

    q_MR
    q_kappa
    q_RR
    rho_RE
    theta_min
    theta_max
    face_angle_min
    face_angle_max
    solid_angle_min
    solid_angle_max

plus:
- exact validity,
- sizing/gradation,
- solver/formulation context.

Research-only optional fields:

    q_MR_inv
    chi_spec
    pathology_label_candidate.

A pathology label is explanatory metadata, never the mathematical source of truth.

## 17. Verification requirements created here

New research gates should verify:

1. classical pathology taxonomy is represented by independent coordinate fixtures,
2. face and dihedral maximum-angle conditions are tested separately,
3. flat and needle spectral families have identical q_kappa but different q_MR asymptotics,
4. q_MR_inv swaps the canonical flat/needle spectral families,
5. right-rotation families preserve q_MR/q_kappa while angle/radius diagnostics vary,
6. face-angle atan2 formula agrees with independent triangle geometry,
7. dihedral atan2 formula agrees with normal and volume/area identities,
8. solid-angle formula reproduces the regular-tetra golden and pathology limits,
9. no named pathology class becomes a release threshold without solver evidence.

These are documentation/verification-design gates only. They do not authorize mesher implementation.
