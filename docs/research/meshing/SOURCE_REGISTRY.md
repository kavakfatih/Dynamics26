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


## M1.4 point-location / topology / scalability sources

| ID | Source | Relevance | URL | Reviewed |
|---|---|---|---|---|
| M1.4-TH-001 | O. Devillers, S. Pion, M. Teillaud, *Walking in a Triangulation*, IJFCS 13(2), 2002 | Point location by adjacency walk in 2D/3D | https://doi.org/10.1142/S0129054102001047 | 2026-09-05 |
| M1.4-TH-002 | N. Amenta, S. Choi, G. Rote, *Incremental Constructions con BRIO*, SoCG 2003 | Biased randomized insertion order and memory locality | https://doi.org/10.1145/777792.777824 | 2026-09-05 |
| M1.4-TH-003 | O. Devillers, *The Delaunay Hierarchy*, IJFCS 13(2), 2002 | Hierarchical point-location accelerator | https://doi.org/10.1142/S0129054102001035 | 2026-09-05 |
| M1.4-TH-004 | H. Si, *TetGen, a Delaunay-Based Quality Tetrahedral Mesh Generator*, ACM TOMS 41(2), 2015 | Tetra data structure, spatial point sorting, incremental insertion | https://doi.org/10.1145/2629697 | 2026-09-05 |
| M1.4-TH-005 | C. Marot, J. Pellerin, J.-F. Remacle, *One machine, one minute, three billion tetrahedra*, IJNME 117(9), 2019 | Compact serial data structure, spatial ordering, Moore-curve parallelization | https://doi.org/10.1002/nme.5987 | 2026-09-05 |
| M1.4-TH-006 | C. Marot, J.-F. Remacle, *Quality tetrahedral mesh generation with HXT*, 2020 | Parallel cavity/locality/quality architecture | https://arxiv.org/abs/2008.08508 | 2026-09-05 |
| M1.4-DOC-001 | CGAL 3D Triangulations | Geometry/combinatorics separation, typed locate, hints and location policy | https://doc.cgal.org/latest/Triangulation_3/ | 2026-09-05 |
| M1.4-OS-001 | Gmsh `src/mesh/delaunay3d.cpp` | Source study: neighbor walk, cavity buffers, spatial ordering, stable storage concern | https://github.com/live-clones/gmsh/blob/master/src/mesh/delaunay3d.cpp | 2026-09-05 |
| M1.4-OS-002 | Gmsh/HXT `hxt_tetDelaunay.c` | Source study: walk-to-cavity, BFS cavity, Moore/Hilbert ordering, partition conflicts | https://github.com/live-clones/gmsh/blob/master/contrib/hxt/tetMesh/src/hxt_tetDelaunay.c | 2026-09-05 |
| M1.4-OS-003 | Netgen `meshing3.cpp` | Source study: local advancing-front extraction and spatial/local-size structures | https://github.com/NGSolve/netgen/blob/master/libsrc/meshing/meshing3.cpp | 2026-09-05 |
| M1.4-ANS-001 | ANSYS Parallel Part Meshing | CPU/memory-aware parallel meshing behavior | https://ansyshelp.ansys.com/public/Views/Secured/corp/v242/en/wb_msh/msh_ppm_best.html | 2026-09-05 |
| M1.4-COM-001 | COMSOL 6.4 Shared-Memory Parallel COMSOL | 3D tet mesher parallelism over faces/domains and single-domain limitation | https://doc.comsol.com/6.4/doc/com.comsol.help.comsol/comsol_ref_running.38.24.html | 2026-09-05 |
| M1.4-COM-002 | COMSOL 6.4 Free Tetrahedral | Domain-scoped tetra meshing and sizing controls | https://doc.comsol.com/6.4/doc/com.comsol.help.comsol/comsol_ref_mesh.24.64.html | 2026-09-05 |
| M1.4-MAR-001 | Marc 2024.2 Program Input — ADAPT GLOBAL | Mesh density fields, hard/soft topology and remeshing controls | https://documentation-be.hexagon.com/bundle/Marc_2024.2-Volume_C_Program_Input/raw/resource/enus/Marc_2024.2-Volume_C_Program_Input.pdf | 2026-09-05 |
| M1.4-MAR-002 | Hexagon Nexus Marc 2026.1 | Current Mentat meshing improvements | https://nexus.hexagon.com/home/product/marc/ | 2026-09-05 |


## M1.5 verification-harness / commercial-quality sources

| ID | Source | Relevance | URL | Reviewed |
|---|---|---|---|---|
| M1.5-DOC-001 | CMake `add_test` | Native CTest executable integration | https://cmake.org/cmake/help/latest/command/add_test.html | 2026-09-05 |
| M1.5-DOC-002 | CTest `LABELS` | Test grouping/filtering for meshing gates | https://cmake.org/cmake/help/latest/prop_test/LABELS.html | 2026-09-05 |
| M1.5-DOC-003 | CTest `FIXTURES_REQUIRED` | Generated-corpus setup dependency option | https://cmake.org/cmake/help/latest/prop_test/FIXTURES_REQUIRED.html | 2026-09-05 |
| M1.5-DOC-004 | CTest `RESOURCE_LOCK` | Shared-resource serialization if later needed | https://cmake.org/cmake/help/latest/prop_test/RESOURCE_LOCK.html | 2026-09-05 |
| M1.5-ANS-001 | ANSYS 2026 R1 Mesh Evaluation | Mesh statistics and multiple quality metrics | https://ansyshelp.ansys.com/public/Views/Secured/corp/v261/en/discovery/UDA/user_manual/meshing/topics/c_mesh_evaluation.html | 2026-09-05 |
| M1.5-ANS-002 | ANSYS 2026 R1 Element Quality | Composite quality definition and tetra constant | https://ansyshelp.ansys.com/public/Views/Secured/corp/v261/en/discovery/UDA/user_manual/meshing/topics/c_mesh_element_quality.html | 2026-09-05 |
| M1.5-ANS-003 | ANSYS 2026 R1 Orthogonal Quality | 0–1 cell quality definition | https://ansyshelp.ansys.com/public/Views/Secured/corp/v261/en/wb_msh/msh_orthogonal_quality.html | 2026-09-05 |
| M1.5-ANS-004 | ANSYS Meshing User Guide — quality worksheet | Average/worst/warning/error/failed-count workflow | https://ansyshelp.ansys.com/public/Views/Secured/corp/v251/en/pdf/ANSYS_Meshing_Users_Guide.pdf | 2026-09-05 |
| M1.5-COM-001 | COMSOL 6.4 Mesh Information and Statistics | min/mean quality, counts, volume | https://doc.comsol.com/6.4/doc/com.comsol.help.comsol/application_programming_guide.15.79.html | 2026-09-05 |
| M1.5-COM-002 | COMSOL 6.4 `mphmeshstats` | quality distributions, growth, element types | https://doc.comsol.com/6.4/doc/com.comsol.help.llmatlab/llmatlab_ug_ref.9.46.html | 2026-09-05 |
| M1.5-COM-003 | COMSOL 6.4 Inspecting/Troubleshooting Meshes | mesh statistics, plots, warnings/errors | https://doc.comsol.com/6.4/doc/com.comsol.help.comsol/comsol_ref_mesh.24.23.html | 2026-09-05 |
| M1.5-COM-004 | COMSOL 6.4 Free Tetrahedral | quality optimization levels and minimum-quality targets | https://doc.comsol.com/6.4/doc/com.comsol.help.comsol/comsol_ref_mesh.24.64.html | 2026-09-05 |
| M1.5-MAR-001 | Hexagon Nexus Marc 2026.1 | Current Mentat meshing improvements | https://nexus.hexagon.com/home/product/marc/ | 2026-09-05 |
| M1.5-MAR-002 | Marc Community — mesh size / strain localization | Hexagon support guidance on refinement and local adaptive remeshing | https://nexus.hexagon.com/community/public/marc/f/marc-community-forum/143880/how-to-decide-proper-mesh-size-and-avoid-strain-localization-in-elements | 2026-09-05 |
| M1.5-MAR-003 | Marc Community — global remeshing / local adaptivity | Distinct remeshing workflows | https://nexus.hexagon.com/community/public/marc/f/marc-community-forum/149663/can-i-do-remeshing-in-marc-mentat | 2026-09-05 |


## M2.0 Delaunay reference-architecture sources

M2.0 also reuses TH-001 (Bowyer), TH-006/TH-007 (Shewchuk robust predicates),
M1-TH-004 (Simulation of Simplicity), M1.4-TH-001 (walking) and M1.4-DOC-001
(3D triangulation representation).

| ID | Source | Relevance | URL | Reviewed |
|---|---|---|---|---|
| M2-TH-001 | D. F. Watson, Computing the n-dimensional Delaunay tessellation with application to Voronoi polytopes, The Computer Journal 24(2), 1981 | Independent incremental Delaunay/topological construction foundation | https://doi.org/10.1093/comjnl/24.2.167 | 2026-09-05 |
| M2-TH-002 | O. Devillers, M. Teillaud, Perturbations for Delaunay and weighted Delaunay 3D triangulations, Computational Geometry 44(3), 2011 | Lift-only symbolic perturbation, degenerate 3D deterministic subdivision, no flat tetrahedra, infinite-cell extension | https://doi.org/10.1016/j.comgeo.2010.09.010 | 2026-09-05 |
| M2-DOC-001 | CGAL Delaunay_triangulation_3 reference | Public conflict-hole connectivity/boundary semantics and finite/infinite sphere-side behavior; architecture/behavior cross-check only | https://doc.cgal.org/latest/Triangulation_3/classCGAL_1_1Delaunay__triangulation__3.html | 2026-09-05 |
| M2-TH-003 | J. R. Shewchuk, General-Dimensional Constrained Delaunay and Constrained Regular Triangulations, I: Combinatorial Properties, DCG 39, 2008 | General-dimensional Delaunay Lemma; local regular/Delaunay facet legality implies global regular/Delaunay triangulation | https://doi.org/10.1007/s00454-008-9060-3 | 2026-09-05 |
| M2-TH-004 | E. P. Muecke, A Robust Implementation for Three-Dimensional Delaunay Triangulations, IJCGA 8(2), 1998 | Robust 3D implementation history; symbolic perturbation; quadratic worst-case construction complexity context | https://doi.org/10.1142/S0218195998000138 | 2026-09-05 |
| M2-TH-005 | J. Erickson, Nice Point Sets Can Have Nasty Delaunay Triangulations, DCG 30(1), 2003 | 3D Delaunay complexity can be near/quadratic even for practically constrained/surface-like sets; resource-policy adversaries | https://doi.org/10.1007/s00454-003-2927-4 | 2026-09-05 |


## M6 early tetra-quality / FEM-correlation sources

M6 also reuses TH-013/TH-014 (tetra mesh improvement), TH-015 (Sliver Exudation),
TH-016 (TetGen) and the commercial quality-reporting references in M1.5.

| ID | Source | Relevance | URL | Reviewed |
|---|---|---|---|---|
| M6-TH-001 | A. Liu, B. Joe, Relationship between tetrahedron shape measures, BIT 34, 1994 | Radius ratio, mean ratio, solid-angle shape relationships | https://doi.org/10.1007/BF01955874 | 2026-09-05 |
| M6-TH-002 | V. N. Parthasarathy, C. M. Graichen, A. F. Hathaway, A comparison of tetrahedron quality measures, Finite Elements in Analysis and Design 15(3), 1994 | Comparative tetra metric behavior and computational considerations | https://doi.org/10.1016/0168-874X(94)90033-7 | 2026-09-05 |
| M6-TH-003 | P. M. Knupp, Algebraic Mesh Quality Metrics, SIAM Journal on Scientific Computing 23(1), 2001 | Weighted Jacobian, singular-value/condition metrics, distance to degeneracy, mean-ratio equivalence | https://doi.org/10.1137/S1064827500371499 | 2026-09-05 |
| M6-TH-004 | J. R. Shewchuk, What Is a Good Linear Element? Interpolation, Conditioning, and Quality Measures, 11th International Meshing Roundtable, 2002 | Element shape versus interpolation, stiffness conditioning and anisotropy | https://www.cs.cmu.edu/~jrs/jrspapers.html | 2026-09-05 |
| M6-TH-005 | S.-W. Cheng, T. K. Dey, H. Edelsbrunner, M. A. Facello, S.-H. Teng, Sliver Exudation, JACM 47(5), 2000 | Sliver pathology and finite weighted/regular-Delaunay quality treatment | https://doi.org/10.1145/355483.355487 | 2026-09-05 |
| M6-TH-006 | L. A. Freitag, P. M. Knupp, Tetrahedral mesh improvement via optimization of the element condition number, IJNME 53, 2002 | Condition-based tetra shape objective; average plus worst-quality optimization evidence | https://doi.org/10.1002/nme.341 | 2026-09-05 |
| M6-TH-007 | T. Sorgente et al., A Survey of Indicators for Mesh Quality Assessment, Computer Graphics Forum, 2023 | Modern taxonomy; metric blind spots; relation to FEM error/conditioning | https://doi.org/10.1111/cgf.14779 | 2026-09-05 |
| M6-FEM-001 | E. A. de Souza Neto, D. Peric, M. Dutko, D. R. J. Owen, Design of simple low order finite elements for large strain analysis of nearly incompressible solids, IJSS 33, 1996 | Large-strain nearly-incompressible element formulation / locking background | https://doi.org/10.1016/0020-7683(95)00259-6 | 2026-09-05 |
| M6-FEM-002 | E. Karabelas et al., An accurate, robust, and efficient finite element framework with applications to anisotropic, nearly and fully incompressible elasticity, CMAME, 2022 | Stabilized low-order elements; volumetric locking and mixed/stabilized formulation separation | https://pmc.ncbi.nlm.nih.gov/articles/PMC7612621/ | 2026-09-05 |
| M6-FEM-003 | J. A. Schönherr, P. Schneider, C. Mittelstedt, Robust hybrid/mixed finite elements for rubber-like materials under severe compression, Computational Mechanics 70, 2022 | Rubber-like quasi-incompressibility, volumetric locking and severe-distortion formulation behavior | https://doi.org/10.1007/s00466-022-02157-y | 2026-09-05 |
| M6-WATCH-001 | A. Quiriny, J. Lambrechts, N. Moës, V. Kučera, J.-F. Remacle, Taming Slivers: A Robust TFEM Framework for Reliable Computations on Degenerate Tetrahedral Meshes, 2026 preprint | Recent solver-side sliver research; research-watch only, not architecture authority | https://arxiv.org/abs/2606.14301 | 2026-09-05 |
| M6-FEM-004 | M. Křížek, *On the Maximum Angle Condition for Linear Tetrahedral Elements*, SIAM J. Numer. Anal. 29(2), 1992 | Tetrahedral maximum-angle condition; certain degenerating families retain standard linear interpolation-error order | https://doi.org/10.1137/0729031 | 2026-09-05 |
| M6-FEM-005 | R. E. Bank, L. R. Scott, *On the Conditioning of Finite Element Equations with Highly Refined Meshes*, SIAM J. Numer. Anal. 26(6), 1989 | Local refinement, nondegenerate meshes, natural basis scaling and global condition-number bounds | https://doi.org/10.1137/0726080 | 2026-09-05 |
| M6-FEM-006 | Q. Du, D. Wang, L. Zhu, *On Mesh Geometry and Stiffness Matrix Conditioning for General Finite Element Spaces*, SIAM J. Numer. Anal. 47(2), 2009 | Refined mesh-geometry/stiffness-conditioning relations for simplicial finite-element spaces | https://doi.org/10.1137/080718486 | 2026-09-05 |
| M6-ANG-001 | A. Hannukainen, S. Korotov, M. Křížek, *On Synge-type angle condition for d-simplices*, Applications of Mathematics 62(1), 2017 | Skinny/flat tetra taxonomy; face/dihedral angle-condition separation; degenerating needle/splinter/wedge interpolation examples | https://doi.org/10.21136/AM.2017.0132-16 | 2026-09-06 |
| M6-ANG-002 | S. Korotov, M. Křížek, V. Kučera, *On degenerating finite element tetrahedral partitions*, Numerische Mathematik 152, 2022 | Pathology-by-pathology maximum-angle behavior; interpolation counterexamples for spike/cap families | https://doi.org/10.1007/s00211-022-01317-9 | 2026-09-06 |
| M6-GEO-001 | A. Van Oosterom, J. Strackee, *The Solid Angle of a Plane Triangle*, IEEE Trans. Biomedical Engineering BME-30(2), 1983 | Robust atan2-style solid-angle formula for tetra vertex diagnostics | https://doi.org/10.1109/TBME.1983.325207 | 2026-09-06 |
| M6-TH-008 | H. Edelsbrunner, *Triangulations and meshes in computational geometry*, Acta Numerica 9, 2000 | Classical tetra mesh/pathology taxonomy and computational-geometry context | https://doi.org/10.1017/S0962492900001331 | 2026-09-06 |


## M6 local optimization / reconnection sources

This section extends the M6 metric sources with operation/scheduling research.

| ID | Source | Relevance | URL | Reviewed |
|---|---|---|---|---|
| M6-OPT-001 | B. Joe, Three-Dimensional Triangulations from Local Transformations, SIAM J. Sci. Stat. Comput. 10, 1989 | 2<->3 local transformations and 3D local triangulation theory | https://doi.org/10.1137/0910044 | 2026-09-05 |
| M6-OPT-002 | L. Freitag, C. Ollivier-Gooch, Tetrahedral Mesh Improvement Using Swapping and Smoothing, IJNME 40, 1997 | Face/edge swapping, smart/optimization smoothing, combined schedule evidence | https://ftp.mcs.anl.gov/pub/tech_reports/reports/P657.pdf | 2026-09-05 |
| M6-OPT-003 | L. Freitag, P. Plassmann, Local Optimization-Based Simplicial Mesh Untangling and Improvement, IJNME 49, 2000 | Local feasible region; max-min signed volume untangling via affine simplex volume | https://ftp.mcs.anl.gov/pub/tech_reports/reports/P749.pdf | 2026-09-05 |
| M6-OPT-004 | J. R. Shewchuk, Two Discrete Optimization Algorithms for the Topological Improvement of Tetrahedral Meshes, 2002 manuscript | Dynamic-programming edge removal and multi-face removal research | https://www.cs.cmu.edu/~jrs/jrspapers.html | 2026-09-05 |
| M6-OPT-005 | C. Marot, J.-F. Remacle, Quality tetrahedral mesh generation with HXT, 2020 | 2<->3/3<->2 limits, edge removal, composite operations, accepted Laplacian smoothing and improvement schedule | https://arxiv.org/abs/2008.08508 | 2026-09-05 |
| M6-OPT-006 | T. Munson, Mesh Shape-Quality Optimization Using the Inverse Mean-Ratio Metric, Argonne TM, 2004 | Aggregate inverse-mean-ratio optimization / r-refinement research | https://www.mcs.anl.gov/~tmunson/papers/shape.pdf | 2026-09-05 |
| M6-OPT-007 | Z. Chen, W. Wang, B. Levy, L. Liu, F. Sun, Revisiting Optimal Delaunay Triangulation for 3D Graded Mesh Generation, SISC 36(3), 2014 | ODT energy, Delaunay consistency, graded optimization and sliver-suppression research | https://doi.org/10.1137/120875132 | 2026-09-05 |
| M6-OPT-008 | B. M. Klingner, J. R. Shewchuk, *Aggressive Tetrahedral Mesh Improvement*, 16th International Meshing Roundtable, 2007 | Local-optimum search-space view, lexicographic quality vector, composite operations, rollback and operation scheduling | https://doi.org/10.1007/978-3-540-75103-8_1 | 2026-09-06 |
| M6-OPT-009 | J. Liu, S. Sun, *Small Polyhedron Reconnection: A New Way to Eliminate Poorly-Shaped Tetrahedra*, 15th International Meshing Roundtable, 2006 | SPR general cavity reconnection concept beyond elementary flips | https://doi.org/10.1007/978-3-540-34958-7_14 | 2026-09-06 |
| M6-OPT-010 | J. Liu, Y. Q. Chen, S. L. Sun, *Small polyhedron reconnection for mesh improvement and its implementation based on advancing front technique*, IJNME 79(8), 2009 | Practical SPR workflow; larger local reconnection after elementary transforms/smoothing | https://doi.org/10.1002/nme.2605 | 2026-09-06 |
| M6-OPT-011 | C. Marot, K. Verhetsel, J.-F. Remacle et al., *Reviving the Search for Optimal Tetrahedralizations*, 28th International Meshing Roundtable, 2020 | Branch-and-bound optimal fixed-cavity tetrahedralization, pruning heuristics and post-smoothing/edge-removal gains | https://doi.org/10.5281/zenodo.3653420 | 2026-09-06 |
