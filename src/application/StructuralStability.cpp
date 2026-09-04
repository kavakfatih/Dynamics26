#include "femcae/application/StructuralStability.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <numeric>
#include <set>
#include <unordered_map>
#include <utility>

namespace femcae::application {
namespace {

using meshing::MeshEntityId;

constexpr std::size_t RigidBodyModeCount = 6;
constexpr double RelativeRankTolerance = 1.0e-10;

struct DisjointSet {
    explicit DisjointSet(const std::size_t size) : parent(size), rank(size, 0)
    {
        std::iota(parent.begin(), parent.end(), std::size_t{0});
    }

    std::size_t find(const std::size_t value)
    {
        if (parent[value] != value) {
            parent[value] = find(parent[value]);
        }
        return parent[value];
    }

    void unite(const std::size_t left, const std::size_t right)
    {
        std::size_t a = find(left);
        std::size_t b = find(right);
        if (a == b) {
            return;
        }
        if (rank[a] < rank[b]) {
            std::swap(a, b);
        }
        parent[b] = a;
        if (rank[a] == rank[b]) {
            ++rank[a];
        }
    }

    std::vector<std::size_t> parent;
    std::vector<unsigned char> rank;
};

bool finite(const geometry::Vec3 &value)
{
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

StructuralStabilityResult failure(const StructuralStabilityError error,
                                  std::string detail)
{
    return {.error = error, .detail = std::move(detail), .components = {}};
}

// B_rb row convention:
//   u = t + omega x ((r-r_c)/L_c)
// Coordinate normalization rotational columns'i translational columns ile
// dimensionless ve karşılaştırılabilir yapar; rank kararı model unit/scale'ine
// bağlı kalmaz.
std::array<double, RigidBodyModeCount> rigidBodyRow(
    const geometry::Vec3 &normalizedPosition, const int component)
{
    const double x = normalizedPosition.x;
    const double y = normalizedPosition.y;
    const double z = normalizedPosition.z;
    switch (component) {
    case 1: return {1.0, 0.0, 0.0, 0.0, z, -y};
    case 2: return {0.0, 1.0, 0.0, -z, 0.0, x};
    case 3: return {0.0, 0.0, 1.0, y, -x, 0.0};
    default: return {};
    }
}

// Altı B_rb column'u için pivotlu modified Gram-Schmidt uygulanır. İkinci
// orthogonalization pass'i yakın-bağımlı columns'ta round-off birikimini azaltır.
// B^T B oluşturulmadığı için condition number karesine çıkmaz. Bu sonuç tangent
// matrix pivot diagnostic'i değildir; yalnız kinematic rigid-body basis rank'idir.
int rankWithColumnPivoting(
    const std::vector<std::array<double, RigidBodyModeCount>> &rows)
{
    if (rows.empty()) {
        return 0;
    }

    std::array<std::vector<double>, RigidBodyModeCount> columns;
    for (std::size_t column = 0; column < RigidBodyModeCount; ++column) {
        columns[column].reserve(rows.size());
        for (const auto &row : rows) {
            columns[column].push_back(row[column]);
        }
    }

    const auto squaredNorm = [](const std::vector<double> &column) {
        return std::inner_product(column.cbegin(), column.cend(),
                                  column.cbegin(), 0.0);
    };
    double referenceNorm = 0.0;
    for (const auto &column : columns) {
        referenceNorm = std::max(referenceNorm, std::sqrt(squaredNorm(column)));
    }
    if (referenceNorm == 0.0) {
        return 0;
    }
    const double threshold = referenceNorm * RelativeRankTolerance;

    int rank = 0;
    for (std::size_t k = 0; k < RigidBodyModeCount; ++k) {
        std::size_t pivot = k;
        double pivotNormSquared = 0.0;
        for (std::size_t candidate = k; candidate < RigidBodyModeCount; ++candidate) {
            const double candidateNormSquared = squaredNorm(columns[candidate]);
            if (candidateNormSquared > pivotNormSquared) {
                pivot = candidate;
                pivotNormSquared = candidateNormSquared;
            }
        }
        const double pivotNorm = std::sqrt(pivotNormSquared);
        if (pivotNorm <= threshold) {
            break;
        }
        std::swap(columns[k], columns[pivot]);
        for (double &value : columns[k]) {
            value /= pivotNorm;
        }
        ++rank;

        for (std::size_t column = k + 1; column < RigidBodyModeCount; ++column) {
            for (int pass = 0; pass < 2; ++pass) {
                const double projection = std::inner_product(
                    columns[k].cbegin(), columns[k].cend(),
                    columns[column].cbegin(), 0.0);
                for (std::size_t row = 0; row < rows.size(); ++row) {
                    columns[column][row] -= projection * columns[k][row];
                }
            }
        }
    }
    return rank;
}

} // namespace

bool StructuralStabilityResult::stable() const noexcept
{
    return success() && !components.empty()
        && std::all_of(components.cbegin(), components.cend(),
                       [](const StructuralComponentStability &component) {
                           return component.stable();
                       });
}

StructuralStabilityResult StructuralStabilityDiagnostic::evaluate(
    const meshing::SimulationMesh &mesh,
    const std::vector<StructuralConstraintDof> &constraints)
{
    if (mesh.nodes.empty() || mesh.elements.empty()) {
        return failure(StructuralStabilityError::EmptyMesh,
                       "Structural stability diagnostic en az bir node ve HEX8 element gerektirir.");
    }

    std::unordered_map<MeshEntityId, std::size_t> nodeIndex;
    nodeIndex.reserve(mesh.nodes.size());
    for (std::size_t i = 0; i < mesh.nodes.size(); ++i) {
        const auto &node = mesh.nodes[i];
        if (node.id < 0 || !nodeIndex.emplace(node.id, i).second) {
            return failure(StructuralStabilityError::DuplicateNodeId,
                           "Mesh node ID'leri geçerli ve tekil olmalıdır.");
        }
        if (!finite(node.x)) {
            return failure(StructuralStabilityError::NonFiniteCoordinate,
                           "Structural stability reference koordinatları finite olmalıdır.");
        }
    }

    DisjointSet sets(mesh.nodes.size());
    for (const auto &element : mesh.elements) {
        if (element.topology != meshing::MeshTopology::Hex8) {
            return failure(StructuralStabilityError::InvalidConnectivity,
                           "Structural stability diagnostic yalnız HEX8 connectivity kabul eder.");
        }
        const auto first = nodeIndex.find(element.nodeIds.front());
        if (first == nodeIndex.end()) {
            return failure(StructuralStabilityError::InvalidConnectivity,
                           "HEX8 connectivity mevcut olmayan node ID'si içeriyor.");
        }
        std::set<MeshEntityId> localNodes;
        for (const MeshEntityId nodeId : element.nodeIds) {
            const auto current = nodeIndex.find(nodeId);
            if (current == nodeIndex.end() || !localNodes.insert(nodeId).second) {
                return failure(StructuralStabilityError::InvalidConnectivity,
                               "HEX8 connectivity sekiz farklı ve mevcut node ID'si içermelidir.");
            }
            sets.unite(first->second, current->second);
        }
    }

    std::set<std::pair<MeshEntityId, int>> uniqueConstraints;
    for (const StructuralConstraintDof &constraint : constraints) {
        if (!nodeIndex.contains(constraint.nodeId)
            || constraint.component < 1 || constraint.component > 3) {
            return failure(StructuralStabilityError::InvalidConstraint,
                           "Constraint mevcut node ve 1..3 displacement component gerektirir.");
        }
        uniqueConstraints.emplace(constraint.nodeId, constraint.component);
    }

    struct ComponentWork {
        std::size_t root{0};
        MeshEntityId minimumNodeId{meshing::InvalidMeshId};
        std::vector<std::size_t> nodeIndices;
        std::vector<MeshEntityId> elementIds;
    };
    std::unordered_map<std::size_t, std::size_t> componentByRoot;
    std::vector<ComponentWork> work;
    for (std::size_t i = 0; i < mesh.nodes.size(); ++i) {
        const std::size_t root = sets.find(i);
        auto [it, inserted] = componentByRoot.emplace(root, work.size());
        if (inserted) {
            work.push_back(ComponentWork{root, mesh.nodes[i].id, {}, {}});
        }
        ComponentWork &component = work[it->second];
        component.nodeIndices.push_back(i);
        component.minimumNodeId = std::min(component.minimumNodeId, mesh.nodes[i].id);
    }
    for (const auto &element : mesh.elements) {
        const std::size_t root = sets.find(nodeIndex.at(element.nodeIds.front()));
        work[componentByRoot.at(root)].elementIds.push_back(element.id);
    }
    std::sort(work.begin(), work.end(), [](const ComponentWork &left, const ComponentWork &right) {
        return left.minimumNodeId < right.minimumNodeId;
    });

    StructuralStabilityResult result;
    result.components.reserve(work.size());
    for (std::size_t workIndex = 0; workIndex < work.size(); ++workIndex) {
        const ComponentWork &source = work[workIndex];
        StructuralComponentStability component;
        component.index = workIndex + 1;
        component.elementIds = source.elementIds;
        component.nodeIds.reserve(source.nodeIndices.size());

        geometry::Vec3 centroid{};
        for (const std::size_t index : source.nodeIndices) {
            const auto &node = mesh.nodes[index];
            component.nodeIds.push_back(node.id);
            centroid.x += node.x.x;
            centroid.y += node.x.y;
            centroid.z += node.x.z;
        }
        const double nodeCount = static_cast<double>(source.nodeIndices.size());
        centroid.x /= nodeCount;
        centroid.y /= nodeCount;
        centroid.z /= nodeCount;

        for (const std::size_t index : source.nodeIndices) {
            const auto &point = mesh.nodes[index].x;
            const double dx = point.x - centroid.x;
            const double dy = point.y - centroid.y;
            const double dz = point.z - centroid.z;
            component.characteristicLength = std::max(
                component.characteristicLength, std::sqrt(dx * dx + dy * dy + dz * dz));
        }
        const double normalizationLength = component.characteristicLength
            > std::numeric_limits<double>::min() ? component.characteristicLength : 1.0;

        std::vector<std::array<double, RigidBodyModeCount>> rigidBodyRows;
        for (const auto &[nodeId, dofComponent] : uniqueConstraints) {
            const auto indexIt = nodeIndex.find(nodeId);
            if (indexIt == nodeIndex.end() || sets.find(indexIt->second) != source.root) {
                continue;
            }
            const auto &point = mesh.nodes[indexIt->second].x;
            const geometry::Vec3 normalized{
                (point.x - centroid.x) / normalizationLength,
                (point.y - centroid.y) / normalizationLength,
                (point.z - centroid.z) / normalizationLength};
            rigidBodyRows.push_back(rigidBodyRow(normalized, dofComponent));
            ++component.constrainedDofCount;
        }

        component.restraintRank = rankWithColumnPivoting(rigidBodyRows);
        component.freeRigidBodyModeCount = static_cast<int>(RigidBodyModeCount)
            - component.restraintRank;
        result.components.push_back(std::move(component));
    }

    return result;
}

const char *structuralStabilityErrorMessage(const StructuralStabilityError error) noexcept
{
    switch (error) {
    case StructuralStabilityError::None: return "No error";
    case StructuralStabilityError::EmptyMesh: return "Empty mesh";
    case StructuralStabilityError::DuplicateNodeId: return "Invalid or duplicate node ID";
    case StructuralStabilityError::InvalidConnectivity: return "Invalid HEX8 connectivity";
    case StructuralStabilityError::InvalidConstraint: return "Invalid displacement constraint";
    case StructuralStabilityError::NonFiniteCoordinate: return "Non-finite coordinate";
    }
    return "Unknown structural stability diagnostic error";
}

} // namespace femcae::application
