#include <femcae/geometry/OcctStepImporter.h>
#include <femcae/meshing/GeometryMeshBridge.h>
#include <femcae/meshing/StructuredHexMesher.h>
#include <BRepPrimAPI_MakeBox.hxx>
#include <IFSelect_ReturnStatus.hxx>
#include <STEPControl_StepModelType.hxx>
#include <STEPControl_Writer.hxx>
#include <filesystem>
#include <iostream>

namespace {
int fail(const char* message) {
    std::cerr << "VER-V130-002 FAIL: " << message << '\n';
    return 1;
}
}

int main() {
    using namespace femcae::geometry;
    using namespace femcae::meshing;

    const auto path = std::filesystem::temp_directory_path() / "femcae_v130_box.step";
    STEPControl_Writer writer;
    const auto box = BRepPrimAPI_MakeBox(2.0, 1.0, 0.5).Shape();
    if (writer.Transfer(box, STEPControl_AsIs) != IFSelect_RetDone) return fail("STEP transfer failed");
    if (writer.Write(path.string().c_str()) != IFSelect_RetDone) return fail("STEP write failed");

    GeometryDocument doc("v130-step-box");
    OcctStepImporter importer;
    const auto imported = importer.importFile(path.string(), doc);
    std::filesystem::remove(path);
    if (!imported.success) {
        std::cerr << "VER-V130-002 import message: " << imported.message << '\n';
        return fail("STEP import failed");
    }

    const auto bodies = doc.entitiesOfKind(GeometryEntityKind::Body);
    if (bodies.size() != 1) return fail("expected exactly one imported body");

    const auto descriptor = importer.axisAlignedBoxDescriptor(bodies.front());
    if (!descriptor) return fail("axis-aligned STEP box descriptor could not be recovered");

    StructuredHexMesherOptions options;
    options.nx = 2;
    options.ny = 1;
    options.nz = 1;
    StructuredHexMesher mesher;
    BoxBoundaryGeometry boundary{
        descriptor->bodyId,
        descriptor->xMinFace,
        descriptor->xMaxFace,
        descriptor->yMinFace,
        descriptor->yMaxFace,
        descriptor->zMinFace,
        descriptor->zMaxFace
    };
    const auto mesh = mesher.meshBox({descriptor->min, descriptor->max}, boundary, doc.revision(), options);
    const auto map = buildGeometryAssociationMap(mesh);

    if (mesh.elements.size() != 2) return fail("expected two HEX8 elements");
    if (mesh.sourceGeometryRevision != doc.revision()) return fail("geometry revision provenance mismatch");
    const auto* xMin = map.find(descriptor->xMinFace);
    const auto* xMax = map.find(descriptor->xMaxFace);
    if (xMin == nullptr || xMax == nullptr) return fail("x-face provenance missing");
    if (xMin->femFacetIds.size() != 1) return fail("unexpected x-min boundary facet count");

    std::cout << "V0.13 native STEP box -> CAD face provenance -> HEX8 mesh PASS\n";
    return 0;
}
