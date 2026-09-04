#pragma once

#include "femcae/meshing/MeshTypes.h"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace femcae::meshing {

// Derived result field semantics. This metadata belongs to solver/recovery output;
// it is deliberately not part of persistent ResultDefinition / project Undo state.
enum class ResultAssociation {
    Unknown,
    Node,
    Element
};

struct ResultFieldMetadata {
    std::string physicalQuantity;
    std::string measure;
    ResultAssociation association{ResultAssociation::Unknown};
    std::string sourceLocation;
    std::string recoveryMethod;
    std::string storageUnit;
    std::string displayUnit;
};

struct NodeVectorField {
    std::string name;
    ResultFieldMetadata metadata;
    std::unordered_map<MeshEntityId, geometry::Vec3> values;
};

struct ElementScalarField {
    std::string name;
    ResultFieldMetadata metadata;
    std::unordered_map<MeshEntityId, double> values;
};

struct ProbeResult {
    MeshEntityId nodeId{InvalidMeshId};
    geometry::Vec3 location;
    geometry::Vec3 vectorValue;
    double distance{0.0};
};

struct PlaneCut {
    geometry::Vec3 point;
    geometry::Vec3 normal;
    double tolerance{1.0e-9};
};

class ResultDatabase {
public:
    void clear();
    void setDisplacement(NodeVectorField field);
    void setElementScalar(ElementScalarField field);
    [[nodiscard]] const NodeVectorField* displacement() const noexcept;
    [[nodiscard]] const ElementScalarField* elementScalar(std::string_view name) const noexcept;
    [[nodiscard]] std::optional<ProbeResult> probeNearestNode(const SimulationMesh& mesh,
                                                              const geometry::Vec3& point) const;
    [[nodiscard]] std::vector<MeshEntityId> cutElements(const SimulationMesh& mesh,
                                                        const PlaneCut& plane) const;
    void exportCsv(const SimulationMesh& mesh, const std::filesystem::path& path) const;
    void exportLegacyVtk(const SimulationMesh& mesh, const std::filesystem::path& path,
                         double deformationScale = 1.0) const;
private:
    std::optional<NodeVectorField> displacement_;
    std::vector<ElementScalarField> elementScalars_;
};

} // namespace femcae::meshing
