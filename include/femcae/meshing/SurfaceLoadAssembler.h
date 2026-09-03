#pragma once

// Reference-configuration surface load assembly.
//
// Total Force seçili surface scope'un tamamı için tek resultant'tır:
//   t_ref = F_total / A_ref
//   f_e   = integral_Gamma_e N^T t_ref dGamma
//
// Display tessellation/glyph verisi bu API'ye giremez. Yalnız gerçek FEM
// boundary facet'leri, onların QUAD4 node kimlikleri ve CAD Face provenance'ı
// tüketilir.

#include "femcae/meshing/MeshTypes.h"

#include <vector>

namespace femcae::meshing {

enum class SurfaceLoadAssemblyError {
    None = 0,
    EmptyScope,
    InvalidScope,
    NoMatchingFacets,
    InvalidFacet,
    MissingNode,
    DegenerateFacet,
    NonFiniteInput
};

struct NodalVectorLoad {
    MeshEntityId nodeId{InvalidMeshId};
    geometry::Vec3 value;
};

struct SurfaceLoadAssemblyResult {
    SurfaceLoadAssemblyError error{SurfaceLoadAssemblyError::None};
    double referenceArea{0.0};
    // integral_Gamma X dGamma; equilibrium/moment verification için saklanır.
    geometry::Vec3 referenceFirstMoment;
    geometry::Vec3 uniformReferenceTraction;
    std::vector<NodalVectorLoad> nodalLoads;

    [[nodiscard]] bool success() const noexcept
    {
        return error == SurfaceLoadAssemblyError::None && referenceArea > 0.0
            && !nodalLoads.empty();
    }

    [[nodiscard]] geometry::Vec3 resultant() const noexcept;
};

[[nodiscard]] SurfaceLoadAssemblyResult assembleUniformTotalForce(
    const SimulationMesh &mesh,
    const std::vector<geometry::GeometryEntityId> &geometryFaceIds,
    const geometry::Vec3 &totalForce);

[[nodiscard]] const char *surfaceLoadAssemblyErrorMessage(
    SurfaceLoadAssemblyError error) noexcept;

} // namespace femcae::meshing
