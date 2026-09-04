#pragma once

#include "femcae/meshing/MeshTypes.h"

#include <filesystem>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace femcae::meshing {

// Bir field'in adı tek başına mühendislik anlamını taşımaz. Bu eksenler
// solver recovery'sini viewport sunumundan ayırır ve gelecekte nodal
// extrapolation gibi alternatiflerin açıkça temsil edilmesini sağlar.
enum class ResultPhysicalQuantity : std::uint8_t {
    Unknown = 0,
    Displacement = 1,
    Stress = 2,
    ReactionForce = 3
};

enum class ResultMeasure : std::uint8_t {
    Unknown = 0,
    Magnitude = 1,
    CauchyVonMises = 2,
    Vector = 3
};

enum class ResultAssociation : std::uint8_t {
    Unknown = 0,
    Node = 1,
    Element = 2
};

enum class ResultSourceLocation : std::uint8_t {
    Unknown = 0,
    MeshNode = 1,
    IntegrationPoints = 2,
    ConstrainedDegreesOfFreedom = 3
};

enum class ResultRecoveryMethod : std::uint8_t {
    Unknown = 0,
    Direct = 1,
    ArithmeticMean = 2,
    EquilibriumRecovery = 3
};

enum class ResultUnit : std::uint8_t {
    Unitless = 0,
    Meter = 1,
    Millimeter = 2,
    Pascal = 3,
    MegaPascal = 4,
    Newton = 5
};

enum class ResultConfiguration : std::uint8_t {
    Unknown = 0,
    Reference = 1,
    FinalConverged = 2
};

struct ResultFieldMetadata {
    ResultPhysicalQuantity quantity{ResultPhysicalQuantity::Unknown};
    ResultMeasure measure{ResultMeasure::Unknown};
    ResultAssociation association{ResultAssociation::Unknown};
    ResultSourceLocation sourceLocation{ResultSourceLocation::Unknown};
    ResultRecoveryMethod recovery{ResultRecoveryMethod::Unknown};
    ResultUnit storageUnit{ResultUnit::Unitless};
    ResultUnit displayUnit{ResultUnit::Unitless};
    ResultConfiguration configuration{ResultConfiguration::Unknown};
    int integrationPointCount{0};
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

struct ElementProbeResult {
    MeshEntityId facetId{InvalidMeshId};
    MeshEntityId elementId{InvalidMeshId};
    double scalarValue{0.0};
};

struct VectorResultant {
    geometry::Vec3 value;
    geometry::Vec3 centroid;
    std::size_t nodeCount{0};
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
    void setReaction(NodeVectorField field);
    void setElementScalar(ElementScalarField field);
    [[nodiscard]] const NodeVectorField* displacement() const noexcept;
    [[nodiscard]] const NodeVectorField* reaction() const noexcept;
    [[nodiscard]] const ElementScalarField* elementScalar(std::string_view name) const noexcept;
    [[nodiscard]] std::optional<ProbeResult> probeNearestNode(const SimulationMesh& mesh,
                                                              const geometry::Vec3& point) const;
    [[nodiscard]] std::optional<ElementProbeResult> probeBoundaryFacet(
        const SimulationMesh& mesh, MeshEntityId facetId, std::string_view fieldName) const;
    [[nodiscard]] std::optional<VectorResultant> reactionResultant(
        const SimulationMesh& mesh, const std::vector<MeshEntityId>& nodeIds) const;
    [[nodiscard]] std::vector<MeshEntityId> cutElements(const SimulationMesh& mesh,
                                                        const PlaneCut& plane) const;
    void exportCsv(const SimulationMesh& mesh, const std::filesystem::path& path) const;
    void exportLegacyVtk(const SimulationMesh& mesh, const std::filesystem::path& path,
                         double deformationScale = 1.0) const;
private:
    std::optional<NodeVectorField> displacement_;
    std::optional<NodeVectorField> reaction_;
    std::vector<ElementScalarField> elementScalars_;
};

} // namespace femcae::meshing
