# Meshing Source Registry

**Access-date convention:** dates below record when the source was reviewed for the Dynamics26 meshing program.  
**Legal note:** license summaries are engineering research notes, not legal advice. Production use of third-party code requires an explicit license review even when a source is listed here.

## A. Fundamental computational-geometry and meshing literature

| ID | Source | Relevance | URL | Reviewed |
|---|---|---|---|---|
| TH-001 | A. Bowyer, *Computing Dirichlet Tessellations*, The Computer Journal 24(2), 1981 | Incremental Delaunay / cavity insertion foundation | https://doi.org/10.1093/comjnl/24.2.162 | 2026-09-05 |
| TH-002 | C. L. Lawson, *Transforming Triangulations*, Discrete Mathematics, 1972 | Flip-based Delaunay transformation concept | https://hjemmesider.diku.dk/~rfonseca/literature/lawson/index.html | 2026-09-05 |
| TH-003 | J. Ruppert, *A Delaunay Refinement Algorithm for Quality 2-Dimensional Mesh Generation*, Journal of Algorithms 18(3), 1995 | Delaunay refinement, size/shape guarantees | https://doi.org/10.1006/jagm.1995.1021 | 2026-09-05 |
| TH-004 | J. R. Shewchuk, *Delaunay Refinement Mesh Generation*, PhD thesis, 1997 | 2D/3D Delaunay refinement, implementation/data-structure theory | https://www.cs.cmu.edu/~jrs/jrspapers.html | 2026-09-05 |
| TH-005 | J. R. Shewchuk, *Tetrahedral Mesh Generation by Delaunay Refinement*, SoCG 1998 | Core 3D refinement theory | https://www.cs.cmu.edu/~jrs/jrspapers.html | 2026-09-05 |
| TH-006 | J. R. Shewchuk, *Robust Adaptive Floating-Point Geometric Predicates*, SoCG 1996 | Robust orientation / in-circle / in-sphere predicates | https://doi.org/10.1145/237218.237337 | 2026-09-05 |
| TH-007 | J. R. Shewchuk, *Adaptive Precision Floating-Point Arithmetic and Fast Robust Geometric Predicates*, DCG 18(3), 1997 | Adaptive exact arithmetic foundation | https://www.cs.cmu.edu/~quake/robust.html | 2026-09-05 |
| TH-008 | R. Löhner, P. Parikh, *Generation of Three-Dimensional Unstructured Grids by the Advancing-Front Method*, IJNMF 8, 1988 | 3D advancing-front workflow and search/data-structure requirements | https://doi.org/10.1002/fld.1650081003 | 2026-09-05 |
| TH-009 | P. L. George, F. Hecht, E. Saltel, *Automatic Mesh Generator with Specified Boundary*, CMAME 92, 1991 | Boundary recovery / exact fitting problem | https://doi.org/10.1016/0045-7825(91)90017-Z | 2026-09-05 |
| TH-010 | M. A. Yerry, M. S. Shephard, *Automatic Mesh Generation for Three-Dimensional Solids*, Computers & Structures 20, 1985 | Modified-octree meshing and CAD-solid approximation | https://doi.org/10.1016/0045-7949(85)90050-1 | 2026-09-05 |
| TH-011 | J.-D. Boissonnat, S. Oudot, *Provably Good Sampling and Meshing of Surfaces*, Graphical Models 67(5), 2005 | Restricted-Delaunay surface sampling, topology/geometric approximation | https://doi.org/10.1016/j.gmod.2005.01.004 | 2026-09-05 |
| TH-012 | S.-W. Cheng, T. Dey, E. Ramos, T. Ray, *Sampling and Meshing a Surface with Guaranteed Topology and Geometry*, SIAM J. Computing | Feature/topology-aware surface meshing theory | https://doi.org/10.1137/060665889 | 2026-09-05 |
| TH-013 | L. Freitag, C. Ollivier-Gooch, *Tetrahedral Mesh Improvement Using Swapping and Smoothing* | Local topology changes and smoothing | https://doi.org/10.1002/(SICI)1097-0207(19971115)40:21%3C3979::AID-NME251%3E3.0.CO;2-9 | 2026-09-05 |
| TH-014 | L. Freitag, P. Knupp, *Tetrahedral Mesh Improvement via Optimization of the Element Condition Number*, IJNME 53, 2002 | Optimization-based quality metric and smoothing | https://doi.org/10.1002/nme.341 | 2026-09-05 |
| TH-015 | S.-W. Cheng et al., *Sliver Exudation*, JACM 47(5), 2000 | Delaunay tetrahedral sliver improvement | https://people.eecs.berkeley.edu/~jrs/meshs08/present.html | 2026-09-05 |
| TH-016 | H. Si, *TetGen, a Delaunay-Based Quality Tetrahedral Mesh Generator*, ACM TOMS 41(2), 2015 | CDT/refinement/quality reference | https://doi.org/10.1145/2629697 | 2026-09-05 |
| TH-017 | C. Marot, J.-F. Remacle, *Quality Tetrahedral Mesh Generation with HXT*, 2020 | Parallel Delaunay + mesh-improvement schedule | https://arxiv.org/abs/2008.08508 | 2026-09-05 |
| TH-018 | J. Schöberl, *NETGEN — An Advancing Front 2D/3D Mesh Generator Based on Abstract Rules*, 1997 | Advancing-front decomposition and rule-based generation | https://doi.org/10.1007/s007910050004 | 2026-09-05 |
| TH-019 | C. Geuzaine, J.-F. Remacle, *Gmsh: a 3-D Finite Element Mesh Generator...*, IJNME 79(11), 2009 | Meshing-system architecture and algorithm orchestration | https://doi.org/10.1002/nme.2579 | 2026-09-05 |

## B. Commercial CAE benchmark sources

Commercial products are behavior references only; implementation source is unavailable.

| ID | Product / topic | Observation used | URL | Reviewed |
|---|---|---|---|---|
| CA-ANS-001 | ANSYS Patch Conforming Tetra | Delaunay tetra mesher with advancing-front point insertion for refinement | https://ansyshelp.ansys.com/public/Views/Secured/corp/v242/en/wb_msh/msh_Patch_Conf_Algor.html | 2026-09-05 |
| CA-ANS-002 | ANSYS Patch Independent Tetra | Top-down spatial subdivision; dirty-geometry tolerance benchmark | https://ansyshelp.ansys.com/public/Views/Secured/corp/v242/en/wb_msh/msh_Patch_Ind_Algor.html | 2026-09-05 |
| CA-ANS-003 | ANSYS Local Sizing | Body/Face/Edge/Vertex and Named Selection scoped sizing | https://ansyshelp.ansys.com/public/Views/Secured/corp/v252/en/wb_msh/msh_Use_Sizing_Control.html | 2026-09-05 |
| CA-ANS-004 | ANSYS Named Selection | Criteria-based selection and mesh-node association | https://ansyshelp.ansys.com/public/Views/Secured/corp/v261/en/wb_sim/ds_selection_name_ns.html | 2026-09-05 |
| CA-ANS-005 | ANSYS MultiZone | Mixed/sweep-oriented meshing and protected named-selection behavior | https://ansyshelp.ansys.com/public/Views/Secured/corp/v261/en/wb_msh/ds_multizone_method_option.html | 2026-09-05 |
| CA-COM-001 | COMSOL Free Tetrahedral | Domain-scoped unstructured tetra operation; manual/named selection | https://doc.comsol.com/6.4/doc/com.comsol.help.comsol/comsol_ref_mesh.24.64.html | 2026-09-05 |
| CA-COM-002 | COMSOL Named Selections in Mesh Sequence | Mesh-operation selections and boundary tracking | https://doc.comsol.com/6.4/doc/com.comsol.help.comsol/comsol_ref_visualizationselection.22.35.html | 2026-09-05 |
| CA-MAR-001 | Marc/Mentat Remeshing | Global Remeshing and Local Adaptivity as distinct capabilities | https://nexus.hexagon.com/community/public/marc/f/marc-community-forum/149663/can-i-do-remeshing-in-marc-mentat | 2026-09-05 |
| CA-MAR-002 | Marc/Mentat release guide | Dedicated automatic 2D/3D remeshing and hexahedral mesher capabilities | https://documentation-be.hexagon.com/bundle/Marc_2024.1-Release_Guide/raw/resource/enus/Marc_2024.1-Release_Guide.pdf | 2026-09-05 |
| CA-MAR-003 | Marc 2020 FP1 | Mesh-on-mesh, voxel mesher, Parasolid body support | https://documentation-be.hexagon.com/bundle/Marc_2020_FP1-Release_Guide/raw/resource/enus/Marc_2020_FP1-Release_Guide.pdf | 2026-09-05 |

## C. Open-source repository studies

These repositories are **study references, not code sources for Dynamics26 implementation**.

| ID | Project | License observation | Research use | URL | Reviewed |
|---|---|---|---|---|---|
| OS-NET-001 | NGSolve/Netgen | GitHub reports LGPL-2.1; repo README describes automatic tetra meshing, optimization and refinement | Advancing-front architecture, OCC separation, test corpus ideas | https://github.com/NGSolve/netgen | 2026-09-05 |
| OS-GMS-001 | Gmsh | GPL-family project; license must be reviewed before any reuse | Multi-algorithm orchestration, entity classification, benchmark organization | https://gmsh.info/doc/texinfo/gmsh.html | 2026-09-05 |
| OS-GMS-002 | Gmsh source mirror | Large \`src/mesh\` plus isolated algorithm/contrib modules | Source-tree architecture study only | https://github.com/live-clones/gmsh | 2026-09-05 |
| OS-MMG-001 | MMG | README/code identify LGPL terms | Metric-based remeshing, adaptation, local operations | https://github.com/MmgTools/mmg | 2026-09-05 |
| OS-TET-001 | TetGen | Current README lists AGPLv3 for 1.5/1.6 and commercial dual-license path | CDT/refinement architecture and failure/test study only | https://github.com/TetGen/TetGen | 2026-09-05 |
| OS-CGA-001 | CGAL Mesh_3 | Package documentation; package licensing must be checked independently | Restricted Delaunay, feature protection, refinement/optimization research map | https://doc.cgal.org/latest/Mesh_3/index.html | 2026-09-05 |

## D. Dynamics26 internal sources

| ID | Path | Relevance |
|---|---|---|
| D26-001 | \`gui/services/MeshService.*\` | Current mesh lifecycle and source-of-truth boundary |
| D26-002 | \`include/femcae/meshing/MeshTypes.h\` | SimulationMesh / MeshFacet provenance model |
| D26-003 | \`src/meshing/GeometryMeshBridge.cpp\` | GeometryAssociationMap construction |
| D26-004 | \`include/femcae/meshing/MeshingPlan.h\` | Existing global/local sizing concept |
| D26-005 | \`src/meshing/StructuredHexMesher.cpp\` | Existing deterministic structured baseline |
| D26-006 | \`gui/services/NamedSelectionService.*\` | Persistent geometry/mesh scope lifecycle |

## E. Source-entry rules

When adding a source:

- prefer DOI or official author page for theory,
- prefer official vendor docs for product behavior,
- record exact repository and license observation for source studies,
- state whether the source is THEORY, BENCHMARK, ARCHITECTURE or TROUBLESHOOTING,
- never paste substantial source code into this library,
- record implementation decisions separately in \`DECISION_LOG.md\`.
