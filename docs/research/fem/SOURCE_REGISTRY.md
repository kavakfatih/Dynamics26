# FEM Research Source Registry

Reviewed: 2026-09-05

## TET4 nearly-incompressible / mixed-formulation sources

| ID | Source | Role | URL |
|---|---|---|---|
| T4-TH-001 | D. Chapelle, K. J. Bathe, The inf-sup test, Computers & Structures 47(4-5), 1993 | Numerical discrete inf-sup qualification methodology | https://doi.org/10.1016/0045-7949(93)90340-J |
| T4-TH-002 | K. J. Bathe, The inf-sup condition and its evaluation for mixed finite element methods, Computers & Structures 79(2), 2001 | Inf-sup evaluation and mixed-FE stability review | https://doi.org/10.1016/S0045-7949(00)00123-1 |
| T4-TH-003 | E. Karabelas et al., Versatile stabilized finite element formulations for nearly and fully incompressible solid mechanics, Computational Mechanics 65, 2020 | Tetra MINI and pressure-projection stabilized P1-P1 formulation family | https://doi.org/10.1007/s00466-019-01760-w |
| T4-TH-004 | E. Karabelas, M. A. F. Gsell, G. Haase et al., An accurate, robust, and efficient finite element framework with applications to anisotropic, nearly and fully incompressible elasticity, CMAME 394, 2022 | Large-scale tetra/hexa stabilized P1-P1 and MINI evidence; P1-P0 locking/stress warning | https://doi.org/10.1016/j.cma.2022.114887 |
| T4-TH-005 | E. A. de Souza Neto, F. M. Andrade Pires, D. R. J. Owen, F-bar-based linear triangles and tetrahedra for finite strain analysis of nearly incompressible solids. Part I, IJNME 62, 2005 | Patch-volume F-bar linear simplex formulation and exact tangent | https://doi.org/10.1002/nme.1187 |
| T4-TH-006 | J. A. Schönherr, P. Schneider, C. Mittelstedt, Robust hybrid/mixed finite elements for rubber-like materials under severe compression, Computational Mechanics 70, 2022 | Higher-order hybrid/mixed tetra research for rubber-like severe compression | https://doi.org/10.1007/s00466-022-02157-y |

## Public commercial behavior benchmarks

| ID | Source | Observation used | URL |
|---|---|---|---|
| T4-ANS-001 | ANSYS continuum element technologies / mixed u-P | Public evidence that incompressibility can produce locking/checkerboard/divergence and current mixed u-P solids use independent pressure variables | https://ansyshelp.ansys.com/public/Views/Secured/corp/v252/en/ans_elem/Hlp_E_continuumelems.html |
| T4-ANS-002 | ANSYS SOLID285 | Public 4-node tetra behavior: stabilized mixed u-P, nodal hydrostatic pressure, large-strain/hyperelastic support | https://ansyshelp.ansys.com/public/Views/Secured/corp/v251/en/ans_elem/Hlp_E_SOLID285.html |
| T4-ABA-001 | Abaqus Hybrid elements | Public statement that pure displacement is inadequate for incompressible material and hybrid elements add pressure unknowns | https://docs.software.vt.edu/abaqusv2025/English/SIMACAEGSARefMap/simagsa-c-ctmhybrid.htm |
| T4-ABA-002 | Abaqus Selecting continuum elements | Public warning that linear tetrahedra generally require very fine meshes; quadratic tetrahedra are preferred for general use | https://docs.software.vt.edu/abaqusv2025/English/SIMACAEGSARefMap/simagsa-c-ctmselecting.htm |
| T4-ABA-003 | Abaqus Hyperelastic behavior | Public hybrid-vs-displacement guidance for almost-incompressible hyperelasticity | https://docs.software.vt.edu/abaqusv2025/English/SIMACAEMATRefMap/simamat-c-hyperelastic.htm |

## Source-use rule

- Theory papers support equations, approximation-space reasoning and verification design.
- Vendor documentation supports only observable product/capability sanity checks.
- No vendor internal algorithm is a Dynamics26 source specification.
