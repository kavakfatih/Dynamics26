#pragma once

#include "femcae/geometry/GeometryTypes.h"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace femcae::application {

inline constexpr std::uint32_t AnalysisSnapshotApiVersion = 1;
inline constexpr std::uint32_t AnalysisSnapshotSchemaVersion = 1;

enum class SnapshotAnalysisKind : std::uint8_t {
    LinearStatic = 1,
    NonlinearStatic = 2
};

enum class SnapshotHex8Formulation : std::uint8_t {
    SmallStrainDisplacement = 1,
    TotalLagrangianDisplacement = 2
};

// Beta.3 product nonlinear yolu, Linear Elastic authoring verisini finite-
// deformation elemaninda St. Venant-Kirchhoff reference response olarak tüketir.
// Bu kimlik hyperelastic bir malzeme modeli değildir.
enum class SnapshotMaterialModel : std::uint8_t {
    LinearElastic = 1
};

enum class SnapshotLinearBackend : std::uint8_t {
    DenseReference = 1,
    SparseCg = 2,
    AppleAccelerateSparseDirect = 3
};

enum class SnapshotMatrixSymmetry : std::uint8_t {
    Symmetric = 1,
    Unsymmetric = 2,
    Unknown = 3
};

enum class SnapshotMatrixDefiniteness : std::uint8_t {
    SpdExpected = 1,
    Indefinite = 2,
    Unknown = 3
};

enum class SnapshotNonlinearMethod : std::uint8_t {
    FullNewton = 1,
    ModifiedNewton = 2
};

enum class SnapshotResultField : std::uint8_t {
    TotalDeformation = 1,
    EquivalentStress = 2,
    ReactionForce = 3
};

struct SnapshotNode {
    std::int64_t id{-1};
    geometry::Vec3 coordinatesSI;
};

struct SnapshotHex8Element {
    std::int64_t id{-1};
    std::array<std::int64_t, 8> nodeIds{};
    std::int64_t materialId{-1};
    SnapshotHex8Formulation formulation{SnapshotHex8Formulation::SmallStrainDisplacement};
};

struct SnapshotMaterial {
    std::int64_t id{-1};
    std::string name;
    SnapshotMaterialModel model{SnapshotMaterialModel::LinearElastic};
    double youngModulusPa{0.0};
    double poissonRatio{0.0};
};

struct SnapshotConstraint {
    std::int64_t nodeId{-1};
    int component{0}; // 1=x, 2=y, 3=z
    double prescribedValueSI{0.0};
};

struct SnapshotNodalLoad {
    std::int64_t nodeId{-1};
    int component{0}; // 1=x, 2=y, 3=z
    double valueSI{0.0};
};

struct SnapshotNonlinearControls {
    SnapshotNonlinearMethod method{SnapshotNonlinearMethod::FullNewton};
    int maximumIterations{25};
    int maximumStepAttempts{200};
    bool adaptiveStepping{true};
    double initialLoadIncrement{0.25};
    double minimumLoadIncrement{1.0e-4};
    double maximumLoadIncrement{0.50};
    double cutbackFactor{0.50};
    double growthFactor{1.50};
    int targetIterations{6};
    bool lineSearch{true};
    int lineSearchMaximumIterations{8};
    double lineSearchReduction{0.50};
    double lineSearchMinimumAlpha{1.0e-4};
    bool useResidualCriterion{true};
    bool useDisplacementCriterion{true};
    double residualRelativeTolerance{1.0e-8};
    double residualAbsoluteTolerance{1.0e-10};
    double displacementRelativeTolerance{1.0e-8};
};

struct SnapshotLinearSystem {
    SnapshotLinearBackend backend{SnapshotLinearBackend::DenseReference};
    SnapshotMatrixSymmetry symmetry{SnapshotMatrixSymmetry::Unknown};
    SnapshotMatrixDefiniteness definiteness{SnapshotMatrixDefiniteness::Unknown};
};

// Mutable draft yalnız document/service sınırında yaşar. Başarılı build'den
// sonra solver consumer'a sadece AnalysisSnapshot'ın const erişimi verilir.
struct AnalysisSnapshotDraft {
    std::uint32_t apiVersion{AnalysisSnapshotApiVersion};
    std::uint32_t schemaVersion{AnalysisSnapshotSchemaVersion};
    SnapshotAnalysisKind analysisKind{SnapshotAnalysisKind::LinearStatic};
    bool largeDeformation{false};
    std::vector<SnapshotNode> nodes;
    std::vector<SnapshotHex8Element> elements;
    SnapshotMaterial material;
    std::vector<SnapshotConstraint> constraints;
    std::vector<SnapshotNodalLoad> nodalLoads;
    SnapshotNonlinearControls nonlinearControls;
    SnapshotLinearSystem linearSystem;
    std::vector<SnapshotResultField> requestedResults;
};

enum class AnalysisSnapshotError : std::uint8_t {
    None = 0,
    UnsupportedVersion,
    EmptyMesh,
    DuplicateNodeId,
    DuplicateElementId,
    NonFiniteCoordinate,
    InvalidConnectivity,
    InvalidMaterial,
    InvalidConstraint,
    InvalidLoad,
    InvalidKinematics,
    InvalidNonlinearControls
};

class AnalysisSnapshot final {
public:
    AnalysisSnapshot(const AnalysisSnapshot &) = default;
    AnalysisSnapshot(AnalysisSnapshot &&) noexcept = default;
    AnalysisSnapshot &operator=(const AnalysisSnapshot &) = delete;
    AnalysisSnapshot &operator=(AnalysisSnapshot &&) = delete;

    [[nodiscard]] std::uint32_t apiVersion() const noexcept { return data_.apiVersion; }
    [[nodiscard]] std::uint32_t schemaVersion() const noexcept { return data_.schemaVersion; }
    [[nodiscard]] SnapshotAnalysisKind analysisKind() const noexcept { return data_.analysisKind; }
    [[nodiscard]] bool largeDeformation() const noexcept { return data_.largeDeformation; }
    [[nodiscard]] const std::vector<SnapshotNode> &nodes() const noexcept { return data_.nodes; }
    [[nodiscard]] const std::vector<SnapshotHex8Element> &elements() const noexcept { return data_.elements; }
    [[nodiscard]] const SnapshotMaterial &material() const noexcept { return data_.material; }
    [[nodiscard]] const std::vector<SnapshotConstraint> &constraints() const noexcept { return data_.constraints; }
    [[nodiscard]] const std::vector<SnapshotNodalLoad> &nodalLoads() const noexcept { return data_.nodalLoads; }
    [[nodiscard]] const SnapshotNonlinearControls &nonlinearControls() const noexcept
    {
        return data_.nonlinearControls;
    }
    [[nodiscard]] const SnapshotLinearSystem &linearSystem() const noexcept { return data_.linearSystem; }
    [[nodiscard]] const std::vector<SnapshotResultField> &requestedResults() const noexcept
    {
        return data_.requestedResults;
    }

private:
    explicit AnalysisSnapshot(AnalysisSnapshotDraft data) : data_(std::move(data)) {}

    AnalysisSnapshotDraft data_;
    friend class AnalysisSnapshotBuilder;
};

struct AnalysisSnapshotBuildResult {
    std::optional<AnalysisSnapshot> snapshot;
    AnalysisSnapshotError error{AnalysisSnapshotError::None};
    std::string detail;

    [[nodiscard]] bool success() const noexcept { return snapshot.has_value(); }
};

class AnalysisSnapshotBuilder final {
public:
    [[nodiscard]] static AnalysisSnapshotBuildResult build(AnalysisSnapshotDraft draft);
};

[[nodiscard]] const char *analysisSnapshotErrorMessage(AnalysisSnapshotError error) noexcept;

} // namespace femcae::application
