#include "femcae/application/AnalysisSnapshot.h"

#include <cmath>
#include <unordered_set>

namespace femcae::application {
namespace {

bool finite(const geometry::Vec3 &value)
{
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

bool validNonlinearControls(const SnapshotNonlinearControls &controls)
{
    const bool methodValid = controls.method == SnapshotNonlinearMethod::FullNewton
        || controls.method == SnapshotNonlinearMethod::ModifiedNewton;
    return methodValid
        && controls.maximumIterations > 0
        && controls.maximumStepAttempts > 0
        && controls.targetIterations > 0
        && std::isfinite(controls.initialLoadIncrement)
        && std::isfinite(controls.minimumLoadIncrement)
        && std::isfinite(controls.maximumLoadIncrement)
        && controls.initialLoadIncrement > 0.0
        && controls.initialLoadIncrement <= 1.0
        && controls.minimumLoadIncrement > 0.0
        && controls.maximumLoadIncrement >= controls.minimumLoadIncrement
        && controls.initialLoadIncrement >= controls.minimumLoadIncrement
        && controls.initialLoadIncrement <= controls.maximumLoadIncrement
        && std::isfinite(controls.cutbackFactor)
        && controls.cutbackFactor > 0.0
        && controls.cutbackFactor < 1.0
        && std::isfinite(controls.growthFactor)
        && controls.growthFactor >= 1.0
        && (!controls.lineSearch
            || (controls.lineSearchMaximumIterations > 0
                && controls.lineSearchReduction > 0.0
                && controls.lineSearchReduction < 1.0
                && controls.lineSearchMinimumAlpha > 0.0
                && controls.lineSearchMinimumAlpha <= 1.0))
        && (controls.useResidualCriterion || controls.useDisplacementCriterion)
        && std::isfinite(controls.residualRelativeTolerance)
        && controls.residualRelativeTolerance >= 0.0
        && std::isfinite(controls.residualAbsoluteTolerance)
        && controls.residualAbsoluteTolerance >= 0.0
        && std::isfinite(controls.displacementRelativeTolerance)
        && controls.displacementRelativeTolerance >= 0.0;
}

AnalysisSnapshotBuildResult failure(const AnalysisSnapshotError error, std::string detail)
{
    return {.snapshot = std::nullopt, .error = error, .detail = std::move(detail)};
}

} // namespace

AnalysisSnapshotBuildResult AnalysisSnapshotBuilder::build(AnalysisSnapshotDraft draft)
{
    if (draft.apiVersion != AnalysisSnapshotApiVersion
        || draft.schemaVersion != AnalysisSnapshotSchemaVersion) {
        return failure(AnalysisSnapshotError::UnsupportedVersion,
                       "AnalysisSnapshot API/schema version desteklenmiyor.");
    }
    if (draft.nodes.empty() || draft.elements.empty()) {
        return failure(AnalysisSnapshotError::EmptyMesh,
                       "AnalysisSnapshot en az bir node ve HEX8 element gerektirir.");
    }

    std::unordered_set<std::int64_t> nodeIds;
    nodeIds.reserve(draft.nodes.size());
    for (const SnapshotNode &node : draft.nodes) {
        if (node.id < 0 || !nodeIds.insert(node.id).second) {
            return failure(AnalysisSnapshotError::DuplicateNodeId,
                           "AnalysisSnapshot node ID'leri geçerli ve tekil olmalıdır.");
        }
        if (!finite(node.coordinatesSI)) {
            return failure(AnalysisSnapshotError::NonFiniteCoordinate,
                           "AnalysisSnapshot SI koordinatları finite olmalıdır.");
        }
    }

    std::unordered_set<std::int64_t> elementIds;
    elementIds.reserve(draft.elements.size());
    for (const SnapshotHex8Element &element : draft.elements) {
        if (element.id < 0 || !elementIds.insert(element.id).second) {
            return failure(AnalysisSnapshotError::DuplicateElementId,
                           "AnalysisSnapshot element ID'leri geçerli ve tekil olmalıdır.");
        }
        std::unordered_set<std::int64_t> localNodes;
        for (const std::int64_t nodeId : element.nodeIds) {
            if (!nodeIds.contains(nodeId) || !localNodes.insert(nodeId).second) {
                return failure(AnalysisSnapshotError::InvalidConnectivity,
                               "HEX8 connectivity sekiz farklı ve mevcut node ID'si içermelidir.");
            }
        }
        if (element.materialId != draft.material.id) {
            return failure(AnalysisSnapshotError::InvalidMaterial,
                           "HEX8 material assignment snapshot material kimliğiyle uyuşmuyor.");
        }
    }

    if (draft.material.id < 0
        || draft.material.model != SnapshotMaterialModel::LinearElastic
        || !std::isfinite(draft.material.youngModulusPa)
        || draft.material.youngModulusPa <= 0.0
        || !std::isfinite(draft.material.poissonRatio)
        || draft.material.poissonRatio <= -1.0
        || draft.material.poissonRatio >= 0.5) {
        return failure(AnalysisSnapshotError::InvalidMaterial,
                       "Linear Elastic snapshot malzemesi geçerli E ve Poisson oranı gerektirir.");
    }

    for (const SnapshotConstraint &constraint : draft.constraints) {
        if (!nodeIds.contains(constraint.nodeId)
            || constraint.component < 1 || constraint.component > 3
            || !std::isfinite(constraint.prescribedValueSI)) {
            return failure(AnalysisSnapshotError::InvalidConstraint,
                           "Constraint mevcut node, 1..3 component ve finite değer gerektirir.");
        }
    }
    for (const SnapshotNodalLoad &load : draft.nodalLoads) {
        if (!nodeIds.contains(load.nodeId)
            || load.component < 1 || load.component > 3
            || !std::isfinite(load.valueSI)) {
            return failure(AnalysisSnapshotError::InvalidLoad,
                           "Equivalent nodal load mevcut node, 1..3 component ve finite değer gerektirir.");
        }
    }
    if (draft.constraints.empty()) {
        return failure(AnalysisSnapshotError::InvalidConstraint,
                       "AnalysisSnapshot en az bir kinematic constraint gerektirir.");
    }
    if (draft.nodalLoads.empty()) {
        return failure(AnalysisSnapshotError::InvalidLoad,
                       "AnalysisSnapshot en az bir equivalent nodal load gerektirir.");
    }

    const SnapshotHex8Formulation expected = draft.analysisKind == SnapshotAnalysisKind::NonlinearStatic
        ? SnapshotHex8Formulation::TotalLagrangianDisplacement
        : SnapshotHex8Formulation::SmallStrainDisplacement;
    for (const SnapshotHex8Element &element : draft.elements) {
        if (element.formulation != expected) {
            return failure(AnalysisSnapshotError::InvalidKinematics,
                           "Analysis kind ile HEX8 formulation snapshot'ta uyuşmuyor.");
        }
    }
    if (draft.analysisKind == SnapshotAnalysisKind::NonlinearStatic) {
        if (!draft.largeDeformation) {
            return failure(AnalysisSnapshotError::InvalidKinematics,
                           "Beta.3 Nonlinear Static snapshot Large Deformation gerektirir.");
        }
        if (!validNonlinearControls(draft.nonlinearControls)) {
            return failure(AnalysisSnapshotError::InvalidNonlinearControls,
                           "Nonlinear solver controls snapshot contract'ına uymuyor.");
        }
    }

    AnalysisSnapshotBuildResult result;
    result.snapshot.emplace(AnalysisSnapshot(std::move(draft)));
    return result;
}

const char *analysisSnapshotErrorMessage(const AnalysisSnapshotError error) noexcept
{
    switch (error) {
    case AnalysisSnapshotError::None: return "No error";
    case AnalysisSnapshotError::UnsupportedVersion: return "Unsupported snapshot version";
    case AnalysisSnapshotError::EmptyMesh: return "Empty mesh";
    case AnalysisSnapshotError::DuplicateNodeId: return "Invalid or duplicate node ID";
    case AnalysisSnapshotError::DuplicateElementId: return "Invalid or duplicate element ID";
    case AnalysisSnapshotError::NonFiniteCoordinate: return "Non-finite coordinate";
    case AnalysisSnapshotError::InvalidConnectivity: return "Invalid HEX8 connectivity";
    case AnalysisSnapshotError::InvalidMaterial: return "Invalid material assignment";
    case AnalysisSnapshotError::InvalidConstraint: return "Invalid constraint";
    case AnalysisSnapshotError::InvalidLoad: return "Invalid equivalent nodal load";
    case AnalysisSnapshotError::InvalidKinematics: return "Invalid analysis kinematics";
    case AnalysisSnapshotError::InvalidNonlinearControls: return "Invalid nonlinear controls";
    }
    return "Unknown snapshot error";
}

} // namespace femcae::application
