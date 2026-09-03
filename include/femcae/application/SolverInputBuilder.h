#pragma once

#include "femcae/application/AnalysisSnapshot.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace femcae::application {

// C ABI array+count contract'ına doğrudan karşılık gelen, owning ve immutable-
// by-convention input. Bütün değerler SI birimlerindedir; hiçbir pointer canlı
// document, Qt nesnesi veya SimulationMesh storage'ına referans vermez.
struct Hex8SolverInput {
    std::uint32_t apiVersion{AnalysisSnapshotApiVersion};
    SnapshotAnalysisKind analysisKind{SnapshotAnalysisKind::LinearStatic};
    SnapshotHex8Formulation formulation{SnapshotHex8Formulation::SmallStrainDisplacement};
    SnapshotLinearSystem linearSystem;
    SnapshotNonlinearControls nonlinearControls;
    std::vector<long long> nodeIds;
    std::vector<double> coordinatesXYZ;
    std::vector<long long> elementIds;
    std::vector<long long> connectivity8;
    double youngModulusPa{0.0};
    double poissonRatio{0.0};
    std::vector<long long> constraintNodeIds;
    std::vector<int> constraintComponents;
    std::vector<double> constraintValues;
    std::vector<long long> loadNodeIds;
    std::vector<int> loadComponents;
    std::vector<double> loadValues;
    std::vector<SnapshotResultField> requestedResults;
};

enum class SolverInputBuildError : std::uint8_t {
    None = 0,
    MixedElementFormulation,
    ConflictingConstraint,
    EmptyEquivalentLoad,
    NonFiniteAggregate
};

struct SolverInputBuildResult {
    std::optional<Hex8SolverInput> input;
    SolverInputBuildError error{SolverInputBuildError::None};
    std::string detail;

    [[nodiscard]] bool success() const noexcept { return input.has_value(); }
};

class SolverInputBuilder final {
public:
    [[nodiscard]] static SolverInputBuildResult buildHex8(const AnalysisSnapshot &snapshot);
};

[[nodiscard]] const char *solverInputBuildErrorMessage(SolverInputBuildError error) noexcept;

} // namespace femcae::application
