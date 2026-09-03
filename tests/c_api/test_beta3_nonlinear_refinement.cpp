#include <femcae/femcae.h>
#include <femcae/meshing/StructuredHexMesher.h>
#include <femcae/meshing/SurfaceLoadAssembler.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <iterator>
#include <limits>
#include <unordered_set>
#include <vector>

namespace {

using namespace femcae::meshing;

constexpr femcae::geometry::GeometryEntityId XMinFace = 11;
constexpr femcae::geometry::GeometryEntityId XMaxFace = 12;
constexpr int HistoryCapacity = 2048;

int failures = 0;

void check(const bool condition, const char *message)
{
    std::cout << (condition ? "PASS  " : "FAIL  ") << message << '\n';
    failures += condition ? 0 : 1;
}

struct RefinementResult {
    bool success{false};
    double averageTipX{0.0};
    double reactionX{0.0};
    double minimumJ{0.0};
};

[[nodiscard]] std::vector<MeshEntityId> faceNodeIds(
    const SimulationMesh &mesh,
    const femcae::geometry::GeometryEntityId faceId)
{
    std::unordered_set<MeshEntityId> unique;
    for (const MeshFacet &facet : mesh.boundaryFacets) {
        if (facet.sourceGeometryId != faceId) {
            continue;
        }
        unique.insert(facet.nodeIds.cbegin(), facet.nodeIds.cend());
    }
    std::vector<MeshEntityId> ids(unique.cbegin(), unique.cend());
    std::sort(ids.begin(), ids.end());
    return ids;
}

[[nodiscard]] RefinementResult solveRefinement(const std::size_t nx)
{
    StructuredHexMesherOptions options;
    options.nx = nx;
    options.ny = 1;
    options.nz = 1;
    options.firstNodeId = static_cast<MeshEntityId>(1000 + 100 * nx);
    options.firstElementId = static_cast<MeshEntityId>(5000 + 100 * nx);
    options.firstFacetId = static_cast<MeshEntityId>(9000 + 100 * nx);
    const SimulationMesh mesh = StructuredHexMesher{}.meshBox(
        {{0.0, 0.0, 0.0}, {2.0, 1.0, 1.0}},
        {10, XMinFace, XMaxFace, 13, 14, 15, 16}, 73, options);

    const SurfaceLoadAssemblyResult surface = assembleUniformTotalForce(
        mesh, {XMaxFace}, {1000.0, 0.0, 0.0});
    if (!surface.success()) {
        return {};
    }

    std::vector<long long> nodeIds;
    std::vector<double> coordinates;
    nodeIds.reserve(mesh.nodes.size());
    coordinates.reserve(3 * mesh.nodes.size());
    for (const MeshNode &node : mesh.nodes) {
        nodeIds.push_back(static_cast<long long>(node.id));
        coordinates.push_back(node.x.x);
        coordinates.push_back(node.x.y);
        coordinates.push_back(node.x.z);
    }

    std::vector<long long> elementIds;
    std::vector<long long> connectivity;
    elementIds.reserve(mesh.elements.size());
    connectivity.reserve(8 * mesh.elements.size());
    for (const MeshElement &element : mesh.elements) {
        elementIds.push_back(static_cast<long long>(element.id));
        for (const MeshEntityId nodeId : element.nodeIds) {
            connectivity.push_back(static_cast<long long>(nodeId));
        }
    }

    const std::vector<MeshEntityId> fixedNodes = faceNodeIds(mesh, XMinFace);
    const std::vector<MeshEntityId> tipNodes = faceNodeIds(mesh, XMaxFace);
    std::vector<long long> constraintNodes;
    std::vector<int> constraintComponents;
    std::vector<double> constraintValues;
    constraintNodes.reserve(3 * fixedNodes.size());
    constraintComponents.reserve(3 * fixedNodes.size());
    constraintValues.reserve(3 * fixedNodes.size());
    for (const MeshEntityId nodeId : fixedNodes) {
        for (int component = 1; component <= 3; ++component) {
            constraintNodes.push_back(static_cast<long long>(nodeId));
            constraintComponents.push_back(component);
            constraintValues.push_back(0.0);
        }
    }

    std::vector<long long> loadNodes;
    std::vector<int> loadComponents;
    std::vector<double> loadValues;
    for (const NodalVectorLoad &load : surface.nodalLoads) {
        if (std::abs(load.value.x) <= 1.0e-30) {
            continue;
        }
        loadNodes.push_back(static_cast<long long>(load.nodeId));
        loadComponents.push_back(1);
        loadValues.push_back(load.value.x);
    }

    std::vector<double> displacement(3 * mesh.nodes.size(), 0.0);
    std::vector<double> reaction(3 * mesh.nodes.size(), 0.0);
    std::vector<double> stress(mesh.elements.size(), 0.0);
    std::array<int, HistoryCapacity> historyAttempt{};
    std::array<int, HistoryCapacity> historyAcceptedBefore{};
    std::array<int, HistoryCapacity> historyIteration{};
    std::array<int, HistoryCapacity> historyConverged{};
    std::array<double, HistoryCapacity> historyLoadFactor{};
    std::array<double, HistoryCapacity> historyIncrement{};
    std::array<double, HistoryCapacity> historyResidual{};
    std::array<double, HistoryCapacity> historyRelativeResidual{};
    std::array<double, HistoryCapacity> historyDisplacementIncrement{};
    std::array<double, HistoryCapacity> historyRelativeDisplacement{};
    std::array<double, HistoryCapacity> historyAlpha{};
    std::array<double, HistoryCapacity> historyMinimumJ{};
    int converged = 0;
    double loadFactor = 0.0;
    double finalResidual = 0.0;
    double minimumJ = 0.0;
    int acceptedSteps = 0;
    int stepAttempts = 0;
    int totalIterations = 0;
    int cutbacks = 0;
    int historyCount = 0;
    int historyRequired = 0;

    const int status = fem_solve_nonlinear_static_hex8_v1(
        FEM_NONLINEAR_STATIC_HEX8_API_VERSION,
        static_cast<int>(mesh.nodes.size()), nodeIds.data(), coordinates.data(),
        static_cast<int>(mesh.elements.size()), elementIds.data(), connectivity.data(),
        1.0e6, 0.30,
        static_cast<int>(constraintNodes.size()), constraintNodes.data(),
        constraintComponents.data(), constraintValues.data(),
        static_cast<int>(loadNodes.size()), loadNodes.data(),
        loadComponents.data(), loadValues.data(),
        FEM_NONLINEAR_METHOD_FULL_NEWTON, 30, 100, 1,
        0.25, 1.0e-4, 0.50, 0.50, 1.50, 6,
        1, 8, 0.50, 1.0e-4, 1, 1,
        1.0e-8, 1.0e-10, 1.0e-8, FEM_LINEAR_BACKEND_DENSE_REFERENCE,
        displacement.data(), reaction.data(), stress.data(),
        &converged, &loadFactor, &finalResidual, &minimumJ,
        &acceptedSteps, &stepAttempts, &totalIterations, &cutbacks,
        HistoryCapacity, &historyCount, &historyRequired,
        historyAttempt.data(), historyAcceptedBefore.data(), historyIteration.data(),
        historyLoadFactor.data(), historyIncrement.data(), historyResidual.data(),
        historyRelativeResidual.data(), historyDisplacementIncrement.data(),
        historyRelativeDisplacement.data(), historyAlpha.data(),
        historyMinimumJ.data(), historyConverged.data());

    bool finite = status == 0 && converged == 1
        && std::abs(loadFactor - 1.0) <= 1.0e-12
        && std::isfinite(finalResidual) && std::isfinite(minimumJ) && minimumJ > 0.0
        && acceptedSteps > 0 && stepAttempts >= acceptedSteps
        && totalIterations > 0 && historyCount > 0 && historyRequired == historyCount;
    for (const double value : displacement) {
        finite = finite && std::isfinite(value);
    }
    for (const double value : stress) {
        finite = finite && std::isfinite(value) && value >= 0.0;
    }

    double reactionX = 0.0;
    for (std::size_t i = 0; i < mesh.nodes.size(); ++i) {
        reactionX += reaction[3 * i];
        finite = finite && std::isfinite(reaction[3 * i])
            && std::isfinite(reaction[3 * i + 1])
            && std::isfinite(reaction[3 * i + 2]);
    }

    double averageTipX = 0.0;
    for (const MeshEntityId tipId : tipNodes) {
        const auto it = std::find_if(mesh.nodes.cbegin(), mesh.nodes.cend(),
                                     [tipId](const MeshNode &node) { return node.id == tipId; });
        if (it == mesh.nodes.cend()) {
            finite = false;
            continue;
        }
        const std::size_t index = static_cast<std::size_t>(
            std::distance(mesh.nodes.cbegin(), it));
        averageTipX += displacement[3 * index];
    }
    if (tipNodes.empty()) {
        finite = false;
    } else {
        averageTipX /= static_cast<double>(tipNodes.size());
    }

    const bool resultantConserved = std::abs(surface.resultant().x - 1000.0) <= 1.0e-10
        && std::abs(surface.resultant().y) <= 1.0e-10
        && std::abs(surface.resultant().z) <= 1.0e-10;
    return {finite && resultantConserved && std::abs(reactionX + 1000.0) <= 1.0e-5,
            averageTipX, reactionX, minimumJ};
}

} // namespace

int main()
{
    const RefinementResult one = solveRefinement(1);
    const RefinementResult two = solveRefinement(2);
    const RefinementResult four = solveRefinement(4);

    check(one.success && two.success && four.success,
          "1/2/4-element arbitrary structured HEX8 models converge with conserved equilibrium");
    check(one.averageTipX > 0.0 && two.averageTipX > 0.0 && four.averageTipX > 0.0,
          "all refinement levels produce finite positive global displacement response");

    // Homogeneous axial reference F L / (E A) = 0.002 m. StVK finite-strain
    // response need not equal the infinitesimal value exactly, but this low-load
    // sequence must remain in its engineering neighborhood and stabilize.
    constexpr double LinearReference = 0.002;
    check(std::abs(four.averageTipX - LinearReference) <= 0.10 * LinearReference,
          "refined nonlinear response remains close to the low-strain analytical scale");
    const double coarseChange = std::abs(two.averageTipX - one.averageTipX);
    const double refinedChange = std::abs(four.averageTipX - two.averageTipX);
    check(refinedChange <= coarseChange + 1.0e-10
              && refinedChange <= 0.05 * std::abs(four.averageTipX) + 1.0e-10,
          "global tip response stabilizes under 1/2/4 axial mesh refinement");
    check(one.minimumJ > 0.0 && two.minimumJ > 0.0 && four.minimumJ > 0.0,
          "all refinement levels retain positive minimum deformation Jacobian");

    std::cout << (failures == 0 ? "Beta.3 nonlinear mesh refinement PASS"
                                : "Beta.3 nonlinear mesh refinement FAIL")
              << " failures=" << failures << '\n';
    return failures == 0 ? 0 : 1;
}
