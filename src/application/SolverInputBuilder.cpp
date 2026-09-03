#include "femcae/application/SolverInputBuilder.h"

#include <cmath>
#include <map>
#include <utility>

namespace femcae::application {
namespace {

using DofAddress = std::pair<std::int64_t, int>;

SolverInputBuildResult failure(const SolverInputBuildError error, std::string detail)
{
    return {.input = std::nullopt, .error = error, .detail = std::move(detail)};
}

} // namespace

SolverInputBuildResult SolverInputBuilder::buildHex8(const AnalysisSnapshot &snapshot)
{
    Hex8SolverInput input;
    input.apiVersion = snapshot.apiVersion();
    input.analysisKind = snapshot.analysisKind();
    input.linearSystem = snapshot.linearSystem();
    input.nonlinearControls = snapshot.nonlinearControls();
    input.youngModulusPa = snapshot.material().youngModulusPa;
    input.poissonRatio = snapshot.material().poissonRatio;
    input.requestedResults = snapshot.requestedResults();

    input.nodeIds.reserve(snapshot.nodes().size());
    input.coordinatesXYZ.reserve(3 * snapshot.nodes().size());
    for (const SnapshotNode &node : snapshot.nodes()) {
        input.nodeIds.push_back(static_cast<long long>(node.id));
        input.coordinatesXYZ.push_back(node.coordinatesSI.x);
        input.coordinatesXYZ.push_back(node.coordinatesSI.y);
        input.coordinatesXYZ.push_back(node.coordinatesSI.z);
    }

    input.elementIds.reserve(snapshot.elements().size());
    input.connectivity8.reserve(8 * snapshot.elements().size());
    input.formulation = snapshot.elements().front().formulation;
    for (const SnapshotHex8Element &element : snapshot.elements()) {
        if (element.formulation != input.formulation) {
            return failure(SolverInputBuildError::MixedElementFormulation,
                           "Tek general HEX8 call içinde karışık formulation desteklenmiyor.");
        }
        input.elementIds.push_back(static_cast<long long>(element.id));
        for (const std::int64_t nodeId : element.nodeIds) {
            input.connectivity8.push_back(static_cast<long long>(nodeId));
        }
    }

    // Overlap eden support scope'ları aynı DOF'u tekrar üretebilir. Aynı değer
    // tek constraint'e canonicalize edilir; fiziksel olarak çelişen değer sessiz
    // precedence ile çözülmez.
    std::map<DofAddress, double> constraints;
    for (const SnapshotConstraint &constraint : snapshot.constraints()) {
        const DofAddress address{constraint.nodeId, constraint.component};
        const auto [it, inserted] = constraints.emplace(address, constraint.prescribedValueSI);
        if (!inserted && std::abs(it->second - constraint.prescribedValueSI) > 1.0e-14) {
            return failure(SolverInputBuildError::ConflictingConstraint,
                           "Aynı nodal DOF için çelişen prescribed değerler bulundu.");
        }
    }
    for (const auto &[address, value] : constraints) {
        input.constraintNodeIds.push_back(static_cast<long long>(address.first));
        input.constraintComponents.push_back(address.second);
        input.constraintValues.push_back(value);
    }

    // Birden çok Face/Force nesnesinin aynı DOF katkıları lineer olarak toplanır.
    // Surface integration daha önce snapshot draft'ına equivalent nodal load
    // üretmiştir; burada geometri veya facet yeniden yorumlanmaz.
    std::map<DofAddress, double> loads;
    for (const SnapshotNodalLoad &load : snapshot.nodalLoads()) {
        loads[{load.nodeId, load.component}] += load.valueSI;
    }
    for (const auto &[address, value] : loads) {
        if (!std::isfinite(value)) {
            return failure(SolverInputBuildError::NonFiniteAggregate,
                           "Equivalent nodal load toplamı finite değil.");
        }
        if (std::abs(value) <= 1.0e-30) {
            continue;
        }
        input.loadNodeIds.push_back(static_cast<long long>(address.first));
        input.loadComponents.push_back(address.second);
        input.loadValues.push_back(value);
    }
    if (input.loadValues.empty()) {
        return failure(SolverInputBuildError::EmptyEquivalentLoad,
                       "Equivalent nodal load'lar toplamda sıfırlandı.");
    }

    SolverInputBuildResult result;
    result.input.emplace(std::move(input));
    return result;
}

const char *solverInputBuildErrorMessage(const SolverInputBuildError error) noexcept
{
    switch (error) {
    case SolverInputBuildError::None: return "No error";
    case SolverInputBuildError::MixedElementFormulation: return "Mixed HEX8 formulation";
    case SolverInputBuildError::ConflictingConstraint: return "Conflicting constraint";
    case SolverInputBuildError::EmptyEquivalentLoad: return "Empty equivalent load";
    case SolverInputBuildError::NonFiniteAggregate: return "Non-finite aggregate";
    }
    return "Unknown solver input error";
}

} // namespace femcae::application
