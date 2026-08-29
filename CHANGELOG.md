# Changelog

## [1.0.2] - 2026-08-29

### Added
- Deterministic/reproducible source ZIP generator and internal SHA256 manifest.
- Repository hygiene gate and first-publication GitHub bootstrap helper.
- CTest repository/reproducibility test with temporary real Git commit/origin creation.
- GitHub Actions source-integrity gate, CI concurrency and source artifact generation in signed release workflow.

### Changed
- Version raised to 1.0.2.
- Shared library VERSION/SOVERSION derive from CMake PROJECT_VERSION.
- CMake geometry target is defined before meshing target to guarantee a real target dependency.

### Validation
- Portable Debug 124/124 and Release 124/124 PASS.
- Installed CLI/C API/C++ consumers PASS.
- Clean meshing-only target dependency build PASS.
- Source compiler warnings: 0.
- No remote FEMCAE repository was visible through the connected GitHub app; native GitHub CI remains pending.

## [1.0.1] - 2026-08-29

### Changed
- Hardened macOS release engineering without changing FEM formulations.
- Added Qt project-schema migration for recognized legacy schema-less FEMCAE project JSON.
- Added arm64-only bundle/RPATH/dyld smoke audit and protected signed/notarized macOS release workflow.
- GUI version now derives from CMake project version rather than a duplicate literal.

### Validation
- Portable Debug 123/123 and Release 123/123 tests pass.
- Native Qt migration/signing/notarization remain macOS CI evidence gates.

## [1.0.0] - 2026-08-29

### Added

- Versioned, checksum-protected nonlinear disk checkpoint/restart I/O with exact real64 bit-pattern serialization.
- V1.0 HEX8 cantilever mesh-convergence verification against an independent Euler-Bernoulli reference.
- Corrupted Abaqus/DXF/checkpoint input rejection tests and public C API invalid-input tests.
- Deterministic linear HEX8 performance smoke baseline.
- AddressSanitizer + UndefinedBehaviorSanitizer release-gate validation path.
- macOS `.app` deployment infrastructure using Qt deployment plus CMake BundleUtilities dependency fixup.
- Mach-O architecture/rpath/dependency audit script.
- Developer ID signing + `notarytool` + stapler notarization script infrastructure.
- Apache-2.0 project LICENSE, NOTICE, third-party license inventory and release notices.
- V1.0 release checklist, verification matrix, security policy and performance baseline documents.

### Changed

- Application/library version raised to 1.0.0; numeric and string contracts are synchronized.
- macOS CI adds release, convergence, checkpoint-I/O, corrupted-file and performance gates.
- GUI project loader rejects oversized, malformed, non-object and schema-mismatched JSON before applying state.
- macOS app RPATH points into `Contents/Frameworks`; deployment scripts bundle non-system runtime dependencies.

### Verified

- Portable Debug: 123/123 PASS.
- Portable Release: 123/123 PASS.
- V1 release gates under ASan+UBSan: 5/5 PASS.
- Installed CLI, C API consumer and C++ geometry/meshing consumer: PASS.
- Release source compiler warnings: 0 after checkpoint integer-kind hardening.

### Open native release gates

- Native macOS arm64 Qt/VTK/OCCT/Accelerate workflow execution.
- Final `.app` `otool`/codesign audit on Apple Silicon.
- Developer ID signing and Apple notarization with real credentials.
- Final binary-bundle third-party license-file inventory.
- Native GUI project migration and Apple Instruments/Leaks audit.

## [0.13.0] - 2026-08-29

### Added

- Separate C++20 `femcae_meshing` library and solver-independent `SimulationMesh`.
- Structured axis-aligned HEX8 mesher with deterministic node/element/facet IDs and CAD provenance.
- Global/face-local structured sizing baseline and center scaled-Jacobian/aspect-ratio quality metrics.
- Portable Abaqus ASCII `*NODE` + `C3D8` external mesh import baseline.
- Geometry-targeted material/section/load/constraint/contact metadata and boundary-facet assignment resolution.
- Geometry-to-mesh provenance bridge into the V0.12 `GeometryAssociationMap`.
- Generic `fem_solve_linear_hex8_mesh` C ABI for arbitrary linear HEX8 node/connectivity/BC/load data.
- ResultDatabase with displacement/scalar fields, nearest-node probe, plane-cut element selection, CSV and legacy VTK export.
- Qt Mesh/Pre-Post source panel and VTK deformed HEX8/von-Mises source visualization path.
- Native OCCT axis-aligned STEP box -> structured HEX8 provenance verification source.
- Six portable V0.13 tests; native OCCT build adds one STEP-to-mesh verification.

### Changed

- Application version raised to 0.13.0; numeric/string version constants are synchronized.
- macOS CI adds meshing/prepost/provenance/external-mesh gates and installed C++ meshing consumer smoke.
- Model/result tree separates CAD geometry, FEM mesh and result objects.

### Scope / Limitations

- V0.13 is not a production general-purpose curved CAD tetra/hex mesher.
- Structured local sizing is not unstructured adaptive refinement.
- Abaqus reader imports only NODE/C3D8 geometry/connectivity.
- Generic mesh C ABI is linear HEX8 only.
- Plane section cut selects intersected elements; it does not generate an interpolated cut-surface contour.


## [0.12.0] - 2026-08-29

### Added

- Separate C++20 `femcae_geometry` CAD/section library.
- GeometryDocument hierarchy with deterministic 64-bit persistent geometry IDs.
- Explicit CAD geometry / display tessellation / FEM mesh separation contract.
- Geometry-to-FEM provenance association map.
- Optional OpenCASCADE XDE/STEPCAF STEP adapter and display tessellation path.
- macOS CI hard gate `FEMCAE_REQUIRE_OCCT=ON` plus conditional native STEP box verification.
- Portable ASCII DXF section reader for LWPOLYLINE, LINE, CIRCLE and ARC.
- Green-theorem section area, centroid, inertia, principal-axis and polar area-moment calculations.
- Qt GeometryPanel and VTK display-tessellation source integration.
- Seven portable CAD/geometry/section tests; native OCCT adds one STEP verification.

### Changed

- Application version raised to 0.12.0; project/result/C-API schema versions remain unchanged.
- GUI project JSON optionally persists STEP and DXF source paths.
- Homebrew native CAD baseline uses the available `opencascade` package; OCCT is not vendored into the source ZIP.

### Fixed

- Nested concentric DXF contours are classified using enclosing contours of larger area, avoiding outer-loop misclassification as a hole.

### Scope / Limitations

- Display tessellation is not a FEM mesh.
- Persistent geometry IDs are not a full CAD-edit topological naming solution.
- `Jp=Ixx+Iyy` is not claimed as a general Saint-Venant torsion constant.
- CAD-to-mesher and full pre/post remain V0.13 work.

## [0.11.0] - 2026-08-29

### Added

- Rigid planar QUAD4 master facet + deformable slave-node contact subsystem.
- Expanded-AABB broad-phase and closest-point narrow-phase search.
- Signed normal gap and penalty contact enforcement.
- Incremental augmented-Lagrangian normal multiplier with committed-gap history.
- Coulomb friction with stick/slip state and analytic contact tangent.
- Contact trial/commit/revert state registry and nonlinear rollback integration.
- Global contact force/tangent assembly and contact-result summaries.
- Five V0.11 verification problems covering tangent, AL state, equilibrium, rollback and global Coulomb friction.
- `fem_demo_contact_hex8` additive C ABI preset.
- Qt/VTK source-level contact verification controls and visualization.

### Changed

- Contact models reject CG backend; direct/reference solver is required by the V0.11 baseline.
- Contact + checkpoint restart is explicitly rejected until contact history serialization is implemented.
- Application version raised to 0.11.0; project/result/C-API schema versions remain unchanged.

### Fixed

- Augmented-Lagrangian re-evaluation no longer double-increments a committed multiplier at the same configuration; update is based on trial-minus-committed gap.

## [0.10.0] - 2026-08-29

### Added / Changed

- Added element-associated `pressure_p0` field and Q1/P0 mixed HEX8 local DOF mapping.
- Added perturbed-Lagrangian mixed functional `W_iso + p(J-1) - p^2/(2K)` with analytic `K_uu`, `K_up`, `K_pu`, `K_pp`.
- Added global sparse mixed assembly and block-aware displacement/pressure convergence norms.
- Added explicit rejection of Conjugate Gradient for symmetric-indefinite mixed tangents.
- Added element-ID based P0 pressure result recovery.
- Added mixed C ABI manufactured simple-shear verification preset and Qt source-level formulation selection.
- Added local/global tangent finite-difference verification, mixed Newton simple shear, error-path tests and a nearly-incompressible locking benchmark.
- Extended macOS CI with `mixed-up`, `incompressibility`, `locking` and `pressure` gates.

### Scope / Limitations

- Baseline mixed element is HEX8/Q1 displacement with one constant P0 pressure DOF per element.
- It is a controlled engineering baseline, not a claim of universal inf-sup stability for arbitrary meshes.
- Fully incompressible `K=∞`, stabilized Q1/Q1, higher-order mixed elements and contact remain future work.

## [0.9.0] - 2026-08-29

### Added / Changed

- Added common constitutive material-point response contract with explicit stress/tangent measures.
- Added Neo-Hookean, Mooney-Rivlin, Yeoh and 1–3 term Ogden hyperelastic models.
- Added isochoric/volumetric energy split and analytic/consistent hyperelastic tangents.
- Added signed-term Ogden validation with positive `sum(mu_i)` initial-shear gate and near-repeated spectral tangent verification.
- Integrated hyperelastic response into Total-Lagrangian HEX8, global nonlinear assembly and V0.8 Newton solver.
- Added small-strain J2 von Mises plasticity with isotropic hardening, radial return, committed/trial state and consistent algorithmic tangent.
- Added hyperelastic C API validation and isochoric uniaxial preview functions.
- Added Qt Material Studio source with model-sensitive parameters, units, engine validation, `G0` display and preview curve.
- Added eight V0.9 verification programs covering tangent, energy/stress, homogeneous deformation, element tangent, Newton equilibrium, J2 return mapping and Ogden spectral robustness.
- Extended macOS CI labels for hyperelastic, plasticity and constitutive gates.

Bu proje [Semantic Versioning](https://semver.org/) mantigini kullanir.

## [0.8.0] - 2026-08-29

### Added
- Full and modified Newton nonlinear static solver over the V0.7 residual/tangent evaluator.
- Residual, displacement-correction and optional energy convergence criteria.
- Backtracking line search, load stepping, adaptive increment, cutback/retry and rollback/commit semantics.
- Nonlinear convergence history and in-memory checkpoint/restart continuation.
- C API Total-Lagrangian HEX8 nonlinear preset with history export.
- Qt Nonlinear Static / Large Displacement source workflow and convergence table.
- VER-V080-001 full Newton finite stretch, VER-V080-002 modified Newton and VER-V080-003 checkpoint/restart verification.

### Changed
- Nonlinear system evaluation accepts an optional load factor and scales external load consistently.
- Project and GUI application version advanced to 0.8.0.
- GUI project JSON can persist nonlinear solver settings.

### Hardening / Scope
- Failed increments revert to the last committed state before cutback/retry.
- Invalid convergence configurations and unsupported nonzero prescribed-displacement load-control cases fail explicitly.
- StVK remains a geometric-nonlinearity verification material; hyperelastic rubber models remain V0.9 scope.
- Checkpoint is in-memory only; durable restart schema is not claimed.
- Native macOS arm64 Qt/VTK/Accelerate execution remains a CI gate.

## [0.7.0] - 2026-08-29

### Added

- Reference/current configuration finite-strain kinematics.
- Deformation gradient `F`, determinant `J`, Green-Lagrange and Euler-Almansi strain utilities.
- PK2 → PK1, Kirchhoff and Cauchy stress transformations.
- `STRESS_FIRST_PIOLA_KIRCHHOFF` stress-measure metadata.
- St. Venant-Kirchhoff Total-Lagrangian reference constitutive response.
- `TOTAL_LAGRANGIAN_HEX8` element formulation.
- Nonlinear Green-Lagrange B matrix, internal-force vector, material tangent and geometric tangent.
- Sparse global nonlinear system evaluator producing `f_int`, `f_ext`, `R=f_ext-f_int` and tangent at a trial state.
- Global nonlinear displacement trial/commit/revert baseline.
- Reference/current follower-load metadata contract.
- Finite-strain affine patch, rigid-rotation objectivity, superposed-rotation objectivity, finite-stretch and element/global tangent finite-difference verification.

### Scope note

- V0.7 does not include Newton-Raphson, line search, adaptive load stepping or a nonlinear GUI workflow; these are V0.8 scope.
- StVK is a verification/reference material for geometric nonlinearity and is not claimed as a valid large-strain rubber model. Hyperelastic models remain V0.9 scope.
- Follower-load metadata is present, but actual surface pressure/traction integration is not yet implemented.

## [0.6.0] - 2026-08-29

### Added

- Consistent/lumped mass matrices for TRUSS2, 2D beam, plane QUAD4, axisymmetric QUAD4 and HEX8.
- Generalized symmetric eigenproblem transformation, normalization and residual utilities.
- Vendor-independent dense reference eigensolver.
- Functional ARPACK-NG `dsaupd/dseupd` backend.
- macOS Accelerate/LAPACK `DSYGV` eigen backend source.
- General `solve_modal_analysis()` driver and full-DOF mode reconstruction.
- Element orientation-frame metadata and model-level frame validation.
- C API two-element axial modal preset.
- Qt modal frequency UI, mode selector and VTK mode-shape animation.
- `VER-V060-001` axial modal and `VER-V060-002` beam modal verification.
- Modal error-path tests and macOS CI ARPACK/modal gate.

### Hardened

- Requested positive mode count is validated instead of silently returning fewer modes.
- Nonzero prescribed displacement is rejected in free-vibration analysis.
- Mass matrix must satisfy symmetry/SPD requirements at the generalized eigen boundary.

### Scope note

- Global K/M assembly is sparse, but V0.6 generalized eigen backends use a dense matrix representation at the eigensolver boundary. Sparse shift-invert/factorization reuse is deferred; no large-model production modal performance claim is made.
- Shell orientation infrastructure is present, but shell stiffness/modal formulation is not complete.

## [0.5.0] - 2026-08-29

### Added

- Isotropic linear-elastic material and structural section registries.
- DOF-ID based nodal loads.
- TRUSS2, 2D Euler-Bernoulli beam, plane stress/strain QUAD4, axisymmetric QUAD4 and HEX8 formulations.
- Stress/strain recovery and von Mises helpers, including plane-strain `sigma_zz`.
- Multi-field/component local DOF mapping.
- General `solve_linear_static()` analysis driver.
- Topology/formulation and section-kind compatibility validation.
- First Qt 6 desktop GUI source target behind the stable C ABI.
- Optional VTK native Qt viewport and axial result visualization.
- Project New/Open/Save, model tree, material/section/load/BC/analysis panels and result/reaction views.
- Dedicated macOS arm64 Qt/VTK GUI CI job.
- Three V0.5 verification problems and V0.5 analysis error-path tests.

### Scope note

- The core is a general linear structural solver for the listed formulations.
- The V0.5 GUI solve path is an integration preview using an axial-bar C-API preset; arbitrary mesh/model editing and generic model-handle C API are not claimed as complete.

## [0.4.0] - 2026-08-29

### Added

- Element DOF/equation maps with prescribed-value retention.
- Dense-adjacency-free sparsity graph and CSR matrix storage.
- Generic matrix/vector local-to-global scatter.
- Nonzero essential-BC RHS correction and DOF-ID based reaction recovery.
- Matrix symmetry/definiteness metadata.
- Backend-independent linear-solver facade.
- Dense partial-pivoting reference solver.
- Jacobi-preconditioned sparse Conjugate Gradient solver.
- Apple Accelerate Sparse direct-solver C adapter behind ISO_C_BINDING.
- Minimal 3D TRUSS2 local stiffness for assembled verification.
- Two-bar assembled analytical verification and reaction-equilibrium check.
- V0.4 assembly/solver error-path tests.

### Fixed

- Removed an invalid Fortran short-circuit assumption in sparsity pair de-duplication that Debug runtime checks could expose as an out-of-bounds access.

### Preserved contracts

- Assembly remains independent from platform/vendor solver APIs.
- `Node ID != Array Index != DOF ID != Equation ID`.
- V0.1-V0.3 numerical, state, schema and C-ABI contracts.

## [0.3.0] - 2026-08-29

### Added

- BAR2, QUAD4 and HEX8 reference-element/natural-coordinate contracts.
- Shape functions and natural gradients.
- 1D/2D/3D Gauss-Legendre tensor-product quadrature.
- Isoparametric mapping, Jacobian/inverse Jacobian and physical gradients.
- Embedded 3D TRUSS2 line metric and direction.
- Plane, solid and axisymmetric small-strain B-matrix helpers.
- Axisymmetric `2*pi*r` integration measure.
- Element result and quality containers.
- Topology/formulation separated element registry.
- Beam/shell prototype rotation and section metadata.
- Four affine element patch tests.

### Numerical hardening

- Jacobian singularity detection made dimensionless and scale-independent.
- Inverted-element detection separated from singularity detection.

### Preserved contracts

- V0.1/V0.2 numerical, state, ID/DOF/equation, schema and C-ABI contracts.

## [0.2.0] - 2026-08-29

### Added

- Node/element persistent-ID mesh storage and connectivity validation.
- BAR2, QUAD4 and HEX8 topology registry.
- Node/element set registry.
- Displacement, pressure and rotation field registry.
- Independent DOF-ID storage by entity/field/component address.
- DOF-ID based essential constraints.
- Deterministic free-DOF equation numbering.
- Local coordinate-frame validation and registry.
- Element material/section ID links.
- `model_t` analysis model aggregate.
- V0.2 model, mixed-field and numbering regression tests.
- V0.2 architecture and release documentation.

### Preserved contracts

- `Node ID != Array Index != DOF ID != Equation ID`.
- Project/result/C API schema versions remain `1`.
- Residual, Newton, Voigt and trial/commit/revert contracts remain unchanged.

## [0.1.1] - 2026-08-29

### Changed

- macOS arm64 CI Debug + Release matrisi olarak sertlestirildi.
- Runner ve binary architecture kontrolleri gercek fail gate haline getirildi.
- Install-layout ve installed C API consumer smoke testleri CI akimina eklendi.
- Uygulama patch surumu 0.1.1'e cikarildi; project/result/C API schema surumleri degistirilmedi.
- Roadmap, mixed u-p, contact/friction, CAD ve meshing adimlarini daha yonetilebilir release'lere ayiracak sekilde revize edildi.

### Added

- `VERSION_ROADMAP.md` kisa surum ve kullanilabilirlik ozeti.
- `unit_tolerances` testi.
- `unit_error_paths` testi.
- `RELEASE_NOTES_V0.1.1.md`.

## [0.1.0] - 2026-08-29

### Added

- Modern Fortran 2018 numerical foundation.
- `real64`, persistent ID ve solver-index tur politikalari.
- Status/error ve logger altyapisi.
- Tolerance ve version/schema metadata.
- Matrix-property, stress-measure ve result-location metadata.
- Trial/commit/revert state buffer.
- Vector, matrix ve tensor/Voigt math modulleri.
- Stable C ABI'nin ilk version/schema sorgu fonksiyonlari ve public C/C++ header.
- Gercek C translation unit ile ABI smoke testi.
- CLI smoke-test uygulamasi.
- Unit, verification ve regression CTest altyapisi.
- `VER-V010-001` axial-bar algebraic verification harness.
- macOS arm64 GitHub Actions workflow.
- Architecture Decision Record seti.
- Code_Aster source-independence policy.

### Architectural contracts frozen

- `R = f_ext - f_int`
- `K_T * du = R`
- Voigt: `XX, YY, ZZ, XY, YZ, XZ`
- strain Voigt'te engineering shear
- trial/commit/revert nonlinear state semantics
- GUI/solver siniri: stable C API
- project/result/API surumlerinin ayrilmasi
