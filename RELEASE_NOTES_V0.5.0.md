# FEMCAE V0.5.0 Release Notes

## Linear Structural FEM

- Isotropic linear-elastic material library.
- Truss / plane / beam section registry.
- DOF-ID based nodal loads.
- TRUSS2, 2D Euler–Bernoulli beam, plane-stress QUAD4, plane-strain QUAD4, axisymmetric QUAD4 and HEX8 stiffness formulations.
- Plane / axisymmetric / 3D strain-stress recovery helpers.
- Plane-strain out-of-plane `sigma_zz` recovery and a dedicated plane-strain von Mises helper.
- Von Mises result helpers.
- Generic multi-field/component element DOF mapping.
- Generic linear-static analysis driver from model to displacements/reactions.

## Architecture hardening

- `Topology != Formulation` is enforced.
- Material, section and formulation IDs remain separate.
- Incompatible section kind is rejected before solve.
- Active DOFs not used by any element formulation are rejected before matrix solve.

## First Qt GUI

- Qt 6 app-bundle source target.
- C ABI only engine boundary.
- New/Open/Save project UI.
- Model tree and property editors.
- Load/BC and analysis panels.
- Solve/result/reaction/log UI.
- Optional VTK `QVTKOpenGLNativeWidget` viewport.
- Gerçek assembled TRUSS2 kullanan axial-bar C-API solve preset with first deformed/stress-colored visualization.
- macOS install RPATH and dedicated GUI CI job.

## Verification

- Plane-stress QUAD4 verification.
- 2D cantilever beam analytical verification.
- Generic high-level linear-static driver verification.
- New analysis error-path tests.

## Known V0.5 GUI limitation

The general Fortran core supports the listed structural formulations. The GUI solve path does not yet expose arbitrary mesh/model construction through an opaque C API; the interactive solve preview is an axial-bar preset. Full arbitrary-model pre/post remains future scope.
