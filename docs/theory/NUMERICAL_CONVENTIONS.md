# V0.1.0 Numerical Conventions

## Residual

\[
\mathbf{R}(\mathbf{u}) = \mathbf{f}_{ext} - \mathbf{f}_{int}(\mathbf{u})
\]

Denge:

\[
\mathbf{R}=0
\]

Newton duzeltmesi:

\[
\mathbf{K}_T \Delta\mathbf{u} = \mathbf{R}
\]

Bu isaret sistemi proje genelinde tek standarttir.

## Voigt

3B simetrik tensor sirasi:

\[
[xx, yy, zz, xy, yz, xz]
\]

Stress vectoru:

\[
[\sigma_{xx},\sigma_{yy},\sigma_{zz},\tau_{xy},\tau_{yz},\tau_{xz}]
\]

Strain vectoru engineering shear kullanir:

\[
[\varepsilon_{xx},\varepsilon_{yy},\varepsilon_{zz},\gamma_{xy},\gamma_{yz},\gamma_{xz}]
\]

ve

\[
\gamma_{ij}=2\varepsilon_{ij}
\]

## Unit policy

Cekirdek herhangi bir gizli unit conversion yapmaz. Kullanilan birim sistemi kendi icinde tutarli olmak zorundadir.

## Nonlinear state

Committed state sadece converged load/time increment sonunda guncellenir. Newton iterasyonlari trial state uzerinde calisir.
