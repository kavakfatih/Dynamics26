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

## D. M1 robust-geometry / numerical-robustness sources

| ID | Source | Relevance | URL | Reviewed |
|---|---|---|---|---|
| M1-TH-001 | S. Fortune, *Numerical Stability of Algorithms for 2D Delaunay Triangulations*, SoCG 1992 / later journal version | Approximate arithmetic and Delaunay numerical-stability problem | https://doi.org/10.1145/142675.142695 | 2026-09-05 |
| M1-TH-002 | S. Fortune, C. J. Van Wyk, *Efficient Exact Arithmetic for Computational Geometry*, SoCG 1993 | Exact arithmetic plus floating-point filters | https://doi.org/10.1145/160985.161015 | 2026-09-05 |
| M1-TH-003 | O. Devillers, F. Preparata, *Further Results on Arithmetic Filters for Geometric Predicates*, Computational Geometry 13(2), 1999 | Certified filtering efficiency, especially cosphericity/insphere class | https://doi.org/10.1016/S0925-7721(99)00011-5 | 2026-09-05 |
| M1-TH-004 | H. Edelsbrunner, E. P. Mücke, *Simulation of Simplicity*, SoCG 1988 | Deterministic symbolic treatment of degeneracy | https://doi.org/10.1145/73393.73406 | 2026-09-05 |
| M1-DOC-001 | CGAL, *Predicates and Constructions* | Predicate correctness controls algorithm flow; construction/predicate separation | https://doc.cgal.org/Manual/3.1/doc_html/cgal_manual/Kernel_d/Chapter_predicates_constructions_d.html | 2026-09-05 |
| M1-DOC-002 | CGAL, Exact Predicates / Inexact Constructions Kernel | Public example of exact predicate + inexact construction architecture | https://doc.cgal.org/latest/Kernel_23/classCGAL_1_1Exact__predicates__inexact__constructions__kernel.html | 2026-09-05 |
| M1-DOC-003 | Clang Compiler User's Manual, floating-point / fast-math controls | Build-contract risk: reassociation, NaN assumptions, FP contraction | https://clang.llvm.org/docs/UsersManual.html | 2026-09-05 |
| M1-DOC-004 | GCC Optimize Options, `-ffast-math` | Independent compiler documentation of unsafe FP transformations | https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html | 2026-09-05 |
| M1-DOC-005 | OCCT `ShapeAnalysis_ShapeTolerance` | CAD B-Rep sub-shape tolerance inspection | https://dev.opencascade.org/doc/refman/html/class_shape_analysis___shape_tolerance.html | 2026-09-05 |
| M1-DOC-006 | OCCT `ShapeFix_ShapeTolerance` | Explicit modification/limiting of CAD sub-shape tolerances | https://dev.opencascade.org/doc/refman/html/class_shape_fix___shape_tolerance.html | 2026-09-05 |
| M1-CA-001 | COMSOL Default Repair Tolerances | Automatic/relative/absolute CAD repair tolerance semantics | https://doc.comsol.com/6.3/doc/com.comsol.help.comsol/comsol_api_geom.48.026.html | 2026-09-05 |
| M1-CA-002 | ANSYS Mesh Defeaturing | Explicit defeature-size semantics | https://ansyshelp.ansys.com/public/Views/Secured/corp/v261/en/wb_msh/msh_auto_defeat.html | 2026-09-05 |
| M1-CA-003 | ANSYS Repair Topology | Explicit short-edge/thin-face repair and topology-protection controls | https://ansyshelp.ansys.com/public/Views/Secured/corp/v261/en/wb_msh/msh_wrkflw_repairtopology.html | 2026-09-05 |
| M1-CA-004 | Marc/Mentat 2020 Release Guide | Public evidence of auto-calculated tolerance / CAD tolerance behavior; no internal predicate disclosure | https://documentation-be.hexagon.com/bundle/Marc_2020-Release_Guide/raw/resource/enus/Marc_2020-Release_Guide.pdf | 2026-09-05 |

## E. M1.1 exact-oracle / filtering sources

| ID | Source | Relevance | URL | Reviewed |
|---|---|---|---|---|
| M1.1-DOC-001 | Python Built-in Types — `float.as_integer_ratio()` | Exact integer ratio of stored float | https://docs.python.org/3/library/stdtypes.html | 2026-09-05 |
| M1.1-DOC-002 | Python Floating-Point Tutorial — exact ratio / hex representation | Exact round-trip fixture representation | https://docs.python.org/3/tutorial/floatingpoint.html | 2026-09-05 |
| M1.1-DOC-003 | Python `fractions.Fraction` | Standard-library exact rational arithmetic for test oracle | https://docs.python.org/3/library/fractions.html | 2026-09-05 |
| M1.1-TH-001 | E. H. Bareiss, *Sylvester's Identity and Multistep Integer-Preserving Gaussian Elimination*, 1968 | Fraction-free exact integer determinant | https://doi.org/10.1090/S0025-5718-1968-0226829-0 | 2026-09-05 |
| M1.1-TH-002 | O. Devillers, F. Preparata, *Further Results on Arithmetic Filters for Geometric Predicates*, 1999 | Filter + exact fallback efficiency | https://doi.org/10.1016/S0925-7721(99)00011-5 | 2026-09-05 |
| M1.1-DOC-004 | CGAL Filtered Kernel / Filtered Predicate documentation | Public architecture example of fast filtering + exact fallback | https://doc.cgal.org/latest/Kernel_23/structCGAL_1_1Filtered__kernel.html | 2026-09-05 |
| M1.1-DOC-005 | Clang User Manual — floating-point behavior | Reassociation, fast-math, FP contraction build risks | https://clang.llvm.org/docs/UsersManual.html | 2026-09-05 |
| M1.1-ANS-001 | ANSYS Repair Topology 2025/2026 docs | Explicit repair operations/topology protection | https://ansyshelp.ansys.com/public/Views/Secured/corp/v252/en/wb_msh/msh_wrkflw_repairtopology.html | 2026-09-05 |
| M1.1-ANS-002 | ANSYS `RepairTopologyTolerance` 2026 R1 API | Explicit repair tolerance as meshing control | https://ansyshelp.ansys.com/public/Views/Secured/corp/v261/en/act_ref/item86166192241305182105861492522144151206411274113.html | 2026-09-05 |
| M1.1-COM-001 | COMSOL 6.3 Default Repair Tolerances | Automatic/relative/absolute geometry-operation tolerance lifecycle | https://doc.comsol.com/6.3/doc/com.comsol.help.comsol/comsol_api_geom.48.026.html | 2026-09-05 |
| M1.1-MAR-001 | Marc 2026.1 product / What's New | Current Mentat meshing capability improvements | https://nexus.hexagon.com/home/product/marc/ | 2026-09-05 |
| M1.1-MAR-002 | Cadence Marc 2026.1 capabilities article | Automatic meshing, refinement transitions, density controls, tetra support | https://community.cadence.com/cadence_blogs_8/b/pss/posts/new-capabilities-in-marc-2026-1-you-should-be-using-for-nonlinear-analysis | 2026-09-05 |

## F. Dynamics26 internal sources

| ID | Path | Relevance |
|---|---|---|
| D26-001 | \`gui/services/MeshService.*\` | Current mesh lifecycle and source-of-truth boundary |
| D26-002 | \`include/femcae/meshing/MeshTypes.h\` | SimulationMesh / MeshFacet provenance model |
| D26-003 | \`src/meshing/GeometryMeshBridge.cpp\` | GeometryAssociationMap construction |
| D26-004 | \`include/femcae/meshing/MeshingPlan.h\` | Existing global/local sizing concept |
| D26-005 | \`src/meshing/StructuredHexMesher.cpp\` | Existing deterministic structured baseline |
| D26-006 | \`gui/services/NamedSelectionService.*\` | Persistent geometry/mesh scope lifecycle |

## G. Source-entry rules

When adding a source:

- prefer DOI or official author page for theory,
- prefer official vendor docs for product behavior,
- record exact repository and license observation for source studies,
- state whether the source is THEORY, BENCHMARK, ARCHITECTURE or TROUBLESHOOTING,
- never paste substantial source code into this library,
- record implementation decisions separately in \`DECISION_LOG.md\`.


## M1.2 certified-filter sources

| ID | Source | Relevance | URL | Reviewed |
|---|---|---|---|---|
| M1.2-TH-001 | N. J. Higham, *Accuracy and Stability of Numerical Algorithms*, 2nd ed. | Standard FP model and gamma_n accumulation | https://eprints.maths.manchester.ac.uk/238/ | 2026-09-05 |
| M1.2-TH-002 | J. R. Shewchuk, *Adaptive Precision Floating-Point Arithmetic and Fast Robust Geometric Predicates* | Adaptive exact-predicate theory | https://people.eecs.berkeley.edu/~jrs/papers/robust-predicates.pdf | 2026-09-05 |
| M1.2-TH-003 | O. Devillers, F. Preparata, *Further Results on Arithmetic Filters for Geometric Predicates* | Rounded evaluation + certificate + exact fallback | https://doi.org/10.1016/S0925-7721(99)00011-5 | 2026-09-05 |
| M1.2-DOC-001 | CGAL Filtered Predicate / Kernel | Public filtered-exact architecture reference | https://doc.cgal.org/latest/Kernel_23/classCGAL_1_1Filtered__predicate.html | 2026-09-05 |
| M1.2-DOC-002 | Clang Compiler User Manual | FP reassociation, contraction and fast-math semantics | https://clang.llvm.org/docs/UsersManual.html | 2026-09-05 |
| M1.2-ANS-001 | ANSYS 2026 R1 Repair Topology | Explicit topology repair/protection | https://ansyshelp.ansys.com/public/Views/Secured/corp/v261/en/wb_msh/msh_wrkflw_repairtopology.html | 2026-09-05 |
| M1.2-ANS-002 | ANSYS 2026 R1 RepairTopologyTolerance | Explicit topology-conditioning tolerance | https://ansyshelp.ansys.com/public/Views/Secured/corp/v261/en/act_ref/item86166192241305182105861492522144151206411274113.html | 2026-09-05 |
| M1.2-COM-001 | COMSOL 6.4 Geometry Node | Automatic/relative/absolute repair tolerance | https://doc.comsol.com/6.4/doc/com.comsol.help.comsol/comsol_ref_geometry.23.011.html | 2026-09-05 |
| M1.2-COM-002 | COMSOL 6.4 Repair | Explicit CAD repair operations | https://doc.comsol.com/6.4/doc/com.comsol.help.cad/cad_ug_ref.6.24.html | 2026-09-05 |
| M1.2-MAR-001 | Hexagon Nexus Marc 2026.1 | Current Mentat meshing advances | https://nexus.hexagon.com/home/product/marc/ | 2026-09-05 |
| M1.2-MAR-002 | Marc Community — free edges prevent tetra meshing | Closed-surface prerequisite / gap diagnosis | https://nexus.hexagon.com/community/public/marc/f/marc-community-forum/144789/free-edges-prevent-tetrahedral-meshing-how-can-i-solve-this-problem | 2026-09-05 |


## M1.3 degeneracy / symbolic-perturbation sources

| ID | Source | Relevance | URL | Reviewed |
|---|---|---|---|---|
| M1.3-TH-001 | H. Edelsbrunner, E. P. Mücke, *Simulation of Simplicity*, ACM TOG 9(1), 1990 | General symbolic perturbation / consistent degeneracy handling | https://doi.org/10.1145/77635.77639 | 2026-09-05 |
| M1.3-TH-002 | M. B. Dillencourt, W. D. Smith, *A Simple Method for Resolving Degeneracies in Delaunay Triangulations*, ICALP 1993 | Alternative Delaunay-degeneracy completion approach | https://doi.org/10.1007/3-540-56939-1_71 | 2026-09-05 |
| M1.3-TH-003 | O. Devillers, M. Teillaud, *Perturbations for Delaunay and weighted Delaunay 3D triangulations*, Computational Geometry 44(3), 2011 | Unique robust 3D Delaunay perturbation research | https://doi.org/10.1016/j.comgeo.2010.09.010 | 2026-09-05 |
| M1.3-DOC-001 | CGAL Delaunay/Triangulation documentation | Coincident insertion + symbolic perturbation for co-spherical ambiguity | https://doc.cgal.org/latest/Triangulation_3/ | 2026-09-05 |
| M1.3-OS-001 | Gmsh `src/mesh/delaunay3d.cpp` | Architecture study: exact-zero insphere enters separate perturbation path | https://github.com/live-clones/gmsh/blob/master/src/mesh/delaunay3d.cpp | 2026-09-05 |
| M1.3-OS-002 | TetGen CHANGELOG | Public note that symbolic perturbation addressed spherical degeneracies | https://github.com/TetGen/TetGen/blob/main/CHANGELOG.md | 2026-09-05 |
| M1.3-ANS-001 | ANSYS Precheck Tool Options | Short/sliver/duplicate/self-intersection/misalignment diagnostics | https://ansyshelp.ansys.com/public/Views/Secured/corp/v252/en/discovery/UDA/user_manual/modeling/prepare/topics/r_st_precheck_tool_guides_options.html | 2026-09-05 |
| M1.3-ANS-002 | ANSYS Tetra Meshing Problems | Duplicate node/face and invalid-surface diagnostics | https://ansyshelp.ansys.com/public/Views/Secured/corp/v242/en/flu_ug/tgd_user_tet_mesh.html | 2026-09-05 |
| M1.3-COM-001 | COMSOL 6.4 Check | Tolerance faults, invalid entities and meshing consequences | https://doc.comsol.com/6.4/doc/com.comsol.help.design/design_ug_function.5.27.html | 2026-09-05 |
| M1.3-COM-002 | COMSOL 6.4 Repair | Invalid manifolds, self-intersections, sliver/small-detail repair | https://doc.comsol.com/6.4/doc/com.comsol.help.cad/cad_ug_cad_import_repair_defeaturing.5.17.html | 2026-09-05 |
| M1.3-COM-003 | COMSOL Techniques for Creating Geometries | Public warning that geometric degeneracies may cause meshing/analysis problems | https://doc.comsol.com/6.4/doc/com.comsol.help.comsol/comsol_ref_geometry.23.003.html | 2026-09-05 |
| M1.3-MAR-001 | Marc Community — node equivalencing | Public evidence of duplicate/coincident node equivalence workflow | https://nexus.hexagon.com/community/public/marc/f/marc-community-forum/145559/how-to-equivalence-nodes-in-mentat | 2026-09-05 |
