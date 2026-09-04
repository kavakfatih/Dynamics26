#pragma once

#include "femcae/meshing/MeshTypes.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace femcae::application {

// Preflight rigid-body restraint diagnostic input'u. Bu satırlar solver
// equation ID'si değil, reference mesh node kimliği ve global displacement
// component'i taşır. Prescribed value rank'i değiştirmediği için burada yoktur.
struct StructuralConstraintDof {
    meshing::MeshEntityId nodeId{meshing::InvalidMeshId};
    int component{0}; // 1=x, 2=y, 3=z
};

enum class StructuralStabilityError : std::uint8_t {
    None = 0,
    EmptyMesh,
    DuplicateNodeId,
    InvalidConnectivity,
    InvalidConstraint,
    NonFiniteCoordinate
};

struct StructuralComponentStability {
    std::size_t index{0}; // deterministic, one-based presentation index
    std::vector<meshing::MeshEntityId> nodeIds;
    std::vector<meshing::MeshEntityId> elementIds;
    int restraintRank{0};
    int freeRigidBodyModeCount{6};
    std::size_t constrainedDofCount{0};
    double characteristicLength{0.0};

    [[nodiscard]] bool stable() const noexcept { return restraintRank == 6; }
};

struct StructuralStabilityResult {
    StructuralStabilityError error{StructuralStabilityError::None};
    std::string detail;
    std::vector<StructuralComponentStability> components;

    [[nodiscard]] bool success() const noexcept
    {
        return error == StructuralStabilityError::None;
    }
    [[nodiscard]] bool stable() const noexcept;
};

// 3B displacement mesh'inin infinitesimal rigid-body hareketlerini yalnız
// reference configuration ve kinematic constraints üzerinden değerlendirir.
// Bu diagnostic stiffness pivotu/definiteness hakkında hüküm vermez ve modele
// weak spring ya da automatic constraint eklemez.
class StructuralStabilityDiagnostic final {
public:
    [[nodiscard]] static StructuralStabilityResult evaluate(
        const meshing::SimulationMesh &mesh,
        const std::vector<StructuralConstraintDof> &constraints);
};

[[nodiscard]] const char *structuralStabilityErrorMessage(
    StructuralStabilityError error) noexcept;

} // namespace femcae::application
