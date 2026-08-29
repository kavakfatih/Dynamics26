# FEMCAE V0.13.0 Release Notes

## Meshing + Full Pre/Post Integration Baseline

V0.13.0, CAD/Geometry katmanı ile solver arasına bağımsız bir FEM mesh ve pre/post katmanı ekler.

### Yeni özellikler

- `femcae_meshing` C++20 library.
- Structured HEX8 box mesher ve deterministic FEM IDs.
- CAD body/face -> FEM element/facet/node provenance.
- Global ve face-local structured sizing baseline.
- Center scaled-Jacobian/aspect-ratio quality report.
- Abaqus ASCII `*NODE` + `C3D8` import baseline.
- Geometry-targeted material/section/load/constraint/contact metadata.
- Boundary-facet based assignment resolution.
- Generic `fem_solve_linear_hex8_mesh` public C ABI.
- Result database, nearest-node probe, plane-cut element selection.
- CSV ve legacy VTK result export.
- Qt `Mesh / Pre-Post` source panel ve VTK deformed/von-Mises visualization.
- Native OCCT axis-aligned STEP box -> structured mesh provenance verification source.

### Verification

Portable final snapshot:

```text
Debug   118/118 PASS
Release 118/118 PASS
```

Installed CLI, installed C API consumer ve installed C++ `femcae_meshing` consumer PASS.

### Sınırlar

V0.13, arbitrary curved CAD için production unstructured tetra/hex mesher değildir. External Abaqus reader yalnız geometry connectivity baseline'dır. Plane cut yalnız intersected elements üretir. Generic C ABI şu aşamada linear HEX8 ile sınırlıdır.

Native macOS Qt/VTK/OCCT/Accelerate execution GitHub `macos-15` release gate'idir.
