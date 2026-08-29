# FEMCAE V0.12.0 — Release Notes

V0.12 solver özelliklerini büyütmek yerine CAE model hazırlama katmanını kurar. Ana sözleşme `CAD Geometry != Display Tessellation != FEM Mesh` ayrımıdır.

## Added

- Ayrı C++20 `femcae_geometry` library.
- GeometryDocument ve 64-bit deterministic geometry IDs.
- Body/face/edge/vertex hierarchy ve geometry provenance association map.
- Optional OCCT XDE/STEPCAF STEP adapter.
- OCCT B-Rep display tessellation data path.
- Native macOS `FEMCAE_REQUIRE_OCCT` release gate.
- Portable ASCII DXF custom-section reader: LWPOLYLINE, LINE, CIRCLE, ARC.
- Green-theorem section properties: A, centroid, Ixx/Iyy/Ixy, principal moments/axis, polar area moment.
- Qt GeometryPanel: STEP import, geometry tree/filter, DXF section import, property summary.
- VTK CAD display tessellation viewport path.
- Optional STEP/DXF source paths in GUI project JSON.
- Seven portable V0.12 tests plus one conditional native OCCT STEP verification.

## Fixed during development

- Eş merkezli nested DXF contour'larda centroid-probe yaklaşımı outer loop'u yanlış hole sınıflandırabiliyordu. Nesting artık yalnız daha büyük enclosing contour'ları sayar.

## Scope boundaries

- Display triangulation kesinlikle FEM mesh değildir.
- Full CAD edit persistent naming yoktur.
- LWPOLYLINE bulge desteklenmez; explicit ARC kullanılır.
- Polar area moment `Jp`, genel Saint-Venant torsion constant değildir.
- CAD-to-mesher ve tam pre/post V0.13 kapsamındadır.


## Portable release validation

```text
Debug CTest                  : 112/112 PASS
Release CTest                : 112/112 PASS
Installed CLI                : PASS
Installed C API consumer     : PASS
Installed Geometry header    : PASS
Installed geometry library   : PASS
Installed Geometry C++ consumer: PASS
Source compiler warnings     : 0
CI YAML parse                : PASS
```

Gerçek OCCT STEP/XDE ve Qt/VTK macOS execution bu Linux hostunda PASS kabul edilmez; `macos-15` native CI release gate'i açık kalır.
